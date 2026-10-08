"""cppreference.com for libycxx's API reference (tools/gen-apidocs).

The source is the official offline archive of cppreference.com
(github.com/PeterFeicht/cppreference-doc, the html-book release that tools/gen-apidocs pins).
Two things come from it:

1. The map from each documented entity to its cppreference page, without a hand-written URL:
   the archive's Doxygen tag file (cppreference-doxygen-web.tag.xml: classes, their members, the
   functions and variables of std and its namespaces) and its symbol index pages
   (cpp/symbol_index*.html: every name of std, std::ranges, std::views, std::chrono…). An entity
   with no page of its own gets its nearest enclosing entity's page, or its header's page; the
   map says which ("x": 0). It is written to site/api/cppref-map.json.

2. The pages the reference links to, hosted with the site (site/cppref/) so the reference can
   show them in a pane on the same origin: cppreference.com itself cannot be framed (it answers
   with X-Frame-Options: SAMEORIGIN). Each page is reduced to its article (no wiki navigation,
   scripts or trackers), styled like the site, and says where it comes from: "From
   cppreference.com, licensed CC-BY-SA 3.0 and GFDL", with a link to the original page. Links
   between hosted pages stay on the site; links to other pages go to cppreference.com in a new tab.
"""
import collections, html, json, pathlib, re, shutil, tarfile, xml.etree.ElementTree as ET

LIVE = 'https://en.cppreference.com/w/'
NOTICE = 'From cppreference.com, licensed CC-BY-SA 3.0 and GFDL'


