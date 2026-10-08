"""Own token k-gram comparison, complementing JPlag.

Two token abstractions (see lexer.py):
  structural  identifiers/literals abstracted (comparable to JPlag), runs of
              three or more literal list elements collapsed (data tables);
  lexical     identifiers kept, reserved-name conventions normalised
              (underscores, member/static prefixes and a 'My' prefix removed, case folded;
              template parameter names collapsed to one placeholder).

For every area and every pair of implementations it reports
  dice        2|K(A) & K(B)| / (|K(A)| + |K(B)|) over distinct k-grams,
  cont_a      |K(A) & K(B)| / |K(A)|  (how much of A is found in B),
  runs        maximal common token runs (greedy tiling, k-gram seeded) of at
              least MIN_RUN tokens, with file and line ranges.
"""
from __future__ import annotations

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import json
import sys
import zlib
from collections import defaultdict
from itertools import combinations
from pathlib import Path

from areas import AREAS
from common import area_files, impls_for_area, read, roots
from lexer import code_tokens, lex, lexical, structural

PARAMS = {
    "structural": {"k": 24, "min_run": 60},
    "lexical": {"k": 10, "min_run": 25},
}


def _collapse_lists(stream: list[str], lines: list[int], files: list[int]):
    """Collapse runs of >= 3 'literal ,' pairs into one LIST token (data tables)."""
    out, ol, of = [], [], []
    i, n = 0, len(stream)
    while i < n:
        j = i
        while j + 1 < n and stream[j] in ("N", "S") and stream[j + 1] == ",":
            j += 2
        if (j - i) // 2 >= 3:
            out.append("LIST")
            ol.append(lines[i])
            of.append(files[i])
            i = j
            continue
        out.append(stream[i])
        ol.append(lines[i])
        of.append(files[i])
        i += 1
    return out, ol, of


class Corpus:
    """Token stream of one implementation's area, with provenance."""

    def __init__(self, impl: str, files: list[tuple[str, Path]], mode: str):
        self.impl = impl
        self.names = [d for d, _ in files]
        toks, lines, fidx = [], [], []
        for fi, (disp, path) in enumerate(files):
            ct = code_tokens(lex(read(path)))
            s = structural(ct) if mode == "structural" else lexical(ct)
            toks.extend(s)
            lines.extend(t.line for t in ct)
            fidx.extend([fi] * len(ct))
            # file separator so k-grams do not span files
            toks.append(f"<EOF{fi}>")
            lines.append(0)
            fidx.append(fi)
        if mode == "structural":
            toks, lines, fidx = _collapse_lists(toks, lines, fidx)
        self.toks, self.lines, self.fidx = toks, lines, fidx
        self.ntok = sum(1 for t in toks if not t.startswith("<EOF"))


def kgrams(c: Corpus, k: int) -> dict[int, list[int]]:
    idx = defaultdict(list)
    t = c.toks
    for i in range(len(t) - k + 1):
        win = t[i:i + k]
        if any(w.startswith("<EOF") for w in win):
            continue
        h = zlib.crc32("\x1f".join(win).encode())
        idx[h].append(i)
    return idx


def runs(a: Corpus, b: Corpus, ka: dict, kb: dict, k: int, min_run: int, max_cand: int = 16):
    """Greedy tiling of A against B seeded by shared k-grams. Returns list of
    (len, ia, ib); each A position is covered at most once."""
    pos_a = sorted(i for h, ps in ka.items() if h in kb for i in ps)
    covered_upto = -1
    out = []
    ta, tb = a.toks, b.toks
    for i in pos_a:
        if i <= covered_upto:
            continue
        h = zlib.crc32("\x1f".join(ta[i:i + k]).encode())
        best, bj = 0, -1
        for j in kb.get(h, [])[:max_cand]:
            l = 0
            while i + l < len(ta) and j + l < len(tb) and ta[i + l] == tb[j + l] and not ta[i + l].startswith("<EOF"):
                l += 1
            if l > best:
                best, bj = l, j
        if best >= min_run:
            out.append((best, i, bj))
            covered_upto = i + best - 1
    return out


def describe(c: Corpus, i: int, l: int) -> dict:
    j = i + l - 1
    ls = [x for x in c.lines[i:j + 1] if x]
    return {"file": c.names[c.fidx[i]], "lines": [min(ls), max(ls)] if ls else [0, 0]}


def compare_area(area: str, impl_files: dict[str, list], mode: str) -> dict:
    k, min_run = PARAMS[mode]["k"], PARAMS[mode]["min_run"]
    corp = {i: Corpus(i, f, mode) for i, f in impl_files.items()}
    grams = {i: kgrams(c, k) for i, c in corp.items()}
    res = {"area": area, "mode": mode, "k": k, "min_run": min_run, "tokens": {i: c.ntok for i, c in corp.items()},
           "pairs": []}
    for a, b in combinations(sorted(corp), 2):
        ka, kb = grams[a], grams[b]
        sa, sb = set(ka), set(kb)
        inter = len(sa & sb)
        dice = 2 * inter / (len(sa) + len(sb)) if sa and sb else 0.0
        rr = runs(corp[a], corp[b], ka, kb, k, min_run)
        rr.sort(reverse=True)
        res["pairs"].append({
            "a": a, "b": b, "dice": dice,
            "cont_a": inter / len(sa) if sa else 0.0, "cont_b": inter / len(sb) if sb else 0.0,
            "run_tokens": sum(r[0] for r in rr), "n_runs": len(rr), "longest": rr[0][0] if rr else 0,
            "runs": [{"tokens": l, "a": describe(corp[a], i, l), "b": describe(corp[b], j, l)} for l, i, j in rr[:40]],
        })
    return res


POSITIVE_CONTROL_PREFIX = ("libcxx/include/", "libcxx/include/__cxx03/")


def positive_control_files(area: str):
    """libc++'s frozen C++03 fork (copied from libc++ in 2024, then evolved separately):
    a known derived pair that shows what derivation scores look like."""
    globs = AREAS[area].get("llvm", [])
    out = {}
    base = roots()["llvm"]
    pre, rep = POSITIVE_CONTROL_PREFIX
    for g in globs:
        if not g.startswith(pre):
            continue
        g2 = rep + g[len(pre):]
        for p in sorted(base.glob(g2)):
            if p.is_file():
                out[p.relative_to(base).as_posix()] = p
    return sorted(out.items())


def main():
    work = Path(sys.argv[1])
    areas = sys.argv[2:] or list(AREAS)
    outdir = work / "kgram"
    outdir.mkdir(parents=True, exist_ok=True)
    for area in areas:
        impls = impls_for_area(area)
        if len(impls) < 2:
            continue
        files = {i: area_files(area, i) for i in impls}
        pc = positive_control_files(area)
        if len(pc) and "llvm" in files:
            files["llvm03"] = pc
        out = {m: compare_area(area, files, m) for m in PARAMS}
        (outdir / f"{area}.json").write_text(json.dumps(out))
        print(area, " ".join(f"{p['a']}-{p['b']}:{p['dice']:.3f}/{out['lexical']['pairs'][n]['dice']:.3f}"
                             for n, p in enumerate(out["structural"]["pairs"])), flush=True)


if __name__ == "__main__":
    main()
