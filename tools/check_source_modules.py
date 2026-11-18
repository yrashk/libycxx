#!/usr/bin/env python3
"""Check libycxx's own module units (src/**/*.cppm, DECISIONS.md section 20) for ABI changes.

A module unit of the library defines what a plain translation unit would: its definitions are in
the global module fragment or in an `extern "C++"` block, so they stay attached to the global
module. A definition attached to the named module is mangled with it (`f@ycxx.hosted.print()`)
and would change libycxx's ABI.

  tools/check_source_modules.py <libycxx.a>
      fails if the archive defines a module-attached symbol (other than a module initializer)
  tools/check_source_modules.py <libycxx.a, YCXX_SOURCE_MODULES=ON> <libycxx.a, =OFF>
      also fails unless both define the same exported (default-visibility) symbols: a unit hides
      its module initializer (`asm((__hide_symbol("_ZGIW4ycxx...")))`)

The archives come from builds of one compiler with the same options but YCXX_SOURCE_MODULES.
The tools are those of the toolchain: nm (Mach-O) or readelf (ELF), and c++filt; $YCXX_LLVM_AR's
directory is searched first.
"""
import os, pathlib, re, shutil, subprocess, sys


def tool(*names):
    dirs = []
    if os.environ.get("YCXX_LLVM_AR"):
        dirs.append(str(pathlib.Path(os.environ["YCXX_LLVM_AR"]).parent))
    for name in names:
        for d in dirs:
            if (pathlib.Path(d) / name).exists():
                return str(pathlib.Path(d) / name)
        if shutil.which(name):
            return shutil.which(name)
    sys.exit(f"check_source_modules: none of {', '.join(names)} found")


def is_macho(archive):
    out = subprocess.run([tool("llvm-nm", "nm"), "-m", archive], capture_output=True, text=True)
    return out.returncode == 0 and "(__TEXT," in out.stdout


def defined_symbols(archive):
    """{name: exported} for every global symbol the archive defines (exported: default visibility)."""
    syms = {}
    if is_macho(archive):
        out = subprocess.run([tool("llvm-nm", "nm"), "-m", archive], capture_output=True, text=True, check=True).stdout
        for line in out.splitlines():
            if "(undefined)" in line or "external" not in line or "non-external" in line:
                continue
            name = line.split()[-1]
            name = name[1:] if name.startswith("_") else name  # Mach-O's '_' prefix
            syms[name] = syms.get(name, False) or "private external" not in line
    else:
        out = subprocess.run([tool("llvm-readelf", "readelf"), "-sW", archive], capture_output=True, text=True,
                             check=True).stdout
        for line in out.splitlines():
            f = line.split()
            # Num: Value Size Type Bind Vis Ndx Name
            if len(f) < 8 or not f[0].endswith(":") or f[6] == "UND" or f[4] not in ("GLOBAL", "WEAK"):
                continue
            syms[f[7]] = syms.get(f[7], False) or f[5] in ("DEFAULT", "PROTECTED")
    return syms


def demangle(names):
    out = subprocess.run([tool("llvm-cxxfilt", "c++filt")], input="\n".join(names), capture_output=True, text=True,
                         check=True).stdout
    return dict(zip(names, out.splitlines()))


INITIALIZER = re.compile(r"^_ZGIW")


def main(argv):
    if len(argv) not in (2, 3):
        sys.exit(__doc__)
    errors = []
    mod = defined_symbols(argv[1])
    names = sorted(mod)
    for name, text in demangle(names).items():
        if "@" in text and not INITIALIZER.match(name):
            errors.append(f"module-attached symbol (put it in `extern \"C++\"`): {text} [{name}]")
    if len(argv) == 3:
        plain = defined_symbols(argv[2])
        exported_mod = {n for n, e in mod.items() if e}
        exported_plain = {n for n, e in plain.items() if e}
        for n in sorted(exported_mod - exported_plain):
            errors.append(f"exported by the module build only: {n}")
        for n in sorted(exported_plain - exported_mod):
            errors.append(f"exported by the plain build only: {n}")
    for e in errors:
        print(e)
    if not errors:
        print(f"check_source_modules: {argv[1]}: no module-attached symbols"
              + (f"; the same exported symbols as {argv[2]}" if len(argv) == 3 else ""))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
