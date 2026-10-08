"""Render libycxx.org/similarity/ from the computed results and the committed judgments.

Pages (all in site/similarity/, which is not committed):
  index.html             overview: the verdict (static text from docs/similarity/verdict.toml, or a
                         "needs review" banner when a threshold stated there is crossed), key numbers
  matrix.html            every pair of libycxx, libstdc++, libc++, MSVC STL and the positive control,
                         all three metrics: symmetric matrices, distributions, per-area heat tables
  fingerprints.html      fingerprint matrices per category (counts only), classifications
  fingerprint-items.html every item libycxx shares exclusively with another library (values shown)
  tuning.html            tuning constants and design choices, one column per library
  findings.html          curated findings with side-by-side excerpts, and everything unreviewed
  method.html            docs/similarity/METHOD.md
The pages that quote other implementations carry their licence and attribution.
"""
from __future__ import annotations

import datetime
import html
import json
import re
import statistics
import subprocess
import sys
from collections import Counter, defaultdict
from itertools import combinations
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from areas import AREAS, CONTROL_AREAS, ERA_AREAS, STYLE_AREAS  # noqa: E402

HERE = Path(__file__).resolve().parent.parent
REPO = HERE.parent.parent
TEMPLATES = HERE / "templates"
SOURCES = json.loads((HERE / "sources.json").read_text())

LIBS = ["ycxx", "gnu", "llvm", "msvc", "llvm03"]
NAMES = {"ycxx": "libycxx", "gnu": "libstdc++", "llvm": "libc++", "msvc": "MSVC STL", "llvm03": "libc++ C++03 fork",
         "cxxrt": "libcxxrt", "all": "all three", "kokkos": "Kokkos mdspan", "beman": "Beman project"}
SHORT = {"ycxx": "libycxx", "gnu": "libstdc++", "llvm": "libc++", "msvc": "MSVC STL", "llvm03": "C++03 fork",
         "cxxrt": "libcxxrt", "kokkos": "Kokkos", "beman": "Beman"}
ALL_LIBS = LIBS + ["kokkos", "beman"]
LICENCE = {"gnu": SOURCES["libstdcxx"]["licence"], "llvm": SOURCES["libcxx"]["licence"],
           "llvm03": SOURCES["libcxx03"]["licence"], "msvc": SOURCES["msvcstl"]["licence"],
           "cxxrt": SOURCES["libcxxrt"]["licence"]}
ORIGIN = {"gnu": "GCC 16.2.0 libstdc++ (gcc.gnu.org)", "llvm": "LLVM 23.1.2 libc++/libc++abi (llvm.org)",
          "llvm03": "LLVM 23.1.2 libc++ (llvm.org)", "msvc": "microsoft/STL (github.com/microsoft/STL)",
          "cxxrt": "libcxxrt (github.com/libcxxrt/libcxxrt)"}
SHARED_ANCESTRY = {"charconv"}
METRICS = [("jplag", "JPlag average similarity", "JPlag 6.2.0, C/C++ scanner, minimum match 12 tokens"),
           ("structural", "k-gram Dice, structural", "identifiers and literals abstracted, k = 24"),
           ("lexical", "k-gram Dice, lexical", "identifiers kept with conventions normalised, k = 10")]
GROUP = {"ycxx": "c1", "base": "c2", "pc": "c3", "pcx": "c4"}
NAV = [("index.html", "01", "Overview"), ("matrix.html", "02", "Cross-library matrix"),
       ("style.html", "03", "Style control"),
       ("fingerprints.html", "04", "Fingerprints"), ("tuning.html", "05", "Tuning and design"),
       ("findings.html", "06", "Findings and excerpts"), ("fingerprint-items.html", "07", "Fingerprint items"),
       ("method.html", "08", "Method")]


def esc(s) -> str:
    return html.escape(str(s), quote=True)


def key(a, b):
    if a in ALL_LIBS and b in ALL_LIBS:
        return tuple(sorted((a, b), key=ALL_LIBS.index))
    return tuple(sorted((a, b), key=str))


def pairs():
    return [(a, b) for a, b in combinations(LIBS, 2)]


def pair_group(a, b):
    s = {a, b}
    if s == {"llvm", "llvm03"}:
        return "pc"
    if "llvm03" in s:
        return "pcx"
    if "ycxx" in s:
        return "ycxx"
    return "base"


def pname(a, b):
    return f"{SHORT[a]} – {SHORT[b]}"


def fmt(v, d=3):
    return "–" if v is None else f"{v:.{d}f}"


# ------------------------------------------------------------------------------------- data


def collect_meta(repo: Path, roots: dict) -> dict:
    def git(*a):
        try:
            return subprocess.run(["git", "-C", str(repo), *a], capture_output=True, text=True).stdout.strip()
        except OSError:
            return ""
    java = ""
    try:
        java = subprocess.run(["java", "-version"], capture_output=True, text=True).stderr.splitlines()
        java = next((l for l in java if "version" in l), "")
    except OSError:
        pass
    return {"commit": git("rev-parse", "HEAD"), "commit_date": git("show", "-s", "--format=%cI", "HEAD"),
            "dirty": bool(git("status", "--porcelain", "--", "include", "src")),
            "generated": datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d %H:%M UTC"),
            "java": java, "python": sys.version.split()[0]}


def load(work: Path) -> dict:
    d = {"meta": json.loads((work / "meta.json").read_text())}
    d["jplag"] = {}
    for f in sorted((work / "jplag").glob("*.json")) if (work / "jplag").exists() else []:
        j = json.loads(f.read_text())
        d["jplag"][j["area"]] = {key(p["a"], p["b"]): p for p in j["pairs"]}
    d["kgram"] = {}
    for f in sorted((work / "kgram").glob("*.json")):
        j = json.loads(f.read_text())
        d["kgram"][j["structural"]["area"]] = {m: {key(p["a"], p["b"]): p for p in j[m]["pairs"]} for m in j}
    for suffix in ("sa", "sn", "sasn"):
        jd, kd = work / f"jplag_{suffix}", work / f"kgram_{suffix}"
        d[f"jplag_{suffix}"] = {}
        for f in sorted(jd.glob("*.json")) if jd.exists() else []:
            j = json.loads(f.read_text())
            d[f"jplag_{suffix}"][j["area"]] = {key(p["a"], p["b"]): p for p in j["pairs"]}
        d[f"kgram_{suffix}"] = {}
        for f in sorted(kd.glob("*.json")) if kd.exists() else []:
            j = json.loads(f.read_text())
            d[f"kgram_{suffix}"][j["structural"]["area"]] = {m: {key(p["a"], p["b"]): p for p in j[m]["pairs"]}
                                                             for m in j}
    d["fp"] = json.loads((work / "fingerprints.json").read_text())
    d["ann"] = json.loads((work / "annotated.json").read_text())
    d["tuning"] = json.loads((work / "tuning.json").read_text())
    return d


