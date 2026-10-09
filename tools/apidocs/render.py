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
import collections, html, json, os, pathlib, posixpath, re, shutil

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
# Reserved names: libycxx spells a template parameter _Xp (the draft's X) and a function
# parameter __x (the draft's x); both are shown as the draft spells them. Any other reserved name
# that reaches a page (a private member type used in a public declaration, __traits::int_type)
# is shown as the draft shows its exposition-only names: in italics and hyphenated.
_RESERVED = re.compile(r'(?<![\w])(?:_([A-Z]\w*)|(?<!::)__([a-z]\w*)\b(?!\s*(?:::|<)))')
_OTHER_RESERVED = re.compile(r'(?<![\w])__[a-z]\w*')
_INTERNAL = re.compile(r'(?:::)?__ycxx::(?:\w+::)*(\w+)')


def plain_name(m):
    """_Tp -> Tp... the standard's T: libycxx's reserved spellings of the draft's names."""
    whole = m.group(0)
    if whole in KEEP_RESERVED or whole.startswith('__ycxx') or whole in ('_Exit',):
        return whole
    name = m.group(1) or m.group(2)
    return {'Ep_': 'ExecutionPolicy', 'Tp': 'T', 'Up': 'U', 'Ip': 'I', 'Sp': 'S', 'Rp': 'R', 'Fp': 'F', 'Vp': 'V', 'Np': 'N',
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


def _opener(line):
    """The index of the bracket that opens a declaration's list: a template head's "<", or the
    parameter list's "(" (the first "(" after a name that is not decltype(…), noexcept(…)…)."""
    if line.startswith('template<'):
        return 8, '<', '>'
    depth = 0
    for i, ch in enumerate(line):
        if ch == '<':
            depth += 1
        elif ch == '>' and depth and line[i - 1] != '-':
            depth -= 1
        elif ch == '(' and depth == 0 and i and (line[i - 1].isalnum() or line[i - 1] in '_>]=+-*/%^&|!~,[]'):
            before = re.search(r'(\w+)\s*$', line[:i])
            if before and before.group(1) in ('decltype', 'noexcept', 'requires', 'sizeof', 'alignof', 'explicit', 'alignas'):
                continue
            return i, '(', ')'
    return None


def _breaks(line, width):
    """Offsets in line after which a long list breaks (after its opener and each comma)."""
    if len(line) <= width:
        return []
    o = _opener(line)
    if not o:
        return []
    i0, op, cl = o
    out, depth = [i0 + 1], 0
    pairs = {'(': ')', '[': ']', '{': '}', '<': '>'}
    closers = set(pairs.values())
    for i in range(i0 + 1, len(line)):
        ch = line[i]
        if ch == cl and depth == 0:
            break
        if ch in pairs and not (ch == '<' and line[i - 1:i + 1] == '<<'):
            depth += 1
        elif ch in closers and depth and not (ch == '>' and line[i - 1] == '-'):
            depth -= 1
        elif ch == ',' and depth == 0:
            out.append(i + 1)
    return out if len(out) > 1 or len(line) > width + 20 else []


def _wrap(segs, width=76):
    """Break a long declaration after the commas of its template head or parameter list."""
    text = ''.join(s for k, s in segs if k == 'text')
    bset, pos = set(), 0
    for line in text.split('\n'):
        bset.update(pos + b for b in _breaks(line, width))
        pos += len(line) + 1
    if not bset:
        return segs
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


def expo_name(name):
    return name.strip('_').replace('_', '-')


def _expo(segs):
    """Split out libycxx's internal names (shown in italics as exposition-only names)."""
    out = []
    for k, s in segs:
        if k != 'text' or '__' not in s:
            out.append((k, s))
            continue
        pos = 0
        for m in re.finditer(_INTERNAL.pattern + '|' + _OTHER_RESERVED.pattern, s):
            whole = m.group(0).lstrip(':')
            if whole in KEEP_RESERVED - {'__ycxx', '__detail', '__adl_free'}:
                continue
            out.append(('text', s[pos:m.start()] + (':: ' if False else '')))
            out.append(('expo', (expo_name(m.group(1) or whole), whole)))
            pos = m.end()
        out.append(('text', s[pos:]))
    return out


def highlight(markup, wrap=True):
    """Clean, wrap and highlight the C++ in markup (text and <a> tags)."""
    segs = _segments(markup)
    segs = [(k, _RESERVED.sub(plain_name, _drop_attributes(s)) if k == 'text' else s) for k, s in segs]
    if wrap:
        segs = _wrap(segs)
    segs = _expo(segs)
    out, in_link = [], 0
    for k, s in segs:
        if k == 'tag':
            if s.startswith('<a '):
                in_link += 1
            elif s == '</a>':
                in_link -= 1
            out.append(s)
            continue
        if k == 'expo':
            out.append(f'<i class="expo" title="exposition only; libycxx: {html.escape(s[1])}">{html.escape(s[0])}</i>')
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


def clean_visible(text):
    """The reserved spellings in the page's visible text outside the synopses (titles, breadcrumbs)."""
    def fix(seg, tags=True):
        if seg.startswith('<'):
            return seg
        seg = _RESERVED.sub(plain_name, seg)
        rx = re.compile(_INTERNAL.pattern + '|' + _OTHER_RESERVED.pattern)
        def rep(m):
            whole = m.group(0).lstrip(':')
            name = expo_name(m.group(1) or whole)
            if not tags:
                return name
            return f'<i class="expo" title="exposition only; libycxx: {html.escape(whole)}">{html.escape(name)}</i>'
        return rx.sub(rep, seg)
    a = text.find('<main id="main">')
    t0 = text.find('<title>')
    head = text[:a]
    if t0 >= 0:
        t1 = text.index('</title>', t0)
        head = text[:t0] + ''.join(fix(x, False) for x in _TAG.split(text[t0:t1])) + text[t1:a]
    return head + ''.join(fix(x) for x in _TAG.split(text[a:]))


def highlight_page(text):
    text = _PRE.sub(lambda m: m.group(1) + highlight(m.group(2).rstrip('\n')) + m.group(3), text)
    text = _CODE.sub(lambda m: m.group(1) + highlight(m.group(2), wrap=False) + m.group(3), text)
    return text


# ---------------------------------------------------------------------------------------------
# The corpus as the pages describe it

_ARTICLE = re.compile(r'<article class="ref" data-kind="([^"]*)" data-q="([^"]*)" data-file="([^"]*)" data-label="([^"]*)">')


class Page:
    __slots__ = ('path', 'rel', 'orig', 'kind', 'q', 'file', 'label', 'header', 'headers', 'sec', 'cppref', 'text')


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
        if pg.kind == 'variable' and re.search(r'<code>[^<]*(?:inline )?constexpr /\* implementation-defined \*/ ', text):
            # A customization point object or an algorithm function object ([customization.point.object],
            # [algorithms.requirements]/2): its type is the implementation's.
            pg.label = 'function object'
            text = text.replace('data-label="constant"', 'data-label="function object"').replace('<p class="label">constant</p>', '<p class="label">function object</p>')
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
                # ycxx/core/new.hpp: <new> first among the headers that include it.
                stem = posixpath.splitext(posixpath.basename(pg.file))[0]
                hs.sort(key=lambda h: h != stem)
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


def group_of(label, q=''):
    if '<' in q:
        return 'Specializations'
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


GROUPS = ['Namespaces', 'Concepts', 'Classes', 'Enumerations', 'Type aliases', 'Functions', 'Variables and constants', 'Other', 'Specializations']


def entity_list(items, depth):
    """A grouped table of entities (header pages, the index)."""
    root = '../' * depth
    by = collections.defaultdict(list)
    for pg in items:
        by[group_of(pg.label, pg.q)].append(pg)
    out = []
    for g in GROUPS:
        if not by.get(g):
            continue
        gid = re.sub(r'\W+', '-', g.lower()).strip('-')
        out.append(f'<section class="ref-sec" aria-labelledby="g-{gid}"><h2 id="g-{gid}">{g} <span class="count">{len(by[g])}</span></h2>'
                   '<ul class="entity-list">')
        for pg in sorted(by[g], key=lambda p: p.q.lower()):
            scope, leaf = split_scope(pg.q)
            out.append(f'<li><a href="{root}{pg.rel}"><code><span class="ref-scope">{html.escape(scope)}</span>{html.escape(leaf)}</code></a>'
                       f'<span class="m-kind">{html.escape(pg.label)}</span></li>')
        out.append('</ul></section>')
    return ''.join(out)


def display_q(q):
    """A qualified name as the reader sees it (reserved spellings as the draft spells them)."""
    q = _RESERVED.sub(plain_name, q)
    return re.sub(_INTERNAL.pattern + '|' + _OTHER_RESERVED.pattern, lambda m: expo_name(m.group(1) or m.group(0)), q)


def split_scope(q):
    """('std::ranges::', 'sort'): q split after its last :: outside template arguments."""
    depth, cut = 0, 0
    for i, ch in enumerate(q):
        if ch == '<':
            depth += 1
        elif ch == '>':
            depth -= 1
        elif depth == 0 and q.startswith('::', i):
            cut = i + 2
    return q[:cut], q[cut:]


def _bare(q):
    """q without template arguments: std::hash<vector<bool>>::operator() -> std::hash::operator()."""
    out, depth = [], 0
    for ch in q:
        if ch == '<':
            depth += 1
        elif ch == '>':
            depth -= 1
        elif depth == 0:
            out.append(ch)
    return ''.join(out)


_RESERVED_PART = re.compile(r'^(?:__|_[A-Z])')


def public_pages(pages, exported):
    """The pages of the std module's exports and their members; not members with reserved names
    (a hook between libycxx's own classes) nor what the module does not export."""
    if not exported:
        return pages, set()
    allowed = {n.replace('::__cpo::', '::') for n in exported}
    scopes = set()
    for n in allowed:
        parts = n.split('::')
        scopes.update('::'.join(parts[:i]) for i in range(1, len(parts)))
    kinds = collections.defaultdict(set)
    for pg in pages:
        kinds[_bare(pg.q)].add(pg.kind)
    memo = {}

    def ok(q):
        if q in memo:
            return memo[q]
        parts = q.split('::')
        if not q.startswith('std'):
            r = True                     # the global allocation functions, the C library's types
        elif any(_RESERVED_PART.match(x) for x in parts):
            r = False
        elif q in allowed or q in scopes:
            r = True
        elif len(parts) > 1:
            parent = '::'.join(parts[:-1])
            r = bool(kinds[parent] & {'record', 'enum'}) and ok(parent)
        else:
            r = False
        memo[q] = r
        return r
    keep = [pg for pg in pages if ok(_bare(pg.q))]
    gone = {pg.rel for pg in pages if not ok(_bare(pg.q))}
    return keep, gone


def _top_args(ty):
    """'X<A, B<C>>' -> ('X', ['A', 'B<C>'])."""
    i = ty.find('<')
    if i < 0:
        return ty, []
    head, inner, depth, cur, args = ty[:i], ty[i + 1:ty.rfind('>')], 0, '', []
    for ch in inner:
        if ch == '<':
            depth += 1
        elif ch == '>':
            depth -= 1
        if ch == ',' and depth == 0:
            args.append(cur.strip())
            cur = ''
        else:
            cur += ch
    if cur.strip():
        args.append(cur.strip())
    return head, args


def call_signatures(fo_dir, fobjs):
    """{object: [declaration markup]}: the call operators of each function object's type
    (MrDocs's second run, over libycxx's classes), renamed to the object's name."""
    if not fobjs or not fo_dir.exists():
        return {}
    ops = collections.defaultdict(list)          # class (as MrDocs names it) -> code blocks
    grouped = set()
    for p in sorted(fo_dir.rglob('*.html')):
        text = p.read_text(encoding='utf-8')
        m = _ARTICLE.search(text)
        if not m or not html.unescape(m.group(2)).endswith('::operator()'):
            continue
        cls = html.unescape(m.group(2))[:-len('::operator()')]
        kind = m.group(1)
        blocks = re.findall(r'<pre class="cpp"><code>(.*?)</code></pre>', text, re.S)
        if kind == 'overloads':
            ops[cls] = blocks
            grouped.add(cls)
        elif cls not in grouped:
            ops[cls].extend(blocks)
    by_bare = collections.defaultdict(list)
    for cls in ops:
        by_bare[_bare(cls)].append(cls)

    def find(name, args=()):
        cands = by_bare.get(_bare(name), [])
        if len(cands) > 1 and args:
            last = re.split(r'::|\.', args[-1])[-1]
            fit = [c for c in cands if re.split(r'::|\.', _top_args(c)[1][-1] if _top_args(c)[1] else '')[-1] == last]
            if fit:
                return fit[0]
        prim = [c for c in cands if '<' not in c]
        return (prim or cands or [None])[0]

    out = {}
    for q, ty in fobjs.items():
        head, args = _top_args(ty)
        found = [find(a) for a in args if a.startswith('__ycxx::')] + [find(head, args)]
        name = q.rsplit('::', 1)[-1]
        decls, seen = [], set()
        for cls in found:
            for b in ops.get(cls, []) if cls else []:
                b = re.sub(r'</?a\b[^>]*>', '', b)
                if b.lstrip().startswith('using '):
                    continue        # using Base::operator(): the base's operators are listed already
                b = re.sub(r'operator\(\)\(', name + '(', b, count=1)
                b = re.sub(r'\) const(?=[ ;\n]|$)', ')', b, count=1)
                key = re.sub(r'\s+', ' ', b)
                if key not in seen:
                    seen.add(key)
                    decls.append(b)
        if decls:
            out[q] = decls
    return out


def calls_section(name, decls):
    items = ''.join(f'<li class="ovl-item" id="call-{i}"><div class="ovl-code"><pre class="cpp"><code>{d}</code></pre></div>'
                    f'<span class="ovl-n" aria-label="call signature {i}">({i})</span></li>' for i, d in enumerate(decls, 1))
    return ('<section class="ref-sec syn" aria-labelledby="sec-calls"><h2 id="sec-calls">Call signatures</h2>'
            f'<p class="note sec-note"><code>{html.escape(name)}</code> is a function object; these are its call operators as libycxx '
            'declares them, written as the functions the draft declares.</p>'
            f'<ol class="ovl">{items}</ol></section>')


def render(work, out, draft, cppref, cppref_out, headers, repo=None, exported=None, fobjs=None):
    repo = repo or pathlib.Path(__file__).resolve().parents[2]
    src = work / 'out-html'
    pages, gone = public_pages(load_pages(src), exported)
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

    # MrDocs also gives each member of an overload set a page of its own; the set's page shows
    # every overload, numbered, so those pages go, and links to them go to the set.
    moved = {pg.rel: canonical[pg.q].rel for pg in pages
             if pg.kind == 'function' and canonical[pg.q] is not pg and canonical[pg.q].kind == 'overloads'}
    pages = [pg for pg in pages if pg.rel not in moved]

    # Plain URLs: MrDocs names a page name-xx.html when several symbols share its name (a class
    # and its deduction guides, its specializations); the entity's own page takes name.html
    # (and its members' directory name/) whenever nothing else has it.
    for pg in pages:
        pg.orig = pg.rel
    taken = {pg.rel for pg in pages}
    dirs = set()
    for r in taken:
        parts = r.split('/')
        dirs.update('/'.join(parts[:i]) for i in range(1, len(parts)))
    order = {'namespace': 0, 'record': 1, 'concept': 2, 'enum': 2, 'typedef': 3, 'overloads': 4, 'variable': 5, 'function': 6}
    file_new, dir_new = {}, {}
    for pg in sorted({id(p): p for p in canonical.values()}.values(), key=lambda p: (order.get(p.kind, 9), p.rel)):
        if pg.rel in gone or pg.rel in moved:
            continue
        d, base = posixpath.split(pg.rel)
        m = re.match(r'^(.+)-[0-9a-f]{2,4}\.html$', base)
        if not m:
            continue
        clean = posixpath.join(d, m.group(1))
        old_dir = pg.rel[:-5]
        if clean + '.html' in taken or (clean in dirs and clean != old_dir):
            continue
        taken.add(clean + '.html')
        dirs.add(clean)
        file_new[pg.rel] = m.group(1) + '.html'
        if old_dir in dirs:
            dir_new[old_dir] = m.group(1)

    def final(orig):
        parts = orig.split('/')
        for i in range(len(parts) - 1):
            key = '/'.join(orig.split('/')[:i + 1])
            if key in dir_new:
                parts[i] = dir_new[key]
        if orig in file_new:
            parts[-1] = file_new[orig]
        return '/'.join(parts)

    for pg in pages:
        pg.rel = final(pg.orig)

    def relink(pg, text):
        here, new_here = posixpath.dirname(pg.orig), posixpath.dirname(pg.rel)
        def fix(m):
            href = m.group(1)
            if '://' in href or href.startswith(('#', 'mailto:', '<!--')):
                return m.group(0)
            path, _, frag = href.partition('#')
            if not path:
                return m.group(0)
            target = posixpath.normpath(posixpath.join(here, path))
            if target in gone:
                return 'href="#ycxx-gone"'
            target = final(moved.get(target, target))
            return 'href="' + posixpath.relpath(target, new_here or '.') + ('#' + frag if frag else '') + '"'
        return re.sub(r'href="([^"]+)"', fix, text)

    calls = call_signatures(work / 'out-fo', fobjs)
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)
    for pg in pages:
        depth = pg.rel.count('/')
        text = relink(pg, pg.text)
        if '#ycxx-gone' in text:
            # Rows of members that are not part of the API go; other mentions stay as text.
            text = re.sub(r'<tr>(?:(?!</tr>).)*?href="#ycxx-gone".*?</tr>\n?', '', text, flags=re.S)
            text = re.sub(r'<a href="#ycxx-gone">(.*?)</a>', r'\1', text, flags=re.S)
            text = re.sub(r'<section class="ref-sec" aria-labelledby="[^"]*">\s*<h2[^>]*>[^<]*</h2>\s*<div class="table-wrap"><table class="data members">\s*<thead>.*?</thead>\s*<tbody>\s*</tbody></table></div>\s*</section>', '', text, flags=re.S)
        text = text.replace('<!--ycxx:facts-->', facts(pg, cpp_map.get(pg.q), draft))
        if pg.q in calls and pg.kind == 'variable':
            i = text.find('</section>', text.find('id="sec-syn"'))
            if i >= 0:
                i += len('</section>')
                text = text[:i] + '\n' + calls_section(pg.q.rsplit('::', 1)[-1], calls[pg.q]) + text[i:]
        text = clean_visible(highlight_page(text))
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
        body.append(entity_list(items, 1) if items else '<p class="note">This header declares macros only, or only what other headers declare as well.</p>')
        body.append('</article>')
        text = set_title(head, f'<{h}>') + '\n'.join(body) + tail
        (out / 'headers' / f'{h}.html').write_text(clean_visible(fill(text, 1, h)), encoding='utf-8')

    # The index: headers grouped as the draft's clauses group them, and the namespaces.
    groups = collections.OrderedDict()
    for h in sorted(public):
        clause = (syn.get(h, '') or 'other').split('.')[0]
        groups.setdefault(h[0] if not syn.get(h) else clause, []).append(h)
    body = ['<article class="ref ref-index">',
            '<header class="ref-head"><p class="label">API reference · libycxx</p><h1 class="ref-name"><span class="ref-leaf">The C++26 standard library, as libycxx declares it</span></h1>',
            '<p class="lede">Every entity the <code>std</code> module exports, with its synopsis as libycxx declares it. '
            'Each page names the header, the section of the working draft '
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
        (out / 'global.html').write_text(fill(clean_visible(highlight_page(relink(g, g.text).replace('<!--ycxx:facts-->', ''))), 0), encoding='utf-8')

    # The sidebar's index and the search index.
    nav = {'headers': {}, 'namespaces': []}
    for h in sorted(public):
        nav['headers'][h] = [[display_q(pg.q), pg.rel, group_of(pg.label)[0]] for pg in sorted(by_header.get(h, []), key=lambda p: p.q.lower())
                             if '<' not in pg.q]
    nav['namespaces'] = [[pg.q, pg.rel] for pg in namespaces]
    (out / 'nav.json').write_text(json.dumps(nav, separators=(',', ':')), encoding='utf-8')
    search = [[display_q(pg.q), pg.label, pg.rel, pg.header or ''] for pg in sorted(canonical.values(), key=lambda p: (p.q.count('::'), len(p.q), p.q))
              if pg.q and pg.rel != 'index.html' and '<' not in pg.q]
    (out / 'search.json').write_text(json.dumps(search, separators=(',', ':')), encoding='utf-8')
    if cpp_map:
        (out / 'cppref-map.json').write_text(json.dumps({display_q(q): v for q, v in cpp_map.items()}, separators=(',', ':'), sort_keys=True), encoding='utf-8')
    tag = out / 'reference.tag.xml'
    if tag.exists():
        tag.unlink()
    print(f'render: {len(pages)} pages, {len(top)} namespace-scope entities, {len(public)} headers', flush=True)
    return pages
