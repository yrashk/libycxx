"""Declarations of the draft's synopses (tools/spec_audit/part3).

parse(text) splits one code block of a synopsis (a block whose top level opens a namespace) into
declarations, each with its scope: the enclosing namespaces, classes (with their template heads)
and access. Nothing here knows C++ fully; it knows the shapes the library clauses use.
"""
import re

IT0, IT1, EXPOS = "\x01", "\x02", "\x03"

_TOK = re.compile(r"\n|\x01[^\x02]*\x02|\x03|\x04[^\x05]*\x05|[A-Za-z_]\w*|<=>|::|->|\.\.\.|==|!=|\+\+|--|&&|\|\||"
                  r"\+=|-=|\*=|/=|%=|&=|\|=|\^=|\.?\d[\w.']*|\"(?:\\.|[^\"\\\n])*\"|'(?:\\.|[^'\\\n])*'|\S")


class Tok(str):
    line = 0


def tokens(text):
    out, line = [], 0
    for t in _TOK.findall(text):
        if t == "\n":
            line += 1
            continue
        k = Tok(t)
        k.line = line
        out.append(k)
    return out


def is_italic(t):
    return t.startswith(IT0)


def italic_text(t):
    return t[1:-1]


KEYWORD_TYPES = {"void", "bool", "char", "char8_t", "char16_t", "char32_t", "wchar_t", "short", "int", "long",
                 "signed", "unsigned", "float", "double", "auto"}
CV = {"const", "volatile"}
SPECIFIERS = {"constexpr", "consteval", "constinit", "static", "inline", "explicit", "virtual", "friend",
              "extern", "thread_local", "mutable"}


class Decl:
    """One declaration: its tokens (without the template heads), the template heads (the
    enclosing classes' first, then its own), its scope and what it is."""
    def __init__(self, toks, heads, scope, access, expos, fs, body=False):
        self.toks, self.heads, self.scope, self.access = toks, heads, scope, access
        self.expos, self.fs, self.body = expos, fs, body
        self.kind = self.name = None
        self.info = {}

    @property
    def ns(self):
        return "::".join(s[1] for s in self.scope if s[0] == "ns")

    @property
    def classes(self):
        return [s for s in self.scope if s[0] == "class"]

    def text(self):
        return render(self.toks)

    def __repr__(self):
        return f"<{self.kind} {self.ns}::{'::'.join(c[1] for c in self.classes)}{'::' if self.classes else ''}{self.name}>"


def render(toks):
    s = ""
    prev = ""
    for t in toks:
        if t.startswith("\x04"):
            continue
        if is_italic(t):
            w = "⟨" + italic_text(t) + "⟩"
        else:
            w = str(t)
        if s and (re.match(r"[\w⟨]", w) and re.search(r"[\w⟩]$", prev)):
            s += " "
        elif s and prev in (",",) :
            s += " "
        elif s and w in ("=",) or prev in ("=",):
            s += " "
        elif s and (w in ("&&", "&", "*") and False):
            pass
        s += w
        prev = w
    return s


def match_close(toks, i, o, c):
    """Index just after the bracket that closes toks[i] (== o)."""
    depth = 0
    n = len(toks)
    while i < n:
        t = toks[i]
        if t == o:
            depth += 1
        elif t == c:
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    return n


def match_angle(toks, i):
    """toks[i] == '<': index after its matching '>' (parentheses nest inside)."""
    depth = 0
    n = len(toks)
    while i < n:
        t = toks[i]
        if t in ("(", "[", "{"):
            i = match_close(toks, i, t, {"(": ")", "[": "]", "{": "}"}[t])
            continue
        if t == "<":
            depth += 1
        elif t == ">":
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    return n


def split_top(toks, sep=","):
    """Split at top-level `sep` (outside (), [], {}, <>), counting `<` after a name or `>` as an
    angle bracket (not after `operator`)."""
    parts, cur, i, n = [], [], 0, len(toks)
    while i < n:
        t = toks[i]
        if t in ("(", "[", "{"):
            j = match_close(toks, i, t, {"(": ")", "[": "]", "{": "}"}[t])
            cur += toks[i:j]
            i = j
            continue
        if t == "<" and i > 0 and toks[i - 1] != "operator" and (re.match(r"\w", toks[i - 1]) or is_italic(toks[i - 1]) or toks[i - 1] == ">"):
            j = match_angle(toks, i)
            cur += toks[i:j]
            i = j
            continue
        if t == sep:
            parts.append(cur)
            cur = []
        else:
            cur.append(t)
        i += 1
    if cur or parts:
        parts.append(cur)
    return parts