def metric_values(d, metric, suffix="", areas=None):
    """pair -> [(area, value)], every area where the pair was compared. suffix selects the style
    control's runs ("sa": era and control areas, "sn": style-normalised, "sasn": both)."""
    out = defaultdict(list)
    jk, kk = ("jplag_" + suffix, "kgram_" + suffix) if suffix else ("jplag", "kgram")
    for area in (areas if areas is not None else AREAS):
        src = d[jk].get(area) if metric == "jplag" else (d[kk].get(area) or {}).get(metric)
        if not src:
            continue
        for (a, b) in pairs():
            p = src.get(key(a, b))
            if p is None:
                continue
            out[(a, b)].append((area, p["avg"] if metric == "jplag" else p["dice"]))
    return out


def stats(vals):
    vals = sorted(v for v in vals)
    if not vals:
        return None
    q = statistics.quantiles(vals, n=4) if len(vals) >= 2 else [vals[0]] * 3
    return {"n": len(vals), "median": statistics.median(vals), "p25": q[0], "p75": q[2], "max": vals[-1],
            "min": vals[0]}


def pair_stats(d):
    """metric -> pair -> stats over areas, shared-ancestry areas excluded for every pair."""
    out = {}
    for m, _, _ in METRICS:
        mv = metric_values(d, m)
        out[m] = {p: stats([v for a, v in mv.get(p, []) if a not in SHARED_ANCESTRY]) for p in pairs()}
    return out


# ------------------------------------------------------------------------------------- verdict


def evaluate(d, ps, j, items, matches, findings) -> list[str]:
    """Return the thresholds crossed (docs/similarity/verdict.toml); empty means the static verdict
    stands."""
    t = j["verdict"].get("thresholds", {})
    crossed = []
    base_pairs = [("gnu", "llvm"), ("gnu", "msvc"), ("llvm", "msvc")]
    for m, label, _ in METRICS:
        lim = t.get(f"{m}_median_ratio")
        if lim is None:
            continue
        bmax = max(ps[m][p]["median"] for p in base_pairs if ps[m][p])
        for o in ("gnu", "llvm", "msvc"):
            s = ps[m][("ycxx", o)]
            if s and bmax and s["median"] / bmax > lim:
                crossed.append(f"{label}: libycxx–{SHORT[o]} median {s['median']:.3f} is more than {lim}× the "
                               f"largest median of the established-library pairs ({bmax:.3f})")
    crossed += style_crossed(d, t)
    pc = ps["jplag"][("llvm", "llvm03")]
    if t.get("positive_control_separation") and pc:
        for o in ("gnu", "llvm", "msvc"):
            s = ps["jplag"][("ycxx", o)]
            if s and s["p75"] >= pc["p25"]:
                crossed.append(f"JPlag: libycxx–{SHORT[o]} upper quartile {s['p75']:.3f} reaches the positive "
                               f"control's lower quartile {pc['p25']:.3f}")
    if t.get("max_unreviewed", 0) is not None:
        n = sum(1 for x in items + matches if x["disposition"] == "unreviewed")
        if n > t.get("max_unreviewed", 0):
            crossed.append(f"{n} long match(es) or fingerprint item(s) are not covered by a finding, a reviewed "
                           f"group or an automatic rule")
    if t.get("no_significant", True):
        sig = [f["id"] for f in findings if f.get("severity") == "significant"]
        if sig:
            crossed.append(f"significant finding(s): {', '.join(sig)}")
    stale = [f["id"] for f in findings if f.get("stale")]
    if stale:
        crossed.append(f"finding anchors no longer found in libycxx: {', '.join(stale)}")
    return crossed


# ------------------------------------------------------------------------------------- html helpers


def page(out: Path, name: str, title: str, body: str, meta: dict, description: str = "") -> None:
    tpl = (TEMPLATES / "page.html").read_text()
    nav = '<ul class="toc">' + "".join(
        '<li><a href="' + h + '"' + (' class="on"' if h == name else '') + f'><span>{n}</span>{esc(t)}</a></li>'
        for h, n, t in NAV) + '<li><a href="../index.html"><span>←</span>libycxx.org</a></li></ul>'
    commit = meta.get("commit", "")
    rep = {"title": esc(title), "description": esc(description or title), "nav": nav, "body": body,
           "commit": esc(commit), "commit_short": esc(commit[:12] or "unknown"),
           "generated": esc(meta.get("generated", ""))}
    s = re.sub(r"\{\{(\w+)\}\}", lambda m: rep[m.group(1)], tpl)
    (out / name).write_text(s)


def gh(meta, file, line=None):
    c = meta.get("commit") or "HEAD"
    return f"https://github.com/yrashk/libycxx/blob/{c}/{file}" + (f"#L{line}" if line else "")


def md_inline(s: str) -> str:
    s = esc(s)
    s = re.sub(r"`([^`]+)`", r"<code>\1</code>", s)
    s = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", s)
    s = re.sub(r"(?<![\w*])\*([^*\s][^*]*)\*(?![\w*])", r"<em>\1</em>", s)
    s = re.sub(r"\[([^\]]+)\]\(([^)\s]+)\)", r'<a href="\2">\1</a>', s)
    return s


def markdown(text: str) -> str:
    out, lines, i = [], text.split("\n"), 0
    while i < len(lines):
        l = lines[i]
        if l.startswith("```"):
            j = i + 1
            buf = []
            while j < len(lines) and not lines[j].startswith("```"):
                buf.append(lines[j])
                j += 1
            out.append("<pre><code>" + esc("\n".join(buf)) + "</code></pre>")
            i = j + 1
            continue
        m = re.match(r"^(#{1,4}) (.*)$", l)
        if m:
            n = len(m.group(1))
            hid = re.sub(r"[^a-z0-9]+", "-", m.group(2).lower()).strip("-")
            out.append(f'<h{n} id="{hid}">{md_inline(m.group(2))}</h{n}>')
            i += 1
            continue
        if l.startswith("|"):
            rows = []
            while i < len(lines) and lines[i].startswith("|"):
                rows.append([c.strip() for c in lines[i].strip().strip("|").split("|")])
                i += 1
            h = '<div class="table-wrap"><table class="data"><thead><tr>' + "".join(
                f"<th>{md_inline(c)}</th>" for c in rows[0]) + "</tr></thead><tbody>"
            for r in rows[2:]:
                h += "<tr>" + "".join(f"<td>{md_inline(c)}</td>" for c in r) + "</tr>"
            out.append(h + "</tbody></table></div>")
            continue
        if re.match(r"^\s*([-*]|\d+\.) ", l):
            ordered = bool(re.match(r"^\s*\d+\. ", l))
            items = []
            while i < len(lines) and (re.match(r"^\s*([-*]|\d+\.) ", lines[i]) or
                                      (lines[i].startswith("   ") and lines[i].strip())):
                if re.match(r"^\s*([-*]|\d+\.) ", lines[i]):
                    items.append(re.sub(r"^\s*([-*]|\d+\.) ", "", lines[i]))
                else:
                    items[-1] += " " + lines[i].strip()
                i += 1
            tag = "ol" if ordered else "ul"
            out.append(f"<{tag}>" + "".join(f"<li>{md_inline(x)}</li>" for x in items) + f"</{tag}>")
            continue
        if l.strip():
            buf = [l]
            i += 1
            while i < len(lines) and lines[i].strip() and not re.match(r"^(#|```|\||\s*([-*]|\d+\.) )", lines[i]):
                buf.append(lines[i])
                i += 1
            out.append(f"<p>{md_inline(' '.join(buf))}</p>")
            continue
        i += 1
    return "\n".join(out)