class Archive:
    def __init__(self, archive, cache, version, url):
        self.version, self.url = version, url
        self.root = cache
        marker = cache / '.extracted'
        if not marker.exists():
            if cache.exists():
                shutil.rmtree(cache)
            cache.mkdir(parents=True)
            with tarfile.open(archive) as t:
                t.extractall(cache, filter='data' if hasattr(tarfile, 'data_filter') else None)
            marker.write_text(version)
        self.ref = cache / 'reference' / 'en'
        self._map = None

    # -- the symbol map ------------------------------------------------------------------------

    def _load(self):
        if self._map is not None:
            return self._map
        m = {}
        tag = ET.parse(self.root / 'cppreference-doxygen-web.tag.xml').getroot()
        for c in tag.findall('compound'):
            kind, name, fn = c.get('kind'), c.findtext('name'), c.findtext('filename') or ''
            if kind == 'file':
                continue
            if fn:
                m.setdefault(name, fn)
            for mem in c.findall('member'):
                af = mem.findtext('anchorfile')
                if af:
                    m.setdefault(f'{name}::{mem.findtext("name")}', af)
        # The symbol index pages: <a href="path.html" title="cpp/..."><tt>name()</tt></a>
        idx = self.ref / 'cpp'
        pages = [('std', idx / 'symbol_index.html')] + [
            (None, p) for p in sorted((idx / 'symbol_index').glob('*.html'))]
        cands = collections.defaultdict(list)
        for ns, p in pages:
            text = p.read_text(encoding='utf-8', errors='replace')
            if ns is None:
                h = re.search(r'<h1[^>]*>\s*(?:Symbol index for |)?[^<]*?(std::[\w:]+)', text)
                head = re.findall(r'<tt>(std(?:::\w+)+)</tt>', text[:20000])
                ns = head[-1] if head else None
                if not ns:
                    continue
            base = p.parent
            for href, name in re.findall(r'<a href="([^"#]+\.html)(?:#[^"]*)?" title="cpp/[^"]*"><tt>([^<]+)</tt></a>', text):
                n = html.unescape(name)
                n = re.sub(r'(<>|\(\)|<>\(\))$', '', n).strip()
                if not re.fullmatch(r'[A-Za-z_]\w*', n):
                    continue
                path = (base / href).resolve().relative_to(self.ref.resolve()).as_posix()[:-5]
                if path.startswith('cpp/symbol_index'):
                    continue
                cands[f'{ns}::{n}'].append(path)
        for q, paths in cands.items():
            m.setdefault(q, paths[0])
            m.setdefault('#all:' + q, paths)
        self._map = m
        return m

    def lookup(self, q, header=None):
        """(path, exact) of q's own page, or None."""
        m = self._load()
        for cand in (q, q.replace('std::ranges::views::', 'std::views::'), q.replace('std::ranges::__cpo::', 'std::ranges::')):
            alts = m.get('#all:' + cand)
            if alts and header and len(alts) > 1:
                fit = [p for p in alts if f'/{header}/' in f'/{p}/' or p.split('/')[-2:-1] == [header]]
                if fit:
                    return fit[0]
            if cand in m:
                return m[cand]
        return None

    def header_page(self, h):
        p = f'cpp/header/{h}'
        if (self.ref / (p + '.html')).exists():
            return {'p': p, 'h': 1 if p in getattr(self, 'hosted', ()) else 0, 'x': 1, 't': f'<{h}>'}
        return None

    # -- the map for the reference, and the hosted pages ---------------------------------------

    def build(self, canonical, out):
        """{qualified name: {"p": path, "x": exact?, "h": hosted?, "t": title}} for every
        entity of the reference; writes the hosted pages to out."""
        res, wanted = {}, set()
        for q, pg in canonical.items():
            if not q.startswith('std'):
                continue
            path, exact = self.lookup(q, pg.header), 1
            parts = q.split('::')
            while path is None and len(parts) > 2:
                parts = parts[:-1]
                path, exact = self.lookup('::'.join(parts), pg.header), 0
            title = '::'.join(parts) if path else None
            if path is None and pg.header and (self.ref / f'cpp/header/{pg.header}.html').exists():
                path, exact, title = f'cpp/header/{pg.header}', 0, f'<{pg.header}>'
            if path is None:
                continue
            if not (self.ref / (path + '.html')).exists():
                continue
            res[q] = {'p': path, 'x': exact, 't': title}
            wanted.add(path)
        for h in {pg.header for pg in canonical.values() if pg.header}:
            if (self.ref / f'cpp/header/{h}.html').exists():
                wanted.add(f'cpp/header/{h}')
        self.hosted = wanted
        self.write(out, wanted)
        for v in res.values():
            v['h'] = 1
        return res

    def _article(self, path):
        text = (self.ref / (path + '.html')).read_text(encoding='utf-8', errors='replace')
        t = re.search(r'<h1 id="firstHeading"[^>]*>(.*?)</h1>', text, re.S)
        title = re.sub(r'<[^>]+>', '', t.group(1)).strip() if t else path
        a = text.index('<div id="mw-content-text"')
        a = text.index('>', a) + 1
        b = text.index('<!-- /bodycontent -->', a)
        body = text[a:b]
        body = body[:body.rstrip().rfind('</div>')]
        oldid = re.search(r'title=([^&"]+)&amp;oldid=(\d+)', text)
        return title, body, (oldid.group(2) if oldid else None)

    @staticmethod
    def _drop_div(body, cls):
        """Remove each <div class="cls…"> with its content."""
        while True:
            i = body.find(f'<div class="{cls}')
            if i < 0:
                return body
            depth, j = 0, i
            for m in re.finditer(r'<(/?)div\b', body[i:]):
                depth += -1 if m.group(1) else 1
                if depth == 0:
                    j = i + m.end()
                    j = body.index('>', j) + 1
                    break
            body = body[:i] + body[j:]

    def write(self, out, wanted):
        if out.exists():
            shutil.rmtree(out)
        out.mkdir(parents=True)
        total = 0
        for path in sorted(wanted):
            title, body, oldid = self._article(path)
            body = self._drop_div(body, 't-navbar')
            body = re.sub(r'<script\b.*?</script>', '', body, flags=re.S)
            body = re.sub(r'<!--.*?-->', '', body, flags=re.S)
            body = re.sub(r'<span class="editsection">.*?</span>', '', body, flags=re.S)
            body = re.sub(r'<img\b[^>]*>', '', body)
            # Smaller, same text: no tooltips repeating link targets, no spans around punctuation
            # (GeSHi's brackets and symbols), no runs of blank space.
            body = re.sub(r' title="[^"]*"', '', body)
            body = re.sub(r'<span class="(?:br0|sy[0-9])">([^<]*)</span>', r'\1', body)
            parts = re.split(r'(<pre\b.*?</pre>)', body, flags=re.S)
            body = ''.join(x if x.startswith('<pre') else re.sub(r'\n\s*\n+', '\n', re.sub(r'[ \t]+', ' ', x)) for x in parts)
            here = pathlib.PurePosixPath(path).parent
            depth = path.count('/')

            def link(m):
                href, frag = m.group(1), m.group(2) or ''
                if href.startswith(('http:', 'https:', 'mailto:')):
                    return f'href="{href}{frag}"'
                target = (here / href).as_posix()
                target = re.sub(r'[^/]+/\.\./', '', target)
                while '/../' in target:
                    target = re.sub(r'[^/]+/\.\./', '', target, count=1)
                target = target[:-5] if target.endswith('.html') else target
                if target in wanted:
                    return f'href="{"../" * depth}{target}.html{frag}" target="_self"'
                return f'href="{LIVE}{target}{frag}"'
            body = re.sub(r'href="([^"#]*\.html|https?:[^"#]*)(#[^"]*)?"', link, body)
            body = re.sub(r'href="(#[^"]*)"', r'href="\1" target="_self"', body)
            body = body.replace('<span class="mw-geshi cpp source-cpp">', '<span class="mw-geshi">')
            live = LIVE + path
            rev = f' (revision <a href="https://en.cppreference.com/mwiki/index.php?oldid={oldid}" target="_blank" rel="noopener">{oldid}</a>)' if oldid else ''
            root = '../' * (depth + 1)
            page = f'''<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="color-scheme" content="light dark">
<meta name="robots" content="noindex">
<title>{html.escape(title)} — cppreference.com (copy)</title>
<link rel="canonical" href="{live}">
<link rel="stylesheet" href="{root}assets/site.css">
<link rel="stylesheet" href="{root}assets/cppref.css">
<base target="_blank">
</head>
<body class="cppref">
<p class="cppref-notice">{NOTICE}: <a href="{live}" target="_blank" rel="noopener">the original page ↗</a>{rev}, from the offline archive of {self.version[:4]}-{self.version[4:6]}-{self.version[6:]} (<a href="{root}cppref/LICENSE.txt" target="_self">licence and attribution</a>).</p>
<main class="cppref-article">
<h1>{html.escape(title)}</h1>
{body}
</main>
</body>
</html>
'''
            dst = out / (path + '.html')
            dst.parent.mkdir(parents=True, exist_ok=True)
            dst.write_text(page, encoding='utf-8')
            total += len(page.encode())
        (out / 'README.txt').write_text(README.format(version=self.version, url=self.url, n=len(wanted), mb=total / 1e6))
        (out / 'LICENSE.txt').write_text(LICENSE)
        print(f'cppref: {len(wanted)} pages, {total / 1e6:.1f} MB', flush=True)


