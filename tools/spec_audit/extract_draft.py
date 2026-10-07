#!/usr/bin/env python3
"""Extract the declarations of draft clauses for the spec-coverage audit (docs/SPEC_COVERAGE.md).

    tools/spec_audit/extract_draft.py OUT.json CLAUSE.html...

Reads saved pages of https://eel.is/c++draft/<clause> (one clause per page) and writes, for every
code block and item declaration outside examples and notes, its subclause (stable name), its kind
("code": a synopsis or class definition; "body": a code block of an item's description, the code
of its Effects; "decl": an item declaration), and its text, in which italic runs (exposition-only
names and placeholders) are wrapped in @...@.
"""
import json, sys
from html.parser import HTMLParser

VOID = {'br', 'img', 'hr', 'meta', 'link', 'input', 'wbr'}


class _Parser(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.sec = None
        self.out = []
        self.cur = None
        self.stack = []          # (tag, flags)
        self.example = self.descr = 0

    def handle_starttag(self, tag, attrs):
        if tag in VOID:
            if tag == 'br' and self.cur is not None:
                self.cur['text'].append('\n')
            return
        a = dict(attrs)
        cls = a.get('class') or ''
        flags = []
        if tag == 'div' and cls == 'section' and 'id' in a:
            self.sec = a['id']
        if 'example' in cls.split() or 'note' in cls.split():
            flags.append('e')
            self.example += 1
        if 'itemdescr' in cls.split():
            flags.append('d')
            self.descr += 1
        if self.cur is None and ((tag == 'span' and cls == 'codeblock') or
                                 (tag == 'code' and cls == 'itemdeclcode')):
            kind = 'decl' if tag == 'code' else ('body' if self.descr else 'code')
            self.cur = {'sec': self.sec, 'kind': kind, 'text': [], 'example': bool(self.example)}
            flags.append('code')
        elif self.cur is not None and tag == 'i':
            self.cur['text'].append('@')
            flags.append('i')
        self.stack.append((tag, flags))

    def handle_endtag(self, tag):
        if tag in VOID:
            return
        while self.stack:
            t, flags = self.stack.pop()
            for f in flags:
                if f == 'e':
                    self.example -= 1
                elif f == 'd':
                    self.descr -= 1
                elif f == 'i' and self.cur is not None:
                    self.cur['text'].append('@')
                elif f == 'code':
                    self.cur['text'] = ''.join(self.cur['text'])
                    if not self.cur.pop('example'):
                        self.out.append(self.cur)
                    self.cur = None
            if t == tag:
                break

    def handle_data(self, data):
        if self.cur is not None:
            self.cur['text'].append(data)


def main():
    res = []
    for f in sys.argv[2:]:
        p = _Parser()
        p.feed(open(f, encoding='utf-8').read().replace('­', '').replace('​', ''))
        clause = f.rsplit('/', 1)[-1].rsplit('.', 1)[0]
        for r in p.out:
            r['clause'] = clause
        res += p.out
    with open(sys.argv[1], 'w', encoding='utf-8') as o:
        json.dump(res, o, indent=0)
    kinds = {}
    for r in res:
        kinds[r['kind']] = kinds.get(r['kind'], 0) + 1
    print(len(res), 'blocks', kinds)


if __name__ == '__main__':
    main()
