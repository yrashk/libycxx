#!/usr/bin/env python3
"""Cross-check of constexpr and noexcept between the draft's declarations and libycxx's, Part 1
(docs/SPEC_COVERAGE.md). A screening aid: it lists candidates, which the probes then confirm.

    tools/spec_audit/part1/specifiers.py [--html FILE]

For every function the draft declares in the code blocks of clauses 17 to 22 (entities.tsv's
scopes), it gathers the specifiers of its overloads (constexpr/consteval, noexcept, conditional
noexcept) and does the same over libycxx's headers (include/, include/ycxx/), matched by scope and
name (template arguments of partial specializations reduced to their shape: optional<T&> and
optional<_Tp&> are both optional<&>). It reports
  constexpr  the draft has a constexpr or consteval overload and libycxx none of that name
  noexcept   the draft has an unconditionally noexcept overload and libycxx no noexcept one
A name libycxx declares only in a base class or as a hidden friend is not found and not reported.
"""
import argparse, pathlib, re, sys, urllib.request

HERE = pathlib.Path(__file__).resolve().parent
REPO = HERE.parents[2]
sys.path.insert(0, str(HERE.parents[1]))
sys.path.insert(0, str(HERE))
import gen_draft_names as g  # noqa: E402
import inventory as inv      # noqa: E402


def shape(scope):
    """optional<T &> and optional<_Tp&> -> optional<&>; std::__1:: inline namespaces dropped."""
    s = re.sub(r"\b_?[A-Z]\w*\b", "", scope)
    s = re.sub(r"\s+", "", s)
    return s


def source_text(path):
    s = path.read_text(encoding="utf-8", errors="replace")
    s = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), s, flags=re.S)
    s = re.sub(r"//[^\n]*", "", s)
    # preprocessor lines, with their continuations
    s = re.sub(r"^[ \t]*#(?:[^\n]*\\\n)*[^\n]*", "", s, flags=re.M)
    s = re.sub(r"\[\[.*?\]\]", "", s)                       # attributes
    s = re.sub(r"\balignas\s*\([^()]*\)", "", s)
    s = re.sub(r"\b(_YCXX_\w+|__ycxx_\w+)\b(?!\s*\()", "", s)   # attribute-like macros
    toks = []
    for t in g._TOK.findall(s):
        if t == "<" and not (toks and toks[-1] == "operator"):
            toks.append(g.LT)
        elif t == ">" and not (toks and toks[-1] == "operator"):
            toks.append(g.GT)
        elif t == ">>" and not (toks and toks[-1] == "operator"):
            toks += [g.GT, g.GT]
        else:
            toks.append(t)
    return " ".join("\n" if t == "\n" else t for t in toks)


def collect_ycxx():
    out = {}
    files = [p for p in (REPO / "include").rglob("*") if p.is_file() and p.suffix in ("", ".hpp", ".h")]
    for p in files:
        try:
            text = source_text(p)
        except Exception:
            continue
        for scope, name, kind, spec in inv.scoped_decls(text, ""):
            if "function" not in kind:
                continue
            out.setdefault((shape(scope), name), []).append(spec)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--html")
    a = ap.parse_args()
    page = (pathlib.Path(a.html).read_text(encoding="utf-8") if a.html
            else urllib.request.urlopen(g.URL).read().decode("utf-8"))
    secs = inv.sections(page)
    top = {n.split(".")[0]: s for s, n, _ in secs if "." not in n}
    clause_of = {s: top.get(n.split(".")[0], "") for s, n, _ in secs}
    p = g._Extract()
    p.feed(page)
    draft = {}
    for sec, kind, text in p.regions:
        if kind != "code" or clause_of.get(sec) not in inv.CLAUSES or sec in inv.NOT_DECLS:
            continue
        dns = inv.DEFAULT_NS.get(sec) or ("std::meta" if sec.startswith("meta.reflection") else "std")
        for scope, name, k, spec in inv.scoped_decls(text, dns):
            if "function" in k and not k.endswith("-private"):
                draft.setdefault((shape(scope), name), []).append((sec, spec))
    ycxx = collect_ycxx()
    rows = []
    for (scope, name), decls in sorted(draft.items()):
        mine = ycxx.get((scope, name)) or ycxx.get((scope.replace("std::", "", 1), name))
        if mine is None:
            continue
        secs_ = sorted({s for s, _ in decls})
        # a synopsis and the class's own subclause repeat declarations: the subclause with most
        by_sec = {}
        for s, sp in decls:
            by_sec.setdefault(s, []).append(sp)
        dspec = max(by_sec.values(), key=len)
        cx = lambda specs: sum(1 for s in specs if {"constexpr", "consteval"} & set(s.split()))
        nx = lambda specs: sum(1 for s in specs if "noexcept" in s.split())
        anynx = lambda specs: sum(1 for s in specs if "noexcept" in s)
        if cx(dspec) > cx(mine):
            rows.append(f"constexpr\t{scope}::{name}\tdraft {cx(dspec)}/{len(dspec)}, libycxx {cx(mine)}/{len(mine)}\t{','.join(secs_)}")
        if nx(dspec) > anynx(mine):
            rows.append(f"noexcept\t{scope}::{name}\tdraft {nx(dspec)}/{len(dspec)}, libycxx {anynx(mine)}/{len(mine)}\t{','.join(secs_)}")
    print("\n".join(rows))
    print(f"# {len(draft)} draft functions, {len(rows)} candidates", file=sys.stderr)


if __name__ == "__main__":
    sys.exit(main())
