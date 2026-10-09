"""Run JPlag per area: one submission per implementation, files flattened.

Every input file is first normalised by blanking comments and preprocessor
directives (licence headers, include guards and configuration live there);
line numbers are preserved so matches map back to the original files.

Output: <work>/jplag/<area>.json with the pairwise similarities and the
match list (file, line ranges and token length per match).
"""
from __future__ import annotations

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import json
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

from common import area_set
from common import area_files, impls_for_area, read
from kgram import positive_control_files
from lexer import blank_noncode

SUFFIX = ".cpp"


def flat_name(disp: str) -> str:
    return disp.replace("/", "~") + SUFFIX


def stage(area: str, dest: Path) -> dict[str, dict[str, str]]:
    """Write normalised copies; return impl -> {flat_name: display_path}."""
    mapping = {}
    for impl in impls_for_area(area):
        d = dest / impl
        d.mkdir(parents=True, exist_ok=True)
        mapping[impl] = {}
        for disp, path in area_files(area, impl):
            fn = flat_name(disp)
            (d / fn).write_text(blank_noncode(read(path)), encoding="utf-8")
            mapping[impl][fn] = disp
    pc = positive_control_files(area)
    if pc and "llvm" in mapping:
        d = dest / "llvm03"
        d.mkdir(parents=True, exist_ok=True)
        mapping["llvm03"] = {}
        for disp, path in pc:
            fn = flat_name(disp)
            (d / fn).write_text(blank_noncode(read(path)), encoding="utf-8")
            mapping["llvm03"][fn] = disp
    return mapping


def run_area(area: str, work: Path, jar: Path, language: str, min_tokens: int) -> dict:
    stage_dir = work / "stage" / area
    if stage_dir.exists():
        shutil.rmtree(stage_dir)
    mapping = stage(area, stage_dir)
    result_base = work / "jplag_raw" / area
    result_base.parent.mkdir(parents=True, exist_ok=True)
    for old in result_base.parent.glob(area + ".jplag*"):
        old.unlink()
    cmd = [
        "java", f"-XX:ActiveProcessorCount={os.environ.get('SIM_JOBS', '2')}", "-Xmx3g", "-jar", str(jar), "-l", language, "-t", str(min_tokens), "-M", "RUN",
        "-r", str(result_base), "--overwrite", "--cluster-skip", "-n", "-1", "--csv-export", "-p", SUFFIX,
        str(stage_dir),
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    zpath = result_base.with_suffix(".jplag")
    if not zpath.exists():
        raise RuntimeError(f"JPlag failed for {area}: {proc.stdout[-2000:]} {proc.stderr[-2000:]}")
    out = {"area": area, "language": language, "min_tokens": min_tokens, "pairs": []}
    with zipfile.ZipFile(zpath) as z:
        for n in z.namelist():
            if n.startswith("comparisons/") and n.endswith(".json"):
                c = json.loads(z.read(n))
                a, b = c["firstSubmissionId"], c["secondSubmissionId"]
                sims = c["similarities"]
                fa, fb = c.get("firstSimilarity") or 0.0, c.get("secondSimilarity") or 0.0
                matched = sum(m["lengthOfFirst"] for m in c["matches"])
                pair = {
                    "a": a, "b": b, "avg": sims["AVG"], "max": sims["MAX"], "longest": sims["LONGEST_MATCH"],
                    "matched_tokens": matched,
                    "len_a": round(matched / fa) if fa else None, "len_b": round(matched / fb) if fb else None,
                    "matches": [
                        {
                            "fa": mapping[a][m["firstFileName"].split("/", 1)[1]],
                            "fb": mapping[b][m["secondFileName"].split("/", 1)[1]],
                            "la": [m["startInFirst"]["line"], m["endInFirst"]["line"]],
                            "lb": [m["startInSecond"]["line"], m["endInSecond"]["line"]],
                            "tokens": m["lengthOfFirst"],
                        }
                        for m in c["matches"]
                    ],
                }
                out["pairs"].append(pair)
            elif n == "runInformation.json":
                out["run_information"] = json.loads(z.read(n))
    out["mapping"] = mapping
    zpath.unlink()
    shutil.rmtree(stage_dir)
    return out


def main():
    work, jar, language, min_tokens = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3], int(sys.argv[4])
    areas = sys.argv[5:] or list(area_set())
    outdir = work / os.environ.get("SIM_OUTNAME", "jplag")
    outdir.mkdir(parents=True, exist_ok=True)
    for area in areas:
        if len(impls_for_area(area)) < 2:
            continue
        try:
            res = run_area(area, work, jar, language, min_tokens)
        except RuntimeError as e:
            print(f"ERROR {area}: {e}", flush=True)
            continue
        (outdir / f"{area}.json").write_text(json.dumps(res))
        print(area, [(p["a"], p["b"], round(p["avg"], 3)) for p in res["pairs"]], flush=True)


if __name__ == "__main__":
    main()
