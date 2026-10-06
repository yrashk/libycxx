#!/usr/bin/env python3
"""Keep libycxx's identifiers out of the program's way ([macro.names], [lex.name]; DECISIONS §2).

A translation unit that includes a standard library header may #define any identifier that is
not a keyword, not reserved to the implementation ([lex.name]/4: `__x`, `_X`) and not declared in
a standard library header ([macro.names]/1). libycxx's headers therefore spell every name of
their own (template and function parameters, locals, members that are not standard, internal
namespaces, helpers and macros) as a reserved identifier. This tool finds and renames them.

    tools/uglify.py                 rename, in place, every such identifier in include/, src/
                                    and the files that spell libycxx's names in other languages
                                    (CMake, the compiler wrapper); idempotent. The names
                                    renamed in include/ join tools/data/uglify/nasty-macros.txt,
                                    and the nasty-macros tests are regenerated (--gen-tests)
    tools/uglify.py --check         fail (exit 1) if include/ or src/'s headers declare or use
                                    an identifier that is neither reserved, standard, nor
                                    listed as user-facing (tools/check-all, policy stage)
    tools/uglify.py --list          print those identifiers with a count and a first location
    tools/uglify.py --map           print the renaming (old new) of the identifiers --list prints
    tools/uglify.py --stats         count include/'s identifiers by class
    tools/uglify.py --fetch-index   refresh tools/data/uglify/library-index.txt from the draft

The text of a file is split into preprocessing tokens; only identifier tokens are changed. Not
renamed: keywords, reserved identifiers, directive names, `#pragma` lines, header names,
string and character literals (so asm labels keep the C library's assembler names), standard
attribute-tokens, and the names that are allowed (below). In comments, only code is renamed:
backquoted spans that are not file paths, and `ycxx::`-qualified names and `ycxx_`/`YCXX_`
words outside them.

What is allowed (never renamed):
  - the names the standard library declares: the draft's index of library names
    (tools/data/uglify/library-index.txt), the names the std and std.compat modules export
    (modules/*.cppm, which tools/gen_std_module.py derives from the headers) and the names of
    tools/data/uglify/allowed.txt's [standard] section (declared by the draft but missing from
    its index: members, C library members and macros);
  - [attribute]: the standard attribute-tokens ([cpp.replace.general]/9 makes them reserved);
  - [user]: libycxx's documented user-facing names (YCXX_HARDENED, the -fno-exceptions error
    handler, ...);
  - [platform]: names libycxx's sources take from the C library or the operating system that
    no standard header declares.

The renaming (DECISIONS §2) depends only on the spelling:
  - `ycxx` -> `__ycxx`, `detail` -> `__detail`, any lowercase-initial name x -> `__x`
    (`first1` -> `__first1`, `size_` -> `__size_`, `ycxx_pal_wait` -> `__ycxx_pal_wait`);
  - a single capital letter X -> `_Xp` (`T` -> `_Tp`, `C` -> `_Cp`; `_C` and its kind are macros
    of some C libraries' <ctype.h>);
  - any other uppercase-initial name X -> `_X` (`Alloc` -> `_Alloc`, `T1` -> `_T1`,
    `YCXX_HAS_RTTI` -> `_YCXX_HAS_RTTI`);
  - inside an attribute, the `gnu` namespace and its attribute names: `[[gnu::cold]]` ->
    `[[__gnu__::__cold__]]`, `__attribute__((unused))` -> `__attribute__((__unused__))`;
  - a spelling listed in tools/data/uglify/avoid.txt (a keyword or builtin of the compilers, a
    macro of some platform's headers) is replaced by `__y_x` / `_Y_X`.
Because the new names are reserved, a second run changes nothing, and a merge that brings plain
names back is fixed by running the tool again.
"""
import argparse, html, pathlib, re, subprocess, sys, urllib.request

HERE = pathlib.Path(__file__).resolve().parent
REPO = HERE.parent
DATA = HERE / "data" / "uglify"

# ---------------------------------------------------------------------------------------------
# Lexing

_ID = re.compile(r"[A-Za-z_][A-Za-z_0-9]*")
_WS = re.compile(r"(?:[ \t\f\v]|\\\n)+")
_NUM = re.compile(r"\.?[0-9](?:[eEpP][+-]|'[0-9A-Za-z_]|[0-9A-Za-z_.])*")
_PUNCT = re.compile(r"<=>|->\*|\.\.\.|<<=|>>=|::|->|\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%^&|]=|##"
                    r"|[\[\](){}<>;:,.?~!=+\-*/%^&|#@$`\\]")
