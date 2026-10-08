#!/usr/bin/env python3
"""Enforce the preprocessor policy (DECISIONS.md section 1).

- Only include/ycxx/config.hpp may inspect compiler/target macros.
- #define is allowed only in config.hpp and in files listed in MANDATED_MACRO_FILES (which
  define standard-mandated macros such as INT_MAX or __cpp_lib_*).
- Elsewhere, #if/#ifdef/#ifndef/#elif may only test _YCXX_HAS_* / _YCXX_* switches.
- Every header starts with #pragma once.
- The transitive includes (DECISIONS §19): _YCXX_TRANSITIVE_INCLUDES (config.hpp's form of the
  user's YCXX_NO_TRANSITIVE_INCLUDES) is tested by exactly one pattern, the block at the end of a
  public header that tools/gen_transitive_includes.py writes: `#if _YCXX_TRANSITIVE_INCLUDES`,
  #include lines of public headers, at most one nested `#if _YCXX_HOSTED` ... `#endif` of the same,
  `#endif`, and nothing after it. Nothing else may test it, and no other file may name it.
- The mode switch (DECISIONS §20.3): outside config.hpp, the user's YCXX_SHARED is never named,
  and _YCXX_VISIBILITY appears only as the argument of a visibility attribute,
  `[[__gnu__::__visibility__(_YCXX_VISIBILITY)]]` (include/ and src/).
"""
import pathlib, re, sys

ROOT = pathlib.Path(__file__).resolve().parent.parent / "include"
CONFIG = ROOT / "ycxx/config.hpp"
# Files whose job is to define macros the standard mandates.
MANDATED_MACRO_FILES = {
    "ycxx/core/cstdint.hpp", "ycxx/core/climits.hpp", "ycxx/core/cstddef.hpp",
    "ycxx/core/version.hpp", "cassert", "ycxx/core/cfloat.hpp", "ycxx/pal.h", "cwchar", "cuchar",
    "atomic", "stdatomic.h",
    "ycxx/core/cmath.hpp", "ycxx/core/cmath_c_macros.hpp", "math.h",
    "cstring", "cstdio", "ctime", "cinttypes", "csetjmp", "ycxx/core/cstdarg.hpp", "ycxx/core/c_stdlib.hpp",
    "ycxx/core/cerrno_macros.hpp", "stdbit.h", "stdckdint.h",
    # C library headers wrapped for C and C++ (#ifdef __cplusplus), and the <c...> headers that
    # rename C declarations while reading the C library's (tools/gen_cheaders.py, DECISIONS §3).
    "stdlib.h", "inttypes.h", "string.h", "wchar.h", "time.h", "uchar.h", "stddef.h", "complex.h", "tgmath.h",
    "cstdlib", "ycxx/hosted/c_wchar.hpp",
}
# Headers the standard requires to be re-includable with different effect.
REINCLUDABLE = {"cassert"}
TRANSITIVE = "_YCXX_TRANSITIVE_INCLUDES"
TINC = re.compile(r"^#(  |    )include <([^<>]+)>$")


def check_transitive_block(rel, lines, errors):
    """The one sanctioned use of _YCXX_TRANSITIVE_INCLUDES (DECISIONS §19)."""
    uses = [n for n, line in enumerate(lines) if not line.lstrip().startswith("//")
            and (TRANSITIVE in line or "YCXX_NO_TRANSITIVE_INCLUDES" in line)]
    if not uses:
        return
    if rel.startswith("ycxx/") or "/" in rel:
        errors.append(f"{rel}:{uses[0] + 1}: {TRANSITIVE} outside a public header's transitive-include block")
        return
    start = uses[0]
    if len(uses) > 1 or lines[start] != f"#if {TRANSITIVE}":
        errors.append(f"{rel}:{start + 1}: {TRANSITIVE} may only be tested by one `#if {TRANSITIVE}` block")
        return
    hosted = None
    for n in range(start + 1, len(lines)):
        line = lines[n]
        m = TINC.match(line)
        if m and (m.group(1) == "  ") == (hosted is None or hosted is False):
            if not (ROOT / m.group(2)).is_file() or m.group(2).startswith("ycxx/"):
                errors.append(f"{rel}:{n + 1}: a transitive include must name a public header: {line}")
        elif line == "#  if _YCXX_HOSTED" and hosted is None:
            hosted = True
        elif line == "#  endif" and hosted is True:
            hosted = False
        elif line == "#endif" and hosted is not True:
            if any(l.strip() for l in lines[n + 1:]):
                errors.append(f"{rel}:{n + 2}: nothing may follow the transitive-include block")
            return
        else:
            errors.append(f"{rel}:{n + 1}: only #include lines (and one nested #if _YCXX_HOSTED) may "
                          f"appear in the transitive-include block: {line}")
            return
    errors.append(f"{rel}:{start + 1}: unterminated transitive-include block")


COND = re.compile(r"^\s*#\s*(if|ifdef|ifndef|elif|elifdef|elifndef)\b(.*)")
DEFINE = re.compile(r"^\s*#\s*(define|undef)\s+(\w+)")
SWITCH = re.compile(r"^\s*!?\s*(_YCXX_[A-Z0-9_]+)(\s*(&&|\|\|)\s*!?\s*_YCXX_[A-Z0-9_]+)*\s*$")

errors = []
for path in sorted(p for p in ROOT.rglob("*") if p.is_file()):
    rel = path.relative_to(ROOT).as_posix()
    text = path.read_text()
    if path == CONFIG:
        continue
    if "#pragma once" not in text and not rel.endswith(".h") and rel not in REINCLUDABLE:
        errors.append(f"{rel}: missing #pragma once")
    mandated = rel in MANDATED_MACRO_FILES
    check_transitive_block(rel, text.splitlines(), errors)
    for n, line in enumerate(text.splitlines(), 1):
        m = DEFINE.match(line)
        if m and not mandated:
            errors.append(f"{rel}:{n}: #{m.group(1)} {m.group(2)} outside config.hpp / mandated-macro files")
        m = COND.match(line)
        if m and not mandated:
            if not SWITCH.match(m.group(2).split("//")[0]):
                errors.append(f"{rel}:{n}: preprocessor conditional must test only _YCXX_* switches: {line.strip()}")

VIS_USE = "__gnu__::__visibility__(_YCXX_VISIBILITY)"
SRC = ROOT.parent / "src"
for path in sorted([p for p in ROOT.rglob("*") if p.is_file()] +
                   [p for p in SRC.rglob("*") if p.suffix in (".cpp", ".hpp", ".c", ".h")]):
    if path == CONFIG:
        continue
    rel = path.relative_to(ROOT.parent).as_posix()
    for n, line in enumerate(path.read_text().splitlines(), 1):
        code = line.split("//")[0]
        if "_YCXX_VISIBILITY" in code.replace(VIS_USE, ""):
            errors.append(f"{rel}:{n}: _YCXX_VISIBILITY may only be the argument of a visibility attribute: {VIS_USE}")
        if re.search(r"(?<![A-Z_])YCXX_SHARED\b", code):
            errors.append(f"{rel}:{n}: YCXX_SHARED is read only by ycxx/config.hpp (cfg::__shared elsewhere)")

for e in errors:
    print(e)
sys.exit(1 if errors else 0)
