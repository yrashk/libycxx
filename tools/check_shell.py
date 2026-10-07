#!/usr/bin/env python3
"""Shell portability check for libycxx's POSIX sh scripts (tools/, tests/cmake/, tests/integration/,
cmake/*.in).

Flags a parameter expansion written without braces and directly followed by a non-ASCII
character ("$ui_dim│"): bash 3.2, macOS's /bin/sh, reads the bytes of a multibyte character in a
UTF-8 locale as part of the name, so the expansion names another, unset variable. Write
"${ui_dim}│" instead.
"""
import os, re, sys

repo = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BAD = re.compile(r'\$[A-Za-z_][A-Za-z0-9_]*[^\x00-\x7f]')


def scripts():
    for d in ('tools', 'tools/lib', 'tools/toolchain', 'tests/cmake', 'tests/integration', 'cmake', 'bench'):
        full = os.path.join(repo, d)
        if not os.path.isdir(full):
            continue
        for name in sorted(os.listdir(full)):
            p = os.path.join(full, name)
            if not os.path.isfile(p):
                continue
            if name.endswith('.sh'):
                yield p
                continue
            with open(p, 'rb') as f:
                first = f.readline()
            if first.startswith(b'#!') and b'sh' in first and b'python' not in first:
                yield p


bad = 0
for p in scripts():
    with open(p, encoding='utf-8', errors='replace') as f:
        for n, line in enumerate(f, 1):
            if BAD.search(line):
                print(f'{os.path.relpath(p, repo)}:{n}: unbraced expansion before a non-ASCII character: {line.strip()}')
                bad += 1
print(f'shell check: {bad} problem(s)')
sys.exit(1 if bad else 0)
