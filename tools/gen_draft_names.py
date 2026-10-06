#!/usr/bin/env python3
"""The names the draft's library spells in code for a program to use or provide (DECISIONS §2).

    tools/gen_draft_names.py [--html FILE]

writes tools/data/uglify/draft-names.txt from the draft (https://eel.is/c++draft/full, or a saved
copy of that page), recording the revision of github.com/Eelis/draft it was built from.
`tools/uglify.py --check` fails when a name of that list is renamed in include/ (the draft's index
of library names misses it) unless allowed.txt's [standard] section keeps it or its
[draft-internal] section says why programs never spell it.

Which identifiers count. The library's clauses ([library] to [exec], and Annex D) are read, but
not their examples (a program's own names), nor Annex C or the other annexes. In their code
(code blocks, item declarations, and inline code for the second kind below), an identifier is
listed when it is
  - a name declared at namespace or class scope: the declarator-id of a declaration (followed by
    `(`, `;`, `=`, `{`, `[`, `,` or `:` outside parentheses and template argument lists), the
    name a `using` alias, a class, a concept, an enumeration or an enumerator declares; or
  - a name used as a member or a qualified name: after `.`, `->` or `::` (`Rcvr::make_receiver_for`,
    `env.query(...)`, the designators of an aggregate, `remove_cvref_t<Sndr>::sender_concept`).
Not listed: what the draft sets in italics (exposition-only names and placeholders, which eel.is
renders as <i>), the declarations followed by a comment `// exposition only` and the members of a
class whose name is in italics, function parameters and template parameters (inside parentheses
or template argument lists), and the names declared in a block (locals of the exposition's code,
requires-expressions' parameters). eel.is marks template argument lists (<span
class='anglebracket'>), so they are told apart from `<` and `>` operators.
"""
import argparse, html, pathlib, re, sys, urllib.request
from html.parser import HTMLParser

HERE = pathlib.Path(__file__).resolve().parent
OUT = HERE / "data" / "uglify" / "draft-names.txt"
URL = "https://eel.is/c++draft/full"

ITALIC, EXPOS, LT, GT = "\x01", "\x02", "\x03", "\x04"   # markers in the extracted code
CODE = {"codeblock": "code", "itemdeclcode": "decl", "texttt": "inline"}



