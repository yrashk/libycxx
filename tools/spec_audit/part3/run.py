#!/usr/bin/env python3
"""Run the part-3 spec-audit probes (tools/spec_audit/part3).

    tools/spec_audit/part3/run.py [-c gcc|clang]... [-j N] [--libdir-root DIR] [--filter SUBSTR]

Compiles every probes/<subclause>.cpp with tools/ycxx-cxx (-fsyntax-only, warnings off), maps
each diagnostic to the probe line (a check) it was raised for, and compares the failed checks
with gaps.tsv, the known gaps (each with its class, docs/SPEC_COVERAGE.md part 3). Writes
results-<compiler>.tsv. Exit status 1 when a check fails that gaps.tsv does not list, or a listed
gap passes (an XPASS: remove its line).
"""
import argparse, concurrent.futures, os, pathlib, re, subprocess, sys

HERE = pathlib.Path(__file__).resolve().parent
REPO = HERE.parents[2]
LINE_ID = re.compile(r"// (\S+#\d+) ")


def check_ids(path):
    ids = {}
    for n, line in enumerate(path.read_text().splitlines(), 1):
        m = LINE_ID.search(line)
        if m:
            ids[n] = m.group(1)
    return ids


def compile_one(cc, path, libdir):
    cmd = [str(REPO / "tools" / "ycxx-cxx"), cc]
    if libdir:
        cmd.append(f"--libdir={libdir}")
    cmd += ["-fsyntax-only", "-w", "-fmax-errors=0" if cc == "gcc" else "-ferror-limit=0", str(path)]
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace")
    return p.returncode, p.stdout


def compile_freestanding(cc, path):
    """As tools/check_freestanding.sh compiles a core header: no C library, no hosted headers."""
    cxx = os.environ.get("YCXX_GXX", "g++-16") if cc == "gcc" else os.environ.get("YCXX_CLANGXX", "clang++-23")
    inc = subprocess.run([cxx, "-print-file-name=include"], capture_output=True, text=True).stdout.strip()
    cmd = [cxx, "-std=c++26", "-ffreestanding", "-nostdinc", "-nostdinc++", "-isystem", str(REPO / "include"),
           "-isystem", inc, "-fno-exceptions", "-fno-rtti", "-fsyntax-only", "-w",
           "-fmax-errors=0" if cc == "gcc" else "-ferror-limit=0", str(path)]
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace")
    return p.returncode, p.stdout


def analyse(path, rc, out, groups=None):
    """The failed checks of one probe file: {id: first message}; `groups` collects, per check,
    the diagnostics that follow a mention of its line."""
    ids = check_ids(path)
    failed = {}
    name = re.escape(path.name)
    if rc != 0:
        pat = re.compile(r"(?:^|[\s/])" + name + r":(\d+):(?:\d+:)?\s*(.*)$")
        current = None
        for line in out.splitlines():
            m = pat.search(line)
            if m:
                ln = int(m.group(1))
                if ln in ids:
                    msg = m.group(2).strip()
                    failed.setdefault(ids[ln], msg)
                    current = ids[ln]
            if current is not None and groups is not None:
                groups.setdefault(current, []).append(line)
        if not failed:
            # a fatal error before any check (a missing header): every check fails
            first = next((l for l in out.splitlines() if "error" in l), out[:200])
            for i in ids.values():
                failed[i] = "file: " + first.strip()
    return ids, failed


# A constexpr check (gen.py constexpr_check) fails for the library only when the probed function
# itself is not usable in constant evaluation; an evaluation that fails inside it for the sample
# arguments (a pole error, a precondition, arithmetic on a null pointer) leaves it undecided.
NON_CONSTEXPR = re.compile(r"non-.?constexpr.? function|never produces a constant expression|"
                           r"not usable in a constant expression|consteval function .* is not a constant expression")


def load_checks():
    checks = {}
    for line in (HERE / "checks.tsv").read_text().splitlines()[1:]:
        p = line.split("\t")
        checks[p[0]] = p
    return checks


