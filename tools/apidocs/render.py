"""Finish MrDocs's pages of libycxx's API reference (tools/gen-apidocs).

MrDocs renders each entity's page through tools/apidocs/addons; its corpus does not know the
standard's view of an entity, so this step adds it:
  - the facts row: the header (draft-map.json, from the header synopses of the draft), the draft
    section (linked to eel.is), the namespace and the cppreference page (cppref.py);
  - the synopses: libycxx's reserved names for template parameters and function parameters
    (_Tp, __x) are shown as the standard spells them (T, x), compiler attributes ([[__gnu__::…]])
    are dropped, long parameter lists are wrapped, and the code is highlighted;
  - the index pages: api/index.html (by header and by namespace) and api/headers/<h>.html;
  - api/nav.json (the sidebar's index) and api/search.json (the search box's index).
"""
import collections, html, json, os, pathlib, re, shutil

# ---------------------------------------------------------------------------------------------
# Synopsis text: reserved names, attributes, wrapping, highlighting

KEYWORDS = set('''alignas alignof auto bool break case catch char char8_t char16_t char32_t class
concept const consteval constexpr constinit const_cast continue co_await co_return co_yield
decltype default delete do double dynamic_cast else enum explicit export extern false float for
friend goto if inline int long mutable namespace new noexcept nullptr operator private protected
public reinterpret_cast requires return short signed sizeof static static_assert static_cast
struct switch template this thread_local throw true try typedef typeid typename union unsigned
using virtual void volatile wchar_t while final override'''.split())
TYPES = set('bool char char8_t char16_t char32_t double float int long short signed unsigned void wchar_t auto'.split())
# libycxx's own namespaces stay visible if they ever reach a page: the leak check looks for them.
KEEP_RESERVED = {'__ycxx', '__detail', '__adl_free', '__cpo'}

_TAG = re.compile(r'(<[^>]+>)')
_TOK = re.compile(r'''(?P<c>/\*.*?\*/|//[^\n]*)|(?P<s>"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')|(?P<n>\b\d[\w.']*)|(?P<i>[A-Za-z_]\w*)|(?P<o>.)''', re.S)
_ATTR = re.compile(r'\[\[[^\[\]]*\]\]\s*')
_RESERVED = re.compile(r'\b(?:_([A-Z]\w*)|__([a-z]\w*))\b')


def plain_name(m):
    """_Tp -> Tp... the standard's T: libycxx's reserved spellings of the draft's names."""
    whole = m.group(0)
    if whole in KEEP_RESERVED or whole.startswith('__ycxx'):
        return whole
    name = m.group(1) or m.group(2)
    return {'Tp': 'T', 'Up': 'U', 'Ip': 'I', 'Sp': 'S', 'Rp': 'R', 'Fp': 'F', 'Vp': 'V', 'Np': 'N',
            'Ep': 'E', 'Op': 'O', 'Ap': 'A', 'Bp': 'B', 'Cp': 'C', 'Dp': 'D', 'Gp': 'G', 'Kp': 'K',
            'Pp': 'P', 'Xp': 'X', 'Yp': 'Y', 'Tp1': 'T1', 'Tp2': 'T2', 'Up1': 'U1', 'Up2': 'U2',
            'Ip1': 'I1', 'Ip2': 'I2', 'Sp1': 'S1', 'Sp2': 'S2', 'Rp1': 'R1', 'Rp2': 'R2',
            'Op1': 'O1', 'Op2': 'O2'}.get(name, name)


def _drop_attributes(text):
    def keep(m):
        a = m.group(0)
        return '' if '__' in a or 'gnu::' in a or 'clang::' in a else a
    return _ATTR.sub(keep, text)


def _segments(markup):
    """Split markup into ('tag', s) and ('text', unescaped s) parts."""
    out = []
    for part in _TAG.split(markup):
        if not part:
            continue
        if part.startswith('<'):
            out.append(('tag', part))
        else:
            out.append(('text', html.unescape(part)))
    return out


