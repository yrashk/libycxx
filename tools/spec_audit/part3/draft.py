#!/usr/bin/env python3
"""The draft's code, per subclause, for the part-3 spec audit (docs/SPEC_COVERAGE.md, part 3).

    tools/spec_audit/part3/draft.py --html FULL.html [-o regions.json]

reads a saved copy of https://eel.is/c++draft/full and writes, for every subclause of the clauses
part 3 audits ([text], [numerics], [time], [input.output], [thread], [exec], Annex D [depr] and
[zombie.names]), its number, title, parent and its code regions: code blocks ("code") and item
declarations ("decl"), outside examples and notes. Italic runs (exposition-only names and
placeholders) are kept between \\x01 and \\x02 (`\\x01integer-type\\x02`), the angle brackets of
template argument lists are plain `<` `>`, and a comment saying "exposition only" becomes the
marker \\x03. Other comments are dropped.
"""
import argparse, json, pathlib, re, sys
from html.parser import HTMLParser

ROOTS = ("text", "numerics", "time", "input.output", "thread", "exec", "depr", "zombie.names")
IT0, IT1, EXPOS = "\x01", "\x02", "\x03"


class Extract(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.stack = []
        self.sections = {}     # id -> {num, title, level, parent, regions}
        self.order = []
        self.cur_sec = None
        self.path = []         # (level, id) of the open headings
        self.in_h = None       # collecting a heading's text
        self.code = self.ital = self.comment = self.example = self.descr = 0
        self.cur = None
        self.kind = None
        self.comment_text = []
        self.secnum = False

    def handle_starttag(self, tag, attrs):
        if tag in ("br", "img", "meta", "link", "hr", "wbr", "input"):
            return
        a = dict(attrs)
        cls = (a.get("class") or "").split()
        flags = []
        if tag == "div" and "section" in cls and "id" in a:
            self.cur_sec = a["id"]
            self.pending = a["id"]
        if re.fullmatch(r"h[1-7]", tag) and getattr(self, "pending", None):
            level = int(tag[1])
            sid = self.pending
            self.pending = None
            while self.path and self.path[-1][0] >= level:
                self.path.pop()
            parent = self.path[-1][1] if self.path else None
            self.path.append((level, sid))
            self.sections[sid] = {"num": "", "title": "", "level": level, "parent": parent, "regions": []}
            self.order.append(sid)
            self.in_h = sid
            flags.append("h")
        if tag == "a" and "secnum" in cls and self.in_h:
            self.secnum = True
            flags.append("secnum")
        if tag == "a" and "abbr_ref" in cls and self.in_h:
            flags.append("abbr")
        if "codeblock" in cls or "itemdeclcode" in cls:
            flags.append("code")
            if self.code == 0:
                self.kind = "decl" if "itemdeclcode" in cls else ("body" if self.descr else "code")
                self.cur = []
            self.code += 1
        if tag == "i" or "textit" in cls:
            flags.append("i")
            if self.code and self.cur is not None and not self.comment:
                if self.ital == 0:
                    self.cur.append(IT0)
            self.ital += 1
        if "comment" in cls:
            flags.append("c")
            self.comment += 1
        if "example" in cls or "note" in cls:
            flags.append("e")
            self.example += 1
        if "itemdescr" in cls:
            flags.append("d")
            self.descr += 1
        self.stack.append((tag, flags))

    def handle_endtag(self, tag):
        while self.stack:
            t, flags = self.stack.pop()
            for f in flags:
                if f == "h":
                    self.in_h = None
                elif f == "secnum":
                    self.secnum = False
                elif f == "abbr":
                    pass
                elif f == "code":
                    self.code -= 1
                    if self.code == 0:
                        if not self.example and self.cur and self.cur_sec in self.sections:
                            self.sections[self.cur_sec]["regions"].append((self.kind, "".join(self.cur)))
                        self.cur = None
                elif f == "i":
                    self.ital -= 1
                    if self.code and self.cur is not None and not self.comment and self.ital == 0:
                        self.cur.append(IT1)
                elif f == "c":
                    self.comment -= 1
                    if self.comment == 0 and self.cur is not None:
                        txt = "".join(self.comment_text)
                        if re.match(r"\s*(//|/\*)\s*(for\s+)?exposition[ -]only", txt, re.I):
                            self.cur.append(EXPOS)
                        elif re.search(r"\boptional\b", txt):
                            self.cur.append("\x04optional\x05")
                        elif re.search(r"freestanding", txt):
                            self.cur.append("\x04" + ("freestanding-deleted" if "deleted" in txt else "freestanding") + "\x05")
                        self.comment_text = []
                elif f == "e":
                    self.example -= 1
                elif f == "d":
                    self.descr -= 1
            if t == tag:
                break

    def handle_data(self, d):
        if self.in_h:
            s = self.sections[self.in_h]
            if self.secnum:
                s["num"] += d.strip()
            else:
                s["title"] += d
            return
        if not self.code or self.cur is None:
            return
        d = d.replace("​", "").replace("\xad", "").replace("\xa0", " ")
        if self.comment:
            self.comment_text.append(d)
        else:
            self.cur.append(d)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--html", required=True)
    ap.add_argument("-o", "--output", default=None)
    a = ap.parse_args()
    page = pathlib.Path(a.html).read_text(encoding="utf-8")
    m = re.search(r"github\.com/Eelis/draft/tree/([0-9a-f]{40})/", page)
    p = Extract()
    p.feed(page)
    keep = set()
    for sid in p.order:
        s = p.sections[sid]
        anc = sid
        while anc is not None:
            if anc in ROOTS:
                keep.add(sid)
                break
            anc = p.sections[anc]["parent"] if anc in p.sections else None
    out = {"revision": m.group(1) if m else "unknown",
           "sections": [dict(id=sid, **{k: (v.strip() if isinstance(v, str) else v)
                                        for k, v in p.sections[sid].items()})
                        for sid in p.order if sid in keep]}
    dest = pathlib.Path(a.output) if a.output else pathlib.Path(__file__).parent / "cache" / "regions.json"
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_text(json.dumps(out, indent=0, ensure_ascii=False))
    print(f"{dest}: {len(out['sections'])} subclauses, revision {out['revision']}")


if __name__ == "__main__":
    sys.exit(main())