_STRPFX = re.compile(r"(?:u8|u|U|L)?R?")


def lex(text):
    """Splits C or C++ source into (kind, text) tokens whose texts concatenate to `text`.
    Kinds: ws, nl, comment, string, char, number, ident, punct, header, other. An identifier
    token gets kind 'directive' when it names a directive, 'pragma' on a #pragma line."""
    out = []
    i, n = 0, len(text)
    line_start = True     # nothing but whitespace and comments so far on this line
    pp_hash = False       # a '#' that starts a directive was the last token
    directive = None      # the current line's directive
    expect_header = False  # after #include (_next) or __has_include (
    while i < n:
        c = text[i]
        if c == "\n":
            out.append(("nl", c)); i += 1
            line_start, pp_hash, directive, expect_header = True, False, None, False
            continue
        m = _WS.match(text, i)
        if m:
            out.append(("ws", m.group())); i = m.end(); continue
        if text.startswith("//", i):
            j = i
            while True:
                k = text.find("\n", j)
                if k < 0:
                    k = n
                    break
                if text[k - 1] == "\\":
                    j = k + 1
                    continue
                break
            out.append(("comment", text[i:k])); i = k; continue
        if text.startswith("/*", i):
            k = text.find("*/", i + 2)
            k = n if k < 0 else k + 2
            out.append(("comment", text[i:k])); i = k; continue
        if expect_header and c in "<\"":
            close = ">" if c == "<" else "\""
            k = text.find(close, i + 1)
            nl = text.find("\n", i + 1)
            if k >= 0 and (nl < 0 or k < nl):
                out.append(("header", text[i:k + 1])); i = k + 1
                expect_header = line_start = False
                continue
        m = _STRPFX.match(text, i)
        j = m.end()
        if j < n and text[j] in "\"'" and (j > i or c in "\"'"):
            q = text[j]
            if "R" in m.group() and q == '"':
                p = text.find("(", j)
                delim = text[j + 1:p]
                k = text.find(")" + delim + '"', p) + len(delim) + 2
            else:
                k = j + 1
                while k < n and text[k] != q and text[k] != "\n":
                    k += 2 if text[k] == "\\" else 1
                k += 1
            s = _ID.match(text, k)
            if s:
                k = s.end()  # user-defined literal suffix
            out.append(("string" if q == '"' else "char", text[i:k])); i = k
            line_start = expect_header = False
            continue
        m = _NUM.match(text, i)
        if m:
            out.append(("number", m.group())); i = m.end(); line_start = False; continue
        m = _ID.match(text, i)
        if m:
            w = m.group()
            i = m.end()
            expect_header = False
            if pp_hash:
                out.append(("directive", w))
                directive, pp_hash = w, False
                expect_header = w in ("include", "include_next", "import", "embed")
            elif directive == "pragma":
                out.append(("pragma", w))
            else:
                out.append(("ident", w))
                if w in ("__has_include", "__has_include_next", "__has_embed"):
                    j = i
                    while j < n and text[j] in " \t":
                        j += 1
                    if j < n and text[j] == "(":
                        expect_header = True
            line_start = False
            continue
        m = _PUNCT.match(text, i)
        if m:
            p = m.group()
            pp_hash = p == "#" and line_start
            out.append(("punct", p)); i = m.end()
            if p != "(":
                expect_header = expect_header and p == "("
            line_start = False
            continue
        out.append(("other", c)); i += 1; line_start = False
    return out


# ---------------------------------------------------------------------------------------------
# Classification

KEYWORDS = set("""
alignas alignof asm auto bool break case catch char char8_t char16_t char32_t class concept const
consteval constexpr constinit const_cast continue contract_assert co_await co_return co_yield
decltype default delete do double dynamic_cast else enum explicit export extern false float for
friend goto if inline int long mutable namespace new noexcept nullptr operator private protected
public register reinterpret_cast requires return short signed sizeof static static_assert
static_cast struct switch template this thread_local throw true try typedef typeid typename union
unsigned using virtual void volatile wchar_t while
and and_eq bitand bitor compl not not_eq or or_eq xor xor_eq
final import module override post pre
defined
""".split())
# C keywords in the C sources (src/pal) and the headers they share with C (ycxx/pal.h).
C_KEYWORDS = {"restrict", "typeof", "typeof_unqual", "nullptr_t"}
# [dcl.attr]: the standard attribute-tokens. [cpp.replace.general]/9: no macro may have their
# names (likely and unlikely may be function-like macros, which an attribute does not expand).
STD_ATTRIBUTES = {"assume", "deprecated", "fallthrough", "indeterminate", "likely", "unlikely",
                  "maybe_unused", "nodiscard", "noreturn", "no_unique_address"}


