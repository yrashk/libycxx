"""Fetch the pinned sources (tools/similarity/sources.json) into a cache directory.

Layout of the cache:
    <cache>/gcc/<top>/libstdc++-v3/...   (subset of the GCC release tarball, SHA-256 checked)
    <cache>/gcc/<top>/libiberty/cp-demangle.[ch]
    <cache>/llvm, <cache>/msvc, <cache>/libcxxrt, <cache>/draft   (sparse git checkouts of a commit)
    <cache>/jplag.jar                                               (SHA-256 checked)
Each entry has a .pin stamp; an entry whose stamp matches its pin is reused, so the CI cache key
can be the hash of sources.json.
"""
from __future__ import annotations

import hashlib
import json
import os
import shutil
import subprocess
import urllib.request
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
SOURCES = json.loads((HERE / "sources.json").read_text())


def log(msg: str) -> None:
    print(f"[similarity] {msg}", flush=True)


def _stamp_ok(dest: Path, pin: dict) -> bool:
    st = dest / ".pin"
    return st.exists() and st.read_text() == json.dumps(pin, sort_keys=True)


def _stamp(dest: Path, pin: dict) -> None:
    (dest / ".pin").write_text(json.dumps(pin, sort_keys=True))


def _sha256(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()


def _download(url: str, dest: Path, sha: str) -> None:
    tmp = dest.with_suffix(dest.suffix + ".part")
    log(f"downloading {url}")
    with urllib.request.urlopen(url, timeout=600) as r, open(tmp, "wb") as f:
        shutil.copyfileobj(r, f, 1 << 20)
    got = _sha256(tmp)
    if got != sha:
        tmp.unlink()
        raise RuntimeError(f"SHA-256 mismatch for {url}: {got} != {sha}")
    tmp.rename(dest)


def _git(*args, cwd=None):
    subprocess.run(["git", *args], cwd=cwd, check=True, stdout=subprocess.DEVNULL)


def _git_commit(dest: Path, repo: str, commit: str, paths: list[str]) -> None:
    if dest.exists():
        shutil.rmtree(dest)
    dest.mkdir(parents=True)
    _git("init", "-q", cwd=dest)
    _git("remote", "add", "origin", repo, cwd=dest)
    _git("config", "remote.origin.promisor", "true", cwd=dest)
    _git("config", "remote.origin.partialclonefilter", "blob:none", cwd=dest)
    _git("sparse-checkout", "set", "--cone", *paths, cwd=dest)
    _git("fetch", "-q", "--depth", "1", "--filter=blob:none", "origin", commit, cwd=dest)
    _git("checkout", "-q", "FETCH_HEAD", cwd=dest)
    head = subprocess.run(["git", "rev-parse", "HEAD"], cwd=dest, capture_output=True, text=True).stdout.strip()
    if head != commit:
        raise RuntimeError(f"{repo}: got {head}, pinned {commit}")
    shutil.rmtree(dest / ".git")


def fetch_all(cache: Path, gcc_tarball: str | None = None) -> dict[str, Path]:
    cache.mkdir(parents=True, exist_ok=True)
    # GCC subset
    g = SOURCES["libstdcxx"]
    gdir = cache / "gcc"
    if not _stamp_ok(gdir, g):
        if gdir.exists():
            shutil.rmtree(gdir)
        gdir.mkdir(parents=True)
        tar = Path(gcc_tarball) if gcc_tarball else cache / "gcc.tar.xz"
        if gcc_tarball:
            if _sha256(tar) != g["sha256"]:
                raise RuntimeError(f"{tar}: SHA-256 does not match the pin")
        else:
            _download(g["url"], tar, g["sha256"])
        log("extracting the libstdc++ and libiberty subset")
        members = [f"{g['top']}/{p}" for p in g["extract"]]
        subprocess.run(["tar", "-xJf", str(tar), "-C", str(gdir), "--exclude=*/testsuite/*", "--exclude=ChangeLog*",
                        *members], check=True)
        if not gcc_tarball:
            tar.unlink()
        _stamp(gdir, g)
    for key, sub in (("libcxx", "llvm"), ("msvcstl", "msvc"), ("libcxxrt", "libcxxrt"), ("draft", "draft"),
                     ("kokkos_mdspan", "kokkos"), ("beman_optional", "beman/optional"),
                     ("beman_inplace_vector", "beman/inplace_vector")):
        pin = SOURCES[key]
        d = cache / sub
        if not _stamp_ok(d, pin):
            log(f"fetching {pin['name']} {pin['commit'][:12]}")
            _git_commit(d, pin["repo"], pin["commit"], pin["paths"])
            _stamp(d, pin)
    j = SOURCES["jplag"]
    jar = cache / "jplag.jar"
    if not jar.exists() or _sha256(jar) != j["sha256"]:
        _download(j["url"], jar, j["sha256"])
    top = gdir / g["top"]
    return {
        "gnu": top / "libstdc++-v3",
        "gnu_extra": top,
        "llvm": cache / "llvm",
        "msvc": cache / "msvc",
        "cxxrt": cache / "libcxxrt",
        "draft": cache / "draft" / "source",
        "kokkos": cache / "kokkos",
        "beman": cache / "beman",
        "jplag": jar,
    }
