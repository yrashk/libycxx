#!/usr/bin/env python3
"""Inventory of the entities the draft declares in Part 1's clauses ([library] .. [utilities]),
for the spec-coverage probes (docs/SPEC_COVERAGE.md, "Part 1").

    tools/spec_audit/part1/inventory.py [--html FILE]

reads https://eel.is/c++draft/full (or a saved copy) and writes data/entities.tsv, one line per
declared entity of the code blocks (synopses and class definitions) of clauses 16 to 22:
    subclause  header  scope  name  kind
  scope   the enclosing namespace (std, std::ranges, ...) or class (std::optional, with the
          template arguments of a partial specialization: std::optional<T&>)
  kind    class, enum, enumerator, concept, alias, function, variable, macro, or member-*
          (member-function, member-type, member-variable) at class scope
Exposition-only names (italics, `// exposition only`) are left out, as are friends (hidden friends
are found by ADL, not by name), constructors, destructors and specializations of templates
declared elsewhere (`struct hash<optional<T>>`). The header is the one of the subclause's
synopsis ("Header <h> synopsis") or of the nearest synopsis before it in its clause.
The tokenizer and the scope rules are tools/gen_draft_names.py's.
"""
import argparse, pathlib, re, sys, urllib.request

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[1]))
import gen_draft_names as g  # noqa: E402

# [library] declares no entities of its own (its code is exposition: bitmask types, ...).
FS, HOSTED = "\x05", "\x06"   # markers: a comment saying freestanding / hosted


class Extract(g._Extract):
    """gen_draft_names' extractor, keeping the comments that say freestanding (freestanding,
    freestanding-deleted, partially/mostly/all freestanding) or hosted as FS / HOSTED markers
    ([freestanding.item]/4)."""
    def handle_data(self, d):
        if self.code and self.comment and self.cur is not None:
            if "freestanding" in d:
                self.cur.append(FS)
            elif re.search(r"\bhosted\b", d):
                self.cur.append(HOSTED)
        super().handle_data(d)


CLAUSES = ("support", "concepts", "diagnostics", "mem", "meta", "utilities")
# The subclauses whose declarations belong to a synopsis outside their own subclause.
HEADER_OF = {"smartptr": "memory", "ptrtag": "memory", "mem.composite.types": "memory",
             "pairs": "utility", "intseq": "utility", "concepts.lang": "concepts",
             "concepts.compare": "concepts", "concepts.object": "concepts",
             "concepts.callable": "concepts"}

# Code blocks at the top level that declare in another namespace than their header's.
DEFAULT_NS = {"func.bind.place": "std::placeholders"}
# Code blocks that are not declarations of the library (a description's code).
NOT_DECLS = {"coroutine.traits.primary"}


def sections(page):
    """[(id, number, title)] in document order."""
    out = []
    for m in re.finditer(r"<div id='([^']+)' class='section'><h\d[^>]*><a class='secnum'[^>]*>([^<]*)</a>(.*?)</h\d>",
                         page, re.S):
        title = g.re.sub(r"<(?:[^>'\"]|'[^']*'|\"[^\"]*\")*>", "", m.group(3))
        import html
        out.append((m.group(1), m.group(2), html.unescape(title).replace("­", "").strip()))
    return out


