#!/usr/bin/env python3
"""Spec-coverage probes, Part 1: [library], [support], [concepts], [diagnostics], [mem], [meta],
[utilities] (docs/SPEC_COVERAGE.md, "Part 1").

    tools/spec_audit/part1/run.py [-c gcc|clang]... [--libdir-root DIR] [-j N] [-v]
                                  [--tables-only | --skip-tables] [--summary FILE] [probe...]

Compile-only checks against libycxx's headers, on each compiler:
  headers   every header of Tables 24, 25 ([headers]) and 47 ([support.c.headers]) compiles, and
            every header of Table 27 ([compliance]) with -ffreestanding -fno-exceptions -fno-rtti
  version   every macro of [version.syn] (data/version.tsv) has the draft's value in <version> and
            in each header the synopsis says it is "also in"; a freestanding one also in a
            freestanding <version>; the hardened ones of /3 with -DYCXX_HARDENED=1 (and not
            without it). The two "see below" macros must be defined (any value)
  names     every entity of data/entities.tsv (inventory.py) is declared by its header: a
            using-declaration per namespace-scope name, a derived class's using-declaration per
            member (on the specialization data/samples.tsv gives), #ifndef per macro; the
            freestanding items ([freestanding.item]) again with the freestanding flags
  probes    each probes/<stable.name>.cpp compiles with -fsyntax-only (static_asserts and
            requires-expressions check presence, shape, constexpr, noexcept and constraints).
            `// FREESTANDING` also compiles it with -ffreestanding -fno-exceptions -fno-rtti;
            `// FLAGS: ...` adds flags; `// REQUIRES: gcc|clang` limits it to one compiler;
            `// GAP: gcc|clang|any <id> <reason>` marks an expected failure that the gap list of
            docs/SPEC_COVERAGE.md describes (a probe that then passes is reported as XPASS)
data/version.tsv and data/headers.tsv come from the draft (draft_tables.py); data/expected.tsv
lists the known failures of the header, version and name checks (compiler, regex over the
failure, gap id of docs/SPEC_COVERAGE.md, reason), reported as XFAIL; an expectation that nothing
matched is reported as STALE. Exit status 0 iff nothing failed unexpectedly.
Compilers: $YCXX_GXX / $YCXX_CLANGXX, else g++-16 / clang++-23 (as tools/ycxx-cxx).
"""
import argparse, concurrent.futures, os, pathlib, re, subprocess, sys

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
    rc, out = run(base(cc, libroot) + header_flags(cc, header) + list(extra) + ["-x", "c++", "-", "-E", "-dM"], f"#include <{header}>\n")
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


def header_flags(cc, header):
    """<meta> needs GCC's -freflection (DECISIONS §13); Clang 23 has no reflection."""
    return ["-freflection"] if header == "meta" and cc == "gcc" else []


# Entities the synopsis declares only under a condition (#if in the synopsis).
CONDITIONAL = {f"std::{t}": f"defined(__STDCPP_{t.upper()[:-2]}_T__)"
               for t in ("float16_t", "float32_t", "float64_t", "float128_t", "bfloat16_t")}
# [ratio.syn]/1: the SI prefixes beyond 10^18 exist only where intmax_t represents their constants.
CONDITIONAL.update({f"std::{p}": "__INTMAX_WIDTH__ > 64"
                    for p in ("quecto", "ronto", "yocto", "zepto", "zetta", "yotta", "ronna", "quetta")})


def name_probe(scope, name, kind, samples):
    """One line of C++ that names the entity, or (None, why) when it cannot be named so."""
    if kind == "macro":
        return f"#ifndef {name}", None
    if kind.endswith("-private"):
        return None, "private member (not nameable by a program)"
    if name.startswith("operator ") and name != "operator bool":
        return None, "conversion function to a type that depends on the template's parameters"
    if kind.startswith("member"):
        sample = samples.get(scope)
        if sample is None:
            return None, "no sample specialization in data/samples.tsv"
        if sample == "-":
            return None, "not probed by name (data/samples.tsv)"
        return f"struct P : {sample} {{ using {sample}::{name}; }};", None
    if kind == "enumerator":
        return f"using {scope}::{name};", None
    q = f"{scope}::{name}" if scope else f"::{name}"
    return f"using {q};", None


