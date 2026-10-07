#!/usr/bin/env python3
"""The part-3 inventory: every entity the synopses of [text], [numerics], [time], [input.output],
[thread], [exec] and Annex D declare (tools/spec_audit/part3).

    tools/spec_audit/part3/inventory.py [--regions cache/regions.json] [--dump]

Reads draft.py's output and writes inventory.tsv: one line per declaration (subclause, header,
scope, kind, name, declaration). Exposition-only declarations and private members are left out.
"""
import argparse, json, pathlib, re, sys
sys.path.insert(0, str(pathlib.Path(__file__).parent))
import decls as D

HERE = pathlib.Path(__file__).parent


def load(path):
    data = json.loads(pathlib.Path(path).read_text(encoding="utf-8"))
    return data


def section_headers(data):
    """Header of each .syn subclause."""
    hdr = {}
    for s in data["sections"]:
        m = re.match(r"Header <([\w./]+)> synopsis", s["title"])
        if m:
            hdr[s["id"]] = m.group(1)
    hdr.setdefault("stdatomic.h.syn", "stdatomic.h")
    return hdr


def synopsis_blocks(sec):
    """The code blocks of a subclause that declare: their top level opens a namespace."""
    for kind, text in sec["regions"]:
        if kind != "code":
            continue
        # `#include <ostream>` and the macros' `#define`s before the namespace
        text = re.sub(r"(?m)^[ \t]*#.*$", "", text)
        t = text.lstrip()
        if re.match(r"(export\s+)?(inline\s+)?namespace\b", t):
            yield text


def entities(data):
    hdr_of_syn = section_headers(data)
    secs = data["sections"]
    out = []
    name_hdr = {}
    # pass 1: names declared at namespace scope by the header synopses
    for s in secs:
        if s["id"] not in hdr_of_syn:
            continue
        for text in synopsis_blocks(s):
            for d in D.parse(text):
                if not d.scope_is_class() and d.name:
                    name_hdr.setdefault((d.ns, d.name), []).append(hdr_of_syn[s["id"]])
    last_hdr = None
    for s in secs:
        if s["id"] in hdr_of_syn:
            last_hdr = hdr_of_syn[s["id"]]
        for text in synopsis_blocks(s):
            for d in split_declarators(D.parse(text)):
                top = d.classes[0][1] if d.classes else d.name
                cands = name_hdr.get((d.ns, top), [])
                if s["id"] in hdr_of_syn:
                    h = hdr_of_syn[s["id"]]
                elif last_hdr in cands or not cands:
                    h = last_hdr
                else:
                    h = next((c for c in cands if c != "iosfwd"), cands[0])
                d.sec = s["id"]
                d.header = h
                # members of an exposition-only class are exposition-only
                if any(c[3] is not None and (c[3].expos or c[1].startswith(D.IT0)) for c in d.classes):
                    d.expos = True
                out.append(d)
    # `enum class text_encoding::id`, `class locale::facet` defined outside their class: the
    # enclosing class's definition fills in the scope
    prim = {}
    for d in out:
        if d.kind == "classdef" and not d.info.get("args"):
            path = tuple(c[1] for c in d.classes) + (d.name,)
            prim.setdefault((d.ns, path), d)
    for d in out:
        q = d.info.get("qual") if d.kind in ("classdef", "enumdef") else None
        if q:
            d.scope = d.scope + [("class", n, None, None) for n in q]
        if any(s[0] == "class" and s[3] is None for s in d.scope):
            new, path = [], ()
            for s in d.scope:
                if s[0] == "class":
                    path = path + (s[1],)
                    if s[3] is None:
                        s = ("class", s[1], None, prim.get((d.ns, path)))
                new.append(s)
            d.scope = new
    return out


def split_declarators(ds):
    """`static const category none = 0, collate = 0x010, ...;` declares several variables."""
    for d in ds:
        if d.kind == "variable" and d.info.get("init") and "," in d.info["init"] and not d.heads:
            parts = D.split_top(d.toks2)
            if len(parts) > 1 and all(len(p) >= 1 for p in parts[1:]):
                first = parts[0]
                typ = d.info["type"]
                specs = d.info["specs"]
                yield d
                for p in parts[1:]:
                    e = D.Decl(p, d.heads, d.scope, d.access, d.expos, d.fs)
                    e.kind = "variable"
                    e.name = str(p[0])
                    e.info = dict(d.info)
                    e.info["init"] = p[1:]
                    e.toks2 = p
                    yield e
                continue
        yield d


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--regions", default=str(HERE / "cache" / "regions.json"))
    ap.add_argument("--dump", action="store_true")
    a = ap.parse_args()
    data = load(a.regions)
    ents = entities(data)
    for d in ents:
        if a.dump:
            cls = "::".join(c[1] for c in d.classes)
            print(f"{d.sec}\t{d.header}\t{d.ns}\t{cls}\t{d.access}\t{'X' if d.expos else ''}\t{d.kind}\t{d.name}\t{d.text()[:150]}")


if __name__ == "__main__":
    main()
