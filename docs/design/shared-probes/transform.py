#!/usr/bin/env python3
"""Probe-only source transformation (docs/design/shared-library.md): rewrites a COPY of libycxx's
include/ and src/ so that every file-scope `std` block opens the inline ABI namespace, and sets the
visibility of the `std` and `__ycxx` blocks for a static or a shared build.

  transform.py TREE static|shared [--plain FILE]

  static   std { inline namespace __y1 {  ... }}   hidden, as today (DECISIONS §2)
  shared   the same, with default visibility on the std and __ycxx blocks; __cxxabiv1 and the
           C-linkage ABI entry points keep their own hidden attributes

An opening whose line ends with `// y1:plain` (plain-std.patch adds those) stays in plain `std`
too.  --plain FILE lists `path:line` openings (relative to TREE, the line of the original opening) that
stay in plain `std` (the entities the compilers name there, docs/design/shared-library.md §4);
those blocks keep hidden visibility in both modes.  Never run on the repository itself: the real
change is written by hand (implementation plan, step 2).
"""
import pathlib, re, sys

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent.parent.parent / "tools"))
from check_visibility import scan_braces  # noqa: E402  (comment- and literal-aware brace scan)

HIDDEN = '[[__gnu__::__visibility__("hidden")]]'
DEFAULT = '[[__gnu__::__visibility__("default")]]'
OPEN = re.compile(r'^namespace \[\[__gnu__::__visibility__\("hidden"\)\]\] (std|__ycxx)\b')


def transform(text, mode, plain_lines):
    starts = {}  # offset of the opening's '{' -> (line start, line number, name)
    pos = 0
    for n, line in enumerate(text.splitlines(keepends=True), 1):
        m = OPEN.match(line)
        if m:
            brace = line.index("{", m.end())
            starts[pos + brace] = (pos, n, m.group(1), m.end())
        pos += len(line)
    if not starts:
        return text
    edits = []
    stack = []
    for off, c in scan_braces(text):
        if c == "{":
            stack.append(off)
            continue
        o = stack.pop()
        if o not in starts:
            continue
        line_start, n, name, name_end = starts[o]
        plain = n in plain_lines or text[line_start:text.index("\n", line_start)].rstrip().endswith("// y1:plain")
        vis = DEFAULT if mode == "shared" and not plain else HIDDEN
        head = f"namespace {vis} {name} {{"
        if name == "std" and not plain:
            head += " inline namespace __y1 {"
            edits.append((off, 1, "}}"))
        edits.append((line_start, o + 1 - line_start, head))
    for off, length, new in sorted(edits, reverse=True):
        text = text[:off] + new + text[off + length:]
    return text


def main():
    tree = pathlib.Path(sys.argv[1]).resolve()
    mode = sys.argv[2]
    assert mode in ("static", "shared")
    plain = {}
    if "--plain" in sys.argv:
        for entry in pathlib.Path(sys.argv[sys.argv.index("--plain") + 1]).read_text().split("\n"):
            entry = entry.split("#")[0].strip()
            if entry:
                path, line = entry.rsplit(":", 1)
                plain.setdefault(path, set()).add(int(line))
    count = 0
    for root in ("include", "src", "modules"):
        for p in sorted((tree / root).rglob("*")):
            if not p.is_file() or (root != "include" and p.suffix not in (".cpp", ".hpp", ".cppm")):
                continue
            rel = p.relative_to(tree).as_posix()
            text = p.read_text()
            new = transform(text, mode, plain.get(rel, set()))
            if new != text:
                p.write_text(new)
                count += 1
    print(f"transform.py: {count} files rewritten ({mode})")


if __name__ == "__main__":
    main()
