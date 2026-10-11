#!/usr/bin/env python3
"""The shared library's export list against its committed baseline (DECISIONS §20.7, step 9).

libycxx.so.0.<minor> (libycxx.0.<minor>.dylib) exports what the visibility attributes and
src/export.txt let out. abi/<os>-<arch>-<compiler>.abilist records that list for one platform and
compiler: one line per exported symbol, its kind (F function, O object, T thread-local, W weak
function, V weak object) and, for data, its size. A change to the exports (a symbol added, removed
or changed in kind or size) must come with the updated baseline in the same commit, so that every
ABI change is visible in review; during 0.x such changes are allowed (no stability promise), but
never unnoticed.

Every export must also be libycxx's own: std::__y1, __ycxx, the runtime's entry points
(__ycxx_abi_*), the allocation table and the shared marker (src/export.txt); anything else fails.

    tools/check_abi.py [--check] [gcc] [clang]   compare build/<compiler>-shared's library with its
                                                 baseline (tools/test's policy stage; the build
                                                 stage with --shared after building it). A compiler
                                                 without such a build, or a baseline made with
                                                 another compiler version, is skipped with a notice
    tools/check_abi.py --update [gcc] [clang]    write the baselines from the builds
    tools/check_abi.py --library PATH --compiler gcc|clang [--check|--update]
                                                 the same for a library elsewhere (an installation)

The baselines come from build/<compiler>-shared (RelWithDebInfo, as tools/test builds it): the
inline functions and template instantiations a library exports depend on the optimization level.
"""
import os
import platform
import re
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DARWIN = sys.platform == 'darwin'
OWN = re.compile(r'St4__y1|6__ycxx|^__ycxx_abi_|^__ycxx_allocation_functions$|^__ycxx_linkage_shared_v1$')


def run(argv):
    return subprocess.run(argv, capture_output=True, text=True, errors='replace', check=True).stdout


def library_of(build):
    name = 'libycxx.0.1.dylib' if DARWIN else 'libycxx.so.0.1'
    path = os.path.join(build, name)
    return path if os.path.exists(path) else None


def exports(lib):
    """{symbol: 'K[ size]'} of the library's defined, exported symbols."""
    out = {}
    if DARWIN:
        # nm -m: "(__TEXT,__text) [weak] external [automatically hidden] _name"
        for line in run(['nm', '-m', '-U', lib]).splitlines():
            m = re.search(r'\((__\w+),(__\w+)\) (weak )?external (?:\[[^]]*\] )?_(\S+)$', line)
            if not m or 'non-external' in line:
                continue
            seg, sect, weak, name = m.groups()
            data = seg != '__TEXT' or sect not in ('__text', '__stubs')
            tlv = sect in ('__thread_vars', '__thread_data', '__thread_bss')
            kind = 'T' if tlv else ('V' if weak and data else 'W' if weak else 'O' if data else 'F')
            out[name] = kind
        return out
    for line in run(['readelf', '--dyn-syms', '-W', lib]).splitlines():
        f = line.split()
        if len(f) < 8 or not f[0].endswith(':') or f[6] == 'UND':
            continue
        size, typ, bind, vis, name = f[2], f[3], f[4], f[5], f[7].split('@')[0]
        if bind not in ('GLOBAL', 'WEAK', 'UNIQUE') or vis not in ('DEFAULT', 'PROTECTED') or typ in ('SECTION', 'FILE'):
            continue
        if re.fullmatch(r'YCXX_\d+\.\d+', name):
            continue  # the version node's own symbol
        weak = bind in ('WEAK', 'UNIQUE')
        if typ == 'TLS':
            kind = 'T'
        elif typ == 'OBJECT':
            kind = 'V' if weak else 'O'
        else:
            kind = 'W' if weak else 'F'
        size = int(size, 0) if size.startswith('0x') else int(size)
        out[name] = f'{kind} {size}' if kind in ('O', 'V', 'T') else kind
    return out


def compiler_version(compiler):
    cxx = os.environ.get('YCXX_GXX', 'g++-16') if compiler == 'gcc' else os.environ.get('YCXX_CLANGXX', 'clang++-23')
    try:
        return run([cxx, '-dumpfullversion' if compiler == 'gcc' else '-dumpversion']).strip()
    except (OSError, subprocess.CalledProcessError):
        return 'unknown'


def baseline_path(compiler):
    system = 'darwin' if DARWIN else platform.system().lower()
    machine = platform.machine().lower().replace('aarch64', 'arm64').replace('amd64', 'x86_64')
    return os.path.join(REPO, 'abi', f'{system}-{machine}-{compiler}.abilist')


