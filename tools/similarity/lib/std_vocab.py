"""Build the standard's own vocabulary from the C++ working draft sources
(github.com/cplusplus/draft, source/*.tex).

A name that the standard itself uses -- a library name, an exposition-only name
(written with hyphens in the standard: 'movable-box' -> movable_box), or a
code-font identifier -- is evidence of reading the standard, not of reading
another implementation, so the fingerprint extractors discount it.

Usage: std_vocab.py <draft/source dir> <out.json>
"""
import json
import re
import sys
from pathlib import Path


def main():
    src, out = Path(sys.argv[1]), Path(sys.argv[2])
    vocab = set()
    expos = set()
    for tex in sorted(src.glob("*.tex")):
        s = tex.read_text(encoding="utf-8", errors="replace")
        for m in re.finditer(r"\\(?:expos(?:id|concept|idnc|idv)?|placeholder|placeholdernc|term|defn|"
                             r"defnx|exposidnc)\{([^{}]*)\}", s):
            name = m.group(1).replace("\\-", "").replace("\\", "").strip()
            if name:
                n = name.replace("-", "_").lower()
                expos.add(n)
                vocab.add(n)
        # code-font and code blocks: identifiers with an underscore or hyphenated names
        for m in re.finditer(r"\\(?:tcode|libmember|libglobal|libheader|grammarterm|placeholder|exposid|"
                             r"exposidnc|libconcept|deflibconcept|indexlibrary\w*)\{([^{}]*)\}", s):
            for w in re.findall(r"[A-Za-z_][A-Za-z0-9_\-]*", m.group(1).replace("\\-", "").replace("\\_", "_")):
                vocab.add(w.replace("-", "_").lower())
        for block in re.findall(r"\\begin\{(?:codeblock|itemdecl|codeblocktu|outputblock)\}(.*?)\\end\{", s, re.S):
            b = re.sub(r"@[^@]*@", lambda m: m.group(0).replace("-", "_"), block)
            b = b.replace("\\-", "").replace("\\exposid{", " ").replace("\\exposconcept{", " ")
            for w in re.findall(r"[A-Za-z_][A-Za-z0-9_]*", b):
                vocab.add(w.lower())
    # the standard spells exposition-only members with a trailing underscore (ptr_);
    # implementations add their own prefixes, so compare stripped forms too
    vocab |= {w.strip("_") for w in vocab}
    expos |= {w.strip("_") for w in expos}
    json.dump({"exposition_only": sorted(expos), "vocabulary": sorted(vocab)}, open(out, "w"))
    print(f"{len(expos)} exposition-only names, {len(vocab)} vocabulary entries")


if __name__ == "__main__":
    main()
