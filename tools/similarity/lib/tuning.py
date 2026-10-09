"""Tuning constants and design choices, read from the pinned sources at build time.

Where to look is pinned in tools/similarity/tuning-pins.json (paths and line numbers only).
Each kind of extractor below is a generic reading rule; none contains another implementation's
names or values. A cell the rule cannot read says "not determined" with the reason, rather than
guessing. The libycxx column is committed (docs/similarity/tuning.toml); for the kinds that work
on a whole area (intrinsic-count, tuple-shape, recursive-union, tag-dispatch) libycxx is also
measured, as a cross-check of the committed text.

Output: <work>/tuning.json
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from common import area_files, read, roots  # noqa: E402
from lexer import code_tokens, lex  # noqa: E402

PINS = json.loads((Path(__file__).resolve().parent.parent / "tuning-pins.json").read_text())

ALGORITHM_NAMES = {
    "introsort": r"introsort|introspective sort",
    "pdqsort": r"pdqsort|pattern[- ]defeating",
    "block partitioning": r"blockquicksort|block[- ]?partition|bitset partition",
    "median of three": r"median[- ]of[- ](?:three|3)",
    "ninther": r"ninther",
    "heapsort fallback": r"heap ?sort",
    "Ryu": r"\bryu\b",
    "Grisu": r"\bgrisu",
    "Dragon4": r"dragon4",
    "Dragonbox": r"dragonbox",
    "Schubfach": r"schubfach",
    "Eisel-Lemire": r"eisel",
    "fast_float": r"fast[_ -]float",
    "strtod-based": r"\bstrto[df]\b",
    "big-integer fallback": r"big ?int|big[- ]integer|bignum",
}


def _path(impl: str, rel: str) -> Path:
    r = roots()
    return r[impl] / rel


def _code_lines(impl: str, rel: str, a: int, b: int) -> tuple[list[str], str]:
    p = _path(impl, rel)
    if not p.exists():
        raise LookupError(f"pinned file {rel} is missing")
    lines = read(p).split("\n")
    if b > len(lines):
        raise LookupError(f"pinned lines {a}-{b} are past the end of {rel} ({len(lines)} lines)")
    raw = lines[a - 1:b]
    code = [re.sub(r"//.*$", "", l) for l in raw]
    return raw, "\n".join(code)


def _ints(text: str) -> list[int]:
    return [int(x) for x in re.findall(r"(?<![\w.])(\d+)(?![\w.])", text)]


def _isprime(n: int) -> bool:
    """Deterministic Miller-Rabin for n < 3.3e24 (covers 64-bit tables)."""
    if n < 2:
        return False
    small = [p for p in range(2, 42) if all(p % q for q in range(2, p))]
    for p in small:
        if n % p == 0:
            return n == p
    d, r = n - 1, 0
    while d % 2 == 0:
        d //= 2
        r += 1
    for a in small:
        x = pow(a, d, n)
        if x in (1, n - 1):
            continue
        for _ in range(r - 1):
            x = x * x % n
            if x == n - 1:
                break
        else:
            return False
    return True


def _area_tokens(impl: str, areas: list[str]):
    for area in areas:
        for disp, path in area_files(area, impl):
            yield disp, code_tokens(lex(read(path)))


def extract(impl: str, spec: dict) -> dict:
    kind = spec["kind"]
    out = {"kind": kind}
    try:
        if kind in ("int", "text", "halving", "growth"):
            raw, code = _code_lines(impl, spec["file"], *spec["lines"])
            out.update(file=spec["file"], lines=spec["lines"], source="\n".join(raw))
            if kind == "int":
                v = _ints(code)
                if not v:
                    raise LookupError("no integer literal on the pinned line")
                out["value"] = ", ".join(str(x) for x in v)
            elif kind == "text":
                out["value"] = None  # shown as the pinned source
            elif kind == "halving":
                if re.search(r"/=\s*2\b|/\s*2\b|>>=\s*1\b|>>\s*1\b", code):
                    out["value"] = "halves the request until an allocation succeeds"
                else:
                    raise LookupError("no halving expression in the pinned lines")
            elif kind == "growth":
                flat = re.sub(r"\s+", " ", code)
                if re.search(r"(\w+) ?\+ ?\1 ?/ ?2\b", flat) or re.search(r"(\w+) ?\+ ?\(?\1 ?>> ?1", flat):
                    out["value"] = "1.5× (adds half the capacity)"
                elif re.search(r"\b2 ?\* ?\w+|\w+ ?\* ?2\b", flat):
                    out["value"] = "2× (doubles the capacity)"
                elif re.search(r"(\w+\(\)) ?\+ ?\(?(?:std::)?\(?max\)? ?(?:<[^>]*>)? ?\( ?\1", flat):
                    out["value"] = "2× (adds the current size)"
                else:
                    raise LookupError("no recognised growth expression in the pinned lines")
        elif kind == "algorithm-names":
            found = {}
            for rel in spec["files"]:
                p = _path(impl, rel)
                if not p.exists():
                    raise LookupError(f"pinned file {rel} is missing")
                text = read(p).lower()
                for name, rx in ALGORITHM_NAMES.items():
                    n = len(re.findall(rx, text))
                    if n:
                        found[name] = found.get(name, 0) + n
            out["files"] = spec["files"]
            if not found:
                raise LookupError("none of the known algorithm names occurs in the pinned files")
            out["value"] = ", ".join(f"{k} ({v})" for k, v in sorted(found.items(), key=lambda kv: -kv[1]))
        elif kind == "prime-table":
            out["files"] = spec["files"]
            best = None
            for rel in spec["files"]:
                p = _path(impl, rel)
                if not p.exists():
                    raise LookupError(f"pinned file {rel} is missing")
                toks = code_tokens(lex(read(p)))
                i = 0
                while i < len(toks):
                    if toks[i].kind == "num":
                        vals, j = [], i
                        while j < len(toks) and toks[j].kind == "num":
                            try:
                                vals.append(int(re.sub(r"[uUlLzZ']", "", toks[j].text), 0))
                            except ValueError:
                                break
                            j += 1
                            if j < len(toks) and toks[j].text == ",":
                                j += 1
                            else:
                                break
                        if len(vals) >= 8 and sum(_isprime(v) for v in vals) >= 0.9 * len(vals):
                            if best is None or len(vals) > len(best):
                                best = vals
                        i = max(j, i + 1)
                    else:
                        i += 1
            if not best:
                raise LookupError("no table of prime numbers in the pinned files")
            out["value"] = f"prime bucket counts: a table of {len(best)} entries, largest {max(best)}"
        elif kind == "intrinsic-count":
            calls, specs = 0, 0
            for _, toks in _area_tokens(impl, [spec["area"]]):
                for k, t in enumerate(toks[:-1]):
                    if t.kind == "id" and re.match(r"^__(?:is|has|remove|add|underlying|make|decay|reference|"
                                                    r"builtin_is|array_rank|array_extent)\w*$", t.text) \
                            and toks[k + 1].text == "(":
                        calls += 1
                    if t.kind == "kw" and t.text in ("struct", "class") and toks[k + 1].kind == "id" \
                            and k + 2 < len(toks) and toks[k + 2].text == "<" and k >= 2 and toks[k - 1].text == ">":
                        specs += 1
            out["value"] = f"{calls} compiler-intrinsic calls, {specs} class-template specializations"
        elif kind == "tuple-shape":
            rec, flat = 0, 0
            for _, toks in _area_tokens(impl, [spec["area"]]):
                for k, t in enumerate(toks):
                    if t.kind == "kw" and t.text in ("struct", "class") and k + 1 < len(toks) and toks[k + 1].kind == "id":
                        name = toks[k + 1].text
                        j = k + 2
                        depth = 0
                        while j < len(toks) and not (toks[j].text in (":", "{", ";") and depth == 0):
                            depth += toks[j].text == "<"
                            depth -= toks[j].text == ">"
                            depth -= 2 * (toks[j].text == ">>")
                            j += 1
                        if j < len(toks) and toks[j].text == ":":
                            e = j
                            while e < len(toks) and toks[e].text not in ("{", ";"):
                                e += 1
                            base = [x.text for x in toks[j + 1:e]]
                            if name in base and "..." in base:
                                rec += 1
                            elif "..." in base and len(base) > 2:
                                flat += 1
            if not rec and not flat:
                raise LookupError("no class template with a recursive or pack-expanded base in the area")
            out["value"] = f"{rec} self-recursive base clauses, {flat} pack-expanded base clauses"
        elif kind == "recursive-union":
            # a union (or a class holding an anonymous union) with a member of its own template:
            # the recursive-union storage
            n_rec = 0
            for _, toks in _area_tokens(impl, [spec["area"]]):
                for k, t in enumerate(toks):
                    if t.kind == "kw" and t.text in ("union", "class", "struct") and k + 1 < len(toks) \
                            and toks[k + 1].kind == "id":
                        name = toks[k + 1].text.rstrip("_")
                        j = k + 2
                        while j < len(toks) and toks[j].text not in ("{", ";"):
                            j += 1
                        if j < len(toks) and toks[j].text == "{":
                            depth, e = 0, j
                            while e < len(toks):
                                depth += toks[e].text == "{"
                                depth -= toks[e].text == "}"
                                if depth == 0:
                                    break
                                e += 1
                            body = toks[j + 1:e]
                            has_union = t.text == "union" or any(x.text == "union" for x in body)
                            if has_union and any(x.kind == "id" and x.text.rstrip("_") == name and
                                                 m + 1 < len(body) and body[m + 1].text == "<"
                                                 for m, x in enumerate(body)):
                                n_rec += 1
            if not n_rec:
                raise LookupError("no union storage that nests its own template in the area (it may be "
                                  "generated by a macro, which normalisation removes)")
            out["value"] = f"recursive union ({n_rec} definitions)"
        elif kind == "tag-dispatch":
            tag_params, constexpr_ifs = 0, 0
            for _, toks in _area_tokens(impl, spec["areas"]):
                for k, t in enumerate(toks[:-1]):
                    if t.kind == "id" and t.text.endswith("iterator_tag") and toks[k + 1].text in (")", ","):
                        tag_params += 1
                    if t.kind == "kw" and t.text == "if" and toks[k + 1].kind == "kw" and toks[k + 1].text == "constexpr":
                        window = " ".join(x.text for x in toks[k + 2:k + 30])
                        if re.search(r"iterator_tag|random_access_iterator|bidirectional_iterator|forward_iterator|"
                                     r"contiguous_iterator", window):
                            constexpr_ifs += 1
            out["value"] = f"{tag_params} overloads on an unnamed tag parameter, {constexpr_ifs} if-constexpr on the category"
        else:
            raise LookupError(f"unknown kind {kind}")
        out["status"] = "ok"
    except LookupError as e:
        out["status"] = "not determined"
        out["reason"] = str(e)
    return out


AREA_KINDS = {"intrinsic-count", "tuple-shape", "recursive-union", "tag-dispatch"}


def run(work: Path) -> dict:
    res = {}
    for row, libs in PINS.items():
        if row.startswith("_"):
            continue
        res[row] = {}
        for impl in ("gnu", "llvm", "msvc"):
            spec = libs.get(impl)
            if spec is None:
                res[row][impl] = {"status": "not determined",
                                  "reason": "no location is pinned: the reviewer found no such constant or choice"}
            else:
                res[row][impl] = extract(impl, spec)
        some = next((s for s in libs.values() if s), None)
        if some and some["kind"] in AREA_KINDS:
            res[row]["ycxx_measured"] = extract("ycxx", some)
    (work / "tuning.json").write_text(json.dumps(res, indent=1))
    return res