class _Extract(HTMLParser):
    """Collects (section, kind, text) of each code region of the library clauses. In the text,
    an italic run is ITALIC, a comment that says "exposition only" is EXPOS (other comments are
    dropped), and the angle brackets of template argument lists are LT and GT."""
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.stack, self.regions, self.cur = [], [], None
        self.code = self.ital = self.comment = self.angle = self.example = self.descr = self.cell = self.column = 0
        self.sec, self.in_lib, self.kind = None, False, None
        self.cell_start = False  # nothing but white space so far in a table's first cell
        self.cell_region = None  # the index of the region that is all of that cell, so far
        self.comment_text = []

    def handle_starttag(self, tag, attrs):
        if tag in ("br", "img", "meta", "link", "hr", "wbr", "input"):
            return
        a = dict(attrs)
        cls = (a.get("class") or "").split()
        if tag == "div" and "section" in cls and "id" in a:
            self.sec = a["id"]
            if self.sec in ("library", "depr"):  # Clauses 16 to 33 follow [library]
                self.in_lib = True
            elif self.sec in ("gram", "implimits", "diff", "uaxid", "ub", "ifndr", "bibliography"):
                self.in_lib = False
        flags = []
        k = next((CODE[c] for c in cls if c in CODE), None)
        if k:
            flags.append("code")
            if self.code == 0 and self.cell_region is not None:
                self.cell_region = None  # a second piece of code in the cell
            if self.code == 0:
                # A code block in an item's description is the code of its Effects: a body.
                self.kind, self.cur = ("body" if k == "code" and self.descr else k), []
            self.code += 1
        if tag == "i" or "textit" in cls:
            flags.append("i")
            self.ital += 1
        if "comment" in cls:
            flags.append("c")
            self.comment += 1
        if "anglebracket" in cls:
            flags.append("a")
            self.angle += 1
        if "example" in cls or "note" in cls:
            flags.append("e")
            self.example += 1
        if "itemdescr" in cls:
            flags.append("d")
            self.descr += 1
        if tag == "tr":
            self.column = 0
        if tag == "td":
            flags.append("td")
            self.cell += 1
            self.column += 1
            self.cell_start = self.column == 1  # a table's first column names what it lists
            self.cell_region = None
        href = a.get("href") or ""
        if tag == "a" and self.cur is not None and re.fullmatch(r"#[a-z][\w.]*", href):
            # a reference to a subclause ([numeric.limits]) in the code: not code
            flags.append(("ref", len(self.cur)))
        self.stack.append((tag, flags))

    def handle_endtag(self, tag):
        while self.stack:
            t, flags = self.stack.pop()
            for f in flags:
                if f == "code":
                    self.code -= 1
                    if self.code == 0:
                        if self.in_lib and not self.example and self.cur:
                            # inline code alone in a table's first cell: a name the table lists
                            first = self.kind == "inline" and self.cell and self.cell_start
                            self.cell_region = len(self.regions) if first else None
                            self.regions.append((self.sec, self.kind, "".join(self.cur)))
                        self.cur = None
                        self.cell_start = False
                elif f == "td":
                    self.cell -= 1
                    if self.cell_region is not None:
                        sec, _, text = self.regions[self.cell_region]
                        self.regions[self.cell_region] = (sec, "cell", text)
                        self.cell_region = None
                elif isinstance(f, tuple):
                    if self.cur is not None:
                        del self.cur[f[1]:]
                elif f == "i":
                    self.ital -= 1
                elif f == "c":
                    self.comment -= 1
                    if self.comment == 0 and self.cur is not None:
                        if re.match(r"\s*(//|/\*)\s*(for\s+)?exposition[ -]only", "".join(self.comment_text), re.I):
                            self.cur.append(EXPOS)
                        self.comment_text = []
                elif f == "a":
                    self.angle -= 1
                elif f == "e":
                    self.example -= 1
                elif f == "d":
                    self.descr -= 1
            if t == tag:
                break

    def handle_data(self, d):
        if not self.code or self.cur is None:
            if re.search(r"\w", d):  # (not the 🔗 of a row)
                self.cell_start = False
                self.cell_region = None
            return
        d = d.replace("\u200b", "").replace("\xad", "").replace("\xa0", " ")  # eel.is's break hints
        if self.comment:
            self.comment_text.append(d)
        elif self.ital:
            if not self.cur or self.cur[-1] != ITALIC:
                self.cur.append(ITALIC)
        elif self.angle:
            self.cur.append(d.replace("<", LT).replace(">", GT))
        else:
            self.cur.append(d)


_TOK = re.compile(r"\n|[\x01-\x04]|[A-Za-z_]\w*|::|->\*?|\.\.\.|<=>|<<=|>>=|[-+*/%^&|=!<>]=|&&|\|\||\+\+|--|<<|>>"
                  r"|\.?\d[\w.']*|\"(?:\\.|[^\"\\\n])*\"|'(?:\\.|[^'\\\n])*'|\S")
# A hyphenated word is an exposition-only name whose italics the source lost (`extents_-type`).
_HYPHENATED = re.compile(r"(?<![\w-])[A-Za-z_]\w*(?:-[A-Za-z_]\w*)+(?![\w-])")
_IDENT = re.compile(r"[A-Za-z_]\w*\Z")
# A capital letter, maybe with digits: a placeholder ([structure.requirements]), never a name.
_PLACEHOLDER = re.compile(r"[A-Z]\d*\Z")
_CLASS_KEYS = {"struct", "class", "union"}
# The keywords of C++26, and the identifiers with special meaning; never a name of the library's.
KEYWORDS = set("""
alignas alignof asm auto bool break case catch char char8_t char16_t char32_t class concept const
consteval constexpr constinit const_cast continue contract_assert co_await co_return co_yield
decltype default delete do double dynamic_cast else enum explicit export extern false float for
friend goto if inline int long mutable namespace new noexcept nullptr operator private protected
public register reinterpret_cast requires return short signed sizeof static static_assert
static_cast struct switch template this thread_local throw true try typedef typeid typename union
unsigned using virtual void volatile wchar_t while and and_eq bitand bitor compl not not_eq or
or_eq xor xor_eq final import module override post pre replaceable_if_eligible
trivially_relocatable_if_eligible
""".split())
# The statements that make a code block's top level a function body.
_STATEMENTS = {"return", "if", "for", "while", "switch", "do", "co_return", "co_yield", "co_await", "throw",
               "try", "goto", "break", "continue"}