class TParam:
    def __init__(self, toks):
        self.toks = toks
        self.pack = "..." in toks
        self.default = None
        if "=" in toks:
            k = toks.index("=")
            self.default = toks[k + 1:]
            toks = toks[:k]
        t = [x for x in toks if x != "..."]
        self.name = t[-1] if t and (re.match(r"[A-Za-z_]\w*$", t[-1])) and len(t) > 1 else None
        head = t[:-1] if self.name else t
        if head and head[0] == "template":
            self.kind = "template"
        elif head in (["class"], ["typename"]):
            self.kind = "type"
        elif head and (re.match(r"[A-Za-z_][\w:]*$", "".join(head)) or (head[-1] == ">" )) and \
                not any(x in KEYWORD_TYPES for x in head) and "".join(head) not in ("size_t", "ptrdiff_t", "intmax_t", "uintmax_t", "chars_format", "range_format", "uint_least32_t", "memory_order") and not "".join(head).startswith(("std::size_t",)) and \
                not (head[0] in ("class", "typename")):
            # a type-constraint: `floating_point T`, `ranges::input_range R`, `sender Sndr`
            self.kind = "type"
            self.constraint = head
        elif head and is_italic(head[0]) and len(head) <= 4 and head[-1] != "*":
            self.kind = "type"
            self.constraint = head
        else:
            self.kind = "value"
            self.type = head
        if not hasattr(self, "constraint"):
            self.constraint = None

    def __repr__(self):
        return f"TParam({self.kind} {self.name}{'...' if self.pack else ''})"


def parse_head(toks, i):
    """toks[i] == 'template': (params, index after the head)."""
    j = match_angle(toks, i + 1)
    inner = toks[i + 2:j - 1]
    params = [TParam(p) for p in split_top(inner)] if inner else []
    # `UIntType a` after `class UIntType`: a non-type parameter whose type is an earlier one
    seen = set()
    for p in params:
        if p.kind == "type" and p.constraint and len(p.constraint) == 1 and str(p.constraint[0]) in seen | {
                "size_t", "ptrdiff_t", "bool", "int", "unsigned"}:
            p.kind, p.type, p.constraint = "value", p.constraint, None
        if p.name:
            seen.add(str(p.name))
    return params, j


def parse(text):
    toks = tokens(text)
    expos_lines = {t.line for t in toks if t == EXPOS}
    first_on_line = {}
    for t in toks:
        first_on_line.setdefault(t.line, t)
    # an EXPOS marker alone on its line marks the next declaration
    lone = {ln for ln in expos_lines if first_on_line[ln] == EXPOS}
    toks_noexp = [t for t in toks]
    out = []
    _seq(toks_noexp, 0, [], [], "public", out, expos_lines, lone)
    return out


def _decl_expos(dt, expos_lines, lone):
    if not dt:
        return False
    first, last = dt[0].line, dt[-1].line
    if any(ln in expos_lines for ln in range(first, last + 1)):
        # a marker after the declaration on its lines (not one that belongs to a nested line)
        return True
    if (first - 1) in lone:
        return True
    return False