def constexpr_status(check, group):
    """FAIL when a non-constexpr function named like the probed one was called, else UNDECIDED."""
    short = re.sub(r"<.*", "", check[3].split("::")[-1]) if check else ""
    text = "\n".join(group)
    for m in NON_CONSTEXPR.finditer(text):
        line = text[m.start():text.find("\n", m.start())]
        if short and re.search(r"\b" + re.escape(short) + r"\b", line):
            return "FAIL"
    return "UNDECIDED"


def load_gaps():
    gaps = {}
    f = HERE / "gaps.tsv"
    if f.exists():
        for line in f.read_text().splitlines():
            if not line.strip() or line.startswith("#"):
                continue
            parts = line.split("\t")
            gaps[parts[0]] = parts[1:]
    return gaps


def applies(gap, cc):
    comp = gap[1] if len(gap) > 1 else "any"
    return comp in ("any", cc)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-c", "--compiler", action="append")
    ap.add_argument("-j", "--jobs", type=int, default=2)
    ap.add_argument("--libdir-root", help="directory holding <compiler>/ builds (default: REPO/build)")
    ap.add_argument("--filter", action="append")
    ap.add_argument("-v", "--verbose", action="store_true")
    a = ap.parse_args()
    ccs = a.compiler or ["gcc", "clang"]
    # hosted probes, then the freestanding declarations' probes (their IDs prefixed with fs:)
    files = sorted((HERE / "probes").glob("*.cpp")) + sorted((HERE / "probes" / "freestanding").glob("*.cpp"))
    if a.filter:
        files = [f for f in files if any(s in f.name for s in a.filter)]
    gaps = load_gaps()
    bad = 0
    for cc in ccs:
        libdir = str(pathlib.Path(a.libdir_root) / cc) if a.libdir_root else None
        results = {}
        undecided = set()
        checks = load_checks()
        with concurrent.futures.ThreadPoolExecutor(a.jobs) as ex:
            futs = {ex.submit(compile_freestanding if f.parent.name == "freestanding" else compile_one, cc, f,
                              *([] if f.parent.name == "freestanding" else [libdir])): f for f in files}
            for fut in concurrent.futures.as_completed(futs):
                f = futs[fut]
                rc, out = fut.result()
                groups = {}
                ids, failed = analyse(f, rc, out, groups)
                pre = "fs:" if f.parent.name == "freestanding" else ""
                for i in ids.values():
                    results[pre + i] = failed.get(i)
                    c = checks.get(i)
                    if failed.get(i) and c and c[4].startswith("constexpr") and \
                            constexpr_status(c, groups.get(i, [])) == "UNDECIDED":
                        undecided.add(pre + i)
                if a.verbose and failed:
                    print(out)
        unexpected, xpass, known = [], [], 0
        for i, msg in sorted(results.items()):
            if i in undecided:
                continue
            g = gaps.get(i)
            g = g if g and applies(g, cc) else None
            if msg and not g:
                unexpected.append((i, msg))
            elif msg and g:
                known += 1
            elif not msg and g:
                xpass.append(i)
        with open(HERE / f"results-{cc}.tsv", "w") as out:
            for i, msg in sorted(results.items()):
                st = "UNDECIDED" if i in undecided else "FAIL" if msg else "pass"
                out.write(f"{i}\t{st}\t{msg or ''}\n")
        total = len(results)
        nfail = sum(1 for i, m in results.items() if m and i not in undecided)
        print(f"{cc}: {total} checks, {total - nfail - len(undecided)} pass, {nfail} fail ({known} known gaps, "
              f"{len(unexpected)} unexpected, {len(xpass)} XPASS), {len(undecided)} constexpr checks undecided "
              f"(the sample arguments violate a precondition)")
        for i, msg in unexpected:
            print(f"  FAIL {i}: {msg[:200]}")
        for i in xpass:
            print(f"  XPASS {i}")
        bad += len(unexpected) + len(xpass)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
