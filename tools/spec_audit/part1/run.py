#!/usr/bin/env python3
"""Spec-coverage probes, Part 1: [library], [support], [concepts], [diagnostics], [mem], [meta],
[utilities] (docs/SPEC_COVERAGE.md, "Part 1").

    tools/spec_audit/part1/run.py [-c gcc|clang]... [--libdir-root DIR] [-j N] [-v] [probe...]

Compile-only checks against libycxx's headers, on each compiler:
  headers   every header of Tables 24 and 25 ([headers]) compiles; every header of Table 27
            ([compliance]) compiles with -ffreestanding -fno-exceptions -fno-rtti
  version   every macro of [version.syn] (data/version.tsv) has the draft's value in <version> and
            in each header the synopsis says it is "also in"; a freestanding one also in a
            freestanding <version>; the hardened ones of /3 with -DYCXX_HARDENED=1 (and not
            without it). The two "see below" macros must be defined (any value)
  probes    each probes/<stable.name>.cpp compiles with -fsyntax-only (static_asserts and
            requires-expressions check presence, shape, constexpr, noexcept and constraints).
            `// FREESTANDING` also compiles it with -ffreestanding -fno-exceptions -fno-rtti;
            `// FLAGS: ...` adds flags; `// REQUIRES: gcc|clang` limits it to one compiler;
            `// GAP: gcc|clang|any <id> <reason>` marks an expected failure that the gap list of
            docs/SPEC_COVERAGE.md describes (a probe that then passes is reported as XPASS)
data/version.tsv and data/headers.tsv come from the draft (draft_tables.py); data/expected.tsv
lists the known failures of the header and version checks (compiler, regex over the failure, gap
id of docs/SPEC_COVERAGE.md, reason), reported as XFAIL. Exit status 0 iff nothing failed unexpectedly.
Compilers: $YCXX_GXX / $YCXX_CLANGXX, else g++-16 / clang++-23 (as tools/ycxx-cxx).
"""
import argparse, concurrent.futures, os, pathlib, re, subprocess, sys, tempfile

HERE = pathlib.Path(__file__).resolve().parent
REPO = HERE.parents[2]
FS_FLAGS = ["-ffreestanding", "-fno-exceptions", "-fno-rtti"]


def compiler(cc):
    return os.environ.get("YCXX_GXX", "g++-16") if cc == "gcc" else os.environ.get("YCXX_CLANGXX", "clang++-23")


def base(cc, libroot):
    gen = pathlib.Path(libroot) / cc / "generated" / "include"
    if not gen.is_dir():
        sys.exit(f"run.py: no {gen}: build libycxx for {cc} first (tools/test build), or pass --libdir-root")
    flags = [compiler(cc), "-std=c++26", "-nostdinc++", "-isystem", str(REPO / "include"), "-isystem", str(gen)]
    if cc == "gcc":
        flags += ["-Wno-attributes"]
    return flags


def run(cmd, src=None):
    p = subprocess.run(cmd, input=src, capture_output=True, text=True)
    return p.returncode, p.stdout + p.stderr


def macros(cc, libroot, header, extra=()):
    rc, out = run(base(cc, libroot) + list(extra) + ["-x", "c++", "-", "-E", "-dM"], f"#include <{header}>\n")
    if rc:
        return None, out
    return {m.group(1): m.group(2) for m in re.finditer(r"^#define (__cpp_lib_\w+) (\S+)", out, re.M)}, out


def load(name):
    rows = []
    for line in (HERE / "data" / name).read_text().splitlines():
        if line and not line.startswith("#"):
            rows.append(line.split("\t"))
    return rows


def check_headers(cc, libroot, pool):
    rows = load("headers.tsv")
    jobs = {}
    for h, kind in rows:
        extra = FS_FLAGS if kind == "fs" else []
        jobs[(h, kind)] = pool.submit(run, base(cc, libroot) + extra + ["-x", "c++", "-", "-fsyntax-only"],
                                      f"#include <{h}>\n")
    fails = []
    for (h, kind), f in jobs.items():
        rc, out = f.result()
        if rc:
            fails.append(f"header <{h}> ({kind}): does not compile\n{out[:2000]}")
    return len(rows), fails


HARDENED = ["-DYCXX_HARDENED=1"]