def heat(v, vmax):
    """Background for a heat cell: the accent hue, alpha by value."""
    if v is None:
        return ""
    a = 0 if vmax <= 0 else min(1.0, v / vmax)
    strong = " cell-strong" if a > 0.55 else ""
    return f' style="background:rgba(29,95,209,{0.08 + 0.85 * a:.2f})" class="num{strong}"'


def strip_svg(mv: dict, title: str) -> str:
    rows = [p for p in pairs()]
    W, left, right, rowh, top = 960, 250, 24, 30, 26
    allv = [v for p in rows for _, v in mv.get(p, [])] or [1]
    vmax = min(1.0, max(0.2, max(allv) * 1.05))
    H = top + rowh * len(rows) + 36

    def x(v):
        return left + (W - left - right) * v / vmax

    s = [f'<svg viewBox="0 0 {W} {H}" width="100%" role="img" aria-label="{esc(title)}">']
    step = 0.1 if vmax <= 0.5 else 0.2
    t = 0.0
    while t <= vmax + 1e-9:
        s.append(f'<line class="axis" x1="{x(t):.1f}" y1="{top - 12}" x2="{x(t):.1f}" y2="{H - 28}"/>')
        s.append(f'<text x="{x(t):.1f}" y="{H - 10}" text-anchor="middle">{t:.1f}</text>')
        t += step
    for i, (a, b) in enumerate(rows):
        y = top + i * rowh + rowh / 2
        g = GROUP[pair_group(a, b)]
        s.append(f'<text x="{left - 10}" y="{y + 4}" text-anchor="end">{esc(pname(a, b))}</text>')
        vals = sorted(mv.get((a, b), []), key=lambda t: t[1])
        for k, (area, v) in enumerate(vals):
            jit = ((k % 5) - 2) * 3
            s.append(f'<circle cx="{x(v):.1f}" cy="{y + jit:.1f}" r="4.5" fill="var(--{g})" stroke="var(--bg)" '
                     f'stroke-width="1.5"><title>{esc(pname(a, b))} · {esc(area)}: {v:.3f}</title></circle>')
        if vals:
            med = statistics.median(v for _, v in vals if _ not in SHARED_ANCESTRY) if any(
                _ not in SHARED_ANCESTRY for _, v in vals) else None
            if med is not None:
                s.append(f'<line x1="{x(med):.1f}" y1="{y - 12}" x2="{x(med):.1f}" y2="{y + 12}" stroke="var(--fg)" '
                         f'stroke-width="2.5"><title>median {med:.3f}</title></line>')
    s.append("</svg>")
    return "\n".join(s)


CHART_VARS = ('<style>.chart{--c1:#2a78d6;--c2:#eb6834;--c3:#1baf7a;--c4:#8a929c}'
              '@media (prefers-color-scheme:dark){.chart{--c1:#3987e5;--c2:#d95926;--c3:#199e70;--c4:#6c757f}}'
              '</style>')
LEGEND = ('<div class="legend"><span><i style="background:var(--c1)"></i>libycxx with an established library</span>'
          '<span><i style="background:var(--c2)"></i>two established libraries</span>'
          '<span><i style="background:var(--c3)"></i>positive control: libc++ with its C++03 fork</span>'
          '<span><i style="background:var(--c4)"></i>C++03 fork with the others</span></div>')


def excerpt(roots: dict, impl: str, file: str, a: int, b: int, ctx: int = 1, maxlines: int = 40, meta=None) -> str:
    base = roots["ycxx" if impl == "ycxx" else impl]
    p = Path(base) / file
    if not p.exists() and impl == "gnu":
        p = Path(roots["gnu_extra"]) / file
    if not p.exists():
        return f'<figure class="excerpt"><figcaption>{esc(file)}</figcaption><pre>(file not found)</pre></figure>'
    lines = p.read_text(errors="replace").split("\n")
    a0, b0 = max(1, a - ctx), min(len(lines), b + ctx)
    cut = ""
    if b0 - a0 + 1 > maxlines:
        b0 = a0 + maxlines - 1
        cut = f"\n      … ({b - b0} more lines)"
    body = "\n".join(f"{k:5d}  {lines[k - 1][:150]}" for k in range(a0, b0 + 1)) + cut
    if impl == "ycxx":
        cap = f'libycxx <a href="{gh(meta, file, a)}">{esc(file)}</a> {a}–{b}'
        lic = ""
    else:
        cap = f"{esc(NAMES[impl])} {esc(file)} {a}–{b}"
        lic_text = LICENCE[impl]
        if impl == "gnu":
            lic_text = (lic_text.split(" (")[1].rstrip(")") if file.startswith("libiberty/") else lic_text.split(" (")[0])
        lic = f'<div class="lic">Excerpt from {esc(ORIGIN[impl])}, {esc(lic_text)}. Quoted for analysis.</div>'
    return f'<figure class="excerpt"><figcaption>{cap}</figcaption><pre>{esc(body)}</pre>{lic}</figure>'


# ------------------------------------------------------------------------------------- pages