def reserved(w):
    return "__" in w or (len(w) > 1 and w[0] == "_" and w[1].isupper())


def _read_list(path):
    """Names and `re:` patterns of a data file, by [section]; '#' starts a comment."""
    sections, cur = {}, "default"
    for line in path.read_text().splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        if line.startswith("[") and line.endswith("]"):
            cur = line[1:-1]
            continue
        for w in line.split():
            sections.setdefault(cur, []).append(w)
    return sections


class Names:
    def __init__(self):
        self.index = set(_read_list(DATA / "library-index.txt").get("default", []))
        self.modules = set()
        for f in ("std.cppm", "std.compat.cppm"):
            p = REPO / "modules" / f
            if p.exists():
                for m in re.findall(r"\busing\s+([^;=]*);", p.read_text()):
                    self.modules.add(m.split("::")[-1].strip())
        allowed = _read_list(DATA / "allowed.txt")
        # [src-platform]: the C library's and the system's names the runtime's sources use, kept
        # there only (a header that spells one as a name of its own still gets it renamed).
        self.src_platform = set(allowed.pop("src-platform", []))
        rec = DATA / "renamed.txt"
        self.recorded = set(_read_list(rec).get("default", [])) if rec.exists() else set()
        self.allowed, self.allowed_re = {}, []
        for section, words in allowed.items():
            for w in words:
                if w.startswith("re:"):
                    self.allowed_re.append((re.compile(w[3:] + r"\Z"), section))
                else:
                    self.allowed[w] = section
        avoid = _read_list(DATA / "avoid.txt")
        self.avoid = {w for ws in avoid.values() for w in ws if not w.startswith("re:")}
        self.avoid_re = [re.compile(w[3:] + r"\Z") for ws in avoid.values() for w in ws if w.startswith("re:")]
        self._cache = {}
        # Names an earlier run renamed that the allow-list now names (a standard name the lists
        # missed): their new spellings are turned back.
        self.restore = {self.new_name(w): w for w in self.recorded if self.kind(w) is not None}
        self.recorded -= set(self.restore.values())

    def kind(self, w):
        """None for a name to rename, else why it stays."""
        k = self._cache.get(w, 0)
        if k != 0:
            return k
        if w in KEYWORDS or w in C_KEYWORDS:
            k = "keyword"
        elif reserved(w):
            k = "reserved"
        elif w in STD_ATTRIBUTES:
            k = "attribute"
        elif w in self.allowed:
            k = self.allowed[w]
        elif w in self.index or w in self.modules:
            k = "standard"
        else:
            k = next((s for r, s in self.allowed_re if r.match(w)), None)
        self._cache[w] = k
        return k

    def _avoided(self, w):
        return w in self.avoid or any(r.match(w) for r in self.avoid_re)

    def new_name(self, w):
        if w[0].isupper():
            # `T` -> `_Tp`; a name that is already such a letter and `p` gets a trailing `_`
            new = f"_{w}p" if len(w) == 1 else f"_{w}_" if re.fullmatch(r"[A-Z]p", w) else f"_{w}"
            return new if not self._avoided(new) else f"_Y_{w}"
        new = "__" + w.lstrip("_")
        if w.startswith("_"):  # `_x`: reserved only at global scope ([lex.name]/4.2)
            new = "__" + w[1:]
        return new if not self._avoided(new) else f"__y_{w.lstrip('_')}"

    def check_injective(self, words):
        """The new spellings of `words` are distinct and do not occur among `words` themselves."""
        seen = {}
        for w in sorted(words):
            n = self.new_name(w)
            if n in seen:
                raise SystemExit(f"uglify: {seen[n]} and {w} would both become {n} (tools/data/uglify/avoid.txt)")
            seen[n] = w
        return seen

    def attribute_name(self, w):
        return w if reserved(w) else f"__{w}__"


