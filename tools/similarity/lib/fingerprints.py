"""Per-category fingerprint extraction over whole libraries.

Categories
  ident_exact     reserved (implementation-namespace) identifiers, exact spelling
  ident_core      the same reduced to a convention-free core (see lexer.norm_ident)
  numbers         non-trivial numeric literals outside data tables
  tables          literal tables (>= 8 numbers) and their longest common runs
  strings         string literals (normalised)
  comment_lines   comment lines of >= 6 words, normalised
  comment_shingles  8-word shingles of comment text (calibration metric only)
  typos           rare comment words that look like misspellings of frequent words

For each category and each pair of libraries we count items shared
*exclusively* by the pair (absent from every other compared library), which is
what distinguishes "copied from X" from "common practice".

Nothing here hard-codes content of any implementation: all filters are generic
rules (spelling classes, value classes, vocabulary of the standard as exposed by
libycxx's own public names).

Output: <work>/fingerprints.json
"""
from __future__ import annotations

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import json
import pickle
import re
from collections import Counter, defaultdict
from itertools import combinations
from pathlib import Path

from areas import FOREIGN
from common import corpus_files, read
from lexer import BUILTIN_RE, GENERIC_CORES, KEYWORDS, code_tokens, is_reserved, lex, norm_ident, template_params

LIBS = ["ycxx", "gnu", "llvm", "msvc"]
EXTRA = ["cxxrt"]

# ---------------------------------------------------------------------------
# generic exclusion rules

# Normalised cores whose name is the standard's own vocabulary (taken from the
# non-reserved identifiers libycxx itself exposes) are not evidence of anything.


def _int_value(text: str):
    t = text.replace("'", "").lower()
    t = re.sub(r"(?:ull|llu|ul|lu|ll|uz|zu|u|l|z)$", "", t)
    try:
        if t.startswith("0x"):
            if "p" in t or "." in t:
                return float.fromhex(t)
            return int(t, 16)
        if t.startswith("0b"):
            return int(t[2:], 2)
        if re.fullmatch(r"0[0-7]+", t):
            return int(t, 8)
        if re.fullmatch(r"\d+", t):
            return int(t)
        t2 = re.sub(r"(?:f16|f32|f64|f128|bf16|f|l)$", "", t)
        v = float(t2)
        return int(v) if v.is_integer() and "e" not in t2 and "." not in t2 else v
    except Exception:
        return None


def trivial_number(v) -> bool:
    if v is None:
        return True
    if isinstance(v, float):
        if v.is_integer():
            v = int(v)
        else:
            a = abs(v)
            # simple fractions and round decimals are common knowledge
            for d in (2, 3, 4, 5, 8, 10, 16, 100, 1000):
                if abs(a * d - round(a * d)) < 1e-12:
                    return True
            return False
    a = abs(v)
    if a <= 64:
        return True
    if a & (a - 1) == 0:  # power of two
        return True
    if (a + 1) & a == 0:  # 2^n - 1
        return True
    if (a - 1) & (a - 2) == 0:  # 2^n + 1
        return True
    s = str(a)
    if re.fullmatch(r"10*|9+|[1-9]0{2,}", s):  # powers of ten, 99..9, round numbers
        return True
    if a in (60, 3600, 86400, 604800, 31556952, 2629746, 146097, 719468, 1461, 36524, 365, 366, 1970, 400, 100,
             1000, 1000000, 1000000000, 0x10FFFF, 0xD800, 0xDBFF, 0xDC00, 0xDFFF, 0xFEFF, 0xFFFD, 0x10000,
             0x110000, 0xE000, 0x7F, 0x80, 0xC0, 0xE0, 0xF0, 0xF8):
        return True
    # bit masks: runs of hex f's/0's/5's/a's/3's/c's
    h = f"{a:x}"
    if re.fullmatch(r"(?:f+0*|0*f+|5+|a+|3+|c+|0f+|f0+|(?:ff00)+|(?:00ff)+|(?:0f)+|(?:f0)+|8+0*|7f+)", h):
        return True
    return False


WORD_RE = re.compile(r"[A-Za-z][A-Za-z']+")
LICENSE_RE = re.compile(r"licen[cs]e|warrant|copyright|spdx|free software|apache|gnu general|runtime library "
                        r"exception|merchantab|redistribut|all rights reserved|permission is hereby", re.I)