def build(work: Path, out: Path) -> dict:
    import judge  # noqa: E402
    from common import roots
    d = load(work)
    meta = d["meta"]
    j = judge.load_judgments()
    findings = judge.resolve_findings(j)
    groups = judge.group_index(j)
    matches = judge.dispose_matches(d["ann"], findings, groups)
    items = judge.dispose_items(d["fp"], findings, groups)
    ps = pair_stats(d)
    crossed = evaluate(d, ps, j, items, matches, findings)
    if out.exists():
        for f in out.iterdir():
            f.unlink()
    out.mkdir(parents=True, exist_ok=True)
    (out / "similarity.css").write_text((TEMPLATES / "similarity.css").read_text())
    r = roots()
    disp = Counter(x["disposition"].split(":")[0] for x in items + matches)
    ctx = dict(d=d, meta=meta, j=j, findings=findings, matches=matches, items=items, ps=ps, crossed=crossed, roots=r)
    page(out, "index.html", "Overview", render_index(**ctx), meta,
         "How similar libycxx is to libstdc++, libc++ and the MSVC STL, and how similar those are to each other.")
    page(out, "matrix.html", "Cross-library matrix", render_matrix(**ctx), meta)
    if d.get("jplag_sa") or d.get("kgram_sa"):
        page(out, "style.html", "Style control", render_style(**ctx), meta)
    page(out, "fingerprints.html", "Fingerprints", render_fingerprints(**ctx), meta)
    page(out, "fingerprint-items.html", "Fingerprint items", render_items(**ctx), meta)
    page(out, "tuning.html", "Tuning constants and design choices", render_tuning(**ctx), meta)
    page(out, "findings.html", "Findings and excerpts", render_findings(**ctx), meta)
    method = (REPO / "docs" / "similarity" / "METHOD.md").read_text()
    page(out, "method.html", "Method", '<p class="label">Similarity analysis · method</p><div class="md">' +
         markdown(method) + "</div>", meta)
    summary = {"dispositions": dict(disp), "unreviewed": disp.get("unreviewed", 0), "needs_review": crossed}
    (out / "summary.json").write_text(json.dumps({"meta": meta, **summary,
                                                  "medians": {m: {"-".join(p): (s["median"] if s else None)
                                                                  for p, s in v.items()} for m, v in ps.items()}},
                                                 indent=1))
    return summary


def render_index(d, meta, j, findings, matches, items, ps, crossed, roots):
    v = j["verdict"]
    h = ['<p class="label">Similarity analysis · libycxx ' + esc(meta.get("commit", "")[:12]) + "</p>",
         "<h1>How similar are the C++ standard libraries?</h1>",
         f'<p class="lede">{md_inline(v.get("lede", ""))}</p>']
    if meta.get("dirty"):
        h.append('<div class="banner warn"><p>This build was made from a working tree with uncommitted changes to '
                 'include/ or src/.</p></div>')
    if crossed:
        h.append('<div class="banner warn"><h3>Needs review</h3><p>' + md_inline(v.get("needs_review", "")) +
                 "</p><ul class=\"compact\">" + "".join(f"<li>{esc(c)}</li>" for c in crossed) + "</ul></div>")
    else:
        sv = style_numbers(d) if d.get("jplag_sn") else {}
        h.append('<div class="banner"><h3>Verdict</h3>' + "".join(f"<p>{md_inline(fill(p, sv))}</p>" for p in
                                                                 v.get("verdict", "").strip().split("\n\n")) +
                 '<p class="tiny">' + md_inline(v.get("verdict_condition", "")) + "</p></div>")
    jp = ps["jplag"]
    base = [jp[p]["median"] for p in (("gnu", "llvm"), ("gnu", "msvc"), ("llvm", "msvc")) if jp[p]]
    yc = [jp[("ycxx", o)]["median"] for o in ("gnu", "llvm", "msvc") if jp[("ycxx", o)]]
    pc = jp[("llvm", "llvm03")]
    n_find = Counter(f["severity"] for f in findings)
    unrev = sum(1 for x in items + matches if x["disposition"] == "unreviewed")
    h.append('<div class="kpis">'
             f'<div><b>{fmt(min(yc))}–{fmt(max(yc))}</b><span>libycxx pairs</span><small>JPlag median per pair, '
             f'libycxx with each established library</small></div>'
             f'<div><b>{fmt(min(base))}–{fmt(max(base))}</b><span>established pairs</span><small>the same between '
             f'libstdc++, libc++ and the MSVC STL</small></div>'
             f'<div><b>{fmt(pc["median"] if pc else None)}</b><span>positive control</span><small>libc++ with its own '
             f'C++03 fork</small></div>'
             f'<div><b>{n_find.get("significant", 0)} · {n_find.get("notable", 0)} · {unrev}</b><span>significant · '
             f'notable · unreviewed</span><small>curated findings and uncovered results</small></div></div>')
    if d.get("jplag_sn"):
        sv = style_numbers(d)
        h.append('<p class="note">Style control: with qualification and specifiers normalised for every library, '
                 f'libycxx\'s JPlag medians are {sv["norm_range"]}× the largest established-pair median over all '
                 f'areas; restricted to C++20-era components as well, about {sv["era_norm_residual"]}× '
                 '(<a href="style.html">style control</a>).</p>')
    h.append('<h3>All four libraries, every pair</h3><p>The analysis is symmetric: libstdc++, libc++ and the MSVC '
             'STL are compared with each other exactly as libycxx is compared with them. Each cell is the median '
             'over the library areas of the JPlag similarity (<a href="matrix.html">all three metrics, quartiles, '
             'per-area tables</a>).</p>')
    h.append(sym_matrix(ps["jplag"], compact=True))
    h.append('<h3>How it was measured</h3><ul class="compact">'
             '<li><b>Token and structure metrics</b>: JPlag, and two k-gram comparisons (identifiers abstracted, '
             'and identifiers kept with each library\'s naming conventions normalised), in 43 library areas.</li>'
             '<li><b>Fingerprints</b> over whole libraries: internal names, numeric literals and tables, string '
             'literals, comments, misspellings, and <a href="tuning.html">tuning constants and design choices</a>.</li>'
             '<li><b>Calibration</b>: the established libraries against each other, and a <b>positive control</b> '
             'of known derived code (libc++ against its C++03 fork).</li>'
             '<li><b>Judgments</b> are committed in advance in <a href="https://github.com/yrashk/libycxx/tree/HEAD/docs/'
             'similarity">docs/similarity</a>; every result is matched to a finding, a reviewed group or a stated '
             'rule, or listed as unreviewed.</li>'
             '<li>Regenerated on every site build from the pinned sources; <a href="method.html">method, '
             'normalisation and limitations</a>.</li></ul>')
    h.append(sources_table(meta))
    h.append('<p class="note">' + md_inline(v.get("separation_note", "")) + "</p>")
    return "\n".join(h)


def sources_table(meta):
    rows = [("libycxx", f'<a href="https://github.com/yrashk/libycxx/tree/{esc(meta.get("commit", ""))}">'
                        f'{esc(meta.get("commit", "")[:12])}</a> ({esc(meta.get("commit_date", "")[:10])})',
             "include/, src/")]
    for k, parts in (("libstdcxx", "include/, src/, libsupc++/, config/; libiberty's demangler"),
                     ("libcxx", ", ".join(SOURCES["libcxx"]["paths"])), ("libcxx03", "libcxx/include/__cxx03"),
                     ("msvcstl", ", ".join(SOURCES["msvcstl"]["paths"])), ("libcxxrt", "src/ (ABI areas only)")):
        s = SOURCES[k]
        ver = esc(s["version"]) + (f' <code>{esc(s["commit"][:12])}</code>' if "commit" in s else "")
        rows.append((esc(s["name"]), ver, esc(parts)))
    return ('<h3>Sources</h3><div class="table-wrap"><table class="data"><thead><tr><th>Library</th><th>Version'
            '</th><th>Parts compared</th></tr></thead><tbody>' + "".join(
                f"<tr><td>{a}</td><td>{b}</td><td>{c}</td></tr>" for a, b, c in rows) + "</tbody></table></div>")