# ---------------------------------------------------------------------------------------------
# Renaming

_PATHLIKE = re.compile(r"(^|[\s`])[\w.-]*/|\.(hpp|cpp|h|c|py|cmake|md|txt|sh|cppm)\b|^-|^tools\b")
_QUALIFIED = re.compile(r"(?<![\w/.-])(::)?ycxx(::[A-Za-z_]\w*)+")
_PREFIXED = re.compile(r"(?<![\w/.-])(ycxx_\w+|YCXX_\w+)(?![\w/-])")
_BACKTICK = re.compile(r"`([^`\n]+)`")


class Renamer:
    def __init__(self, names, protect=(), only=None):
        self.names = names
        self.protect = set(protect)  # names never renamed in these files (src: the platform's)
        self.only = only             # if not None: rename only these names
        self.renamed = {}

    def map(self, w):
        old = self.names.restore.get(w)
        if old is not None:  # renamed once, allowed since: spelled as the standard spells it
            return old
        if w in self.protect or (self.only is not None and w not in self.only) or self.names.kind(w) is not None:
            return w
        new = self.names.new_name(w)
        self.renamed[w] = new
        return new

    def code(self, text):
        """Renames the identifiers of a code fragment (in a comment: no directives)."""
        return "".join(self.map(t) if k == "ident" else t for k, t in lex(text))

    def comment(self, text):
        def ticked(m):
            span = m.group(1)
            if _PATHLIKE.search(span) or "[[" in span or "__attribute__" in span or span.startswith("."):
                # (a span that starts with '.' is an assembler directive: `.hidden`)
                return m.group()
            return "`" + self.code(span) + "`"

        def qualified(m):
            return self.code(m.group())

        def prefixed(m):
            return self.map(m.group())
        parts = re.split(r"(`[^`\n]+`)", text)
        for i, p in enumerate(parts):
            if i % 2:
                parts[i] = _BACKTICK.sub(ticked, p)
            else:
                p = _QUALIFIED.sub(qualified, p)
                parts[i] = _PREFIXED.sub(prefixed, p)
        return "".join(parts)

    def source(self, text):
        toks = lex(text)
        out = []
        n = len(toks)

        def next_sig(i):
            i += 1
            while i < n and toks[i][0] in ("ws", "nl", "comment"):
                i += 1
            return i

        def prev_sig(i):
            i -= 1
            while i >= 0 and toks[i][0] in ("ws", "nl", "comment"):
                i -= 1
            return i
        attr_depth = 0      # inside [[ ... ]]: bracket depth at its start + 1
        bracket = 0
        paren = 0           # parentheses inside the attribute
        gnu_style = None    # paren depth of an __attribute__((...)) list, else None
        ns = None           # attribute namespace of the current attribute-token
        i = 0
        while i < n:
            k, t = toks[i]
            if k == "comment":
                out.append(self.comment(t)); i += 1; continue
            if k == "punct":
                if t == "[":
                    j = next_sig(i)
                    if not attr_depth and j < n and toks[j] == ("punct", "["):
                        attr_depth, paren, ns = bracket + 1, 0, None
                        out.extend(tok[1] for tok in toks[i:j + 1])
                        bracket += 2
                        i = j + 1
                        continue
                    bracket += 1
                elif t == "]":
                    bracket -= 1
                    if attr_depth and bracket < attr_depth:
                        attr_depth = 0
                elif t == "(":
                    paren += 1
                elif t == ")":
                    paren -= 1
                    if gnu_style is not None and paren < gnu_style - 1:
                        gnu_style = None
                elif t == "," and attr_depth and paren == 0:
                    ns = None
                out.append(t); i += 1; continue
            if k != "ident":
                out.append(t); i += 1; continue
            # an identifier
            if attr_depth and paren == 0:
                j = next_sig(i)
                p = prev_sig(i)
                if j < n and toks[j] == ("punct", "::"):
                    ns = t
                    out.append({"gnu": "__gnu__", "clang": "_Clang"}.get(t, t)); i += 1; continue
                if p >= 0 and toks[p] == ("punct", "::") and ns in ("gnu", "__gnu__", "clang", "_Clang"):
                    out.append(self.names.attribute_name(t)); i += 1; continue
                if t == "using":
                    out.append(t); i += 1; continue
                if ns is None and self.names.kind(t) is not None:
                    out.append(t); i += 1; continue
                out.append(self.map(t)); i += 1; continue
            if t in ("__attribute__", "__attribute"):
                j = next_sig(i)
                j2 = next_sig(j) if j < n else n
                if j2 < n and toks[j] == ("punct", "(") and toks[j2] == ("punct", "("):
                    out.extend(tok[1] for tok in toks[i:j2 + 1])
                    paren += 2
                    gnu_style = paren
                    i = j2 + 1
                    continue
            if gnu_style is not None and paren == gnu_style:
                p = prev_sig(i)
                if p >= 0 and toks[p][0] == "punct" and toks[p][1] in ("(", ","):
                    out.append(self.names.attribute_name(t)); i += 1; continue
            out.append(self.map(t)); i += 1
        return "".join(out)


