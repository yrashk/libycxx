#!/usr/bin/env python3
"""Check that generated extension API pages are documented, discoverable and locally linked.

Run after tools/gen-apidocs (the Pages workflow does): python tools/apidocs/check_extensions.py
An optional argument names the generated api directory instead of site/api.
"""
from html.parser import HTMLParser
import json
from pathlib import Path
import sys


class Links(HTMLParser):
    def __init__(self):
        super().__init__()
        self.links = []

    def handle_starttag(self, tag, attrs):
        if tag == 'a':
            self.links.extend(v for k, v in attrs if k == 'href')


def check(api):
    search = json.loads((api / 'search.json').read_text())
    nav = json.loads((api / 'nav.json').read_text())
    names = ('ycxx::inplace_function', 'ycxx::uses_inline_storage', 'ycxx::uses_inline_storage_v')
    entries = {entry[0]: entry for entry in search}
    indexed = {entry[0] for entry in nav['headers']['ycxx/functional.hpp']}
    for name in names:
        assert name in entries and name in indexed, f'{name}: missing from search/header navigation'
        page = (api / entries[name][2]).read_text()
        assert 'libycxx extension' in page, f'{name}: missing extension status'
        assert 'ycxx/functional.hpp' in page, f'{name}: missing include header'
    wrapper = (api / entries[names[0]][2]).read_text()
    for text in ('Description', 'Template parameters', 'Specializations', 'never allocates target storage',
                 'independent target', '#include'):
        assert text in wrapper, f'inplace_function: missing documentation: {text}'
    trait = (api / entries[names[1]][2]).read_text()
    assert 'storage properties only' in trait, 'trait: missing limits of the storage guarantee'
    for member in ('operator()', 'swap', 'capacity', 'alignment'):
        assert any(q.startswith('ycxx::inplace_function<') and q.endswith('::' + member) for q in entries), \
            f'inplace_function::{member}: missing from search'
    assert not any('__' in q for q in entries if q.startswith('ycxx::')), 'private names leaked into search'

    header = api / 'headers/ycxx/functional.hpp.html'
    assert 'or <code>import std;</code>' not in header.read_text(), 'extension incorrectly advertised in std module'
    paths = list((api / 'ycxx').rglob('*.html')) + [api / 'ycxx.html', header]
    count = 0
    for path in paths:
        page = path.read_text()
        assert '[object Object]' not in page, f'{path}: unresolved documentation object'
        parser = Links()
        parser.feed(page)
        for href in parser.links:
            if href.startswith(('#', 'http:', 'https:', 'mailto:')):
                continue
            target = (path.parent / href.split('#')[0]).resolve()
            assert target.exists(), f'{path}: broken local link: {href}'
            count += 1
    print(f'extension API docs: {len(names)} APIs, documented/searchable members, {len(paths)} pages, {count} local links checked')


if __name__ == '__main__':
    check(Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[2] / 'site/api')
