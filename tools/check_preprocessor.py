#!/usr/bin/env python3
"""Enforce the preprocessor policy (DECISIONS.md section 1).

- Only include/ycxx/config.hpp may inspect compiler/target macros.
- #define is allowed only in config.hpp and in files listed in MANDATED_MACRO_FILES (which
  define standard-mandated macros such as INT_MAX or __cpp_lib_*).
- Elsewhere, #if/#ifdef/#ifndef/#elif may only test YCXX_HAS_* / YCXX_* switches.
- Every header starts with #pragma once.
"""
import pathlib, re, sys

ROOT = pathlib.Path(__file__).resolve().parent.parent / "include"
CONFIG = ROOT / "ycxx/config.hpp"
# Files whose job is to define macros the standard mandates.
MANDATED_MACRO_FILES = {
    "ycxx/core/cstdint.hpp", "ycxx/core/climits.hpp", "ycxx/core/cstddef.hpp",
    "ycxx/core/version.hpp", "cassert", "ycxx/core/cfloat.hpp", "ycxx/pal.h", "cwchar", "cuchar",
    "ycxx/core/cmath.hpp", "ycxx/core/cmath_c_macros.hpp", "math.h",
}
# Headers the standard requires to be re-includable with different effect.
REINCLUDABLE = {"cassert"}
COND = re.compile(r"^\s*#\s*(if|ifdef|ifndef|elif|elifdef|elifndef)\b(.*)")
DEFINE = re.compile(r"^\s*#\s*(define|undef)\s+(\w+)")
SWITCH = re.compile(r"^\s*!?\s*(YCXX_[A-Z0-9_]+)(\s*(&&|\|\|)\s*!?\s*YCXX_[A-Z0-9_]+)*\s*$")

errors = []
for path in sorted(p for p in ROOT.rglob("*") if p.is_file()):
    rel = path.relative_to(ROOT).as_posix()
    text = path.read_text()
    if path == CONFIG:
        continue
    if "#pragma once" not in text and not rel.endswith(".h") and rel not in REINCLUDABLE:
        errors.append(f"{rel}: missing #pragma once")
    mandated = rel in MANDATED_MACRO_FILES
    for n, line in enumerate(text.splitlines(), 1):
        m = DEFINE.match(line)
        if m and not mandated:
            errors.append(f"{rel}:{n}: #{m.group(1)} {m.group(2)} outside config.hpp / mandated-macro files")
        m = COND.match(line)
        if m and not mandated:
            if not SWITCH.match(m.group(2).split("//")[0]):
                errors.append(f"{rel}:{n}: preprocessor conditional must test only YCXX_* switches: {line.strip()}")

for e in errors:
    print(e)
sys.exit(1 if errors else 0)