# ---------------------------------------------------------------------------------------------
# The tree

def include_files():
    return sorted(p for p in (REPO / "include").rglob("*") if p.is_file())


def src_files():
    return sorted(p for p in (REPO / "src").rglob("*") if p.is_file() and p.suffix in (".cpp", ".hpp", ".c", ".h"))


def checked_files():
    """The files whose identifiers a program's translation unit can see, and the runtime's
    internal headers."""
    return include_files() + [p for p in src_files() if p.suffix in (".hpp", ".h")]


# Files in other languages that spell libycxx's symbols, macros or C++ code: their `ycxx_x` and
# `YCXX_X` words that the map renames are renamed, and so are the `ycxx::` names in the C++ of
# CMake's bracket arguments (the probes, the generated headers).
TEXT_FILES = ["CMakeLists.txt", "cmake/ycxx-c-library.cmake", "cmake/ycxx-link.cmake", "tools/ycxx-cxx"]
_WORDS = re.compile(r"\b(?:ycxx|YCXX)_\w+\b")
_CMAKE_BRACKET = re.compile(r"\[(=*)\[(.*?)\]\1\]", re.S)


def rename_text(text, renamer, cmake):
    if cmake:  # in the C++ of the probes and the generated headers, the `ycxx::` names
        text = _CMAKE_BRACKET.sub(lambda m: f"[{m.group(1)}[" + _QUALIFIED.sub(
            lambda q: renamer.code(q.group()), m.group(2)) + f"]{m.group(1)}]", text)
    return _WORDS.sub(lambda m: renamer.map(m.group()), text)


def rename_tree(names, verbose=True):
    """Renames include/ and src/'s headers completely; then, in the runtime's sources, the names
    renamed in the headers now or by an earlier run (tools/data/uglify/renamed.txt): a source's
    other names (its locals, the C library's and the system's) are its own business."""
    changed = 0

    def run(renamer, paths):
        nonlocal changed
        for p in paths:
            text = p.read_text()
            new = renamer.source(text)
            if new != text:
                p.write_text(new)
                changed += 1
    headers = Renamer(names)
    run(headers, include_files())
    src_headers = Renamer(names, protect=names.src_platform)
    run(src_headers, [p for p in src_files() if p.suffix in (".hpp", ".h")])
    renamed = set(headers.renamed) | set(src_headers.renamed) | names.recorded
    names.check_injective(renamed)
    sources = Renamer(names, protect=names.src_platform, only=renamed)
    run(sources, [p for p in src_files() if p.suffix not in (".hpp", ".h")])
    for rel in TEXT_FILES:
        p = REPO / rel
        if not p.exists():
            continue
        text = p.read_text()
        new = rename_text(text, sources, p.suffix == ".cmake" or p.name == "CMakeLists.txt")
        if new != text:
            p.write_text(new)
            changed += 1
    new_names = renamed - names.recorded
    if new_names or names.restore:
        (DATA / "renamed.txt").write_text(
            "# Every identifier tools/uglify.py has renamed (DECISIONS §2); written by the tool. The runtime's\n"
            "# sources (src/) are renamed by this list, so a merged source that still spells an old name is\n"
            "# fixed by running the tool again.\n" + "".join(f"{w}\n" for w in sorted(renamed)))
    added = add_nasty(set(headers.renamed))
    tests = gen_tests()
    if verbose:
        if added:
            print(f"uglify: {len(added)} names added to {NASTY_LIST.relative_to(REPO)}: {' '.join(sorted(added))}")
        for t in tests:
            print(f"uglify: wrote {t}")
        print(f"uglify: {len(headers.renamed) + len(src_headers.renamed)} identifiers renamed in the headers "
              f"({len(new_names)} new), {changed} files changed")


