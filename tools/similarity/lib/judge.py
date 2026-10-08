"""Attach the committed judgments (docs/similarity/*.toml) to the computed results.

Every long match and every fingerprint item that libycxx shares exclusively with another library
gets exactly one disposition:
  finding:<id>   a curated finding claims it: same libycxx file, overlapping the finding's
                 anchored line range, and the same other library (or a finding for all);
  rule:<name>    an automatic dismissal rule (docs/similarity/METHOD.md, "Automatic rules");
  group:<id>     a reviewed group in findings.toml lists its key (a hash, so the committed file
                 names nothing of the other implementations);
  unreviewed     none of these: it is listed openly on the site and counted in the CI log.

Keys:
  match:  sha256("match2" | other | libycxx file | enclosing libycxx declaration | other file |
          shared internal names)[:16] -- stable under edits inside the declaration and small
          range shifts; new for another declaration, another file or a newly shared name;
  item:   sha256(category | other | value)[:16].
"""
from __future__ import annotations

import hashlib
import re
import sys
import tomllib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from common import read, roots  # noqa: E402

DOCS = Path(__file__).resolve().parents[3] / "docs" / "similarity"


def load_judgments(docs: Path = DOCS) -> dict:
    out = {}
    for name in ("findings", "verdict", "tuning", "fingerprints"):
        p = docs / f"{name}.toml"
        out[name] = tomllib.loads(p.read_text()) if p.exists() else {}
    return out


def h16(*parts: str) -> str:
    return hashlib.sha256("\x1f".join(parts).encode()).hexdigest()[:16]


def item_key(category: str, other: str, value: str) -> str:
    return h16(category, other, str(value))


_NOT_A_DECLARATION = re.compile(r"^(?:$|//|/\*|\*|#|\}|\{|template\b|requires\b|\[\[|\)|,)")


def enclosing_anchor(ycxx_file: str, line: int) -> str:
    """The libycxx declaration a match lies in: the nearest line at or above `line` that starts at
    column 0 and opens a declaration (not a comment, directive, brace, template header or requires
    clause), whitespace-normalised. Edits to statements inside the declaration leave it unchanged."""
    src = read(roots()["ycxx"] / ycxx_file).split("\n")
    for i in range(min(line, len(src)) - 1, -1, -1):
        l = src[i]
        if l[:1] in (" ", "\t") or _NOT_A_DECLARATION.match(l.strip()):
            continue
        return re.sub(r"\s+", " ", l).strip()
    return ""


def match_key(m: dict) -> str:
    """Key of a long match for reviewed groups: the other library, the libycxx file, the enclosing
    libycxx declaration (enclosing_anchor), the other library's file and the internal names the two
    regions share. Stable under edits inside the declaration and under small shifts of the matched
    range between runs; a match in another declaration, against another file, or sharing a new
    internal name gets a new key and is reviewed again."""
    y = m["ycxx"]
    return h16("match2", m["other"], y["file"], enclosing_anchor(y["file"], y["lines"][0]), m["foreign"]["file"],
               ",".join(sorted(m.get("internal", []))))


def line_sha(line: str) -> str:
    return h16("line", re.sub(r"\s+", " ", line).strip())


def anchor_range(at: dict) -> tuple[str, int, int] | None:
    """Locate one libycxx range of a finding from its anchor: `anchor` (a text the line contains)
    or `anchor_sha` (line_sha of the whole line, for lines whose text should not be repeated in the
    judgments). `from`/`to` are offsets from the anchor line. None if the anchor is gone."""
    file = at["file"]
    p = roots()["ycxx"] / file
    if not p.exists():
        return None
    lines = read(p).split("\n")
    if "anchor" not in at and "anchor_sha" not in at:
        return file, 1, len(lines)
    for i, l in enumerate(lines):
        if ("anchor" in at and at["anchor"] in l) or ("anchor_sha" in at and line_sha(l) == at["anchor_sha"]):
            a = max(1, i + 1 + at.get("from", 0))
            b = min(len(lines), i + 1 + at.get("to", 0))
            return file, a, b
    return None


def resolve_findings(j: dict) -> list[dict]:
    out = []
    for f in j["findings"].get("finding", []):
        g = dict(f)
        g["resolved"], g["stale"] = [], []
        for at in f.get("at", []):
            r = anchor_range(at)
            if r is None:
                g["stale"].append(f"{at['file']}: anchor not found")
            else:
                g["resolved"].append({"file": r[0], "lines": [r[1], r[2]]})
        out.append(g)
    return out


def _pair_ok(f: dict, other: str) -> bool:
    return f["other"] == "all" or f["other"] == other or other in f.get("also", [])


def _claim(findings: list[dict], other: str, file: str, a: int, b: int, category: str):
    for f in findings:
        if not _pair_ok(f, other):
            continue
        if f.get("claims") and category not in f["claims"]:
            continue
        for r in f.get("resolved", []):
            if r["file"] == file and a <= r["lines"][1] and b >= r["lines"][0]:
                return f"finding:{f['id']}"
    return None


def group_index(j: dict) -> dict[str, str]:
    idx = {}
    for g in j["findings"].get("group", []):
        for k in g.get("keys", []):
            idx[k] = g["id"]
    return idx


def dispose_matches(annotated: list[dict], findings: list[dict], groups: dict[str, str]) -> list[dict]:
    for m in annotated:
        y = m["ycxx"]
        d = _claim(findings, m["other"], y["file"], y["lines"][0], y["lines"][1], "matches")
        if d is None and m["triage"] in ("standard-shaped", "shape-only"):
            d = f"rule:{m['triage']}"
        if d is None:
            k = match_key(m)
            m["key"] = k
            d = f"group:{groups[k]}" if k in groups else "unreviewed"
        m["disposition"] = d
    return annotated


def dispose_items(fp: dict, findings: list[dict], groups: dict[str, str]) -> list[dict]:
    """Flatten the fingerprint items libycxx shares exclusively with another library."""
    items = []
    for cat, res in fp["categories"].items():
        if cat == "comment_shingles":
            continue  # a calibration count; its matching lines appear under comment_lines
        for p in res["pairs"]:
            if p["a"] != "ycxx":
                continue
            other = p["b"]
            for it in p.get("items", []):
                if cat == "tables":
                    value = ",".join(it["sample"])
                    loc = (it["a"][0], it["a"][1])
                    floc = (it["b"][0], it["b"][1])
                else:
                    value = it["value"]
                    a = it.get("a") or []
                    loc = _first_loc(a)
                    floc = _first_loc(it.get("b") or [])
                rec = {"category": cat, "other": other, "value": value, "ycxx_loc": loc, "foreign_loc": floc,
                       "key": item_key(cat, other, value)}
                d = None
                if loc:
                    d = _claim(findings, other, loc[0], loc[1], loc[1], cat)
                if d is None and rec["key"] in groups:
                    d = f"group:{groups[rec['key']]}"
                rec["disposition"] = d or "unreviewed"
                items.append(rec)
    return items


def _first_loc(v):
    for x in v:
        if not x:
            continue
        if isinstance(x, (list, tuple)) and len(x) >= 2:
            if isinstance(x[1], (list, tuple)):  # (name, (file, line)) for cores
                if x[1]:
                    return (x[1][0], x[1][1])
            elif isinstance(x[1], int):
                return (x[0], x[1])
    return None