def sym_matrix(st, compact=False):
    vmax = max((s["median"] for s in st.values() if s), default=1)
    h = ['<div class="table-wrap"><table class="data mx"><thead><tr><th></th>' +
         "".join(f"<th>{esc(SHORT[x])}</th>" for x in LIBS) + "</tr></thead><tbody>"]
    for a in LIBS:
        h.append(f'<tr><th scope="row">{esc(SHORT[a])}</th>')
        for b in LIBS:
            if a == b:
                h.append('<td class="muted">—</td>')
                continue
            p = (a, b) if (a, b) in st else (b, a)
            s = st.get(p)
            if not s:
                h.append('<td class="muted">–</td>')
                continue
            extra = "" if compact else f"<small>{fmt(s['p25'])}–{fmt(s['p75'])} · {s['n']} areas</small>"
            h.append(f"<td{heat(s['median'], vmax)}>{fmt(s['median'])}{extra}</td>")
        h.append("</tr>")
    h.append("</tbody></table></div>")
    return "".join(h)


def render_matrix(d, meta, j, findings, matches, items, ps, crossed, roots):
    h = ['<p class="label">Similarity analysis · cross-library matrix</p>',
         "<h1>Every pair, every metric</h1>",
         '<p class="lede">libycxx, libstdc++, libc++, the MSVC STL and the positive control (libc++\'s C++03 fork, '
         'known derived code) compared pairwise in 43 library areas. The established libraries are compared with each '
         'other exactly as libycxx is compared with them.</p>',
         '<p class="note">Medians and quartiles leave out charconv (marked †), where the three established libraries '
         'share Ryu code; in the distribution plots they are still drawn (the far-right points of the established pairs). '
         'The C++03 fork exists only for the areas libc++ had in 2024. Hover a point for its area.</p>', CHART_VARS]
    for m, label, note in METRICS:
        mv = metric_values(d, m)
        h.append(f'<h2 id="{m}" style="margin-top:48px">{esc(label)}</h2><p class="note">{esc(note)}. Cell: median '
                 f'over areas, then the interquartile range and the number of areas.</p>')
        h.append(sym_matrix(ps[m]))
        h.append(f'<div class="chart">{LEGEND}{strip_svg(mv, label)}</div>')
    h.append('<h2 style="margin-top:48px">Per area</h2>')
    for m, label, _ in METRICS:
        mv = metric_values(d, m)
        byarea = defaultdict(dict)
        for p, vals in mv.items():
            for a, v in vals:
                byarea[a][p] = v
        vmax = max((v for x in byarea.values() for v in x.values()), default=1)
        h.append(f"<h3>{esc(label)}</h3>")
        h.append('<div class="table-wrap"><table class="data heat"><thead><tr><th>Area</th>' + "".join(
            f'<th class="num">{esc(SHORT[a])}<br>{esc(SHORT[b])}</th>' for a, b in pairs()) + "</tr></thead><tbody>")
        for area in AREAS:
            if area not in byarea:
                continue
            h.append(f"<tr><td>{esc(area)}{' †' if area in SHARED_ANCESTRY else ''}</td>" + "".join(
                f"<td{heat(byarea[area].get(p), vmax)}>{fmt(byarea[area].get(p))}</td>" if p in byarea[area]
                else '<td class="num muted">–</td>' for p in pairs()) + "</tr>")
        h.append("</tbody></table></div>")
    return "\n".join(h)


FP_CATS = [("ident_exact", "Internal identifiers, exact spelling"),
           ("ident_core", "Internal identifiers, convention-free core"),
           ("numbers", "Numeric literals"), ("tables", "Literal tables"), ("strings", "String literals"),
           ("comment_lines", "Comment lines"), ("comment_shingles", "Comment 8-word shingles"),
           ("typos", "Misspellings")]


def fp_counts(fp, cat):
    out = {}
    for p in fp["categories"][cat]["pairs"]:
        out[key(p["a"], p["b"])] = p.get("exclusive", p.get("matching"))
    return out


def render_fingerprints(d, meta, j, findings, matches, items, ps, crossed, roots):
    fpj = j["fingerprints"]
    h = ['<p class="label">Similarity analysis · fingerprints</p>', "<h1>Fingerprints</h1>",
         f'<p class="lede">{md_inline(fpj.get("lede", ""))}</p>',
         '<p class="note">Each cell counts the distinct items shared <em>exclusively</em> by the two libraries, '
         'absent from every other compared library. This page shows counts only; the items themselves, with their '
         'locations and dispositions, are on <a href="fingerprint-items.html">Fingerprint items</a>.</p>']
    libs = LIBS + ["cxxrt"]
    for cat, label in FP_CATS:
        c = fpj.get("category", {}).get(cat, {})
        counts = fp_counts(d["fp"], cat)
        vmax = max([v for (a, b), v in counts.items() if {a, b} != {"llvm", "llvm03"}] or [1])
        h.append(f'<h2 id="{cat}" style="margin-top:48px">{esc(label)}</h2>')
        if c.get("extraction"):
            h.append(f"<p>{md_inline(c['extraction'])}</p>")
        h.append('<div class="table-wrap"><table class="data mx"><thead><tr><th></th>' + "".join(
            f"<th>{esc(SHORT[x])}</th>" for x in libs) + "</tr></thead><tbody>")
        for a in libs:
            h.append(f'<tr><th scope="row">{esc(SHORT[a])}</th>')
            for b in libs:
                if a == b:
                    h.append('<td class="muted">—</td>')
                    continue
                v = counts.get(key(a, b)) if a in LIBS and b in LIBS else counts.get((a, b), counts.get((b, a)))
                h.append(f"<td{heat(v, vmax)}>{v}</td>" if v is not None else '<td class="muted">–</td>')
            h.append("</tr>")
        h.append("</tbody></table></div>")
        if c.get("classification"):
            h.append(f'<p><span class="sev sev-{esc(c.get("severity", "expected"))}">{esc(c.get("severity", ""))}'
                     f'</span> {md_inline(c["classification"])}</p>')
        n = Counter(x["disposition"].split(":")[0] for x in items if x["category"] == cat)
        if n:
            h.append(f'<p class="tiny">libycxx items in this category: {n.get("finding", 0)} claimed by findings, '
                     f'{n.get("group", 0)} in reviewed groups, {n.get("unreviewed", 0)} unreviewed.</p>')
    h.append('<p class="note">libcxxrt is compared only where it has code (the ABI runtime). Pairs with the C++03 '
             'fork count items exclusive of the four main libraries other than the pair\'s own.</p>')
    return "\n".join(h)