def check_version(cc, libroot, pool):
    rows = load("version.tsv")
    headers = {"version"} | {h for r in rows if r[2] != "hardened" for h in r[3].split()}
    hard_headers = {"version"} | {h for r in rows if r[2] == "hardened" for h in r[3].split()}
    jobs = {h: pool.submit(macros, cc, libroot, h) for h in headers}
    jobs.update({h + "/hardened": pool.submit(macros, cc, libroot, h, HARDENED) for h in hard_headers})
    jobs["version/fs"] = pool.submit(macros, cc, libroot, "version", FS_FLAGS)
    got = {h: f.result()[0] for h, f in jobs.items()}
    fails = []
    for name, value, kind, also in rows:
        sfx, note = ("/hardened", " hardened") if kind == "hardened" else ("", "")
        where = [(h + sfx, h + note) for h in ["version"] + also.split()]
        if kind == "fs":
            where.append(("version/fs", "version freestanding"))
        for key, label in where:
            m = got[key]
            if m is None:
                fails.append(f"{name}: <{label}> does not compile")
            elif name not in m:
                fails.append(f"{name}: not defined by <{label}> (draft: {value})")
            elif value != "see below" and m[name] != value:
                fails.append(f"{name}: <{label}> defines {m[name]}, draft {value}")
        if kind == "hardened" and got["version"] and name in got["version"]:
            fails.append(f"{name}: defined by <version> without YCXX_HARDENED "
                         "([version.syn]/3: in a hardened implementation)")
    known = {r[0] for r in rows}
    for key in ("version", "version/hardened"):
        for name in sorted(set(got[key] or {}) - known):
            fails.append(f"{name}: defined by <{key}> but not in [version.syn]")
    return len(rows), fails


def expected(cc, failure):
    """The gap of data/expected.tsv that a failure of the tables is (compiler, regex, gap id and
    class, reason), or None."""
    for row in load("expected.tsv"):
        if row[0] in (cc, "any") and re.search(row[1], failure):
            return f"{row[2]}: {row[3]}"
    return None


DIRECTIVE = lambda key: re.compile(rf"^//\s*{key}:(.*)$", re.M)


def check_probe(cc, libroot, path):
    src = path.read_text()
    req = [r.strip() for m in DIRECTIVE("REQUIRES").finditer(src) for r in m.group(1).split(",")]
    if req and cc not in req:
        return "UNSUPPORTED", ""
    flags = [f for m in DIRECTIVE("FLAGS").finditer(src) for f in m.group(1).split()]
    variants = [[]] + ([FS_FLAGS] if re.search(r"^//\s*FREESTANDING\b", src, re.M) else [])
    out_all, ok = "", True
    for v in variants:
        rc, out = run(base(cc, libroot) + flags + v + ["-fsyntax-only", str(path)])
        if rc:
            ok = False
            out_all += f"[{' '.join(v) or 'hosted'}]\n{out}"
    gaps = [m for m in re.finditer(r"^//\s*GAP:\s*(gcc|clang|any)\s+(\S+)(.*)$", src, re.M)
            if m.group(1) in (cc, "any")]
    if gaps:
        return ("XFAIL" if not ok else "XPASS"), out_all
    return ("PASS" if ok else "FAIL"), out_all


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-c", "--compiler", action="append", choices=["gcc", "clang"])
    ap.add_argument("--libdir-root", default=str(REPO / "build"),
                    help="directory holding <cc>/generated/include (default: build/)")
    ap.add_argument("-j", "--jobs", type=int, default=2)
    ap.add_argument("-v", "--verbose", action="store_true")
    ap.add_argument("--skip-tables", action="store_true", help="only the probes")
    ap.add_argument("probes", nargs="*", help="probe files or stable names (default: all)")
    a = ap.parse_args()
    ccs = a.compiler or ["gcc", "clang"]
    probes = sorted((HERE / "probes").glob("*.cpp"))
    if a.probes:
        want = {p.removesuffix(".cpp").split("/")[-1] for p in a.probes}
        probes = [p for p in probes if p.stem in want]
    bad = 0
    with concurrent.futures.ThreadPoolExecutor(a.jobs) as pool:
        for cc in ccs:
            if not a.skip_tables and not a.probes:
                for what, fn in (("headers", check_headers), ("version", check_version)):
                    n, fails = fn(cc, a.libdir_root, pool)
                    xf = [(f, expected(cc, f)) for f in fails]
                    real = [f for f, e in xf if not e]
                    print(f"{cc} {what}: {n} checked, {len(real)} failed, {len(fails) - len(real)} expected failures")
                    for f, e in xf:
                        if not e:
                            print(f"  FAIL {f}")
                        elif a.verbose:
                            print(f"  XFAIL {f}  [{e}]")
                    bad += len(real)
            futs = {p: pool.submit(check_probe, cc, a.libdir_root, p) for p in probes}
            counts = {}
            for p, f in futs.items():
                status, out = f.result()
                counts[status] = counts.get(status, 0) + 1
                if status in ("FAIL", "XPASS") or (a.verbose and status == "XFAIL"):
                    print(f"  {status} {cc} {p.stem}")
                    if status == "FAIL" or a.verbose:
                        print("    " + out[:6000].replace("\n", "\n    "))
                if status in ("FAIL", "XPASS"):
                    bad += 1
            print(f"{cc} probes: " + ", ".join(f"{k} {v}" for k, v in sorted(counts.items())))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