README = """\
cppreference.com pages hosted with libycxx's API reference
==========================================================

These {n} pages ({mb:.1f} MB) are copies of pages of cppreference.com (https://en.cppreference.com),
the ones libycxx's API reference (../api/) links to. The reference shows them in a pane next
to its own pages; cppreference.com itself cannot be shown in a frame.

Source: the offline archive of cppreference.com, release {version} of
https://github.com/PeterFeicht/cppreference-doc ({url}).
Each page is reduced to its article: the wiki's navigation, scripts and images are removed,
links to pages that are not hosted here point to cppreference.com, and the site's stylesheet
is applied. The text is otherwise unchanged. Each page links to its original.

Licence: cppreference.com's content is licensed under the Creative Commons
Attribution-ShareAlike 3.0 Unported License (CC-BY-SA 3.0) and the GNU Free Documentation
License (GFDL); see LICENSE.txt. The copies here are under the same licences.

Generated by tools/gen-apidocs (tools/apidocs/cppref.py); do not edit.
"""

LICENSE = """\
Attribution
-----------
The pages in this directory are copies of pages of cppreference.com
(https://en.cppreference.com), written by its contributors. Each page names and links its
original. Taken from the offline archive at https://github.com/PeterFeicht/cppreference-doc
and modified: reduced to the article, restyled, links rewritten (see README.txt).

Licences
--------
cppreference.com's content is dual-licensed; you may use these copies under either licence:

  - Creative Commons Attribution-ShareAlike 3.0 Unported (CC-BY-SA 3.0)
    https://creativecommons.org/licenses/by-sa/3.0/
    Legal code: https://creativecommons.org/licenses/by-sa/3.0/legalcode

  - GNU Free Documentation License, version 1.3 or any later version
    https://www.gnu.org/licenses/fdl-1.3.html
    (no Invariant Sections, no Front-Cover Texts, no Back-Cover Texts)

cppreference.com's statement of its licensing:
https://en.cppreference.com/w/Cppreference:Copyright/CC-BY-SA and
https://en.cppreference.com/w/Cppreference:Copyright/GDFL

These licences cover the cppreference.com material in this directory only, not libycxx or the
rest of libycxx.org.
"""