def scoped_decls(text, default_ns="std", with_status=False):
    """(scope, name, kind) of the declarations of one code block."""
    text = g._HYPHENATED.sub(g.ITALIC, text)
    # a placeholder spliced into a name (int<i>N</i>_t) is one italic name
    text = re.sub(g.ITALIC + r"\w+", g.ITALIC, text)
    # preprocessor lines (#if defined(...), #define) are not declarations; keep the line count
    text = re.sub(r"^[ \t]*#.*$", "", text, flags=re.M)
    sig, expos_line, line, seen = [], set(), 0, False
    for t in g._TOK.findall(text):
        if t == "\n":
            line, seen = line + 1, False
        elif t == g.EXPOS:
            expos_line.add(line if seen else line + 1)
        else:
            if t in (g.LT, g.GT) and sig and sig[-1][0] in ("operator", "<", ">"):
                t = "<" if t == g.LT else ">"
            sig.append((t, line))
            seen = True
    depth = 0
    for t, _ in sig:
        depth += (t == "{") - (t == "}")
        if depth == 0 and t in g._STATEMENTS:
            return []
    out = []
    # (kind, qualified name); a block without a namespace declares in the header's namespace
    scope = [("ns", default_ns)]
    paren, angle, head, raw = [0], [0], [], []
    access = ["public"]   # per scope: the access of a class's members so far
    status = {}           # index into out -> "fs" / "hosted" (the comment after its declaration)
    starts = [0, 0]       # where in out the previous and the current declaration begin
    header_default = ""   # a comment before the first declaration: the synopsis's

    def spec(j):
        """The specifiers of the declaration whose declarator-id is at token j: constexpr or
        consteval (in its head), noexcept (unconditional or conditional), deleted, explicit."""
        out = set(x for x in raw if x in ("constexpr", "consteval", "explicit", "static", "virtual"))
        d = 0
        while j < n:
            x = sig[j][0]
            if x in ("(", "[") or x == g.LT:
                d += 1
            elif x in (")", "]") or x == g.GT:
                d -= 1
            elif d == 0 and x in (";", "{", "}"):
                break
            elif d == 0 and x == "noexcept":
                cond = j + 1 < n and sig[j + 1][0] == "("
                out.add("noexcept(...)" if cond and not (j + 2 < n and sig[j + 2][0] == "true") else "noexcept")
            elif d == 0 and x == "delete" and sig[j - 1][0] == "=":
                out.add("deleted")
            j += 1
        return " ".join(sorted(out))

    def emit(sc, name, kind):
        out.append((sc, name, kind if access[-1] == "public" else kind + "-" + access[-1], spec(i - 1)))

    expos = False
    n = len(sig)
    i = 0
    while i < n:
        t, ln = sig[i]
        if t in (FS, HOSTED):
            st = "fs" if t == FS else "hosted"
            if not out and not raw and i == 0:
                header_default = st
            else:
                for k in range(starts[1], len(out)):   # the declaration it follows
                    status.setdefault(k, st)
            sig.pop(i)
            n -= 1
            continue
        if not raw:
            starts = [starts[1], len(out)]
        prev = sig[i - 1][0] if i else ""
        nxt = sig[i + 1][0] if i + 1 < n else ""
        if not raw:
            expos = ln in expos_line
        if t == "{":
            k = g._brace_kind(head, prev, paren[-1], angle[-1], scope[-1][0])
            if expos and k in ("class", "enum"):
                k = "xclass"
            outer = scope[-1][1]
            name = outer
            keys = []
            if k == "ns":
                j = head.index("namespace")
                parts = [x for x in head[j + 1:] if x not in ("inline", g.ITALIC)]   # inline namespace unspecified
                name = "::".join(filter(None, [outer if len(scope) > 1 else "", "".join(parts)]))
            elif k in ("class", "enum"):
                # the class-key outside the template parameter list and the requires-clause
                depth, keys = 0, []
                for x, r in enumerate(raw):
                    depth += (r == g.LT) - (r == g.GT)
                    if depth == 0 and (r in g._CLASS_KEYS or r == "enum"):
                        keys.append(x)
                rest = raw[keys[-1] + 1:] if keys else []
                # the class's name and the template arguments of a specialization
                cut, d = len(rest), 0
                for x, r in enumerate(rest):
                    d += (r == g.LT) - (r == g.GT)
                    if r == ":" and d == 0:
                        cut = x
                        break
                rest = [x for x in rest[:cut] if x not in ("final", "class", "struct", "alignas")]
                cname, args = "".join(rest), ""
                if g.LT in rest:
                    a = rest.index(g.LT)
                    cname = "".join(rest[:a])
                    inner = []
                    for x in rest[a + 1:-1]:
                        inner.append(", " if x == "," else "<" if x == g.LT else ">" if x == g.GT else
                                     "?" if x == g.ITALIC else x + (" " if g._IDENT.match(x) else ""))
                    args = "<" + "".join(inner).replace(" >", ">").replace(" ,", ",").strip() + ">"
                if not outer:
                    outer = default_ns
                name = (outer + "::" if outer else "") + cname + args
            scope.append((k, name))
            access.append("private" if k == "class" and keys and raw[keys[-1]] == "class" else "public")
            paren.append(0); angle.append(0)
            head, raw = [], []
            i += 1
            continue
        if t == "}":
            if len(scope) > 1:
                scope.pop(); paren.pop(); angle.pop(); access.pop()
            head, raw = [], []
            i += 1
            continue
        if t == ";" and not paren[-1]:
            head, raw = [], []
            i += 1
            continue
        if t in ("(", "["):
            paren[-1] += 1
        elif t in (")", "]"):
            paren[-1] = max(0, paren[-1] - 1)
        elif t == g.LT:
            angle[-1] += 1
        elif t == g.GT:
            angle[-1] = max(0, angle[-1] - 1)
        before = head
        if not angle[-1] and t != g.GT:
            head = head + [t]
        raw = raw + [t]
        i += 1
        sk, sname = scope[-1]
        if sk == "class" and t in ("public", "private", "protected") and nxt == ":" and not paren[-1]:
            access[-1] = t
            head, raw = [], []
            continue
        if sk not in ("ns", "class", "enum"):
            continue
        if t == "operator" and not paren[-1] and not angle[-1] and not expos and "friend" not in head and sk != "enum":
            # operator@: the name runs to the `(` of the parameter list
            j, op = i, []
            while j < n and sig[j][0] != "(":
                op.append(sig[j][0]); j += 1
            if j < n and j + 1 < n and sig[j + 1][0] == ")" and op == []:
                pass
            if op == [] and j < n:   # operator()
                op = ["()"]; j += 2
                while j < n and sig[j][0] != "(":
                    j += 1
            op = [x.replace(g.LT, "<").replace(g.GT, ">") for x in op]
            opname = ("operator " + " ".join(op) if op and g._IDENT.match(op[0])   # a conversion
                      else "operator" + "".join(op))
            if op and op[0] not in ("(",) and not any(x in (";", "{", "}") for x in op):
                if not "".join(raw[:-1]).count("::"):
                    emit(sname, opname,
                                "function" if sk == "ns" else "member-function")
            continue
        if not g._IDENT.match(t) or t in g.KEYWORDS or g._PLACEHOLDER.match(t):
            continue
        if prev in (".", "->", "::") or expos or paren[-1] or angle[-1] or "friend" in head:
            continue
        if sk == "enum":
            if nxt in (",", "}", "=", ""):
                emit(sname, t, "enumerator")
            continue
        if prev in ("using", "concept", "namespace"):
            if nxt in ("=", "{", ";"):
                if prev != "namespace":
                    kind = "concept" if prev == "concept" else "alias"
                    emit(sname, t, kind if sk == "ns" else "member-type")
            continue
        if prev in g._CLASS_KEYS or prev == "enum":
            if nxt in ("{", ":", ";", "final"):
                emit(sname, t, ("enum" if prev == "enum" else "class") if sk == "ns" else "member-type")
            elif nxt == g.LT:
                pass   # a specialization of a template declared elsewhere
            continue
        if nxt not in ("(", ";", "=", "{", "[", ",", ":"):
            continue
        if prev == ",":
            if not any(x in g._DECL_WORDS or g._IDENT.match(x) for x in before[:1]) or ":" in before:
                continue
        elif prev in g._NOT_TYPE or not (prev in g._TYPE_END or g._IDENT.match(prev or "-")):
            continue
        if prev == ")" and nxt != ";":
            continue
        if len(scope) == 1 and not any(x in g._DECL_WORDS for x in before):
            continue
        if sk == "class" and t == sname.rsplit("::", 1)[-1].split("<")[0]:
            continue   # a constructor
        if "typedef" in head:
            kind = "alias"
        elif nxt == "(":
            kind = "function"
        else:
            kind = "variable"
        if sk == "class":
            kind = {"alias": "member-type", "function": "member-function"}.get(kind, "member-variable")
        emit(sname, t, kind)
    if not with_status:
        return out
    res = [(a, b, c, status.get(k, "")) for k, (a, b, c, _) in enumerate(out)]
    return ([("", "", "header-default", header_default)] if header_default else []) + res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--html")
    a = ap.parse_args()
    page = (pathlib.Path(a.html).read_text(encoding="utf-8") if a.html
            else urllib.request.urlopen(g.URL).read().decode("utf-8"))
    secs = sections(page)
    number = {s: n for s, n, _ in secs}
    clause_of, header_of, group_of = {}, {}, {}
    id_of = {n: s for s, n, _ in secs}
    top = {n.split(".")[0]: s for s, n, _ in secs if "." not in n}
    cur_hdr = {}
    for s, n, title in secs:
        clause_of[s] = top.get(n.split(".")[0], "")
        group_of[s] = id_of.get(".".join(n.split(".")[:2]), s)
        two = ".".join(n.split(".")[:2])
        m = re.match(r"Header <([\w.]+)> synopsis", title)
        if m:
            cur_hdr[two] = m.group(1)
        header_of[s] = cur_hdr.get(two, "")
        if not header_of[s]:
            sub = next((x for x, nn, _ in secs if nn == two), "")
            header_of[s] = HEADER_OF.get(sub, "")
    p = Extract()
    p.feed(page)
    rows, seen, hdr_fs = [], set(), {}
    for sec, kind, text in p.regions:
        if kind != "code" or clause_of.get(sec) not in CLAUSES or sec in NOT_DECLS:
            continue
        hdr = header_of.get(sec, "")
        dns = DEFAULT_NS.get(sec) or ("std::meta" if sec.startswith("meta.reflection") else
                                      "" if hdr == "new" or hdr.endswith(".h") else "std")
        decls = scoped_decls(text, dns, with_status=True)
        if decls and decls[0][2] == "header-default":
            hdr_fs.setdefault(hdr, decls[0][3])
            decls = decls[1:]
        for scope, name, k, st in decls:
            key = (header_of.get(sec, ""), scope, name, k)
            if key in seen:
                continue
            seen.add(key)
            rows.append([sec, header_of.get(sec, ""), scope, name, k, st])
        for m in re.finditer(r"^\s*#\s*define\s+(\w+)(?=[\s(]|$)", text, re.M):
            key = (header_of.get(sec, ""), "", m.group(1), "macro")
            if key not in seen and not m.group(1).startswith("__cpp_lib"):
                seen.add(key)
                fsm = re.search(r"#\s*define\s+" + m.group(1) + r"\b[^\n]*" + FS, text)
                rows.append([sec, header_of.get(sec, ""), "", m.group(1), "macro", "fs" if fsm else ""])
    # [freestanding.item]/4-5: a declaration is freestanding if its comment says so, or if its
    # synopsis begins with a comment saying freestanding and it is not followed by `hosted`; a
    # member of a freestanding class is freestanding unless it says hosted.
    fs_class = {}
    for r in rows:
        if r[4] in ("class", "enum") and r[2] in ("std", "std::pmr", "std::meta", "std::ranges", "std::contracts"):
            st = r[5] or ("fs" if hdr_fs.get(r[1]) == "fs" else "")
            if st == "fs":
                fs_class[r[2] + "::" + r[3]] = True
    for r in rows:
        st = r[5]
        if st == "hosted":
            r[5] = "hosted"
        elif st == "fs":
            r[5] = "freestanding"
        elif r[4].startswith("member") or r[4] == "enumerator":
            base = re.sub(r"<.*$", "", r[2])
            r[5] = "freestanding" if fs_class.get(base) or fs_class.get(re.sub(r"::[^:]*$", "", base)) else ""
        else:
            r[5] = "freestanding" if hdr_fs.get(r[1]) == "fs" else ""
    rev = g.revision(page)
    (HERE / "data" / "entities.tsv").write_text(
        f"# from {g.URL}, revision {rev} of github.com/Eelis/draft; generated by inventory.py\n"
        "# subclause\theader\tscope\tname\tkind\tfreestanding ([freestanding.item]) or hosted or empty\tgroup (the subclause two levels down: [optional], [smartptr])\n"
        + "".join("\t".join(r + [group_of.get(r[0], "")]) + "\n" for r in rows))
    print(f"{len(rows)} entities (revision {rev})")


if __name__ == "__main__":
    sys.exit(main())
