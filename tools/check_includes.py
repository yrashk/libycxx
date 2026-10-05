#!/usr/bin/env python3
"""Include-graph check: a core header must never reach a hosted header or a C library header.

Walks #include directives transitively starting from each core public header. Allowed targets:
other core public headers, include/ycxx/config.hpp, include/ycxx/core/**, include/ycxx/pal.h.
<stddef.h>, the compiler's own (for ::max_align_t, DECISIONS §3), is the one compiler header allowed.
Anything else (hosted public headers, include/ycxx/hosted/**, <stdio.h>, other compiler headers...)
is an error, reported with the include chain. The headers with a freestanding subset
(FREESTANDING_SUBSET) are walked too; directives in the YCXX_HOSTED branch of an
`#if YCXX_HOSTED` / `#if !YCXX_HOSTED` conditional are not followed (they are not reached
freestanding).
"""
import pathlib, re, sys
sys.path.insert(0, str(pathlib.Path(__file__).parent))
from headers import CORE, HOSTED, ABI, FREESTANDING_SUBSET

ROOT = pathlib.Path(__file__).resolve().parent.parent / "include"
INC = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')
COND = re.compile(r'^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b\s*(.*)')

def freestanding_includes(text):
    """The #include targets of text that a freestanding (YCXX_HOSTED 0) build reaches."""
    stack = []  # per open conditional: whether its current branch is hosted-only
    out = []
    for line in text.splitlines():
        m = COND.match(line)
        if m:
            kw, arg = m.group(1), m.group(2).split("//")[0].strip()
            if kw in ("if", "ifdef", "ifndef"):
                stack.append(True if arg == "YCXX_HOSTED" else False if arg == "!YCXX_HOSTED" else None)
            elif kw == "else" and stack and stack[-1] is not None:
                stack[-1] = not stack[-1]
            elif kw == "elif" and stack:
                stack[-1] = None
            elif kw == "endif" and stack:
                stack.pop()
            continue
        if any(b is True for b in stack):
            continue
        m = INC.match(line)
        if m:
            out.append(m.group(1))
    return out

def allowed(name):
    return (name == "stddef.h" or name in CORE or name in ABI or name in FREESTANDING_SUBSET or name == "ycxx/config.hpp"
            or name == "ycxx/pal.h" or name.startswith("ycxx/core/"))

errors = []
for top in CORE + FREESTANDING_SUBSET:
    seen, stack = set(), [(top, [top])]
    while stack:
        name, chain = stack.pop()
        if name in seen:
            continue
        seen.add(name)
        if not allowed(name):
            errors.append(f"<{top}> reaches <{name}> via " + " -> ".join(chain))
            continue
        if name == "stddef.h":
            continue  # the compiler's header, not libycxx's
        path = ROOT / name
        if not path.exists():
            errors.append(f"<{top}>: <{name}> not found (via {' -> '.join(chain)})")
            continue
        for inc in freestanding_includes(path.read_text()):
            stack.append((inc, chain + [inc]))
for e in errors:
    print(e)
print(f"include-graph check: {len(CORE) + len(FREESTANDING_SUBSET)} core/freestanding-subset headers, {len(errors)} violations")
sys.exit(1 if errors else 0)