def _wrap(segs, width=88):
    """Break a long declaration after the commas of its parameter list."""
    text = ''.join(s for k, s in segs if k == 'text')
    lines, pos, breaks = text.split('\n'), 0, []
    for line in lines:
        if len(line) > width and '(' in line:
            depth_p = depth_a = 0
            start = None
            for i, ch in enumerate(line):
                if ch == '<' and start is None:
                    depth_a += 1
                elif ch == '>' and start is None and depth_a and line[i - 1] != '-':
                    depth_a -= 1
                elif ch == '(':
                    if start is None and depth_a == 0 and i and (line[i - 1].isalnum() or line[i - 1] in '_>]'):
                        before = re.search(r'(\w+)\s*$', line[:i])
                        if not before or before.group(1) not in ('decltype', 'noexcept', 'requires', 'sizeof', 'alignof', 'explicit'):
                            start = i
                            depth_p = 1
                            breaks.append((pos + i + 1, False))
                            continue
                    if start is not None:
                        depth_p += 1
                elif ch == ')' and start is not None:
                    depth_p -= 1
                    if depth_p == 0:
                        breaks.append((pos + i, True))
                        break
                elif start is not None and depth_p == 1:
                    if ch == '<':
                        depth_a += 1
                    elif ch == '>' and depth_a:
                        depth_a -= 1
                    elif ch == ',' and depth_a == 0:
                        breaks.append((pos + i + 1, False))
            if len(breaks) <= 2:
                breaks = [b for b in breaks if b[0] < pos]
        pos += len(line) + 1
    if not breaks:
        return segs
    # Apply: after "(" and each ",": newline + 4 spaces (the space after a comma is dropped);
    # before the closing ")": nothing (cppreference style keeps ")" after the last parameter).
    bset = {b for b, close in breaks if not close}
    out, pos = [], 0
    for k, s in segs:
        if k == 'tag':
            out.append((k, s))
            continue
        buf = []
        for ch in s:
            if pos in bset and ch == ' ':
                pos += 1
                continue
            buf.append(ch)
            pos += 1
            if pos in bset:
                buf.append('\n    ')
        out.append((k, ''.join(buf)))
    return out


def highlight(markup, wrap=True):
    """Clean, wrap and highlight the C++ in markup (text and <a> tags)."""
    segs = _segments(markup)
    segs = [(k, _RESERVED.sub(plain_name, _drop_attributes(s)) if k == 'text' else s) for k, s in segs]
    if wrap:
        segs = _wrap(segs)
    out, in_link = [], 0
    for k, s in segs:
        if k == 'tag':
            if s.startswith('<a '):
                in_link += 1
            elif s == '</a>':
                in_link -= 1
            out.append(s)
            continue
        toks = list(_TOK.finditer(s))
        for i, m in enumerate(toks):
            t = m.group(0)
            e = html.escape(t, quote=False)
            if in_link:
                out.append(e)
            elif m.lastgroup == 'c':
                out.append(f'<span class="t-c">{e}</span>')
            elif m.lastgroup == 's':
                out.append(f'<span class="t-s">{e}</span>')
            elif m.lastgroup == 'n':
                out.append(f'<span class="t-n">{e}</span>')
            elif m.lastgroup == 'i' and t in KEYWORDS:
                out.append(f'<span class="{"t-t" if t in TYPES else "t-k"}">{e}</span>')
            elif m.lastgroup == 'i':
                nxt = next((x.group(0) for x in toks[i + 1:] if not x.group(0).isspace()), '')
                out.append(f'<span class="t-f">{e}</span>' if nxt == '(' else e)
            else:
                out.append(e)
    return ''.join(out)


_PRE = re.compile(r'(<pre class="cpp"><code>)(.*?)(</code></pre>)', re.S)
_CODE = re.compile(r'(<code class="cpp">)(.*?)(</code>)', re.S)


def highlight_page(text):
    text = _PRE.sub(lambda m: m.group(1) + highlight(m.group(2).rstrip('\n')) + m.group(3), text)
    text = _CODE.sub(lambda m: m.group(1) + highlight(m.group(2), wrap=False) + m.group(3), text)
    return text