def comment_text(raw: str) -> list[str]:
    """Comment token -> list of normalised lines."""
    s = raw
    if s.startswith("//"):
        s = s[2:]
    elif s.startswith("/*"):
        s = s[2:-2]
    out = []
    for line in s.split("\n"):
        line = re.sub(r"^\s*[*/!<]+", "", line)
        out.append(line)
    return out


def norm_words(line: str) -> list[str]:
    return [w.lower() for w in WORD_RE.findall(line)]


# ---------------------------------------------------------------------------


class LibFacts:
    """Everything the extractors need from one library, with locations."""

    def __init__(self, lib: str):
        self.lib = lib
        self.ident = defaultdict(list)  # exact spelling -> [(file, line)]
        self.public_idents = set()  # non-reserved identifiers
        self.tparams = set()
        self.numbers = defaultdict(list)  # canonical value -> [(file,line)]
        self.tables = []  # (file, line, [values])
        self.strings = defaultdict(list)  # normalised -> [(file, line, raw)]
        self.comment_lines = defaultdict(list)  # normalised line -> [(file, line, raw)]
        self.comment_words = Counter()
        self.word_locs = defaultdict(list)
        self.shingles = set()
        self.ntokens = 0
        self.nfiles = 0


def _string_value(text: str) -> str:
    m = re.match(r'^(?:u8|u|U|L)?R"([^(]*)\((.*)\)\1"$', text, re.S)
    if m:
        return m.group(2)
    m = re.match(r'^(?:u8|u|U|L)?"(.*)"$', text, re.S)
    return m.group(1) if m else text


def gather(lib: str) -> LibFacts:
    f = LibFacts(lib)
    for disp, path in corpus_files(lib):
        src = read(path)
        toks = lex(src)
        f.nfiles += 1
        ct = code_tokens(toks)
        f.ntokens += len(ct)
        f.tparams |= template_params(ct)
        # identifiers
        for t in ct:
            if t.kind == "id":
                if is_reserved(t.text):
                    if len(f.ident[t.text]) < 6:
                        f.ident[t.text].append((disp, t.line))
                    else:
                        f.ident[t.text].append(None) if len(f.ident[t.text]) < 50 else None
                else:
                    f.public_idents.add(t.text)
        # numbers and tables: a table is a run of >= 8 'number ,' items
        i, n = 0, len(ct)
        while i < n:
            if ct[i].kind == "num" or (ct[i].text == "-" and i + 1 < n and ct[i + 1].kind == "num"):
                j = i
                vals = []
                while j < n:
                    neg = False
                    if ct[j].text == "-" and j + 1 < n and ct[j + 1].kind == "num":
                        neg = True
                        j += 1
                    if ct[j].kind != "num":
                        break
                    v = _int_value(ct[j].text)
                    vals.append(-v if (neg and v is not None) else v)
                    j += 1
                    if j < n and ct[j].text == ",":
                        j += 1
                        continue
                    break
                if len(vals) >= 8:
                    f.tables.append((disp, ct[i].line, vals))
                    i = j
                    continue
                for k in range(i, j if j > i else i + 1):
                    if ct[k].kind == "num":
                        v = _int_value(ct[k].text)
                        if not trivial_number(v):
                            if len(f.numbers[v]) < 8:
                                f.numbers[v].append((disp, ct[k].line, ct[k].text))
                i = max(j, i + 1)
                continue
            i += 1
        # strings
        for t in ct:
            if t.kind == "str":
                val = _string_value(t.text)
                key = re.sub(r"\s+", " ", val.strip().lower())
                if len(key) >= 6 and re.search(r"[a-z]{3}", key):
                    if len(f.strings[key]) < 8:
                        f.strings[key].append((disp, t.line, val[:200]))
        # comments
        for t in toks:
            if t.kind != "comment":
                continue
            lines = comment_text(t.text)
            words_all = []
            for k, line in enumerate(lines):
                if LICENSE_RE.search(line):
                    continue
                w = norm_words(line)
                words_all.extend(w)
                if len(w) >= 6:
                    key = " ".join(w)
                    if len(f.comment_lines[key]) < 8:
                        f.comment_lines[key].append((disp, t.line + k, line.strip()[:240]))
                for x in w:
                    f.comment_words[x] += 1
                    if len(f.word_locs[x]) < 4:
                        f.word_locs[x].append((disp, t.line + k))
            for k in range(len(words_all) - 7):
                f.shingles.add(" ".join(words_all[k:k + 8]))
    return f