def render_items(d, meta, j, findings, matches, items, ps, crossed, roots):
    h = ['<p class="label">Similarity analysis · fingerprint items</p>', "<h1>Fingerprint items shared with libycxx</h1>",
         '<p class="lede">Every item libycxx shares exclusively with one other library, with its libycxx location, '
         'the other library\'s location and its disposition. Values quote the other implementations: '
         + "; ".join(f"{esc(NAMES[k])}: {esc(LICENCE[k])}" for k in ("gnu", "llvm", "msvc", "cxxrt")) + ".</p>"]
    for cat, label in FP_CATS:
        sel = [x for x in items if x["category"] == cat]
        if not sel:
            continue
        h.append(f'<h2 id="{cat}" style="margin-top:40px">{esc(label)} ({len(sel)})</h2>')
        for o in ("gnu", "llvm", "msvc", "cxxrt"):
            s2 = [x for x in sel if x["other"] == o]
            if not s2:
                continue
            un = [x for x in s2 if x["disposition"] == "unreviewed"]
            body = ['<div class="table-wrap"><table class="data"><thead><tr><th>Item</th><th>libycxx</th><th>' +
                    esc(SHORT[o]) + "</th><th>Disposition</th></tr></thead><tbody>"]
            for x in sorted(s2, key=lambda x: (x["disposition"] != "unreviewed", str(x["value"]))):
                yl = x["ycxx_loc"]
                ycell = f'<a href="{gh(meta, yl[0], yl[1])}">{esc(yl[0])}:{yl[1]}</a>' if yl else "–"
                fl = x["foreign_loc"]
                fcell = f"{esc(fl[0])}:{fl[1]}" if fl else "–"
                dsp = x["disposition"]
                dcls = "unreviewed" if dsp == "unreviewed" else "expected"
                body.append(f"<tr><td><code>{esc(str(x['value'])[:160])}</code></td><td>{ycell}</td><td>{fcell}</td>"
                            f'<td><span class="sev sev-{dcls}">{esc(dsp)}</span></td></tr>')
            body.append("</tbody></table></div>")
            h.append(f'<details class="list"{" open" if un else ""}><summary>{esc(NAMES[o])}: {len(s2)} items, '
                     f'{len(un)} unreviewed</summary>' + "".join(body) + "</details>")
    return "\n".join(h)


def render_tuning(d, meta, j, findings, matches, items, ps, crossed, roots):
    tj = j["tuning"]
    tun = d["tuning"]
    h = ['<p class="label">Similarity analysis · tuning constants and design choices</p>',
         "<h1>Tuning constants and design choices</h1>",
         f'<p class="lede">{md_inline(tj.get("lede", ""))}</p>',
         '<p class="note">The cells of the other three libraries are read from the pinned sources at build time by '
         'the extractors in tools/similarity/lib/tuning.py, at the locations pinned in tuning-pins.json; a cell the '
         'extractor cannot read says so. The libycxx column, the row labels and the notes are committed in '
         'docs/similarity/tuning.toml. Source lines quoted here: ' +
         "; ".join(f"{esc(NAMES[k])}: {esc(LICENCE[k])}" for k in ("gnu", "llvm", "msvc")) + ".</p>"]
    h.append('<div class="table-wrap"><table class="data tune"><colgroup><col class="c-dec"><col class="c-y">'
             '<col class="c-o"><col class="c-o"><col class="c-o"><col class="c-n"></colgroup><thead><tr><th>Decision</th>'
             '<th>libycxx</th><th>libstdc++</th><th>libc++</th><th>MSVC STL</th><th>Forced or arbitrary</th></tr>'
             '</thead><tbody>')
    for row in tj.get("row", []):
        rid = row["id"]
        cells = tun.get(rid, {})
        tds = [f"<td><b>{esc(row['label'])}</b></td>", f"<td>{md_inline(row['ycxx'])}"]
        mes = cells.get("ycxx_measured")
        if mes and mes.get("status") == "ok":
            tds[-1] += f'<div class="tiny">measured: {esc(mes["value"])}</div>'
        tds[-1] += "</td>"
        for o in ("gnu", "llvm", "msvc"):
            c = cells.get(o, {"status": "not determined", "reason": "no extractor result"})
            if c.get("status") != "ok":
                tds.append(f'<td class="muted">not determined<div class="tiny">{esc(c.get("reason", ""))}</div></td>')
                continue
            v = esc(c["value"]) if c.get("value") else ""
            src = ""
            if c.get("source"):
                src = (f'<pre class="src">{esc(c["source"].strip()[:600])}</pre><div class="tiny">{esc(c["file"])} '
                       f'{c["lines"][0]}–{c["lines"][1]}</div>')
            elif c.get("files"):
                src = f'<div class="tiny">{esc(", ".join(c["files"]))}</div>'
            tds.append(f"<td>{v}{src}</td>")
        tds.append(f'<td><span class="sev sev-{esc(row.get("kind", "expected"))}">{esc(row.get("verdict", ""))}</span>'
                   f'<div class="tiny" style="margin-top:6px">{md_inline(row.get("note", ""))}</div></td>')
        h.append("<tr>" + "".join(tds) + "</tr>")
    h.append("</tbody></table></div>")
    return "\n".join(h)


