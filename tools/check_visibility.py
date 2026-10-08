#!/usr/bin/env python3
"""Enforce the visibility attributes and the inline ABI namespace (DECISIONS.md §2, §20).

Every namespace-scope opening of `std`, `__ycxx` or `__cxxabiv1` in include/ and in the C++ sources
of src/ must be one of these forms (the attributes' reserved spellings: the program may define
`gnu`, `visibility` and `hidden` as macros, DECISIONS §2):

  namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {
      every standard entity, in the inline ABI namespace (§20.4); closed by `}}`
  namespace [[__gnu__::__visibility__("hidden")]] std { // plain std (DECISIONS §20.5)
      the entities the compilers or the platform name in plain std, hidden in every mode. Such a
      block may declare only the names of tools/data/visibility.txt's [plain-std] section
  namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx {
      the library's internals
  namespace [[__gnu__::__visibility__("hidden")]] __ycxx {
      only in the per-image sources of tools/data/visibility.txt's [per-image] section (the
      default allocation functions and their table, the _Float16 type_info objects: hidden in every
      mode, §20.6), where every __ycxx block has this form
  namespace [[__gnu__::__visibility__("hidden")]] __cxxabiv1 {
      the ABI runtime's classes, hidden in every mode

_YCXX_VISIBILITY is "hidden" in static mode and "default" in shared mode (ycxx/config.hpp). The
attribute applies only to the block it is written on, so a reopening without it would emit
default-visibility (exported) symbols; and a nested namespace definition (`namespace std::ranges {`)
cannot carry attributes, so it is spelled `namespace [[...]] std { inline namespace __y1 { namespace
ranges {` and closed with `}}}`. The runtime's sources are not compiled with -fvisibility=hidden:
what they define outside these namespaces (C-linkage entry points) carries its own attribute.

  tools/check_visibility.py          report openings of another form, and plain-std blocks that
                                     declare names outside the allowlist
  tools/check_visibility.py --fix    rewrite the openings (and the matching closing braces) in place;
                                     plain-std blocks are left as they are
"""
import pathlib, re, sys

REPO = pathlib.Path(__file__).resolve().parent.parent
ROOTS = (REPO / "include", REPO / "src")
SOURCE_SUFFIXES = {".cpp", ".hpp"}  # under src/; include/ has extensionless headers too
DATA = REPO / "tools/data/visibility.txt"
HIDDEN = '[[__gnu__::__visibility__("hidden")]]'
MODE = '[[__gnu__::__visibility__(_YCXX_VISIBILITY)]]'
ABI_NS = "__y1"
PLAIN_MARK = "// plain std (DECISIONS §20.5)"
# Any file-scope opening of the three namespaces, with or without an attribute.
OPEN = re.compile(r'^namespace (?:\[\[__gnu__::__visibility__\((?P<vis>"hidden"|_YCXX_VISIBILITY)\)\]\] )?'
                  r'(?P<name>std|__ycxx|__cxxabiv1)\b(?P<nested>(?:::\w+)*) \{(?P<rest>.*)$')
PREFIXES = ("u8R", "uR", "UR", "LR", "R", "u8", "u", "U", "L")


def load_data():
    sections, cur = {}, None
    for line in DATA.read_text().splitlines():
        line = line.split("#")[0].strip()
        if not line:
            continue
        if line.startswith("[") and line.endswith("]"):
            cur = sections.setdefault(line[1:-1], set())
        else:
            cur.update(line.split())
    return sections


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


def strip_code(text):
    """The text with comments, string and character literals blanked."""
    out, i, n = [], 0, len(text)
    while i < n:
        if text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            out.append(" " * (j - i)); i = j
        elif text.startswith("/*", i):
            j = text.index("*/", i + 2) + 2
            out.append(re.sub(r"[^\n]", " ", text[i:j])); i = j
        elif text[i] in "\"'" and not (i and (text[i - 1].isalnum() or text[i - 1] == "_") and text[i] == "'"):
            q, j = text[i], i + 1
            while text[j] != q:
                j += 2 if text[j] == "\\" else 1
            out.append(" " * (j + 1 - i)); i = j + 1
        else:
            out.append(text[i]); i += 1
    return "".join(out)


KEYWORDS = {"if", "for", "while", "switch", "return", "sizeof", "alignof", "alignas", "decltype",
            "noexcept", "requires", "static_assert", "explicit", "operator", "typename", "template",
            "class", "struct", "union", "enum", "namespace", "using", "const", "constexpr",
            "consteval", "inline", "extern", "static", "noreturn", "default", "delete", "void",
            "auto", "this", "throw", "new", "concept", "virtual", "override", "final", "public",
            "private", "protected", "friend", "volatile", "mutable", "register", "thread_local",
            "char", "wchar_t", "char8_t", "char16_t", "char32_t", "bool", "short", "int", "long",
            "signed", "unsigned", "float", "double"}