def uglify_text(text):
    """For the generators of headers (tools/gen_*.py): their output, renamed."""
    return Renamer(Names()).source(text)


def survey(names, files):
    """{identifier: (count, first 'file:line')} of the identifiers to rename."""
    found = {}
    for p in files:
        text = p.read_text()
        line = 1
        protect = names.src_platform if p.is_relative_to(REPO / "src") else ()
        for k, t in lex(text):
            if k == "ident" and t not in protect and names.kind(t) is None:
                c, where = found.get(t, (0, None))
                found[t] = (c + 1, where or f"{p.relative_to(REPO)}:{line}")
            line += t.count("\n")
    return found


def stats(names):
    counts = {}
    for p in include_files():
        for k, t in lex(p.read_text()):
            if k == "ident":
                counts.setdefault(t, 0)
                counts[t] += 1
    by = {}
    for w in counts:
        by.setdefault(names.kind(w) or "to rename", set()).add(w)
    for k in sorted(by, key=str):
        print(f"{k:>12}: {len(by[k])}")


# ---------------------------------------------------------------------------------------------
# The nasty-macros tests (tests/ycxx/conformance)

TESTS = REPO / "tests" / "ycxx" / "conformance"
NASTY_HEADER = REPO / "tests" / "ycxx" / "support" / "nasty_macros.hpp"
NASTY_EACH = TESTS / "nasty_macros_each"


NASTY_LIST = DATA / "nasty-macros.txt"


def add_nasty(words):
    """Adds the names renamed in include/ to the nasty-macros list (its first section, sorted and
    wrapped; the comment lines above it and the later sections stay). Returns the names added."""
    text = NASTY_LIST.read_text()
    lines = text.splitlines()
    head = 0
    while head < len(lines) and lines[head].startswith("#"):
        head += 1
    tail = next((i for i in range(head, len(lines)) if lines[i].startswith("[")), len(lines))
    lists = _read_list(NASTY_LIST)
    old = set(lists.get("default", []))
    added = set(words) - old - set(lists.get("not-definable", []))
    if not added:
        return added
    body, line = [], ""
    for w in sorted(old | added):
        if line and len(line) + 1 + len(w) > 100:
            body.append(line)
            line = w
        else:
            line = f"{line} {w}" if line else w
    body.append(line)
    rest = lines[tail:]
    NASTY_LIST.write_text("\n".join(lines[:head] + body + ([""] + rest if rest else [])) + "\n")
    return added


def public_headers():
    sys.path.insert(0, str(HERE))
    from headers import CORE, HOSTED, ABI, FREESTANDING_SUBSET
    return sorted(set(CORE + HOSTED + ABI + FREESTANDING_SUBSET))


def nasty_tests():
    """{path: text} of the generated tests."""
    names = Names()  # a name the allow-list gained since is the standard's: not definable
    lists = _read_list(NASTY_LIST)
    skip = set(lists.get("not-definable", []))
    words = [w for w in lists.get("default", []) if w not in skip and names.kind(w) is None]
    gen = "// GENERATED by tools/uglify.py --gen-tests from tools/data/uglify/nasty-macros.txt; do not edit.\n"
    defs = "".join(f"#define {w} NASTY_MACRO_{w} @\n" for w in words)
    out = {NASTY_HEADER: gen + f"""// [macro.names]/1: a translation unit that includes a standard library header may #define any
// identifier that the standard library does not declare and that is not reserved ([lex.name]/4).
// These are the {len(words)} such identifiers that libycxx's headers used as names of their own
// before tools/uglify.py renamed them (DECISIONS §2). Each is defined to tokens that are never
// valid C++ (`@`), so any header that still spells one fails to compile.
#pragma once
""" + defs}
    headers = public_headers()
    out[TESTS / "nasty_macros.compile.pass.cpp"] = gen + """// [macro.names]/1, [lex.name]/4: with every identifier that libycxx's headers used to spell
// defined as a macro (support/nasty_macros.hpp), every public header still compiles.
#include "nasty_macros.hpp"
""" + "".join(f"#include <{h}>\n" for h in headers) + "\nint main() {}\n"
    out[TESTS / "nasty_macros_import.compile.pass.cpp"] = gen + """// [macro.names]/1 with the standard library modules ([std.modules]): the program's macros do not
// reach the modules' declarations. (No header after the import: GCC 16 does not merge a textual
// definition that follows an import, modules/import_then_include.pass.cpp.)
// MODULES: std std.compat
#include "nasty_macros.hpp"
import std;
import std.compat;

// (No names of the program's own: most short ones are macros here.)
int main() {
  std::println("{}", std::vector<std::string>{"a", "b"});
  return std::ranges::count(std::vector<std::string>{"a", "b"}, "a") == 1 ? 0 : 1;
}
"""
    for h in headers:
        name = h.replace(".", "_")
        out[NASTY_EACH / f"{name}.compile.pass.cpp"] = gen + f"""// [macro.names]/1, [lex.name]/4: <{h}> alone, with the macros of support/nasty_macros.hpp
// defined before it.
#include "nasty_macros.hpp"
#include <{h}>
"""
    return out