def check_names(cc, libroot, pool, freestanding=False):
    rows = load("entities.tsv")
    samples = {r[0]: r[1] for r in load("samples.tsv")}
    by_header, unprobed = {}, []
    for sec, header, scope, name, kind, fs, group in rows:
        if freestanding and fs != "freestanding":
            continue
        line, why = name_probe(scope, name, kind, samples)
        if line is None:
            unprobed.append((sec, header, scope, name, why))
            continue
        by_header.setdefault(header, []).append((sec, scope, name, line))
    jobs, lines_of = {}, {}
    for header, ents in by_header.items():
        src, where = [f"#include <{header}>"], {}
        for n, (sec, scope, name, line) in enumerate(ents):
            if line.startswith("#ifndef"):
                src += [line, "#error missing macro", "#endif"]
                where[len(src) - 1] = (sec, scope, name)    # the #error line
            else:
                cond = CONDITIONAL.get(f"{scope}::{name}")
                if cond:
                    src.append(f"#if {cond}")
                src.append(f"namespace probe_{n} {{ {line} }}")
                where[len(src)] = (sec, scope, name)
                if cond:
                    src.append("#endif")
        lines_of[header] = where
        limit = "-ferror-limit=0" if cc == "clang" else "-fmax-errors=0"
        jobs[header] = pool.submit(run, base(cc, libroot) + header_flags(cc, header) + (FS_FLAGS if freestanding else [])
                                   + ["-x", "c++", "-", "-fsyntax-only", limit], "\n".join(src) + "\n")
    fails, note = [], " freestanding" if freestanding else ""
    for header, f in jobs.items():
        rc, out = f.result()
        if not rc:
            continue
        where = lines_of[header]
        bad = {int(m.group(1)) for m in re.finditer(r"^<stdin>:(\d+):\d+: (?:fatal )?error", out, re.M)
               if int(m.group(1)) in where}
        if not bad:
            fails.append(f"names <{header}>{note}: does not compile\n{out[:1500]}")
        for ln in sorted(bad):
            sec, scope, name = where[ln]
            fails.append(f"name {scope + '::' if scope else ''}{name} <{header}>{note} [{sec}]: not declared")
            MISSING.add((cc, freestanding, sec, scope, name))
    return sum(len(v) for v in by_header.values()), fails, unprobed


MISSING = set()   # (compiler, freestanding, subclause, scope, name) of the entities not found


def summary(path, ccs):
    """Per group of entities.tsv: declared, probed by name, and not found per compiler (hosted and
    freestanding), as TSV for docs/SPEC_COVERAGE.md."""
    samples = {r[0]: r[1] for r in load("samples.tsv")}
    groups = {}
    for sec, header, scope, name, kind, fs, group in load("entities.tsv"):
        g = groups.setdefault(group, {"declared": 0, "probed": 0, "fs": 0, **{f"{c}{f}": 0 for c in ccs for f in ("", "-fs")}})
        g["declared"] += 1
        line, _ = name_probe(scope, name, kind, samples)
        if line is None:
            continue
        g["probed"] += 1
        g["fs"] += fs == "freestanding"
        for c in ccs:
            g[c] += (c, False, sec, scope, name) in MISSING
            g[c + "-fs"] += fs == "freestanding" and (c, True, sec, scope, name) in MISSING
    cols = ["declared", "probed", "fs"] + [f"{c}{f}" for c in ccs for f in ("", "-fs")]
    with open(path, "w") as f:
        f.write("group\t" + "\t".join(cols) + "\n")
        for gname, g in groups.items():
            f.write(gname + "\t" + "\t".join(str(g[c]) for c in cols) + "\n")


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
            USED.add((cc, row[1]))
            return f"{row[2]}: {row[3]}"
    return None


USED = set()   # (compiler, regex) of the expected failures that occurred


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
    ap.add_argument("--summary", help="write per-subclause counts of the name checks (TSV) to this file")
    ap.add_argument("--tables-only", action="store_true", help="only the header, version and name checks")
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
                for what, fn in (("headers", check_headers), ("version", check_version), ("names", check_names),
                                 ("freestanding names", lambda *x: check_names(*x, freestanding=True))):
                    res = fn(cc, a.libdir_root, pool)
                    n, fails = res[0], res[1]
                    if what == "names" and a.verbose:  # (the freestanding ones are a subset)
                        for u in res[2]:
                            print(f"  UNPROBED {u[2]}::{u[3]} [{u[0]}]: {u[4]}")
                    xf = [(f, expected(cc, f)) for f in fails]
                    real = [f for f, e in xf if not e]
                    print(f"{cc} {what}: {n} checked, {len(real)} failed, {len(fails) - len(real)} expected failures")
                    for f, e in xf:
                        if not e:
                            print(f"  FAIL {f}")
                        elif a.verbose:
                            print(f"  XFAIL {f}  [{e}]")
                    bad += len(real)
            if a.tables_only:
                continue
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
    if a.summary:
        summary(a.summary, ccs)
    if not a.probes and not a.skip_tables:
        for row in load("expected.tsv"):
            ccs_ = ccs if row[0] == "any" else [row[0]] if row[0] in ccs else []
            if ccs_ and not any((c, row[1]) in USED for c in ccs_):
                print(f"  STALE expectation ({row[0]}): {row[1]}  [{row[2]}]: nothing failed this way")
                bad += 1
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
