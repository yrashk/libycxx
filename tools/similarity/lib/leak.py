"""Check that the committed similarity files contain nothing from the other implementations.

    tools/similarity/leakcheck [--quick]          always available (tools/test policy stage)
    tools/similarity/leakcheck --full --cache DIR also compares token windows with the sources

Checked files: tools/similarity/** (tools, pins, page templates) and docs/similarity/** (method and
judgments). The rendered site/similarity/ is not checked: it is build
output, quotes the other implementations on purpose, and is not committed.

--quick hashes every reserved identifier, every quoted string and code span, and every line of
six or more words in the checked files, and looks them up in tools/similarity/data/foreign-vocab.txt
(truncated SHA-256 of the other implementations' reserved identifiers, string literals and comment
lines, written by `tools/similarity/run --refresh-vocab` from the pinned sources). A committed
hash list names nothing.

--full additionally looks for any 12-token window of the checked files' code tokens in the pinned
sources' code token streams.

Exit status 1 when anything is found.
"""
import argparse
import hashlib
import json
import os
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent
REPO = HERE.parent.parent
sys.path.insert(0, str(HERE / "lib"))

from lexer import BUILTIN_RE, code_tokens, is_reserved, lex  # noqa: E402

VOCAB = HERE / "data" / "foreign-vocab.txt"
HASH_LEN = 10
WINDOW = 12


def h(kind: str, value: str) -> str:
    return hashlib.sha256(f"{kind}\x1f{value}".encode()).hexdigest()[:HASH_LEN]


def norm_string(s: str) -> str:
    return re.sub(r"\s+", " ", s.strip().lower())


def norm_words(s: str) -> str:
    return " ".join(w.lower() for w in re.findall(r"[A-Za-z][A-Za-z']+", s))


def checked_files() -> list[Path]:
    out = []
    for base in (HERE, REPO / "docs" / "similarity"):
        for p in sorted(base.rglob("*")):
            if p.is_file() and p != VOCAB and "__pycache__" not in p.parts:
                out.append(p)
    return [p for p in out if p.exists()]


# Words of the method that look like reserved names but are not taken from any implementation.
ALLOWED = {"__attribute__", "__declspec", "__cxx03", "__init__", "__file__", "__name__", "__main__",
           # stems of the compilers' own type names in the tools' spelling rules (_Float16, __int128, ...)
           "__", "_Float", "__float", "__int", "__m"}


def quick(files) -> list[str]:
    if not VOCAB.exists():
        return [f"{VOCAB.relative_to(REPO)} is missing: run tools/similarity/run --refresh-vocab"]
    vocab = {l.strip() for l in VOCAB.read_text().splitlines() if l and not l.startswith("#")}
    problems = []
    for p in files:
        text = p.read_text(errors="replace")
        rel = p.relative_to(REPO)
        for n, line in enumerate(text.splitlines(), 1):
            # file paths of the other implementations are allowed (their directory names are reserved);
            # regular-expression character classes in the tools are spelling rules, not names
            words_line = re.sub(r"[\w.+~*-]*/[\w./+~*-]+", " ", line)
            words_line = re.sub(r"\\[wdsb]\+?\*?|\[[^\]]*\]|\(\?:", " ", words_line)
            for w in re.findall(r"[A-Za-z_][A-Za-z0-9_]*", words_line):
                if w in ALLOWED or not is_reserved(w) or BUILTIN_RE.match(w):
                    continue
                if h("id", w) in vocab:
                    problems.append(f"{rel}:{n}: {w}: a reserved identifier of another implementation")
            for q in re.findall(r'"([^"\n]{10,})"|`([^`\n]{10,})`', line):
                s = q[0] or q[1]
                if h("str", norm_string(s)) in vocab:
                    problems.append(f"{rel}:{n}: {s[:50]!r}: a string literal of another implementation")
            words = norm_words(line)
            if len(words.split()) >= 6 and h("cmt", words) in vocab:
                problems.append(f"{rel}:{n}: {words[:60]!r}: a comment line of another implementation")
    return problems


