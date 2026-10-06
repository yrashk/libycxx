#!/usr/bin/env python3
"""Generate include/ycxx/core/format_unicode_tables.hpp: the Unicode properties <format> needs.

usage: tools/gen_unicode_tables.py <ucd-dir>

<ucd-dir> holds these files of the Unicode Character Database (https://www.unicode.org/Public/
UCD/latest/ucd/; the subdirectories may be flattened):
  DerivedCoreProperties.txt          Grapheme_Extend, Indic_Conjunct_Break (InCB)
  extracted/DerivedGeneralCategory.txt
  EastAsianWidth.txt
  auxiliary/GraphemeBreakProperty.txt
  emoji/emoji-data.txt               Extended_Pictographic
The version is read from the files' headers and written into the table header.

Two run tables are emitted (each entry: first code point of a run << 8 | the run's property
byte; a run extends to the next entry):
  - `cluster_runs`, for the field width ([format.string.std]/13): bits 0-3 Grapheme_Cluster_Break
    (Hangul LV/LVT are computed, not stored), bit 4 Extended_Pictographic, bits 5-6 InCB, bit 7
    field width 2 (East_Asian_Width W or F, U+4DC0-U+4DFF, U+1F300-U+1F5FF, U+1F900-U+1F9FF);
  - `escape_runs`, for [format.string.escaped]: bit 0 General_Category in the groups Z or C,
    bit 1 Grapheme_Extend.
"""
import pathlib, re, sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from uglify import uglify_text  # noqa: E402  (the headers spell reserved names, DECISIONS §2)

GCB = ["Other", "CR", "LF", "Control", "Extend", "ZWJ", "Regional_Indicator", "Prepend", "SpacingMark",
       "L", "V", "T", "LV", "LVT"]
INCB = {"None": 0, "Linker": 1, "Consonant": 2, "Extend": 3}
MAX = 0x110000


def find(d, name):
    for p in [d / name] + list(d.rglob(name)):
        if p.exists():
            return p
    sys.exit(f"missing {name} in {d}")


def version(text):
    m = re.search(r"-(\d+\.\d+\.\d+)\.txt", text) or re.search(r"Version (\d+\.\d+(?:\.\d+)?)", text)
    return m.group(1) if m else None


def entries(path, missing=True):
    """(first, last, fields) for each data line; @missing lines first (defaults)."""
    out_missing, out = [], []
    for line in path.read_text(encoding="utf-8").splitlines():
        m = re.match(r"#\s*@missing:\s*([0-9A-F]+)\.\.([0-9A-F]+)\s*;\s*([^#]*)", line)
        if m and missing:
            out_missing.append((int(m.group(1), 16), int(m.group(2), 16), [f.strip() for f in m.group(3).split(";")]))
            continue
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        rng, *fields = [f.strip() for f in line.split(";")]
        a, _, b = rng.partition("..")
        out.append((int(a, 16), int(b or a, 16), fields))
    return out_missing + out


def main():
    d = pathlib.Path(sys.argv[1])
    files = {n: find(d, n) for n in ["DerivedCoreProperties.txt", "DerivedGeneralCategory.txt", "EastAsianWidth.txt",
                                     "GraphemeBreakProperty.txt", "emoji-data.txt"]}
    vers = {n: version(p.read_text(encoding="utf-8")[:300]) for n, p in files.items()}
    ucd_version = vers["DerivedCoreProperties.txt"]

    gcb = bytearray(MAX)
    for a, b, f in entries(files["GraphemeBreakProperty.txt"]):
        v = GCB.index(f[0])
        if v in (GCB.index("LV"), GCB.index("LVT")):
            v = 0  # computed: U+AC00 + 28 k is LV, the other Hangul syllables LVT
        for c in range(a, b + 1):
            gcb[c] = v
    extpict = bytearray(MAX)
    for a, b, f in entries(files["emoji-data.txt"], missing=False):
        if f[0] == "Extended_Pictographic":
            for c in range(a, b + 1):
                extpict[c] = 1
    incb = bytearray(MAX)
    gext = bytearray(MAX)
    for a, b, f in entries(files["DerivedCoreProperties.txt"], missing=False):
        if f[0] == "InCB":
            for c in range(a, b + 1):
                incb[c] = INCB[f[1]]
        elif f[0] == "Grapheme_Extend":
            for c in range(a, b + 1):
                gext[c] = 1
    wide = bytearray(MAX)
    for a, b, f in entries(files["EastAsianWidth.txt"]):
        v = 1 if f[0] in ("W", "F") else 0
        for c in range(a, b + 1):
            wide[c] = v
    for a, b in [(0x4DC0, 0x4DFF), (0x1F300, 0x1F5FF), (0x1F900, 0x1F9FF)]:
        for c in range(a, b + 1):
            wide[c] = 1
    zc = bytearray(MAX)
    for c in range(MAX):
        zc[c] = 1  # default Cn
    for a, b, f in entries(files["DerivedGeneralCategory.txt"]):
        v = 1 if f[0][0] in "ZC" else 0
        for c in range(a, b + 1):
            zc[c] = v

    def runs(prop):
        out, prev = [], None
        for c in range(MAX):
            p = prop(c)
            if p != prev:
                out.append((c, p))
                prev = p
        return out

    cluster = runs(lambda c: gcb[c] | extpict[c] << 4 | incb[c] << 5 | wide[c] << 7)
    escape = runs(lambda c: zc[c] | gext[c] << 1)

    def table(name, rs, comment):
        lines = [f"// {comment}", f"inline constexpr unsigned int {name}[{len(rs)}] = {{"]
        row = []
        for c, p in rs:
            row.append(f"0x{(c << 8) | p:x}")
            if len(row) == 10:
                lines.append("    " + ", ".join(row) + ",")
                row = []
        if row:
            lines.append("    " + ", ".join(row) + ",")
        lines.append("};")
        return "\n".join(lines)

    out = pathlib.Path(__file__).resolve().parent.parent / "include/ycxx/core/format_unicode_tables.hpp"
    out.write_text(uglify_text(f"""// libycxx core: Unicode {ucd_version} properties for <format> (field width, escaping).
// Generated by tools/gen_unicode_tables.py from the Unicode Character Database {ucd_version}
// ({", ".join(f"{n} {v}" for n, v in vers.items() if v)}); do not edit.
#pragma once

namespace ycxx::detail::uni {{

inline constexpr char ucd_version[] = "{ucd_version}";

// Grapheme_Cluster_Break values (bits 0-3 of a cluster_runs entry; LV and LVT are computed).
enum gcb : unsigned char {{ {", ".join(f"gcb_{n.lower()}" for n in GCB)} }};
// Indic_Conjunct_Break values (bits 5-6).
enum incb : unsigned char {{ incb_none, incb_linker, incb_consonant, incb_extend }};

// Each entry is (first code point of a run << 8) | the run's properties; a run extends to the
// next entry's first code point.
{table("cluster_runs", cluster, "bits 0-3 Grapheme_Cluster_Break, bit 4 Extended_Pictographic, bits 5-6 InCB, bit 7 field width 2")}

{table("escape_runs", escape, "bit 0 General_Category in Z or C, bit 1 Grapheme_Extend")}

}} // namespace ycxx::detail::uni
"""))
    print(f"Unicode {ucd_version}: cluster_runs {len(cluster)}, escape_runs {len(escape)}")


main()