def _seq(toks, i, scope, heads, access, out, expos_lines, lone, end_at_brace=False):
    n = len(toks)
    cur = []
    cur_heads = []
    fs = None
    while i < n:
        t = toks[i]
        if t == EXPOS:
            i += 1
            continue
        if t.startswith("\x04"):
            fs = t[1:-1]
            if out and out[-1].fs is None and out[-1].toks and t.line == out[-1].toks[-1].line:
                out[-1].fs = fs
                fs = None
            elif cur:
                pass
            i += 1
            continue
        if t == "}":
            return i + 1
        if not cur and not cur_heads and t in ("public", "private", "protected") and i + 1 < n and toks[i + 1] == ":":
            access = str(t)
            i += 2
            continue
        if not cur and t == "template" and i + 1 < n and toks[i + 1] == "<":
            params, j = parse_head(toks, i)
            cur_heads.append(params)
            # a requires-clause after the head belongs to the declaration
            i = j
            continue
        if not cur and t == ";":
            i += 1
            continue
        if t in ("(", "["):
            j = match_close(toks, i, t, {"(": ")", "[": "]"}[t])
            cur += toks[i:j]
            i = j
            continue
        if t == "{":
            kind = _brace_kind(cur)
            if kind == "ns":
                names = [x for x in cur if x not in ("inline", "namespace", "export")]
                inline = "inline" in cur
                nsnames = "".join(names).split("::") if names else [""]
                sc = scope + [("ns", nm, inline) for nm in nsnames]
                i = _seq(toks, i + 1, sc, heads, "public", out, expos_lines, lone)
                cur, cur_heads = [], []
                continue
            if kind in ("class", "enum"):
                d = Decl(cur, heads + cur_heads, scope, access, _decl_expos(cur, expos_lines, lone) or _has_italic_name(cur), fs)
                d.kind = "classdef" if kind == "class" else "enumdef"
                _class_name(d)
                out.append(d)
                fs = None
                if kind == "class":
                    key = cur[0] if cur[0] in ("class", "struct", "union") else next(x for x in cur if x in ("class", "struct", "union"))
                    acc = "private" if key == "class" else "public"
                    sc = scope + [("class", q, None, None) for q in d.info.get("qual", [])] + [("class", d.name, d.info.get("args"), d)]
                    i = _seq(toks, i + 1, sc, heads + cur_heads, acc, out, expos_lines, lone)
                else:
                    j = match_close(toks, i, "{", "}")
                    d.info["enumerators"] = [e[0] for e in split_top(toks[i + 1:j - 1]) if e and not is_italic(e[0])]
                    i = j
                # declarators after the body: `} name;`
                cur, cur_heads = [], []
                while i < n and toks[i] != ";":
                    i += 1
                i += 1
                continue
            if kind == "body":
                j = match_close(toks, i, "{", "}")
                d = Decl(cur, heads + cur_heads, scope, access, _decl_expos(cur, expos_lines, lone), fs, body=True)
                classify(d)
                out.append(d)
                fs = None
                cur, cur_heads = [], []
                i = j
                # `= ...;` cannot follow a body; a stray `;` is skipped above
                continue
            # an initializer
            j = match_close(toks, i, "{", "}")
            cur += toks[i:j]
            i = j
            continue
        if t == ";":
            d = Decl(cur, heads + cur_heads, scope, access, _decl_expos(cur, expos_lines, lone), fs)
            classify(d)
            out.append(d)
            fs = None
            cur, cur_heads = [], []
            i += 1
            continue
        cur.append(t)
        i += 1
    return i


def _has_italic_name(cur):
    keys = [k for k, x in enumerate(cur) if x in ("class", "struct", "union", "enum")]
    if not keys:
        return False
    k = keys[0] + 1
    while k < len(cur) and cur[k] in ("class", "struct", "alignas"):
        k += 1
    return k < len(cur) and is_italic(cur[k])


def _brace_kind(cur):
    c = [x for x in cur if not x.startswith("\x04")]
    if not c:
        return "init"
    if "namespace" in c and "=" not in c:
        return "ns"
    if "=" in c:
        return "init"
    k = 0
    while k < len(c) and c[k] in ("export", "friend"):
        k += 1
    if k < len(c) and c[k] in ("class", "struct", "union") and "(" not in c:
        return "class"
    if k < len(c) and c[k] == "enum":
        return "enum"
    if "(" in c:
        return "body"
    return "init"


def _class_name(d):
    c = d.toks
    k = next(i for i, x in enumerate(c) if x in ("class", "struct", "union", "enum"))
    k += 1
    while k < len(c) and c[k] in ("class", "struct"):
        k += 1
    qual = []
    while True:
        j = k + 1
        if j < len(c) and c[j] == "<":
            j = match_angle(c, j)
        if j + 1 < len(c) and c[j] == "::" and re.match(r"[A-Za-z_]", c[j + 1]):
            qual.append(str(c[k]))
            k = j + 1
            continue
        break
    d.info["qual"] = qual
    name = c[k] if k < len(c) else ""
    d.name = str(name)
    rest = c[k + 1:]
    args = None
    if rest and rest[0] == "<":
        j = match_angle(rest, 0)
        args = rest[:j]
        rest = rest[j:]
    d.info["args"] = args            # a partial or explicit specialization's arguments
    d.info["final"] = "final" in rest[:1]
    if ":" in rest:
        d.info["bases"] = rest[rest.index(":") + 1:]
    d.info["key"] = c[k - 1] if d.kind == "classdef" else "enum"
    if d.kind == "enumdef":
        d.info["scoped"] = "class" in c or "struct" in c
        if ":" in rest:
            d.info["underlying"] = rest[rest.index(":") + 1:]