def load_facts(work: Path, lib: str) -> LibFacts:
    cache = work / f"facts-{lib}.pickle"
    stamp = json.dumps([str(p) for _, p in corpus_files(lib)][:5] + [len(corpus_files(lib))])
    if cache.exists():
        try:
            st, facts = pickle.loads(cache.read_bytes())
            if st == stamp:
                return facts
        except Exception:
            pass
    facts = gather(lib)
    cache.write_bytes(pickle.dumps((stamp, facts)))
    return facts


# ---------------------------------------------------------------------------


def words_of(core: str) -> list[str]:
    parts = re.split(r"_+", core)
    return [p for p in parts if p]


STD_VOCAB: set[str] = set()


def ident_sets(facts: dict[str, LibFacts]):
    """Return (exact, core) dicts lib -> set, after generic exclusions."""
    std_vocab = set(facts["ycxx"].public_idents) | STD_VOCAB
    exact, core = {}, {}
    for lib, f in facts.items():
        e, c = set(), set()
        for name in f.ident:
            if BUILTIN_RE.match(name) or name in f.tparams or len(name) < 6:
                continue
            k = norm_ident(name)
            if k in std_vocab or k in KEYWORDS or len(k) < 4 or k in GENERIC_CORES:
                continue
            e.add(name)
            if len(k) >= 8 and len(words_of(k)) >= 2:
                c.add(k)
        exact[lib], core[lib] = e, c
    return exact, core


def exclusive_pairs(sets: dict[str, set], libs: list[str]):
    """For each pair: items in both and in no other lib of `libs`."""
    out = {}
    for a, b in combinations(libs, 2):
        others = [sets[x] for x in libs if x not in (a, b) and x in sets]
        shared = sets[a] & sets[b]
        excl = {x for x in shared if not any(x in o for o in others)}
        out[(a, b)] = (shared, excl)
    return out


def edit1(a: str, b: str) -> bool:
    if a == b or abs(len(a) - len(b)) > 1:
        return False
    if len(a) == len(b):
        d = [i for i in range(len(a)) if a[i] != b[i]]
        return len(d) == 1 or (len(d) == 2 and d[1] == d[0] + 1 and a[d[0]] == b[d[1]] and a[d[1]] == b[d[0]])
    if len(a) > len(b):
        a, b = b, a
    for i in range(len(b)):
        if b[:i] + b[i + 1:] == a:
            return True
    return False