# ---------------------------------------------------------------------------------------------
# The corpus as the pages describe it

_ARTICLE = re.compile(r'<article class="ref" data-kind="([^"]*)" data-q="([^"]*)" data-file="([^"]*)" data-label="([^"]*)">')


class Page:
    __slots__ = ('path', 'rel', 'kind', 'q', 'file', 'label', 'header', 'headers', 'sec', 'cppref', 'text')


def load_pages(src):
    pages = []
    for p in sorted(src.rglob('*.html')):
        text = p.read_text(encoding='utf-8')
        m = _ARTICLE.search(text)
        if not m:
            continue
        pg = Page()
        pg.path, pg.rel = p, p.relative_to(src).as_posix()
        pg.kind, pg.q, pg.file, pg.label = m.group(1), html.unescape(m.group(2)), m.group(3), html.unescape(m.group(4))
        pg.q = pg.q.replace('__cpo::', '')
        pg.text = text
        pg.header, pg.headers, pg.sec, pg.cppref = None, [], '', None
        pages.append(pg)
    return pages


def include_graph(repo, public):
    """internal header -> the public headers that include it, directly or through other internal ones."""
    inc = re.compile(r'^\s*#\s*include\s*<([^>]+)>', re.M)
    root = repo / 'include'
    direct = {}
    for p in root.rglob('*'):
        if p.is_file():
            rel = p.relative_to(root).as_posix()
            try:
                direct[rel] = inc.findall(p.read_text(encoding='utf-8', errors='replace'))
            except OSError:
                pass
    users = collections.defaultdict(set)
    for h in public:
        seen, todo = set(), list(direct.get(h, []))
        while todo:
            f = todo.pop()
            if f in seen or f in public:
                continue
            seen.add(f)
            users[f].add(h)
            todo.extend(direct.get(f, []))
    return users


def resolve(pages, draft, public, users):
    """Each page's header(s) and draft section."""
    ents, syn = draft.get('entities', {}), draft.get('headers', {})
    by_q = {pg.q: pg for pg in pages if pg.kind != 'function' or True}

    def lookup(q):
        for cand in (q, q.replace('std::views::', 'std::ranges::views::'), q.replace('std::ranges::views::', 'std::views::')):
            if cand in ents:
                return ents[cand]
        return None

    def own(pg):
        hit = lookup(pg.q)
        hs = list(hit[1]) if hit else []
        sec = hit[0] if hit else ''
        if not hs and pg.file:
            if pg.file in public:
                hs = [pg.file]
            elif len(users.get(pg.file, ())) >= 1:
                hs = sorted(users[pg.file])
        return hs, sec

    cache = {}

    def info(q, pg=None):
        if q in cache:
            return cache[q]
        pg = pg or by_q.get(q)
        hs, sec = own(pg) if pg else ((lookup(q) or ['', []])[1], (lookup(q) or [''])[0])
        parent = q.rsplit('::', 1)[0] if '::' in q else ''
        ppg = by_q.get(parent)
        if ppg is not None and ppg.kind == 'record':
            phs, psec = info(parent)
            if phs:
                hs = phs     # a member is declared in its class's header
            sec = sec or psec
        cache[q] = (hs, sec)
        return cache[q]

    for pg in pages:
        hs, sec = info(pg.q, pg)
        # The primary header: the one whose synopsis is nearest the entity's subclause, then
        # the one libycxx's own include graph names for its definition.
        if len(hs) > 1:
            near = sec.split('.')[0] if sec else ''
            fav = [h for h in hs if syn.get(h, '').split('.')[0] == near]
            mine = [h for h in hs if h in users.get(pg.file, ()) or h == pg.file]
            first = (fav or mine or hs)[0]
            hs = [first] + [h for h in hs if h != first]
        pg.headers = hs
        pg.header = hs[0] if hs else None
        pg.sec = sec or (syn.get(pg.header, '') if pg.header else '')


# ---------------------------------------------------------------------------------------------
# Page assembly