# At a code block's top level outside a namespace (the code of an exposition), a declaration
# counts only when it starts with one of these.
_DECL_WORDS = {"template", "using", "concept", "typedef", "constexpr", "consteval", "constinit", "inline",
               "static", "extern", "friend", "virtual", "explicit", "namespace", "enum"} | _CLASS_KEYS
# What may precede a declarator-id: the end of a type.
_TYPE_END = {GT, ITALIC, "*", "&", "&&", "...", "]", ")"}
# Keywords that precede an expression or a name that is not declared.
_NOT_TYPE = {"operator", "return", "goto", "case", "new", "delete", "sizeof", "alignof", "decltype",
             "typename", "template", "throw", "co_return", "co_await", "co_yield", "requires", "else",
             "do", "and", "or", "not", "public", "private", "protected", "virtual", "using", "namespace",
             "concept", "class", "struct", "union", "enum", "typeid", "static_assert", "noexcept"}


def _brace_kind(head, prev, paren, angle, outer):
    """The kind of the scope a `{` opens: ns, class, xclass (an exposition-only class's: its
    name is in italics), enum, block (a function body, a lambda, a requires-expression) or init.
    `head` holds the tokens of the declaration so far outside template argument lists."""
    if paren or angle:
        return "init"
    if outer in ("block", "init"):
        return outer
    if outer == "xclass":  # in an exposition-only class: its nested classes, its functions' bodies
        inner = _brace_kind(head, prev, paren, angle, "class")
        return "xclass" if inner in ("class", "enum", "ns") else inner
    if "namespace" in head:
        return "ns"
    if prev in ("requires", "->"):
        return "block"
    keys = [i for i, x in enumerate(head) if x in _CLASS_KEYS or x == "enum"]
    if keys and "(" not in head[:keys[0]] and prev != ")" and prev not in ("const", "noexcept", "mutable"):
        # the class's name: the last name before its base clause (`X<T>::name : base {`)
        rest = head[keys[0] + 1:]
        if ":" in rest:
            rest = rest[:rest.index(":")]
        rest = [x for x in rest if x not in ("final", "alignas") and x not in _CLASS_KEYS]
        name = rest[-1] if rest else ""
        if head[keys[0]] == "enum":
            return "xclass" if name == ITALIC else "enum"
        return "xclass" if name == ITALIC else "class"
    if ")" in head and prev not in ("=", ","):
        return "block"
    if prev in ("=", ",", "return", "{", "(", GT, ITALIC) or _IDENT.match(prev or "-"):
        return "init"
    return "block"


