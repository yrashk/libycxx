#!/usr/bin/env python3
"""Black-box probe of the transitive includes of standard libraries (DECISIONS §19).

For every public C++ header H of libycxx and every probe item (a representative name of a
standard header G, tools/data/transitive-probe/items.txt), compiles

    #include <H>
    <the item's snippet>

with -fsyntax-only and records only whether it compiles. Nothing else is looked at: no
preprocessor output, no include trees, no headers, no diagnostics (stderr is discarded), so the
probe is safe to run against libstdc++ and libc++ (the clean-room rule, CONTRIBUTING notes in
DECISIONS §6).

    tools/probe_transitive.py [-j N] [--lib NAME]... [--headers H,...] [--items ID,...]

Libraries (--lib, default libstdc++ and libc++):
    libstdc++     $YCXX_GXX (g++-16) -std=c++26, its own library
    libc++        $YCXX_CLANGXX (clang++-23) -stdlib=libc++ -std=c++26
    ycxx          libycxx's headers (tools/ycxx-cxx gcc), default mode
    ycxx-strict   libycxx's headers with -DYCXX_NO_TRANSITIVE_INCLUDES
--ycxx-root DIR probes the libycxx of another checkout (its tools/ycxx-cxx and include/), e.g. an
older commit's, and prints the results instead of writing them.

Before the pairs, two controls: each item compiled after its own header G (an item that fails
there is "n/a" for that library: the library lacks it), and each H alone (a header that does not
compile alone is "n/a"). Results go to tools/data/transitive-probe/<lib>.txt, one line per H:

    <H>: <item> <item> ...        the items that compile after including only H
    <H>: n/a                      H itself does not compile with this library

and a line `# n/a items: ...` for the items the library lacks. tools/gen_transitive_includes.py
reads the libstdc++ and libc++ files to propose and check tools/data/transitive-includes.txt.
"""
import argparse, concurrent.futures, os, pathlib, subprocess, sys, tempfile, time

HERE = pathlib.Path(__file__).resolve().parent
REPO = HERE.parent
DATA = HERE / "data" / "transitive-probe"
ITEMS = DATA / "items.txt"
sys.path.insert(0, str(HERE))


def public_cxx_headers():
    from headers import CORE, HOSTED, ABI, FREESTANDING_SUBSET
    return sorted(h for h in set(CORE + HOSTED + ABI + FREESTANDING_SUBSET) if not h.endswith(".h"))


def read_items():
    """[(id, G, snippet)] from items.txt: `<id> <G> | <snippet>` (snippet: one line, `\\n` for
    newlines). The item id is `<G>:<name>`; the first item of a G is its primary item."""
    out = []
    for line in ITEMS.read_text().splitlines():
        if not line.strip() or line.startswith("#"):
            continue
        head, snippet = line.split("|", 1)
        ident = head.strip()
        g = ident.split(":", 1)[0].removeprefix("posix/")
        out.append((ident, g, snippet.strip().replace("\\n", "\n")))
    return out


YCXX_ROOT = REPO


def compilers(lib):
    gxx = os.environ.get("YCXX_GXX", "g++-16")
    clangxx = os.environ.get("YCXX_CLANGXX", "clang++-23")
    base = ["-std=c++26", "-fsyntax-only", "-w", "-x", "c++"]
    if lib == "libstdc++":
        return [gxx] + base
    if lib == "libc++":
        return [clangxx, "-stdlib=libc++"] + base
    if lib in ("ycxx", "ycxx-strict"):
        cmd = [str(YCXX_ROOT / "tools" / "ycxx-cxx"), "gcc"] + base
        return cmd + (["-DYCXX_NO_TRANSITIVE_INCLUDES"] if lib == "ycxx-strict" else [])
    raise SystemExit(f"unknown library {lib}")


def compiles(cmd, text, tmp):
    fd, path = tempfile.mkstemp(suffix=".cpp", dir=tmp)
    with os.fdopen(fd, "w") as f:
        f.write(text)
    try:
        r = subprocess.run(cmd + [path], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                           timeout=300)
        return r.returncode == 0
    except subprocess.TimeoutExpired:
        return False
    finally:
        os.unlink(path)