def facts(pg, cpp, draft):
    rows = []
    if pg.header:
        more = ''
        if len(pg.headers) > 1:
            extra = pg.headers[1:6]
            more = ' <span class="also">also in ' + ', '.join(
                f'<a href="<!--ycxx:root-->api/headers/{h}.html"><code>&lt;{h}&gt;</code></a>' for h in extra) + \
                (f' and {len(pg.headers) - 6} more' if len(pg.headers) > 6 else '') + '</span>'
        rows.append(('Header', f'<a href="<!--ycxx:root-->api/headers/{pg.header}.html"><code>&lt;{pg.header}&gt;</code></a>{more}'))
    if pg.sec:
        rows.append(('Draft', f'<a href="https://eel.is/c++draft/{pg.sec}" rel="noopener">[{pg.sec}]</a>'))
    ns = '::'.join(p for p in pg.q.split('::')[:-1]) if '::' in pg.q else ''
    if ns:
        rows.append(('Scope', f'<code>{html.escape(ns)}</code>'))
    if cpp:
        live = 'https://en.cppreference.com/w/' + cpp['p']
        attrs = (f'data-cppref="{html.escape(cpp["p"])}" data-hosted="{1 if cpp.get("h") else 0}" '
                 f'data-exact="{1 if cpp.get("x") else 0}" data-title="{html.escape(cpp.get("t", pg.q))}"')
        note = '' if cpp.get('x') else ' <span class="also">nearest page</span>'
        rows.append(('cppreference', f'<a class="cppref-link" href="{live}" target="_blank" rel="noopener" {attrs}>'
                                     f'{html.escape(cpp.get("t", pg.q))} <span aria-hidden="true">↗</span></a>{note}'))
    if not rows:
        return ''
    return '<dl class="facts ref-facts">' + ''.join(f'<div><dt>{k}</dt><dd>{v}</dd></div>' for k, v in rows) + '</dl>'


def shell_parts(text):
    """The page shell around <main>'s content, from one of MrDocs's pages."""
    a = text.index('<main id="main">') + len('<main id="main">')
    b = text.index('<footer>')
    return text[:a], text[b:]


def fill(text, depth, current=''):
    root = '../' * (depth + 1)   # pages are under site/api/; the site's root is one level up
    return text.replace('<!--ycxx:root-->', root).replace('<!--ycxx:current-->', html.escape(current))


def set_title(text, title):
    return re.sub(r'<title>.*?</title>', f'<title>{html.escape(title)} — libycxx API reference</title>', text, count=1, flags=re.S)


KIND_ORDER = ['namespace', 'concept', 'class template', 'class', 'struct template', 'struct', 'union',
              'enumeration', 'scoped enumeration', 'alias template', 'type alias', 'function template',
              'function', 'operator', 'variable template', 'constant', 'variable']


def group_of(label):
    l = label.split(' · ')[0]
    if 'namespace' in l:
        return 'Namespaces'
    if l == 'concept':
        return 'Concepts'
    if l.startswith(('class', 'struct', 'union')):
        return 'Classes'
    if 'enumeration' in l:
        return 'Enumerations'
    if 'alias' in l:
        return 'Type aliases'
    if 'function' in l or 'operator' in l or 'constructor' in l:
        return 'Functions'
    if 'variable' in l or 'constant' in l:
        return 'Variables and constants'
    return 'Other'


GROUPS = ['Namespaces', 'Concepts', 'Classes', 'Enumerations', 'Type aliases', 'Functions', 'Variables and constants', 'Other']


def entity_list(items, depth):
    """A grouped table of entities (header pages, the index)."""
    root = '../' * depth
    by = collections.defaultdict(list)
    for pg in items:
        by[group_of(pg.label)].append(pg)
    out = []
    for g in GROUPS:
        if not by.get(g):
            continue
        gid = re.sub(r'\W+', '-', g.lower()).strip('-')
        out.append(f'<section class="ref-sec" aria-labelledby="g-{gid}"><h2 id="g-{gid}">{g} <span class="count">{len(by[g])}</span></h2>'
                   '<ul class="entity-list">')
        for pg in sorted(by[g], key=lambda p: p.q.lower()):
            leaf = pg.q.split('::')[-1]
            scope = pg.q[:-len(leaf)]
            out.append(f'<li><a href="{root}{pg.rel}"><code><span class="ref-scope">{html.escape(scope)}</span>{html.escape(leaf)}</code></a>'
                       f'<span class="m-kind">{html.escape(pg.label)}</span></li>')
        out.append('</ul></section>')
    return ''.join(out)


