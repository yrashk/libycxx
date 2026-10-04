#!/usr/bin/env python3
"""Include-graph check: a core header must never reach a hosted header or a C library header.

Walks #include directives transitively starting from each core public header. Allowed targets:
other core public headers, include/ycxx/config.hpp, include/ycxx/core/**, include/ycxx/pal.h.
Anything else (hosted public headers, include/ycxx/hosted/**, <stdio.h>, compiler headers...)
is an error, reported with the include chain.
"""
import pathlib, re, sys
sys.path.insert(0, str(pathlib.Path(__file__).parent))
from headers import CORE, HOSTED, ABI

ROOT = pathlib.Path(__file__).resolve().parent.parent / "include"
INC = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.M)

def allowed(name):
    return (name in CORE or name in ABI or name == "ycxx/config.hpp" or name == "ycxx/pal.h"
            or name.startswith("ycxx/core/"))

errors = []
for top in CORE:
    seen, stack = set(), [(top, [top])]
    while stack:
        name, chain = stack.pop()
        if name in seen:
            continue
        seen.add(name)
        if not allowed(name):
            errors.append(f"<{top}> reaches <{name}> via " + " -> ".join(chain))
            continue
        path = ROOT / name
        if not path.exists():
            errors.append(f"<{top}>: <{name}> not found (via {' -> '.join(chain)})")
            continue
        for inc in INC.findall(path.read_text()):
            stack.append((inc, chain + [inc]))
for e in errors:
    print(e)
print(f"include-graph check: {len(CORE)} core headers, {len(errors)} violations")
sys.exit(1 if errors else 0)