def _strip_attrs(c):
    out, i = [], 0
    while i < len(c):
        if c[i] == "[" and i + 1 < len(c) and c[i + 1] == "[":
            j = match_close(c, i, "[", "]")
            i = j
            continue
        out.append(c[i])
        i += 1
    return out


def _requires_split(c):
    """Split off a requires-clause that precedes the declaration (after the template head)."""
    if c and c[0] == "requires":
        # requires-clause: a primary expression or a parenthesized one, possibly with && / ||
        i = 1
        while i < len(c):
            if c[i] == "(":
                i = match_close(c, i, "(", ")")
            else:
                # one name, possibly qualified, with template args
                i += 1
                while i + 1 < len(c) and c[i] == "::":
                    i += 2
                if i < len(c) and c[i] == "<":
                    i = match_angle(c, i)
            if i < len(c) and c[i] in ("&&", "||"):
                i += 1
                continue
            break
        return c[1:i], c[i:]
    return None, c


def classify(d):
    c = [x for x in d.toks if not x.startswith("\x04")]
    c = _strip_attrs(c)
    req, c = _requires_split(c)
    d.info["requires"] = req
    d.toks2 = c
    if not c:
        d.kind = "empty"
        return
    if c[0] == "static_assert":
        d.kind = "static_assert"
        return
    if c[0] == "using":
        if len(c) > 1 and c[1] == "namespace":
            d.kind = "using-directive"
            d.name = render(c[2:])
            return
        if c[1] == "enum":
            d.kind = "using-enum"
            d.name = render(c[2:])
            return
        if "=" in c:
            k = c.index("=")
            d.kind = "alias"
            d.name = str(c[1])
            d.info["target"] = c[k + 1:]
            d.expos = d.expos or is_italic(c[1])
            return
        d.kind = "using-decl"
        tgt = [x for x in c[1:] if x != "typename"]
        d.info["target"] = tgt
        d.name = str(tgt[-1]) if tgt else ""
        if len(tgt) >= 2 and tgt[-2] == "operator":
            d.name = "operator" + tgt[-1]
        return
    if c[0] == "typedef":
        d.kind = "alias"
        d.name = str(c[-1])
        d.info["target"] = c[1:-1]
        return
    if c[0] == "concept" or (len(c) > 1 and c[0] != "(" and "concept" in c[:2]):
        k = c.index("concept")
        d.kind = "concept"
        d.name = str(c[k + 1])
        d.expos = d.expos or is_italic(c[k + 1])
        return
    if c[0] == "friend" and len(c) > 1 and c[1] in ("class", "struct") :
        d.kind = "friend-class"
        return
    k = 0
    while k < len(c) and c[k] in ("export",):
        k += 1
    if c[k] in ("class", "struct", "union", "enum") and "(" not in c:
        # forward declaration or specialization declaration
        d.kind = "classdecl"
        j = k + 1
        while j < len(c) and c[j] in ("class", "struct"):
            j += 1
        if j >= len(c):
            d.kind = "empty"
            return
        d.name = str(c[j])
        d.expos = d.expos or is_italic(c[j])
        rest = c[j + 1:]
        if rest and rest[0] == "<":
            e = match_angle(rest, 0)
            d.info["args"] = rest[:e]
            rest = rest[e:]
        else:
            d.info["args"] = None
        if c[k] == "enum":
            d.kind = "enumdecl"
            if ":" in rest:
                d.info["underlying"] = rest[rest.index(":") + 1:]
        return
    # function or variable: find the declarator's name
    _fun_or_var(d, c)