def declared_names(block):
    """The names a namespace block declares at its own level (a conservative approximation)."""
    code = strip_code(block)
    flat, depth = [], 0
    for ch in code:
        if ch == "{":
            depth += 1
            flat.append("{" if depth == 1 else " ")
        elif ch == "}":
            flat.append("}" if depth == 1 else " ")
            depth -= 1
        elif ch == "\n":
            flat.append("\n")
        else:
            flat.append(ch if depth == 0 else " ")
    top = "".join(flat)
    top = re.sub(r"\[\[.*?\]\]", " ", top)
    top = re.sub(r"<[^<>;{}]*>", " ", top)  # template argument and parameter lists, one level
    top = re.sub(r"<[^<>;{}]*>", " ", top)
    names = set()
    for m in re.finditer(r"\b(?:class|struct|union|enum(?:\s+class|\s+struct)?|namespace|concept)\s+(\w+)", top):
        names.add(m.group(1))
    for m in re.finditer(r"\busing\s+(\w+)\s*=", top):
        names.add(m.group(1))
    for m in re.finditer(r"\busing\s+(?:::)?(?:\w+::)*(\w+)\s*;", top):
        names.add(m.group(1))
    if re.search(r"\boperator\b", top):
        names.add("operator")
    for m in re.finditer(r"\b(\w+)\s*(?:\(|\{|;|=)", top):
        w = m.group(1)
        if w not in KEYWORDS and not w.isdigit():
            names.add(w)
    return names


def per_image(rel, data):
    return any(rel == p or (p.endswith("/") and rel.startswith(p)) for p in data.get("per-image", ()))


def wanted(name, nested, plain, rel, data):
    """The opening a block of `name` must have, as (attribute, inline namespace?)."""
    if name == "__cxxabiv1":
        return HIDDEN, False
    if name == "std":
        return (HIDDEN, False) if plain else (MODE, True)
    return (HIDDEN if per_image(rel, data) else MODE), False


def process(text, rel, data, apply):
    """Returns (new text, errors)."""
    lines = text.splitlines(keepends=True)
    starts = {}  # offset of an opening's '{' -> (line start, line number, match)
    pos = 0
    for n, line in enumerate(lines, 1):
        m = OPEN.match(line.rstrip("\n"))
        if m:
            starts[pos + line.index("{", m.end("nested"))] = (pos, n, m)
        pos += len(line)
    if not starts:
        return text, []
    errors, edits, stack = [], [], []
    for off, c in scan_braces(text):
        if c == "{":
            stack.append(off)
            continue
        o = stack.pop()
        if o not in starts:
            continue
        line_start, n, m = starts[o]
        name, nested, rest = m.group("name"), m.group("nested"), m.group("rest")
        plain = name == "std" and rest.strip() == PLAIN_MARK and not nested
        attr, inline = wanted(name, nested, plain, rel, data)
        has_inline = rest.lstrip().startswith(f"inline namespace {ABI_NS} {{")
        current = HIDDEN if m.group("vis") == '"hidden"' else MODE if m.group("vis") else None
        if plain:
            if current != HIDDEN:
                errors.append(f"{rel}:{n}: a plain-std block is {HIDDEN}")
            body = text[o + 1:off]
            extra = declared_names(body) - data.get("plain-std", set())
            if extra:
                errors.append(f"{rel}:{n}: plain-std block declares {', '.join(sorted(extra))}, not in "
                              f"tools/data/visibility.txt [plain-std] (DECISIONS §20.5)")
            continue
        if current == attr and not nested and (has_inline == inline):
            continue
        if not apply:
            want = f"namespace {attr} {name} {{" + (f" inline namespace {ABI_NS} {{" if inline else "")
            errors.append(f"{rel}:{n}: namespace opening must read `{want}` (tools/check_visibility.py --fix)")
            continue
        # Rewrite: `namespace <attr> name { [inline namespace __y1 {] [namespace nested {...] <rest>`.
        head = f"namespace {attr} {name} {{"
        closers = 0
        if inline and not has_inline:
            head += f" inline namespace {ABI_NS} {{"
            closers += 1
        if not inline and has_inline:
            errors.append(f"{rel}:{n}: `{name}` cannot open the inline namespace {ABI_NS}; fix by hand")
            continue
        for part in nested[2:].split("::") if nested else ():
            head += f" namespace {part} {{"
            closers += 1
        end = line_start + len(m.group(0)) - len(rest)  # just past the opening's '{'
        edits.append((off, 1, "}" * (closers + 1)))
        edits.append((line_start, end - line_start, head))
    for off, length, new in sorted(edits, reverse=True):
        text = text[:off] + new + text[off + length:]
    return text, errors


def fix(text, rel="include/generated"):
    """For the header generators: (text with its openings in the required form, errors)."""
    new, errors = process(text, rel, load_data(), True)
    if errors:
        raise SystemExit("\n".join(errors))
    return new, errors


def main():
    apply = "--fix" in sys.argv[1:]
    data = load_data()
    errors = []
    paths = [p for p in ROOTS[0].rglob("*") if p.is_file()]
    paths += [p for p in ROOTS[1].rglob("*") if p.is_file() and p.suffix in SOURCE_SUFFIXES]
    for path in sorted(paths):
        text = path.read_text()
        rel = path.relative_to(REPO).as_posix()
        new, errs = process(text, rel, data, apply)
        errors += errs
        if apply and new != text:
            path.write_text(new)
            print(f"{rel}: namespace openings rewritten")
    for e in errors:
        print(e, file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
