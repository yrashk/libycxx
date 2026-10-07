#!/usr/bin/env python3
"""libycxx: is a program, shared library, archive or object built against libycxx?

The checks of an image that tools/realworld's linkage proof (tools/lib/realworld.py, LINKAGE) and
tools/ycxx-check-binary share:
  - no libstdc++ or libc++ among an executable's or shared library's needed libraries
    (readelf -d / otool -L);
  - no libstdc++ or libc++ symbol, defined or undefined (std::__cxx11, __gnu_cxx::, std::__1, any
    other std::__ name, which libycxx never uses; GLIBCXX_/CXXABI_ symbol versions);
  - the executable or shared library defines __ycxx_allocation_functions, the exported allocation
    table only libycxx's runtime defines (DECISIONS §2), which libycxx's link options keep in every
    image linked against it.
Standard library only; ELF (binutils' readelf and nm) and Mach-O (otool, nm).
"""
import glob, os, re, shlex, subprocess, sys

FORBIDDEN_DIR = re.compile(r'/include/c\+\+/|/c\+\+/v1(/|$)|/libstdc\+\+|/libc\+\+')
FORBIDDEN_LIB = re.compile(r'^(libstdc\+\+|libc\+\+|libc\+\+abi|libsupc\+\+)[.-]|/(libstdc\+\+|libc\+\+|libc\+\+abi)[.-]')
FORBIDDEN_DEMANGLED = re.compile(r'std::__|__gnu_cxx::|__gnu_debug::')
FORBIDDEN_MANGLED = re.compile(r'_ZN?K?St(7__cxx11|3__1|8__detail)|@@?(GLIBCXX|CXXABI)_')
MARKER = '__ycxx_allocation_functions'
DARWIN = sys.platform == 'darwin'


def image_kind(path):
    """'exe', 'shared', 'archive' or None."""
    try:
        with open(path, 'rb') as f:
            head = f.read(20)
    except OSError:
        return None
    if head.startswith(b'!<arch>\n'):
        return 'archive' if path.endswith('.a') else None
    if head[:4] == b'\x7fELF':
        et = int.from_bytes(head[16:18], 'little' if head[5] == 1 else 'big')
        if et == 2:
            return 'exe'
        if et == 3:
            return 'shared' if re.search(r'\.so(\.|$)', path) else 'exe'
        return None
    magic = int.from_bytes(head[:4], 'little')
    if magic in (0xfeedfacf, 0xfeedface):
        ft = int.from_bytes(head[12:16], 'little')
        return {2: 'exe', 6: 'shared', 8: 'shared'}.get(ft)
    return None


def run(argv):
    r = subprocess.run(argv, capture_output=True, text=True, errors='replace')
    return r.stdout


def needed_libs(path):
    if DARWIN:
        lines = run(['otool', '-L', path]).splitlines()[1:]
        return [l.strip().split(' (')[0] for l in lines if l.strip()]
    return re.findall(r'\(NEEDED\)\s+Shared library: \[([^\]]+)\]', run(['readelf', '-d', path]))


def symbols(path, kind):
    """(raw, demangled) symbol lines of every symbol, defined and undefined."""
    if DARWIN:
        raw, dem = run(['nm', '-m', path]), run(['nm', '-mC', path])
    else:
        raw, dem = run(['nm', '-a', path]), run(['nm', '-aC', path])
        if kind != 'archive':
            # Stripped images have only the dynamic table; read it too (versions show there).
            raw += run(['nm', '-D', '--with-symbol-versions', path])
            dem += run(['nm', '-DC', path])
    return raw, dem


_SAN_SYMBOLS = None