def _fun_or_var(d, c):
    n = len(c)
    # The first top-level `(` that follows a name (or an operator-function-id) starts the
    # parameters, unless it is a parenthesized declarator or part of a decltype/noexcept/
    # alignas/requires.
    i = 0
    angle = 0
    while i < n:
        t = c[i]
        if t == "operator":
            # operator-function-id or conversion-function-id
            j = i + 1
            if j < n and c[j] == "(" and j + 1 < n and c[j + 1] == ")" and j + 2 < n and c[j + 2] == "(":
                name, j = "operator()", j + 2
            elif j < n and c[j] == "[" and j + 1 < n and c[j + 1] == "]":
                name, j = "operator[]", j + 2
            elif j < n and c[j] in ("new", "delete"):
                name = "operator " + c[j]
                j += 1
                if j + 1 < n and c[j] == "[" and c[j + 1] == "]":
                    name += "[]"
                    j += 2
            elif j < n and c[j].startswith('"'):
                name, j = "operator\"\"" + c[j + 1], j + 2
            elif j < n and re.match(r"[^\w(⟨\x01]", c[j]):
                sym = ""
                while j < n and c[j] != "(" and re.match(r"[^\w\x01]", c[j]):
                    sym += c[j]
                    j += 1
                name = "operator" + sym
            else:
                # conversion function: the type up to `(`
                k = j
                while k < n and c[k] != "(":
                    k += 1
                d.info["conv_type"] = c[j:k]
                name, j = "operator " + render(c[j:k]), k
            d.kind = "function"
            d.name = name
            d.info["pre"] = c[:i]
            _fun_rest(d, c, j)
            return
        if t in ("decltype", "noexcept", "alignas", "sizeof", "requires", "explicit") and i + 1 < n and c[i + 1] == "(":
            i = match_close(c, i + 1, "(", ")")
            continue
        if t == "<" and i > 0 and (re.match(r"\w", c[i - 1]) or is_italic(c[i - 1]) or c[i - 1] == ">"):
            i = match_angle(c, i)
            continue
        if t == "(":
            prev = c[i - 1] if i else ""
            if re.match(r"[A-Za-z_~]\w*$", prev) or is_italic(prev) or prev == ">":
                if prev == ">":
                    # X<...>(  : a constructor of a specialization or a deduction guide / call
                    pass
                name_i = i - 1
                d.kind = "function"
                d.name = str(prev)
                if prev == ">":
                    # find the template-name
                    k = i - 1
                    depth = 0
                    while k >= 0:
                        if c[k] == ">":
                            depth += 1
                        elif c[k] == "<":
                            depth -= 1
                            if depth == 0:
                                break
                        k -= 1
                    d.name = str(c[k - 1])
                    name_i = k - 1
                if name_i > 0 and c[name_i - 1] == "~":
                    d.name = "~" + d.name
                    name_i -= 1
                d.info["pre"] = c[:name_i]
                d.expos = d.expos or is_italic(prev)
                _fun_rest(d, c, i)
                return
            i = match_close(c, i, "(", ")")
            continue
        if t in ("=", "{"):
            break
        i += 1
    # a variable (or a member) declaration
    k = c.index("=") if "=" in c else n
    if "{" in c[:k]:
        k = min(k, c.index("{"))
    decl = c[:k]
    extents = []
    while decl and decl[-1] in ("]",):
        b0 = len(decl) - 1
        while b0 >= 0 and decl[b0] != "[":
            b0 -= 1
        extents = decl[b0:] + extents
        # array
        b = len(decl) - 1
        while b >= 0 and decl[b] != "[":
            b -= 1
        decl = decl[:b]
    d.kind = "variable"
    d.name = str(decl[-1]) if decl else ""
    d.info["type"] = [x for x in decl[:-1] if x not in SPECIFIERS] + extents
    d.info["specs"] = [x for x in decl[:-1] if x in SPECIFIERS]
    d.info["init"] = c[k:]
    d.expos = d.expos or (bool(decl) and is_italic(decl[-1]))
    # template specialization of a variable template: name<args>
    if decl and decl[-1] == ">":
        b = len(decl) - 1
        depth = 0
        while b >= 0:
            if decl[b] == ">":
                depth += 1
            elif decl[b] == "<":
                depth -= 1
                if depth == 0:
                    break
            b -= 1
        d.name = str(decl[b - 1])
        d.info["args"] = decl[b:]
        d.info["type"] = [x for x in decl[:b - 1] if x not in SPECIFIERS]
        d.info["specs"] = [x for x in decl[:b - 1] if x in SPECIFIERS]


