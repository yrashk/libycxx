"""Locating sources and listing the files of an area."""
from __future__ import annotations

import fnmatch
import functools
import json
import os
from pathlib import Path

from areas import AREAS, CORPUS, EXCLUDE, SOURCE_SUFFIXES


@functools.cache
def roots() -> dict[str, Path]:
    cfg = json.loads(os.environ["SIM_ROOTS"])  # written by run.sh
    return {k: Path(v) for k, v in cfg.items()}


def _resolve(impl: str, pattern: str) -> tuple[Path, str]:
    r = roots()
    if pattern.startswith("@extra/"):
        return r["gnu_extra"], pattern[len("@extra/"):]
    return r[impl], pattern


def _excluded(impl: str, rel: str) -> bool:
    return any(fnmatch.fnmatch(rel, p) for p in EXCLUDE.get(impl, []))


@functools.cache
def files_for(impl: str, globs: tuple[str, ...]) -> list[tuple[str, Path]]:
    """Return [(display_path, absolute_path)] for the globs, deduplicated, sorted."""
    seen = {}
    for g in globs:
        base, pat = _resolve(impl, g)
        if not base.exists():
            continue
        for p in sorted(base.glob(pat)):
            if not p.is_file():
                continue
            rel = p.relative_to(base).as_posix()
            if g.startswith("@extra/"):
                disp = rel
            else:
                if _excluded(impl, rel):
                    continue
                disp = rel
            if p.suffix not in SOURCE_SUFFIXES:
                continue
            if p.suffix == "" and p.name in ("README", "LICENSE", "CREDITS", "TODO"):
                continue
            seen[disp] = p
    return sorted(seen.items())


def area_files(area: str, impl: str) -> list[tuple[str, Path]]:
    return files_for(impl, tuple(AREAS[area].get(impl, [])))


def corpus_files(impl: str) -> list[tuple[str, Path]]:
    return files_for(impl, tuple(CORPUS[impl]))


@functools.cache
def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def impls_for_area(area: str) -> list[str]:
    return [i for i in ("ycxx", "gnu", "llvm", "msvc", "cxxrt") if AREAS[area].get(i) and area_files(area, i)]