def render_findings(d, meta, j, findings, matches, items, ps, crossed, roots):
    fj = j["findings"]
    h = ['<p class="label">Similarity analysis · findings</p>', "<h1>Findings and excerpts</h1>",
         f'<p class="lede">{md_inline(fj.get("lede", ""))}</p>',
         '<p class="note">Severity: ' + md_inline(fj.get("severity_criteria", "")) + "</p>"]
    sev = Counter(f["severity"] for f in findings)
    h.append('<div class="kpis">' + "".join(
        f'<div><b>{sev.get(s, 0)}</b><span>{s}</span></div>' for s in ("significant", "notable", "expected")) +
        f'<div><b>{sum(1 for x in items + matches if x["disposition"] == "unreviewed")}</b><span>unreviewed</span>'
        f'</div></div>')
    for f in findings:
        h.append(f'<article class="finding" id="{esc(f["id"])}"><h3><span class="sev sev-{esc(f["severity"])}">'
                 f'{esc(f["severity"])}</span>{esc(f["id"])} · {esc(f["title"])}</h3>')
        others = ", ".join(NAMES[x] for x in [f["other"]] + f.get("also", []))
        locs = "; ".join(f'<a href="{gh(meta, r["file"], r["lines"][0])}">{esc(r["file"])} {r["lines"][0]}–'
                         f'{r["lines"][1]}</a>' for r in f["resolved"])
        h.append(f'<p class="meta">{esc(f["category"])} · matches {esc(others)} · libycxx: {locs or "–"}</p>')
        if f.get("stale"):
            h.append(f'<p class="banner warn">{esc("; ".join(f["stale"]))}</p>')
        h.append("".join(f"<p>{md_inline(p)}</p>" for p in f["rationale"].strip().split("\n\n")))
        n_claim = sum(1 for x in items + matches if x["disposition"] == f"finding:{f['id']}")
        if n_claim:
            h.append(f'<p class="tiny">Claims {n_claim} computed result(s) this build.</p>')
        for fo in f.get("foreign", [])[:2]:
            yr = f["resolved"][0] if f["resolved"] else None
            if yr:
                ya, yb = yr["lines"]
                yb = min(yb, ya + 40)
                h.append('<div class="side-by-side">' + excerpt(roots, "ycxx", yr["file"], ya, yb, meta=meta) +
                         excerpt(roots, fo["lib"], fo["file"], fo["lines"][0], fo["lines"][1], meta=meta) + "</div>")
        h.append("</article>")
    # unreviewed
    un_m = [m for m in matches if m["disposition"] == "unreviewed"]
    un_i = [x for x in items if x["disposition"] == "unreviewed"]
    h.append(f'<h2 id="unreviewed" style="margin-top:56px">Unreviewed ({len(un_m) + len(un_i)})</h2>')
    if not un_m and not un_i:
        h.append("<p>Every long match and every fingerprint item of this build is covered by a finding, a reviewed "
                 "group or an automatic rule.</p>")
    for m in un_m:
        h.append(f'<article class="finding"><h3><span class="sev sev-unreviewed">unreviewed</span>{esc(m["area"])} · '
                 f'{esc(NAMES[m["other"]])} · {m["tokens"]} tokens ({esc(m["source"])})</h3>'
                 f'<p class="meta">shared internal names: {esc(", ".join(m["internal"]) or "none")} · lexical ratio '
                 f'{m["lex_ratio"]} · standard vocabulary {m["std_frac"]} · key {esc(m.get("key", ""))}</p>'
                 '<div class="side-by-side">' +
                 excerpt(roots, "ycxx", m["ycxx"]["file"], *m["ycxx"]["lines"], meta=meta) +
                 excerpt(roots, m["other"], m["foreign"]["file"], *m["foreign"]["lines"], meta=meta) +
                 "</div></article>")
    if un_i:
        h.append('<div class="table-wrap"><table class="data"><thead><tr><th>Category</th><th>Item</th><th>libycxx'
                 '</th><th>Other</th><th>Key</th></tr></thead><tbody>')
        for x in un_i:
            yl, fl = x["ycxx_loc"], x["foreign_loc"]
            h.append(f"<tr><td>{esc(x['category'])}</td><td><code>{esc(str(x['value'])[:160])}</code></td><td>" +
                     (f'<a href="{gh(meta, yl[0], yl[1])}">{esc(yl[0])}:{yl[1]}</a>' if yl else "–") +
                     f"</td><td>{esc(SHORT[x['other']])} {esc(fl[0]) + ':' + str(fl[1]) if fl else ''}</td>"
                     f"<td><code>{esc(x['key'])}</code></td></tr>")
        h.append("</tbody></table></div>")
    # groups and rules
    h.append('<h2 style="margin-top:56px">Reviewed groups and automatic rules</h2>')
    rows = []
    gc = Counter(x["disposition"] for x in items + matches)
    for g in fj.get("group", []):
        rows.append((g["id"], g.get("severity", "expected"), g["title"], g["rationale"], gc.get(f"group:{g['id']}", 0)))
    for rid, text in fj.get("rules", {}).items():
        rows.append((rid, "expected", "automatic rule", text, gc.get(f"rule:{rid}", 0)))
    h.append('<div class="table-wrap"><table class="data"><thead><tr><th>Group or rule</th><th>Severity</th><th>'
             'What</th><th class="num">This build</th></tr></thead><tbody>' + "".join(
                 f'<tr><td><code>{esc(a)}</code></td><td><span class="sev sev-{esc(b)}">{esc(b)}</span></td><td><b>'
                 f'{esc(c)}</b><div class="tiny">{md_inline(t)}</div></td><td class="num">{n}</td></tr>'
                 for a, b, c, t, n in rows) + "</tbody></table></div>")
    return "\n".join(h)


def style_stats(d):
    """condition -> metric -> pair -> stats over areas (charconv left out where present)."""
    conds = {"all": ("", None), "era": ("sa", ERA_AREAS), "norm": ("sn", None), "era_norm": ("sasn", ERA_AREAS)}
    out = {}
    for c, (suf, areas) in conds.items():
        out[c] = {}
        for m, _, _ in METRICS:
            mv = metric_values(d, m, suf, areas)
            out[c][m] = {p: stats([v for a, v in mv.get(p, []) if a not in SHARED_ANCESTRY]) for p in pairs()}
    return out


def style_numbers(d):
    """Numbers the committed style sentences quote ({placeholders} in verdict.toml and style.toml)."""
    ss = style_stats(d)
    n = [v for v in elevation(ss["norm"]["jplag"]).values() if v is not None]
    en = [v for v in elevation(ss["era_norm"]["jplag"]).values() if v is not None]
    era = elevation(ss["era"]["jplag"])
    res = [v for v in en if v > 1.0] or en
    return {"norm_range": f"{min(n):.2f}–{max(n):.2f}" if n else "–",
            "era_norm_residual": f"{min(res):.1f}–{max(res):.1f}" if res else "–",
            "era_llvm": f"{era['llvm']:.2f}" if era.get("llvm") else "–",
            "_norm": n, "_era_norm": en}


def style_crossed(d, t) -> list[str]:
    """The style-control thresholds of verdict.toml that this build crosses."""
    if not d.get("jplag_sn"):
        return []
    sn = style_numbers(d)
    out = []
    lim = t.get("style_norm_jplag_median_ratio")
    if lim is not None and any(v > lim for v in sn["_norm"]):
        out.append(f"JPlag, style-normalised, all areas: libycxx's largest ratio to the established pairs is "
                   f"{max(sn['_norm']):.2f}× (limit {lim}×)")
    lim = t.get("style_era_norm_jplag_median_ratio")
    if lim is not None and any(v > lim for v in sn["_era_norm"]):
        out.append(f"JPlag, style-normalised, C++20-era areas: largest ratio {max(sn['_era_norm']):.2f}× "
                   f"(limit {lim}×)")
    gap = t.get("style_era_norm_max_gap")
    en = sorted(sn["_era_norm"], reverse=True)
    if gap is not None and len(en) >= 2 and en[0] - en[1] > gap:
        out.append(f"JPlag, style-normalised, C++20-era areas: the largest ratio exceeds the second by "
                   f"{en[0] - en[1]:.2f} (limit {gap}), so the residual is concentrated on one library")
    return out


