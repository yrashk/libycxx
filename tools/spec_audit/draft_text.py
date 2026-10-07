#!/usr/bin/env python3
"""Plain text of saved draft pages, by subclause (tools/spec_audit).

    tools/spec_audit/draft_text.py PAGE.html... [--sec STABLE-NAME...] [--grep REGEX]

Prints each subclause's text (headings, paragraphs and code) with its stable name; --sec keeps
the named subclauses (and their descendants), --grep only the paragraphs that match.
"""
import argparse, html, re


def sections(path):
    s = open(path, encoding='utf-8').read().replace('­', '').replace('​', '')
    parts = re.split(r"<div id='([^']+)' class='section'>", s)
    out = []
    for k in range(1, len(parts), 2):
        body = parts[k + 1]
        body = re.sub(r'<(br|/p|/div|/li|/tr|/h[1-6])[^>]*>', '\n', body)
        body = re.sub(r'<[^>]+>', '', body)
        body = html.unescape(body)
        body = re.sub(r'\n\s*\n+', '\n', body)
        out.append((parts[k], body))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('pages', nargs='+')
    ap.add_argument('--sec', nargs='*')
    ap.add_argument('--grep')
    a = ap.parse_args()
    for p in a.pages:
        for name, body in sections(p):
            if a.sec and not any(name == x or name.startswith(x + '.') for x in a.sec):
                continue
            if a.grep:
                hits = [l for l in body.split('\n') if re.search(a.grep, l)]
                for l in hits:
                    print(f'[{name}] {l.strip()[:400]}')
            else:
                print(f'==== [{name}]')
                print(body)


if __name__ == '__main__':
    main()