def sanitizer_runtime_symbols():
    """The C++ names the sanitizer runtimes define ($YCXX_RW_CONDITIONS names the compiler and the
    sanitizers: "clang asan"; tools/realworld sets it). Clang links them statically into every image,
    a C program's too, so they do not make an image a C++ one (GCC's runtimes are shared)."""
    global _SAN_SYMBOLS
    if _SAN_SYMBOLS is None:
        _SAN_SYMBOLS = set()
        cond = set(os.environ.get('YCXX_RW_CONDITIONS', '').split())
        if 'clang' in cond and cond & {'asan', 'tsan', 'ubsan'}:
            cxx = shlex.split(os.environ.get('YCXX_CLANGXX', 'clang++-23'))
            d = run(cxx + ['-print-runtime-dir']).strip()
            for a in sorted(glob.glob(os.path.join(d, 'libclang_rt.*.a'))):
                if re.search(r'san|ubsan|interception', os.path.basename(a)):
                    for line in run(['nm', '--defined-only', a]).splitlines():
                        f = line.split()
                        if len(f) == 3 and f[2].startswith(('_Z', '__Z')):
                            _SAN_SYMBOLS.add(f[2])
    return _SAN_SYMBOLS


def defines_marker(path):
    """'exported' when the image exports libycxx's allocation table, 'local' when it defines it but
    a version script or export list keeps it local (libtbb.so's), else ''."""
    if DARWIN:
        if re.search(r'\s_' + MARKER + r'$', run(['nm', '-gU', path]), re.M):
            return 'exported'
        return 'local' if re.search(r'\s[a-zA-Z]\s_' + MARKER + r'$', run(['nm', '-U', path]), re.M) else ''
    if re.search(r'\s' + MARKER + r'$', run(['nm', '-D', '--defined-only', path]), re.M):
        return 'exported'
    return 'local' if re.search(r'\s' + MARKER + r'$', run(['nm', '--defined-only', path]), re.M) else ''


def check_image(path, kind, linked_by_driver):
    """(problems, evidence) for one image."""
    probs, ev = [], {}
    if kind != 'archive':
        libs = needed_libs(path)
        ev['needed'] = libs
        for l in libs:
            if FORBIDDEN_LIB.search(l):
                probs.append(f'needs {l}')
    raw, dem = symbols(path, kind)
    bad = sorted({m.group(0) for m in FORBIDDEN_MANGLED.finditer(raw)} |
                 {l.split(' ', 2)[-1].strip() for l in dem.splitlines() if FORBIDDEN_DEMANGLED.search(l)})
    if bad:
        probs.append(f'{len(bad)} symbols of libstdc++ or libc++, e.g. ' + '; '.join(bad[:3]))
    ev['ycxx_symbols'] = sum(1 for l in dem.splitlines() if '__ycxx::' in l)
    san = sanitizer_runtime_symbols()
    ev['cxx_symbols'] = sum(1 for m in re.finditer(r'\s(_?_Z[^\s@]*)', raw) if m.group(1) not in san)
    if kind != 'archive':
        has = defines_marker(path)
        ev['marker'] = has
        if linked_by_driver and not has:
            probs.append(f'does not define libycxx\'s {MARKER}')
        if not linked_by_driver and ev['cxx_symbols'] and not has:
            probs.append('a C++ image not linked by tools/ycxx-cxx')
    return probs, ev


def find_images(build):
    out = []
    for root, dirs, files in os.walk(build):
        # CMakeFiles/<version>/ holds CMake's compiler identification and ABI probes, not the
        # project's images: the C probes are built by the C compiler, and with a sanitizer its
        # static runtime gives them C++ symbols.
        dirs[:] = [d for d in dirs if d not in ('.git', '.ycxx-cmds') and not d.endswith('-subbuild')
                   and not (os.path.basename(root) == 'CMakeFiles' and re.fullmatch(r'\d+\.\d+\.\d+\S*', d))]
        for f in files:
            p = os.path.join(root, f)
            if os.path.islink(p) or f.endswith(('.o', '.obj')):
                continue
            k = image_kind(p)
            if k:
                out.append((p, k))
    return sorted(out)