def fill(text: str, values: dict) -> str:
    return re.sub(r"\{(\w+)\}", lambda m: str(values.get(m.group(1), m.group(0))), text)


def elevation(st):
    """libycxx's median with each library over the largest median of the established pairs."""
    base = [st[p]["median"] for p in (("gnu", "llvm"), ("gnu", "msvc"), ("llvm", "msvc")) if st.get(p)]
    if not base:
        return {}
    b = max(base)
    return {o: (st[("ycxx", o)]["median"] / b if st.get(("ycxx", o)) and b else None) for o in ("gnu", "llvm", "msvc")}


COND_LABEL = {"all": "all areas", "era": "C++20-era areas", "norm": "all areas, style-normalised",
              "era_norm": "C++20-era areas, style-normalised"}


def render_style(d, meta, j, findings, matches, items, ps, crossed, roots):
    sj = j.get("style", {})
    ss = style_stats(d)
    h = ['<p class="label">Similarity analysis · style control</p>', "<h1>Style control</h1>",
         f'<p class="lede">{md_inline(sj.get("lede", ""))}</p>']
    concl = sj.get("conclusion", "")
    sc = style_crossed(d, j["verdict"].get("thresholds", {}))
    if sc:
        h.append('<div class="banner warn"><h3>Needs review</h3><p>The conclusion written for this page no longer '
                 'matches this build\'s numbers:</p><ul class="compact">' + "".join(f"<li>{esc(c)}</li>" for c in sc) +
                 '</ul><p>Read the tables without it until docs/similarity/style.toml is reviewed.</p></div>')
        concl = ""
    concl = fill(concl, style_numbers(d))
    if concl:
        h.append('<div class="banner"><h3>What the controls show</h3>' + "".join(
            f"<p>{md_inline(p)}</p>" for p in concl.strip().split("\n\n")) + "</div>")
    for p in sj.get("logic", "").strip().split("\n\n"):
        if p:
            h.append(f"<p>{md_inline(p)}</p>")
    # 1. before and after, every pair, every metric
    h.append('<h2 style="margin-top:48px">Every pair, before and after</h2><p class="note">Median over areas. '
             '"Elevation" is libycxx\'s median with that library divided by the largest median among the three '
             'established pairs under the same condition.</p>')
    for m, label, note in METRICS:
        h.append(f"<h3>{esc(label)}</h3>")
        h.append('<div class="table-wrap"><table class="data"><thead><tr><th>Pair</th>' + "".join(
            f'<th class="num">{esc(COND_LABEL[c])}</th>' for c in COND_LABEL) + "</tr></thead><tbody>")
        for p in pairs():
            cells = []
            for c in COND_LABEL:
                st = ss[c][m].get(p)
                cells.append(f'<td class="num">{fmt(st["median"]) if st else "–"}'
                             f'{"<div class=tiny>" + str(st["n"]) + " areas</div>" if st else ""}</td>')
            h.append(f"<tr><td>{esc(pname(*p))}</td>" + "".join(cells) + "</tr>")
        for o in ("gnu", "llvm", "msvc"):
            h.append(f'<tr><td><b>elevation, libycxx – {esc(SHORT[o])}</b></td>' + "".join(
                f'<td class="num"><b>{fmt(elevation(ss[c][m]).get(o), 2)}×</b></td>' for c in COND_LABEL) + "</tr>")
        h.append("</tbody></table></div>")
    # 2. matrices
    h.append('<h2 style="margin-top:48px">Era-restricted areas</h2><p>' + md_inline(sj.get("era_note", "")) +
             '</p><p class="tiny">Areas: ' + esc(", ".join(a[4:] for a in ERA_AREAS)) + ".</p>")
    for m, label, _ in METRICS:
        h.append(f"<h3>{esc(label)} · {esc(COND_LABEL['era'])}</h3>" + sym_matrix(ss["era"][m]))
    h.append('<h2 style="margin-top:48px">Style normalisation</h2><p>' + md_inline(sj.get("norm_note", "")) + "</p>")
    for m, label, _ in METRICS:
        h.append(f"<h3>{esc(label)} · {esc(COND_LABEL['norm'])}</h3>" + sym_matrix(ss["norm"][m]))
    # 3. independent controls
    h.append('<h2 style="margin-top:48px">Independent modern implementations</h2><p>' +
             md_inline(sj.get("control_note", "")) + "</p>")
    for area in CONTROL_AREAS:
        libs = [x for x in ALL_LIBS if STYLE_AREAS[area].get(x)]
        h.append(f"<h3>{esc(area[4:])}</h3>")
        for cond, suf in (("raw", "sa"), ("style-normalised", "sasn")):
            rows = []
            for m, label, _ in METRICS:
                src = d["jplag_" + suf].get(area) if m == "jplag" else (d["kgram_" + suf].get(area) or {}).get(m)
                if not src:
                    continue
                rows.append((label, src))
            if not rows:
                continue
            h.append(f'<div class="table-wrap"><table class="data mx"><thead><tr><th>Pair ({esc(cond)})</th>' +
                     "".join(f"<th>{esc(l)}</th>" for l, _ in rows) + "</tr></thead><tbody>")
            for a, b in combinations(libs, 2):
                k = key(a, b)
                vals = []
                for _, src in rows:
                    p = src.get(k)
                    vals.append(fmt(p["avg"] if "avg" in p else p["dice"]) if p else "–")
                h.append(f'<tr><th scope="row">{esc(SHORT[a])} – {esc(SHORT[b])}</th>' +
                         "".join(f'<td class="num">{v}</td>' for v in vals) + "</tr>")
            h.append("</tbody></table></div>")
    return "\n".join(h)


def unavailable(out: Path, why: str) -> None:
    out.mkdir(parents=True, exist_ok=True)
    for f in out.iterdir():
        f.unlink()
    (out / "similarity.css").write_text((TEMPLATES / "similarity.css").read_text())
    meta = {"generated": datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d %H:%M UTC")}
    try:
        meta["commit"] = subprocess.run(["git", "-C", str(REPO), "rev-parse", "HEAD"], capture_output=True,
                                        text=True).stdout.strip()
    except OSError:
        pass
    body = ('<p class="label">Similarity analysis</p><h1>Similarity report unavailable for this build</h1>'
            '<div class="banner warn"><p>The similarity analysis did not complete when this version of the site was '
            'built, so no numbers are published rather than stale ones. The rest of the site is current.</p>'
            f'<p class="tiny">Reason: {esc(why[:500])}</p></div><p>The method and the curated judgments are in the '
            'repository: <a href="https://github.com/yrashk/libycxx/blob/HEAD/docs/similarity/METHOD.md">'
            'docs/similarity/METHOD.md</a>.</p>')
    for name, *_ in NAV:
        page(out, name, "Report unavailable", body, meta)