def _fun_rest(d, c, i):
    """c[i] == '(': the parameters and what follows."""
    j = match_close(c, i, "(", ")")
    ptoks = c[i + 1:j - 1]
    params = [] if not ptoks or ptoks == ["void"] else split_top(ptoks)
    d.info["params"] = [parse_param(p) for p in params]
    d.info["varargs"] = any(not p["type"] for p in d.info["params"])
    d.info["params"] = [p for p in d.info["params"] if p["type"]]
    rest = c[j:]
    pre = list(d.info["pre"])
    d.info["explicit_cond"] = False
    if "explicit" in pre:
        k = pre.index("explicit")
        if k + 1 < len(pre) and pre[k + 1] == "(":
            e = match_close(pre, k + 1, "(", ")")
            pre = pre[:k] + pre[e:]
            d.info["explicit_cond"] = True
    d.info["specs"] = [x for x in pre if x in SPECIFIERS]
    d.info["ret"] = [x for x in pre if x not in SPECIFIERS]
    quals, k = [], 0
    while k < len(rest) and rest[k] in ("const", "volatile", "&", "&&"):
        quals.append(str(rest[k]))
        k += 1
    d.info["quals"] = quals
    d.info["noexcept"] = None
    if k < len(rest) and rest[k] == "noexcept":
        if k + 1 < len(rest) and rest[k + 1] == "(":
            e = match_close(rest, k + 1, "(", ")")
            d.info["noexcept"] = rest[k + 2:e - 1]
            k = e
        else:
            d.info["noexcept"] = True
            k += 1
    d.info["trailing"] = None
    if k < len(rest) and rest[k] == "->":
        e = k + 1
        while e < len(rest) and rest[e] not in ("requires", "=", "override", "final"):
            if rest[e] == "<":
                e = match_angle(rest, e)
                continue
            if rest[e] == "(":
                e = match_close(rest, e, "(", ")")
                continue
            e += 1
        d.info["trailing"] = rest[k + 1:e]
        k = e
    while k < len(rest) and rest[k] in ("override", "final"):
        k += 1
    d.info["trailing_requires"] = None
    if k < len(rest) and rest[k] == "requires":
        e = k + 1
        while e < len(rest) and rest[e] != "=":
            if rest[e] == "(":
                e = match_close(rest, e, "(", ")")
                continue
            e += 1
        d.info["trailing_requires"] = rest[k + 1:e]
        k = e
    d.info["deleted"] = rest[k:k + 2] == ["=", "delete"]
    d.info["defaulted"] = rest[k:k + 2] == ["=", "default"]
    d.info["pure"] = rest[k:k + 2] == ["=", "0"]
    # deduction guide: no return type, a trailing return type, and a class-template name
    if d.info["trailing"] is not None and not d.info["ret"] and (not d.scope_is_class() or d.name != d.scope[-1][1]):
        d.kind = "guide"


def parse_param(p):
    p = list(p)
    for k in range(len(p) - 3):
        if p[k] == "(" and p[k + 1] in ("*", "&", "&&") and re.match(r"[A-Za-z_]\w*$", p[k + 2]) and p[k + 3] == ")":
            del p[k + 2]
            break
    else:
        # a parameter of function type: `T func(T)` is `T (*)(T)`
        if len(p) >= 4 and p[-1] == ")" and "=" not in p:
            o = next((k for k in range(1, len(p)) if p[k] == "(" and re.match(r"[A-Za-z_]\w*$", p[k - 1])
                      and p[k - 1] not in KEYWORD_TYPES and k >= 2), None)
            if o is not None and match_close(p, o, "(", ")") == len(p):
                p = p[:o - 1] + ["(", "*", ")"] + p[o:]
    default = None
    # a default argument: top-level `=`
    depth = 0
    for k, t in enumerate(p):
        if t in ("(", "[", "{", "<"):
            depth += 1
        elif t in (")", "]", "}", ">"):
            depth -= 1
        elif t == "=" and depth == 0:
            default = p[k + 1:]
            p = p[:k]
            break
    pack = "..." in p
    name = None
    if len(p) > 1 and re.match(r"[A-Za-z_]\w*$", p[-1]) and p[-1] not in KEYWORD_TYPES and p[-2] not in ("::", "const", "volatile", "struct", "class", "typename") \
            and p[-1] not in ("const", "volatile"):
        name = str(p[-1])
        p = p[:-1]
    elif len(p) > 1 and is_italic(p[-1]) and (p[-2] in ("&", "&&", "*", "...", ">") or re.match(r"\w", p[-2])) and \
            italic_text(p[-1]) not in ("integer-type", "floating-point-type") and not italic_text(p[-1]).endswith("-type"):
        name = str(p[-1])
        p = p[:-1]
    typ = [x for x in p if x != "..."]
    return {"type": typ, "name": name, "default": default, "pack": pack}


def _scope_is_class(self):
    return any(s[0] == "class" for s in self.scope)


Decl.scope_is_class = _scope_is_class