def probe(lib, headers, items, jobs, progress=True):
    cmd = compilers(lib)
    with tempfile.TemporaryDirectory(prefix=f"ycxx-probe-{lib}-") as tmp, \
            concurrent.futures.ThreadPoolExecutor(jobs) as pool:
        # Controls.
        ctl_items = {i: pool.submit(compiles, cmd, f"#include <{g}>\n{s}\n", tmp) for i, g, s in items}
        ctl_heads = {h: pool.submit(compiles, cmd, f"#include <{h}>\n", tmp) for h in headers}
        na_items = {i for i, f in ctl_items.items() if not f.result()}
        na_heads = {h for h, f in ctl_heads.items() if not f.result()}
        jobsl = {}
        for h in headers:
            if h in na_heads:
                continue
            for i, g, s in items:
                if g == h or i in na_items:
                    continue
                jobsl[(h, i)] = pool.submit(compiles, cmd, f"#include <{h}>\n{s}\n", tmp)
        done, total, t0 = 0, len(jobsl), time.time()
        res = {}
        for k, f in jobsl.items():
            res[k] = f.result()
            done += 1
            if progress and (done % 200 == 0 or done == total):
                el = time.time() - t0
                eta = el / done * (total - done)
                print(f"\r{lib}: {done}/{total} pairs, {el:.0f}s, eta {eta:.0f}s", end="",
                      file=sys.stderr, flush=True)
        if progress:
            print(file=sys.stderr)
    return na_items, na_heads, res


def write(lib, headers, items, na_items, na_heads, res, cmd):
    ver = subprocess.run([cmd[0] if not cmd[0].endswith("ycxx-cxx") else os.environ.get("YCXX_GXX", "g++-16"),
                          "--version"], capture_output=True, text=True).stdout.splitlines()[0]
    lines = [f"# tools/probe_transitive.py --lib {lib}: what compiles after including only <H>",
             f"# compiler: {ver}",
             f"# command: {' '.join(pathlib.Path(c).name if i == 0 else c for i, c in enumerate(cmd))} <file>",
             f"# {len(headers)} headers x {len(items)} items; "
             f"{sum(res.values())} of {len(res)} pairs compile",
             "# n/a items: " + " ".join(sorted(na_items))]
    for h in headers:
        if h in na_heads:
            lines.append(f"{h}: n/a")
            continue
        got = [i for i, g, s in items if res.get((h, i))]
        lines.append(f"{h}: " + " ".join(got))
    DATA.mkdir(parents=True, exist_ok=True)
    (DATA / f"{lib}.txt").write_text("\n".join(lines) + "\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-j", "--jobs", type=int, default=4)
    ap.add_argument("--lib", action="append")
    ap.add_argument("--headers", help="comma-separated subset of H (prints, does not write)")
    ap.add_argument("--items", help="comma-separated subset of item ids (prints, does not write)")
    ap.add_argument("--ycxx-root", type=pathlib.Path)
    a = ap.parse_args()
    global YCXX_ROOT
    if a.ycxx_root:
        YCXX_ROOT = a.ycxx_root.resolve()
    headers = public_cxx_headers()
    items = read_items()
    partial = bool(a.headers or a.items or a.ycxx_root)
    if a.headers:
        headers = [h for h in headers if h in a.headers.split(",")]
    if a.items:
        items = [it for it in items if it[0] in a.items.split(",")]
    for lib in a.lib or ["libstdc++", "libc++"]:
        na_items, na_heads, res = probe(lib, headers, items, a.jobs)
        if partial:
            for h in headers:
                got = "n/a" if h in na_heads else " ".join(i for i, g, s in items if res.get((h, i)))
                print(f"{lib} {h}: {got}")
            if na_items:
                print(f"{lib} n/a items: {' '.join(sorted(na_items))}")
        else:
            write(lib, headers, items, na_items, na_heads, res, compilers(lib))


if __name__ == "__main__":
    main()