def render(work, out, draft, cppref, cppref_out, headers, repo=None):
    repo = repo or pathlib.Path(__file__).resolve().parents[2]
    src = work / 'out-html'
    pages = load_pages(src)
    public = set(headers)
    users = include_graph(repo, public)
    resolve(pages, draft, public, users)

    # Which page stands for each entity: an overload set's page, not one of its members'.
    canonical = {}
    for pg in pages:
        cur = canonical.get(pg.q)
        if cur is None or (pg.kind == 'overloads' and cur.kind != 'overloads'):
            canonical[pg.q] = pg

    cpp_map = {}
    if cppref is not None:
        cpp_map = cppref.build(canonical, cppref_out)

    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)
    for pg in pages:
        depth = pg.rel.count('/')
        text = pg.text.replace('<!--ycxx:facts-->', facts(pg, cpp_map.get(pg.q), draft))
        text = highlight_page(text)
        text = fill(text, depth, pg.header or '')
        dst = out / pg.rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_text(text, encoding='utf-8')

    # Namespace-scope entities: the index's unit.
    def at_namespace_scope(pg):
        parent = canonical.get(pg.q.rsplit('::', 1)[0]) if '::' in pg.q else None
        return parent is not None and parent.kind == 'namespace'
    top = [pg for pg in canonical.values() if pg.kind != 'namespace' and pg.q.startswith('std::') and at_namespace_scope(pg)]
    namespaces = sorted([pg for pg in canonical.values() if pg.kind == 'namespace' and pg.q], key=lambda p: p.q)

    std_page = next((pg for pg in pages if pg.q == 'std' and pg.kind == 'namespace'), pages[0])
    head, tail = shell_parts(std_page.text)

    by_header = collections.defaultdict(list)
    for pg in top:
        for h in pg.headers[:1]:
            by_header[h].append(pg)
    syn = draft.get('headers', {})
    (out / 'headers').mkdir()
    for h in sorted(public):
        items = by_header.get(h, [])
        sec = syn.get(h, '')
        body = ['<article class="ref ref-header">',
                '<nav class="crumbs" aria-label="Breadcrumbs"><ol><li><a href="<!--ycxx:root-->api/index.html">Reference</a></li>'
                f'<li aria-current="page">&lt;{h}&gt;</li></ol></nav>',
                f'<header class="ref-head"><p class="label">header</p><h1 class="ref-name"><span class="ref-leaf">&lt;{h}&gt;</span></h1>',
                '<dl class="facts ref-facts">',
                f'<div><dt>Include</dt><dd><code>#include &lt;{h}&gt;</code> or <code>import std;</code></dd></div>']
        if sec:
            body.append(f'<div><dt>Draft</dt><dd><a href="https://eel.is/c++draft/{sec}" rel="noopener">[{sec}]</a></dd></div>')
        hp = cppref.header_page(h) if cppref is not None else None
        if hp:
            body.append(f'<div><dt>cppreference</dt><dd><a class="cppref-link" href="https://en.cppreference.com/w/{hp["p"]}" target="_blank" rel="noopener" '
                        f'data-cppref="{hp["p"]}" data-hosted="{1 if hp.get("h") else 0}" data-exact="1" data-title="&lt;{h}&gt;">&lt;{h}&gt; <span aria-hidden="true">↗</span></a></dd></div>')
        body.append(f'<div><dt>Entities</dt><dd>{len(items)} at namespace scope</dd></div></dl></header>')
        body.append(entity_list(items, 2) if items else '<p class="note">This header declares macros only, or only what other headers declare as well.</p>')
        body.append('</article>')
        text = set_title(head, f'<{h}>') + '\n'.join(body) + tail
        (out / 'headers' / f'{h}.html').write_text(highlight_page(fill(text, 1, h)), encoding='utf-8')

    # The index: headers grouped as the draft's clauses group them, and the namespaces.
    groups = collections.OrderedDict()
    for h in sorted(public):
        clause = (syn.get(h, '') or 'other').split('.')[0]
        groups.setdefault(h[0] if not syn.get(h) else clause, []).append(h)
    body = ['<article class="ref ref-index">',
            '<header class="ref-head"><p class="label">API reference · libycxx</p><h1 class="ref-name"><span class="ref-leaf">The C++26 standard library, as libycxx declares it</span></h1>',
            '<p class="lede">Every entity the <code>std</code> module exports, with its synopsis as libycxx declares it, '
            'generated from the headers by MrDocs. Each page names the header, the section of the working draft '
            'and the cppreference page; the specification itself is the draft.</p>',
            f'<dl class="facts ref-facts"><div><dt>Headers</dt><dd>{len(public)}</dd></div>'
            f'<div><dt>Entities</dt><dd>{len(top):,} at namespace scope, {len(canonical):,} with members</dd></div>'
            f'<div><dt>Draft</dt><dd><a href="https://eel.is/c++draft/">eel.is/c++draft</a>, revision <code>{draft.get("revision", "")[:10]}</code></dd></div>'
            '<div><dt>Build</dt><dd><code>find_package(libycxx)</code> and <code>ycxx::ycxx</code> or <code>ycxx::modules</code> in CMake '
            '(<a href="<!--ycxx:root-->index.html#start">getting started</a>)</dd></div></dl></header>',
            '<section class="ref-sec" aria-labelledby="ix-headers"><h2 id="ix-headers">Headers</h2><ul class="header-grid">']
    for h in sorted(public):
        n = len(by_header.get(h, []))
        body.append(f'<li><a href="headers/{h}.html"><code>&lt;{h}&gt;</code><span>{n}</span></a></li>')
    body.append('</ul></section><section class="ref-sec" aria-labelledby="ix-ns"><h2 id="ix-ns">Namespaces</h2><ul class="entity-list">')
    for pg in namespaces:
        body.append(f'<li><a href="{pg.rel}"><code>{html.escape(pg.q)}</code></a><span class="m-kind">namespace</span></li>')
    body.append('</ul></section></article>')
    text = set_title(head, 'Index') + '\n'.join(body) + tail
    (out / 'index.html').write_text(fill(text, 0, ''), encoding='utf-8')

    # Global-namespace page of MrDocs (its index.html) moves aside.
    g = next((pg for pg in pages if pg.rel == 'index.html'), None)
    if g is not None:
        (out / 'global.html').write_text(fill(highlight_page(g.text.replace('<!--ycxx:facts-->', '')), 0), encoding='utf-8')

    # The sidebar's index and the search index.
    nav = {'headers': {}, 'namespaces': []}
    for h in sorted(public):
        nav['headers'][h] = [[pg.q, pg.rel, group_of(pg.label)[0]] for pg in sorted(by_header.get(h, []), key=lambda p: p.q.lower())]
    nav['namespaces'] = [[pg.q, pg.rel] for pg in namespaces]
    (out / 'nav.json').write_text(json.dumps(nav, separators=(',', ':')), encoding='utf-8')
    search = [[pg.q, pg.label, pg.rel, pg.header or ''] for pg in sorted(canonical.values(), key=lambda p: (p.q.count('::'), len(p.q), p.q))
              if pg.q and pg.rel != 'index.html']
    (out / 'search.json').write_text(json.dumps(search, separators=(',', ':')), encoding='utf-8')
    if cpp_map:
        (out / 'cppref-map.json').write_text(json.dumps(cpp_map, separators=(',', ':'), sort_keys=True), encoding='utf-8')
    tag = out / 'reference.tag.xml'
    if tag.exists():
        tag.unlink()
    print(f'render: {len(pages)} pages, {len(top)} namespace-scope entities, {len(public)} headers', flush=True)
    return pages