def main(work: Path | None = None):
    work = work or Path(sys.argv[1])
    vocab_file = work / "std_vocab.json"
    if vocab_file.exists():
        STD_VOCAB.update(json.loads(vocab_file.read_text())["vocabulary"])
    libs = list(LIBS)
    facts = {lib: load_facts(work, lib) for lib in libs}
    have_cxxrt = bool(corpus_files("cxxrt"))
    if have_cxxrt:
        facts["cxxrt"] = load_facts(work, "cxxrt")
    # the positive control: libc++'s C++03 fork
    if corpus_files("llvm03"):
        facts["llvm03"] = load_facts(work, "llvm03")
    out = {"libs": {l: {"files": facts[l].nfiles, "tokens": facts[l].ntokens} for l in facts}, "categories": {}}

    def loc(lib, items, n=4):
        return [x for x in items if x][:n]

    def emit(cat, sets, detail_fn, note):
        res = {"note": note, "sizes": {l: len(s) for l, s in sets.items()}, "pairs": []}
        main_libs = [l for l in LIBS]
        for (a, b), (shared, excl) in exclusive_pairs({l: sets[l] for l in main_libs}, main_libs).items():
            denom = (len(sets[a]) * len(sets[b])) ** 0.5 or 1
            res["pairs"].append({"a": a, "b": b, "shared": len(shared), "exclusive": len(excl),
                                 "shared_norm": len(shared) / denom, "exclusive_norm": len(excl) / denom,
                                 "items": [detail_fn(x, a, b) for x in sorted(excl, key=str)] if "ycxx" in (a, b)
                                 else [], "calib_sample": [str(x) for x in sorted(excl, key=str)[:0]]})
        # the extra libraries (positive control, libcxxrt) against each main one: exclusive of the
        # other main libraries
        for x in ("llvm03", "cxxrt"):
            if x not in sets:
                continue
            for l in main_libs:
                shared = sets[l] & sets[x]
                excl = {v for v in shared if not any(v in sets[o] for o in main_libs if o != l)}
                denom = (len(sets[l]) * len(sets[x])) ** 0.5 or 1
                res["pairs"].append({"a": l, "b": x, "shared": len(shared), "exclusive": len(excl),
                                     "shared_norm": len(shared) / denom, "exclusive_norm": len(excl) / denom,
                                     "items": [detail_fn(v, l, x) for v in sorted(excl, key=str)]
                                     if l == "ycxx" and x == "cxxrt" else []})
        out["categories"][cat] = res

    # identifiers -----------------------------------------------------------
    exact, core = ident_sets(facts)
    emit("ident_exact", exact,
         lambda x, a, b: {"value": x, "a": loc(a, facts[a].ident[x]), "b": loc(b, facts[b].ident[x]),
                          "words": len(words_of(norm_ident(x)))},
         "reserved identifiers, exact spelling; builtins, ABI-specified names, template parameters and "
         "names whose core is standard vocabulary removed")
    core_locs = {l: defaultdict(list) for l in facts}
    for l, f in facts.items():
        for name, ls in f.ident.items():
            k = norm_ident(name)
            if k in core[l] and len(core_locs[l][k]) < 4:
                core_locs[l][k].append((name, ls[0]))
    emit("ident_core", core,
         lambda x, a, b: {"value": x, "a": core_locs[a][x][:3], "b": core_locs[b][x][:3]},
         "convention-free cores of reserved identifiers with >= 2 words and >= 8 characters")

    # numbers ---------------------------------------------------------------
    nums = {l: {v for v in f.numbers} for l, f in facts.items()}
    emit("numbers", nums,
         lambda x, a, b: {"value": repr(x), "a": facts[a].numbers[x][:4], "b": facts[b].numbers[x][:4]},
         "numeric literals outside tables, excluding small values, powers of two/ten, masks, calendar and "
         "Unicode constants")

    # strings ---------------------------------------------------------------
    strs = {l: set(f.strings) for l, f in facts.items()}
    emit("strings", strs,
         lambda x, a, b: {"value": x, "a": facts[a].strings[x][:3], "b": facts[b].strings[x][:3]},
         "string literals of >= 6 characters, case and whitespace normalised")

    # comment lines ---------------------------------------------------------
    cl = {l: set(f.comment_lines) for l, f in facts.items()}
    emit("comment_lines", cl,
         lambda x, a, b: {"value": x, "a": facts[a].comment_lines[x][:3], "b": facts[b].comment_lines[x][:3]},
         "comment lines of >= 6 words, punctuation/case normalised, licence text removed")

    # shingles (counts only) -----------------------------------------------
    sh = {l: f.shingles for l, f in facts.items()}
    emit("comment_shingles", sh, lambda x, a, b: {"value": x}, "8-word shingles of comment text")
    for p in out["categories"]["comment_shingles"]["pairs"]:
        p["items"] = p["items"][:60]

    # typos -----------------------------------------------------------------
    allwords = Counter()
    for f in facts.values():
        allwords.update(f.comment_words)
    frequent = [w for w, c in allwords.items() if c >= 30 and len(w) >= 5]
    by_len = defaultdict(list)
    for w in frequent:
        by_len[len(w)].append(w)

    def misspelling_of(w):
        if allwords[w] > 6 or len(w) < 6:
            return None
        for L in (len(w) - 1, len(w), len(w) + 1):
            for c in by_len.get(L, []):
                # inflections (plural, possessive, -d/-r) and prefix/suffix variants are not typos
                if "'" in w or w.startswith(c) or c.startswith(w) or w.endswith(c) or c.endswith(w):
                    continue
                if c != w and edit1(w, c) and allwords[c] >= 20 * allwords[w]:
                    return c
        return None

    typo = {}
    typo_of = {}
    for l, f in facts.items():
        s = set()
        for w in f.comment_words:
            if allwords[w] <= 6 and len(w) >= 6:
                m = typo_of.get(w)
                if m is None and w not in typo_of:
                    m = misspelling_of(w)
                    typo_of[w] = m
                if m:
                    s.add(w)
        typo[l] = s
    emit("typos", typo,
         lambda x, a, b: {"value": x, "likely": typo_of.get(x), "a": facts[a].word_locs[x][:3],
                          "b": facts[b].word_locs[x][:3]},
         "comment words seen <= 6 times overall that are one edit away from a word seen >= 20x as often")

    # tables ----------------------------------------------------------------
    def lcs_run(x, y):
        best, bi, bj = 0, 0, 0
        pos = defaultdict(list)
        for j, v in enumerate(y):
            pos[v].append(j)
        for i, v in enumerate(x):
            for j in pos.get(v, [])[:64]:
                if i and j and x[i - 1] == y[j - 1]:
                    continue
                l = 0
                while i + l < len(x) and j + l < len(y) and x[i + l] == y[j + l]:
                    l += 1
                if l > best:
                    best, bi, bj = l, i, j
        return best, bi, bj

    def table_kind(vals):
        ints = [v for v in vals if isinstance(v, int)]
        if len(ints) < len(vals):
            return "floating"
        def isprime(n):
            if n < 2:
                return False
            if n % 2 == 0:
                return n == 2
            r = int(n ** 0.5)
            for d in range(3, r + 1, 2):
                if n % d == 0:
                    return False
            return True
        if all(0 <= v < 10**12 and isprime(v) for v in ints[:40]):
            return "primes"
        nz = [v for v in ints if v]
        for base in (10, 5, 2):
            if len(nz) >= 4 and all(nz[i + 1] == nz[i] * base for i in range(min(len(nz) - 1, 20))):
                return f"powers of {base}"
        if all(0 <= v <= 0x10FFFF for v in ints) and ints == sorted(ints):
            return "monotonic code-point-like"
        return "other"

    tab_pairs = []
    pairs_to_check = [("ycxx", o) for o in FOREIGN + (["cxxrt"] if have_cxxrt else [])] + \
                     [(l, "llvm03") for l in LIBS if "llvm03" in facts] + \
                     [(a, b) for a, b in combinations(FOREIGN, 2)]
    for a, b in pairs_to_check:
        hits = []
        idx = defaultdict(set)
        for ti, (_, _, vals) in enumerate(facts[b].tables):
            for k in range(len(vals) - 3):
                idx[tuple(vals[k:k + 4])].add(ti)
        for (fa, la, va) in facts[a].tables:
            cands = set()
            for k in range(len(va) - 3):
                cands |= idx.get(tuple(va[k:k + 4]), set())
            best = None
            for ti in cands:
                fb, lb, vb = facts[b].tables[ti]
                l, i, j = lcs_run(va, vb)
                if l >= 8 and (best is None or l > best[0]):
                    best = (l, i, j, fb, lb, vb)
            if best:
                l, i, j, fb, lb, vb = best
                hits.append({"a": [fa, la], "b": [fb, lb], "run": l, "len_a": len(va), "len_b": len(vb),
                             "kind": table_kind(va[i:i + l]), "sample": [repr(v) for v in va[i:i + min(l, 12)]]})
        tab_pairs.append({"a": a, "b": b, "tables_a": len(facts[a].tables), "tables_b": len(facts[b].tables),
                          "matching": len(hits), "items": hits if a == "ycxx" else hits[:0],
                          "kinds": dict(Counter(h["kind"] for h in hits))})
    out["categories"]["tables"] = {"note": "literal tables of >= 8 numbers; longest common contiguous run >= 8",
                                   "pairs": tab_pairs}

    (work / "fingerprints.json").write_text(json.dumps(out, default=str))
    for cat, res in out["categories"].items():
        print(cat, " ".join(f"{p['a']}-{p['b']}:{p.get('exclusive', p.get('matching'))}" for p in res["pairs"]))


if __name__ == "__main__":
    main()