def names_of(text, kind):
    """(name, how) of the identifiers of one code region that count (see the module's comment).
    `kind`: code (a code block), body (a code block of an item's description: an Effects'
    code), decl (an item declaration), inline (code in a sentence: only the names used as
    members count) or cell (inline code that starts a table's cell: a name alone there is one
    the table lists, such as an enumerator)."""
    if kind == "cell":
        w = text.strip()
        if _IDENT.match(w) and w not in KEYWORDS and not _PLACEHOLDER.match(w):
            return [(w, "listed")]
        kind = "inline"
    text = _HYPHENATED.sub(ITALIC, text)
    sig, expos_line, line, seen = [], set(), 0, False  # significant tokens (token, line)
    for t in _TOK.findall(text):
        if t == "\n":
            line, seen = line + 1, False
        elif t == EXPOS:
            # `// exposition only` after a declaration, or on a line of its own before one
            expos_line.add(line if seen else line + 1)
        else:
            if t in (LT, GT) and sig and sig[-1][0] in ("operator", "<", ">"):
                t = "<" if t == LT else ">"  # operator<<, operator>>, operator<=>
            sig.append((t, line))
            seen = True
    if kind == "code":  # a block whose top level has statements is a body too
        depth = 0
        for t, _ in sig:
            depth += (t == "{") - (t == "}")
            if depth == 0 and t in _STATEMENTS:
                kind = "body"
                break
    out = []
    scope = ["block" if kind == "body" else "ns"]  # the kinds of the open braces' scopes
    paren = [0]      # open parentheses and brackets, per brace
    angle = [0]      # open template argument lists, per brace
    head = []        # the tokens of the current declaration or statement, outside <...>
    expos = False    # the current declaration is exposition-only
    n = len(sig)

    def expos_decl(i):
        """Whether a line of the declaration from token i on ends in `// exposition only`."""
        j = i
        while j < n and sig[j][0] not in (";", "{", "}"):
            j += 1
        last = sig[min(j, n - 1)][1]
        return any(ln in expos_line for ln in range(sig[i][1], last + 1))

    for i, (t, ln) in enumerate(sig):
        prev = sig[i - 1][0] if i else ""
        nxt = sig[i + 1][0] if i + 1 < n else ""
        if not head:
            expos = ln in expos_line
        if t == "{":
            k = _brace_kind(head, prev, paren[-1], angle[-1], scope[-1])
            scope.append("xclass" if expos and k in ("class", "enum") else k)
            paren.append(0); angle.append(0)
            head = []
            continue
        if t == "}":
            if len(scope) > 1:
                scope.pop(); paren.pop(); angle.pop()
            head = []
            continue
        if t == ";" and not paren[-1]:
            head = []
            continue
        if t in ("(", "["):
            paren[-1] += 1
        elif t in (")", "]"):
            paren[-1] = max(0, paren[-1] - 1)
        elif t == LT:
            angle[-1] += 1
        elif t == GT:
            angle[-1] = max(0, angle[-1] - 1)
        before = head
        if not angle[-1] and t != GT:
            head = head + [t]
        if not _IDENT.match(t) or t in KEYWORDS or _PLACEHOLDER.match(t):
            continue
        if prev == "template" and i >= 2 and sig[i - 2][0] in (".", "->", "::"):
            prev = sig[i - 2][0]  # x.template f<...>, T::template X<...>
        if prev in (".", "->"):
            out.append((t, "member"))
            continue
        if prev == "::":
            if not (i >= 2 and sig[i - 2][0] == t):  # not a constructor: X::X
                out.append((t, "member"))
            continue
        if kind == "inline" or expos or paren[-1] or angle[-1]:
            continue
        s = scope[-1]
        if s in ("block", "init"):
            continue
        how = "declared" if s != "xclass" else "exposition class member"
        if s == "enum":
            if nxt in (",", "}", "=", "") and not expos_decl(i):
                out.append((t, "enumerator"))
            continue
        if prev in ("using", "concept", "namespace"):
            if nxt in ("=", "{", ";") and not expos_decl(i):
                out.append((t, how))
            continue
        if prev in _CLASS_KEYS or prev == "enum":
            if nxt in ("{", ":", ";", "final", LT) and not expos_decl(i):
                out.append((t, how))
            continue
        if nxt not in ("(", ";", "=", "{", "[", ",", ":"):
            continue
        if prev == ",":  # `static const category none = 0, collate = ...`, not `: a(x), b(y)`
            if not any(x in _DECL_WORDS or _IDENT.match(x) for x in before[:1]) or ":" in before:
                continue
        elif prev in _NOT_TYPE or not (prev in _TYPE_END or _IDENT.match(prev or "-")):
            continue
        if prev == ")" and nxt != ";":  # a cast's operand: (T)x
            continue
        # A code block's top level outside a namespace is the exposition's code: there only
        # declarations that say so count.
        if kind == "code" and len(scope) == 1 and not any(x in _DECL_WORDS for x in before):
            continue
        if expos_decl(i):
            continue
        out.append((t, how))
    return out


def revision(page):
    m = re.search(r"github\.com/Eelis/draft/tree/([0-9a-f]{40})/", page)
    return m.group(1) if m else "unknown"


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--html", help="a saved copy of " + URL)
    ap.add_argument("--output", default=str(OUT))
    a = ap.parse_args()
    page = pathlib.Path(a.html).read_text() if a.html else urllib.request.urlopen(URL).read().decode()
    p = _Extract()
    p.feed(page)
    first = {}
    for sec, kind, text in p.regions:
        for name, how in names_of(text, kind):
            first.setdefault(name, sec)
    out = pathlib.Path(a.output)
    out.write_text(
        "# The names the draft's library clauses spell in code for a program to use or provide: declared at\n"
        "# namespace or class scope, or used as a member or a qualified name; not exposition-only, not\n"
        "# parameters, not locals (tools/gen_draft_names.py, DECISIONS §2). `tools/uglify.py --check` fails\n"
        "# when include/ renames one that allowed.txt's [draft-internal] section does not excuse.\n"
        f"# Generated by `tools/gen_draft_names.py` from {URL}, revision {revision(page)}\n"
        "# of github.com/Eelis/draft; do not edit. Each name with the first subclause that spells it.\n"
        + "".join(f"{w:<40} # [{first[w]}]\n" for w in sorted(first)))
    print(f"{out}: {len(first)} names")
    return 0


if __name__ == "__main__":
    sys.exit(main())