def header(compiler, version):
    system = 'Darwin' if DARWIN else platform.system()
    return (f'# libycxx ABI export list (tools/check_abi.py --update; DECISIONS §20.7): {system} '
            f'{platform.machine()}, {compiler} {version}, build/{compiler}-shared (RelWithDebInfo).\n'
            '# One line per exported symbol: name, kind (F function, O object, T thread-local, W weak\n'
            '# function, V weak object) and, for data, the size in bytes.\n'
            f'# compiler: {compiler} {version}\n')


def read_baseline(path):
    entries, version = {}, None
    with open(path, encoding='utf-8') as f:
        for line in f:
            if line.startswith('# compiler: '):
                version = line.split(':', 1)[1].strip()
            if not line.strip() or line.startswith('#'):
                continue
            name, _, rest = line.rstrip('\n').partition(' ')
            entries[name] = rest
    return entries, version


def check_one(compiler, lib, update):
    """0 ok, 1 failed, 2 skipped (with a notice)."""
    syms = exports(lib)
    foreign = sorted(n for n in syms if not OWN.search(n))
    if foreign:
        print(f'check_abi: {compiler}: {lib} exports {len(foreign)} names that are not libycxx\'s own, e.g. '
              + ', '.join(foreign[:5]))
        return 1
    version = f'{compiler} {compiler_version(compiler)}'
    path = baseline_path(compiler)
    if update:
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'w', encoding='utf-8') as f:
            f.write(header(compiler, compiler_version(compiler)))
            for n in sorted(syms):
                f.write(f'{n} {syms[n]}\n')
        print(f'check_abi: {compiler}: wrote {os.path.relpath(path, REPO)} ({len(syms)} exports)')
        return 0
    if not os.path.exists(path):
        print(f'check_abi: {compiler}: no baseline {os.path.relpath(path, REPO)} for this platform '
              '(tools/check_abi.py --update writes it): not compared')
        return 2
    base, base_version = read_baseline(path)
    if base_version != version:
        print(f'check_abi: {compiler}: the baseline was made with {base_version}, this is {version}: not compared '
              '(tools/check_abi.py --update)')
        return 2
    added = sorted(set(syms) - set(base))
    removed = sorted(set(base) - set(syms))
    changed = sorted(n for n in set(syms) & set(base) if syms[n] != base[n])
    if not (added or removed or changed):
        print(f'check_abi: {compiler}: {len(syms)} exports, as {os.path.relpath(path, REPO)} records')
        return 0
    print(f'check_abi: {compiler}: the exports differ from {os.path.relpath(path, REPO)}: {len(added)} added, '
          f'{len(removed)} removed, {len(changed)} changed. An ABI change comes with its baseline in the same '
          'commit: tools/check_abi.py --update (DECISIONS §20.7)')
    for title, names, show in (('added', added, lambda n: f'{n} {syms[n]}'),
                               ('removed', removed, lambda n: f'{n} {base[n]}'),
                               ('changed', changed, lambda n: f'{n}: {base[n]} -> {syms[n]}')):
        for n in names[:40]:
            print(f'  {title}: {show(n)}')
        if len(names) > 40:
            print(f'  ... and {len(names) - 40} more {title}')
    return 1


def main(argv):
    update = '--update' in argv
    lib = None
    compilers = []
    i = 0
    while i < len(argv):
        a = argv[i]
        if a in ('--check', '--update'):
            pass
        elif a == '--library':
            i += 1
            lib = argv[i]
        elif a == '--compiler':
            i += 1
            compilers.append(argv[i])
        elif a in ('gcc', 'clang'):
            compilers.append(a)
        elif a in ('-h', '--help'):
            print(__doc__.strip())
            return 0
        else:
            print(f'check_abi: unknown argument {a!r} (--help)', file=sys.stderr)
            return 2
        i += 1
    if lib:
        if len(compilers) != 1:
            print('check_abi: --library needs one --compiler', file=sys.stderr)
            return 2
        return 1 if check_one(compilers[0], lib, update) == 1 else 0
    failed = 0
    for c in compilers or ['gcc', 'clang']:
        path = library_of(os.path.join(REPO, 'build', f'{c}-shared'))
        if path is None:
            print(f'check_abi: {c}: no build/{c}-shared with the shared library (tools/test --shared build): not compared')
            continue
        failed |= check_one(c, path, update) == 1
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
