"""Annotate every token-level match (JPlag matches and k-gram runs that involve
libycxx) so that a reviewer can separate standard-shaped code from evidence.

For each match it records
  lex_ratio      similarity (difflib ratio) of the two regions' lexical token
                 streams -- spelling included, conventions normalised;
  internal       identifier cores that occur in BOTH regions, were spelled as
                 reserved (implementation-internal) names, and are not the
                 standard's own vocabulary (see fingerprints/std_vocab.py);
  std_frac       share of libycxx's identifier tokens in the region that are
                 standard vocabulary;
and an automatic triage:
  standard-shaped   no shared internal names and >= 70% standard vocabulary
  shape-only        no shared internal names and lex_ratio < 0.5
                    (same token structure, different spelling)
  review            anything else -- goes to manual review.

Output: <work>/annotated.json
"""
from __future__ import annotations

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import difflib
import functools
import json
from pathlib import Path

from common import read, roots
from kgram import POSITIVE_CONTROL_PREFIX  # noqa: F401  (documented relationship)
from lexer import BUILTIN_RE, GENERIC_CORES, code_tokens, is_reserved, lex, norm_ident, template_params

IMPL_ROOT_KEYS = {"ycxx": "ycxx", "gnu": "gnu", "llvm": "llvm", "llvm03": "llvm", "msvc": "msvc", "cxxrt": "cxxrt"}


def path_of(impl: str, disp: str) -> Path:
    r = roots()
    p = r[IMPL_ROOT_KEYS[impl]] / disp
    if not p.exists() and impl == "gnu":
        p = r["gnu_extra"] / disp
    return p


@functools.cache
def file_tokens(impl: str, disp: str):
    toks = code_tokens(lex(read(path_of(impl, disp))))
    return toks, template_params(toks)


def region(impl: str, disp: str, lines: list[int]):
    toks, tp = file_tokens(impl, disp)
    return [t for t in toks if lines[0] <= t.line <= lines[1]], tp


def lexstream(toks, tp):
    out = []
    for t in toks:
        if t.kind == "id":
            out.append("_T" if t.text in tp else norm_ident(t.text))
        elif t.kind in ("num", "str", "chr"):
            out.append(t.text)
        else:
            out.append(t.text)
    return out


def annotate(a_impl, a_file, a_lines, b_impl, b_file, b_lines, vocab: set[str]) -> dict:
    ra, tpa = region(a_impl, a_file, a_lines)
    rb, tpb = region(b_impl, b_file, b_lines)
    la, lb = lexstream(ra, tpa), lexstream(rb, tpb)
    sm = difflib.SequenceMatcher(None, la, lb, autojunk=False)
    ratio = sm.ratio() if la and lb else 0.0

    def internals(toks, tp):
        s = set()
        for t in toks:
            if t.kind == "id" and is_reserved(t.text) and t.text not in tp and not BUILTIN_RE.match(t.text):
                k = norm_ident(t.text)
                if len(k) >= 4 and k not in vocab and k not in GENERIC_CORES:
                    s.add(k)
        return s

    shared = sorted(internals(ra, tpa) & internals(rb, tpb))
    ids = [t for t in (ra if a_impl == "ycxx" else rb) if t.kind == "id"]
    tpy = tpa if a_impl == "ycxx" else tpb
    std = sum(1 for t in ids if t.text in tpy or norm_ident(t.text) in vocab)
    std_frac = std / len(ids) if ids else 1.0
    if not shared and std_frac >= 0.7:
        triage = "standard-shaped"
    elif not shared and ratio < 0.5:
        triage = "shape-only"
    else:
        triage = "review"
    return {"lex_ratio": round(ratio, 3), "internal": shared, "std_frac": round(std_frac, 3), "triage": triage}


def main():
    work = Path(sys.argv[1])
    min_jplag, min_lex = int(sys.argv[2]) if len(sys.argv) > 2 else 40, 25
    vocab = set(json.loads((work / "std_vocab.json").read_text())["vocabulary"]) if (work / "std_vocab.json").exists() \
        else set()
    # libycxx's own public names are the standard's names too
    from common import corpus_files
    for _, p in corpus_files("ycxx"):
        for t in lex(read(p)):
            if t.kind == "id" and not is_reserved(t.text):
                vocab.add(t.text.lower())
    out = []
    for f in sorted((work / "jplag").glob("*.json")):
        d = json.loads(f.read_text())
        for p in d["pairs"]:
            if "ycxx" not in (p["a"], p["b"]) or "llvm03" in (p["a"], p["b"]):
                continue
            for m in p["matches"]:
                if m["tokens"] < min_jplag:
                    continue
                if p["a"] == "ycxx":
                    y, yl, o, of, ol = m["fa"], m["la"], p["b"], m["fb"], m["lb"]
                else:
                    y, yl, o, of, ol = m["fb"], m["lb"], p["a"], m["fa"], m["la"]
                ann = annotate("ycxx", y, yl, o, of, ol, vocab)
                out.append({"source": "jplag", "area": d["area"], "other": o, "tokens": m["tokens"],
                            "ycxx": {"file": y, "lines": yl}, "foreign": {"file": of, "lines": ol}, **ann})
    for f in sorted((work / "kgram").glob("*.json")):
        d = json.loads(f.read_text())
        for mode in ("lexical", "structural"):
            for p in d[mode]["pairs"]:
                if "ycxx" not in (p["a"], p["b"]) or "llvm03" in (p["a"], p["b"]):
                    continue
                for r in p["runs"]:
                    if mode == "structural" and r["tokens"] < 80:
                        continue
                    if r["tokens"] < min_lex:
                        continue
                    if p["a"] == "ycxx":
                        y, o, of = r["a"], p["b"], r["b"]
                    else:
                        y, o, of = r["b"], p["a"], r["a"]
                    ann = annotate("ycxx", y["file"], y["lines"], o, of["file"], of["lines"], vocab)
                    out.append({"source": f"kgram-{mode}", "area": d[mode]["area"], "other": o, "tokens": r["tokens"],
                                "ycxx": y, "foreign": of, **ann})
    # merge duplicates (same ycxx range and foreign file)
    seen = {}
    for x in out:
        key = (x["ycxx"]["file"], x["ycxx"]["lines"][0] // 10, x["other"], x["foreign"]["file"])
        if key not in seen or x["tokens"] > seen[key]["tokens"]:
            if key in seen:
                x.setdefault("also", []).append(seen[key]["source"])
            seen[key] = x
    out = sorted(seen.values(), key=lambda x: (x["triage"] != "review", -x["tokens"]))
    (work / "annotated.json").write_text(json.dumps(out, indent=0))
    from collections import Counter
    print(Counter((x["other"], x["triage"]) for x in out))


if __name__ == "__main__":
    main()
