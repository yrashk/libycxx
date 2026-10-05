#!/usr/bin/env python3
"""Enforce hidden visibility for libycxx's namespaces (DECISIONS.md section 2).

Every namespace-scope opening of `std`, `ycxx` or `__cxxabiv1` in include/ must read
`namespace [[gnu::visibility("hidden")]] NAME {`. The attribute applies only to the block it is
written on, so a reopening without it would emit default-visibility (exported) symbols; and a
nested namespace definition (`namespace std::ranges {`) cannot carry attributes, so it is spelled
`namespace [[gnu::visibility("hidden")]] std { namespace ranges {` and closed with `}}`.

  tools/check_visibility.py          report unannotated openings
  tools/check_visibility.py --fix    rewrite them (and the matching closing braces) in place
"""
import pathlib, re, sys

ROOT = pathlib.Path(__file__).resolve().parent.parent / "include"
ATTR = '[[gnu::visibility("hidden")]]'
# An unannotated file-scope opening: `namespace std {`, `namespace ycxx::detail::x {`, ...
OPEN = re.compile(r'^namespace ((?:std|ycxx|__cxxabiv1)\b)((?:::\w+)*) \{')
PREFIXES = ("u8R", "uR", "UR", "LR", "R", "u8", "u", "U", "L")


def scan_braces(text):
    """Yields (offset, char) for every '{' and '}' outside comments and literals."""
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            i = text.find("\n", i)
            i = n if i < 0 else i
        elif text.startswith("/*", i):
            i = text.index("*/", i + 2) + 2
        elif c.isdigit() or (c == "." and i + 1 < n and text[i + 1].isdigit()):
            # pp-number: digit separators (1'000) are not character literals
            i += 1
            while i < n and (text[i].isalnum() or text[i] in "_.'" or
                             (text[i] in "+-" and text[i - 1] in "eEpP")):
                i += 1
        elif c.isalpha() or c == "_":
            j = i
            while j < n and (text[j].isalnum() or text[j] == "_"):
                j += 1
            word = text[i:j]
            if j < n and text[j] == '"' and word in PREFIXES and "R" in word:
                delim = text[j + 1:text.index("(", j)]  # raw string literal
                i = text.index(")" + delim + '"', j) + len(delim) + 2
            elif j < n and text[j] in "\"'" and word in PREFIXES:
                i = j  # encoding prefix: the literal follows
            else:
                i = j
        elif c in "\"'":
            j = i + 1
            while text[j] != c:
                j += 2 if text[j] == "\\" else 1
            i = j + 1
        else:
            if c in "{}":
                yield i, c
            i += 1


def fix(text):
    """Returns (new text, number of openings rewritten)."""
    starts = {}  # offset of an unannotated opening's '{' -> (line start, name, rest)
    pos = 0
    for line in text.splitlines(keepends=True):
        m = OPEN.match(line)
        if m:
            starts[pos + m.end() - 1] = (pos, m.group(1), m.group(2))
        pos += len(line)
    if not starts:
        return text, 0
    edits = []  # (offset, length, replacement)
    stack = []
    for off, c in scan_braces(text):
        if c == "{":
            stack.append(off)
        else:
            o = stack.pop()
            if o in starts:
                line, name, rest = starts[o]
                if rest:
                    opening = f"namespace {ATTR} {name} {{ namespace {rest[2:]} {{"
                    edits.append((off, 1, "}}"))
                else:
                    opening = f"namespace {ATTR} {name} {{"
                edits.append((line, o + 1 - line, opening))
    assert not stack, "unbalanced braces"
    for off, length, new in sorted(edits, reverse=True):
        text = text[:off] + new + text[off + length:]
    return text, len(starts)


def main():
    apply = "--fix" in sys.argv[1:]
    errors = []
    for path in sorted(p for p in ROOT.rglob("*") if p.is_file()):
        text = path.read_text()
        rel = path.relative_to(ROOT).as_posix()
        if apply:
            new, count = fix(text)
            if count:
                path.write_text(new)
                print(f"{rel}: {count} namespace openings annotated")
            continue
        for n, line in enumerate(text.splitlines(), 1):
            if OPEN.match(line):
                errors.append(f"{rel}:{n}: namespace opening without {ATTR} "
                              "(tools/check_visibility.py --fix)")
    for e in errors:
        print(e, file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