def full(files, cache: Path) -> list[str]:
    sys.path.insert(0, str(HERE / "lib"))
    import fetch  # noqa: E402
    r = fetch.fetch_all(cache)
    roots = {"ycxx": str(REPO), "gnu": str(r["gnu"]), "gnu_extra": str(r["gnu_extra"]), "llvm": str(r["llvm"]),
             "llvm03": str(r["llvm"]), "msvc": str(r["msvc"]), "cxxrt": str(r["cxxrt"])}
    os.environ["SIM_ROOTS"] = json.dumps(roots)
    from common import corpus_files, read  # noqa: E402
    windows = {}
    for p in files:
        if p.suffix in (".png", ".woff2"):
            continue
        toks = [t.text for t in code_tokens(lex(p.read_text(errors="replace")))]
        lines = [t.line for t in code_tokens(lex(p.read_text(errors="replace")))]
        for i in range(len(toks) - WINDOW + 1):
            w = toks[i:i + WINDOW]
            # windows made only of punctuation and keywords say nothing
            if sum(1 for x in w if re.match(r"[A-Za-z_]\w{2,}|\d", x)) < 4:
                continue
            windows.setdefault(hash(tuple(w)), (p, lines[i], w))
    problems = []
    for lib in ("gnu", "llvm", "msvc", "cxxrt", "llvm03"):
        for disp, path in corpus_files(lib):
            toks = [t.text for t in code_tokens(lex(read(path)))]
            for i in range(len(toks) - WINDOW + 1):
                k = hash(tuple(toks[i:i + WINDOW]))
                if k in windows and list(toks[i:i + WINDOW]) == windows[k][2]:
                    p, line, _ = windows[k]
                    problems.append(f"{p.relative_to(REPO)}:{line}: a 12-token window also in {lib} {disp}")
    return sorted(set(problems))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--quick", action="store_true")
    ap.add_argument("--full", action="store_true")
    ap.add_argument("--cache", default=str(REPO / "build" / "similarity" / "cache"))
    a = ap.parse_args()
    files = checked_files()
    problems = quick(files)
    if a.full:
        problems += full(files, Path(a.cache))
    if problems:
        print("similarity leak check FAILED:")
        for x in problems:
            print("  " + x)
        return 1
    print(f"similarity leak check passed ({len(files)} files{', with token windows' if a.full else ''})")
    return 0


def refresh_vocab(work: Path) -> None:
    """Called by `tools/similarity/run --refresh-vocab` with SIM_ROOTS set."""
    from common import corpus_files, read  # noqa: E402
    from lexer import lex as _lex
    hs = set()
    for lib in ("gnu", "llvm", "msvc", "cxxrt", "llvm03"):
        for _, path in corpus_files(lib):
            for t in _lex(read(path)):
                if t.kind == "id" and is_reserved(t.text) and not BUILTIN_RE.match(t.text) and t.text not in ALLOWED:
                    hs.add(h("id", t.text))
                elif t.kind == "str":
                    m = re.match(r'^(?:u8|u|U|L)?R?"(.*)"$', t.text, re.S)
                    s = norm_string(m.group(1) if m else t.text)
                    if len(s) >= 10:
                        hs.add(h("str", s))
                elif t.kind == "comment":
                    for line in t.text.split("\n"):
                        w = norm_words(line)
                        if len(w.split()) >= 6:
                            hs.add(h("cmt", w))
    VOCAB.parent.mkdir(parents=True, exist_ok=True)
    VOCAB.write_text("# Truncated SHA-256 of the other implementations' reserved identifiers, string literals and\n"
                     "# comment lines at the versions pinned in ../sources.json. Written by\n"
                     "# `tools/similarity/run --refresh-vocab`; read by tools/similarity/leakcheck.\n" +
                     "\n".join(sorted(hs)) + "\n")
    print(f"[similarity] wrote {len(hs)} hashes to {VOCAB}")


if __name__ == "__main__":
    sys.exit(main())