def gen_tests(check=False):
    stale = []
    tests = nasty_tests()
    if NASTY_EACH.exists():
        for p in NASTY_EACH.iterdir():
            if p not in tests:
                stale.append(p)
                if not check:
                    p.unlink()
    for p, text in tests.items():
        if not p.exists() or p.read_text() != text:
            stale.append(p)
            if not check:
                p.parent.mkdir(parents=True, exist_ok=True)
                p.write_text(text)
    return [p.relative_to(REPO).as_posix() for p in stale]


def fetch_index():
    """The names of the draft's index of library names (https://eel.is/c++draft/libraryindex):
    each entry's name and the class it is a member of, without template arguments; entries of
    exposition-only names (spelled with hyphens) are left out."""
    s = urllib.request.urlopen("https://eel.is/c++draft/libraryindex").read().decode()
    ids = [html.unescape(i) for i in re.findall(r"<div id='lib:([^']*)'", s)]
    names = set()
    for i in ids:
        for part in i.split(","):
            while True:
                q = re.sub(r"<[^<>]*>", "", part)
                if q == part:
                    break
                part = q
            for comp in part.split("::"):
                m = re.match(r"\s*(?:operator\s*\"\"\s*)?([A-Za-z_]\w*)(?![-\w])", comp)
                if m and not re.match(r"\s*[\w]+-", comp):
                    names.add(m.group(1))
    out = DATA / "library-index.txt"
    out.write_text("# The names of the draft's Index of library names (https://eel.is/c++draft/libraryindex),\n"
                   "# without template arguments and exposition-only names. Generated by\n"
                   "# `tools/uglify.py --fetch-index`; do not edit.\n" + "".join(f"{w}\n" for w in sorted(names)))
    print(f"{out.relative_to(REPO)}: {len(names)} names")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    g = ap.add_mutually_exclusive_group()
    g.add_argument("--check", action="store_true")
    g.add_argument("--list", action="store_true")
    g.add_argument("--map", action="store_true")
    g.add_argument("--stats", action="store_true")
    g.add_argument("--fetch-index", action="store_true")
    g.add_argument("--gen-tests", action="store_true")
    a = ap.parse_args()
    if a.fetch_index:
        fetch_index()
        return 0
    if a.gen_tests:
        for p in gen_tests():
            print(f"uglify: wrote {p}")
        return 0
    names = Names()
    if a.stats:
        stats(names)
        return 0
    if a.check or a.list or a.map:
        found = survey(names, checked_files())
        if a.map:
            for w in sorted(found):
                print(w, names.new_name(w))
            return 0
        for w, (c, where) in sorted(found.items()):
            print(f"{where}: {w} ({c}x)" if a.check else f"{c} {w} {where}",
                  file=sys.stderr if a.check else sys.stdout)
        if a.check and found:
            print(f"uglify: {len(found)} identifiers in libycxx's headers are neither reserved nor "
                  "declared by the standard; run tools/uglify.py (DECISIONS §2)", file=sys.stderr)
        stale = gen_tests(check=True) if a.check else []
        for p in stale:
            print(f"{p}: out of date (tools/uglify.py --gen-tests)", file=sys.stderr)
        return 1 if a.check and (found or stale) else 0
    rename_tree(names)
    return 0


if __name__ == "__main__":
    sys.exit(main())
