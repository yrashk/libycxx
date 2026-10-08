"""Which header declares each library entity, and in which subclause of the draft
(tools/gen-apidocs --refresh-draft writes tools/apidocs/draft-map.json).

Read from the working draft at https://eel.is/c++draft (pages cached in build/apidocs/cache):
  - the index of headers: each header's synopsis subclause, the header's bold entry ([vector.syn]);
  - each header's synopsis page, split into declarations by tools/spec_audit (the same parser the
    spec-coverage audit uses): the names a header declares;
  - the library clauses ([support] … [exec]): the subclause that defines each class and member
    ([vector.overview], [vector.modifiers]);
  - the index of library names: the subclause of a name the clauses do not define in a
    declaration of their own (an algorithm's [alg.sort]).
The map is committed so that generating the reference needs no network; refresh it when the
draft moves.
"""
import collections, json, pathlib, re, sys, time, urllib.error, urllib.request

DRAFT = 'https://eel.is/c++draft/'
CLAUSES = ['support', 'concepts', 'diagnostics', 'mem', 'meta', 'utilities', 'containers', 'iterators',
           'ranges', 'algorithms', 'strings', 'text', 'numerics', 'time', 'input.output', 'thread', 'exec']
SYNOPSES = re.compile(r'\.(syn|synop|synopsis)$')


def _get(cache, name):
    f = cache / (name + '.html')
    if not f.exists():
        cache.mkdir(parents=True, exist_ok=True)
        for attempt in range(3):
            try:
                with urllib.request.urlopen(DRAFT + name, timeout=120) as r:
                    data = r.read()
                break
            except urllib.error.HTTPError as e:
                if e.code == 404:
                    print(f'draftmap: {name}: not in the draft', file=sys.stderr)
                    return None
                if attempt == 2:
                    raise
                time.sleep(3)
            except OSError as e:
                if attempt == 2:
                    raise
                print(f'draftmap: {name}: {e}; retrying', file=sys.stderr)
                time.sleep(3)
        f.write_bytes(data)
    return f


def _clean(s):
    return s.replace("<span class='shy'></span>", '')


def _entities(spec_audit, page):
    """(qualified name, kind) of each declaration of a draft page, exposition-only ones excluded."""
    if page is None:
        return []
    sys.path.insert(0, str(spec_audit))
    import extract_draft, inventory
    p = extract_draft._Parser()
    p.feed(page.read_text(encoding='utf-8').replace('\xad', '').replace('​', ''))
    for r in p.out:
        r['clause'] = page.stem
    out = []
    if page is None:
        return out
    for clause, sec, d in inventory.entities(p.out):
        if not d.name or d.kind in ('pp', 'using-decl') or '<' in ''.join(d.cls) or '@' in d.name:
            continue
        q = '::'.join([d.ns] + list(d.cls) + [d.name])
        out.append((sec, q, d.kind))
    return out


def refresh(cache, out, repo):
    spec_audit = repo / 'tools' / 'spec_audit'
    hidx = _clean(_get(cache, 'headerindex').read_text())
    headers = {}
    for sec, h in re.findall(r"<a href='([\w.]+)#header:%3c([^%]+)%3e'><b\s*>", hidx):
        if h.endswith('.h') or h in headers and SYNOPSES.search(headers[h]):
            continue
        headers[h] = sec

    # Header -> the names its synopsis declares.
    declared_in = collections.defaultdict(list)
    for h, sec in sorted(headers.items()):
        ents = _entities(spec_audit, _get(cache, sec))
        if not ents and '.' in sec:
            parent = sec.rsplit('.', 1)[0]
            ents = _entities(spec_audit, _get(cache, parent))
        for _, q, _ in ents:
            if h not in declared_in[q]:
                declared_in[q].append(h)

    # Entity -> the subclause that declares it outside the synopses (its class definition, or
    # the item that specifies it).
    defined = {}
    for c in CLAUSES:
        for sec, q, kind in _entities(spec_audit, _get(cache, c)):
            if '.' not in sec or SYNOPSES.search(sec):
                continue
            if q not in defined or kind in ('class', 'concept', 'enum'):
                defined.setdefault(q, sec)
                if kind in ('class', 'concept', 'enum'):
                    defined[q] = sec

    # The index of library names: name (or "member,class") -> its subclauses.
    lidx = _clean(_get(cache, 'libraryindex').read_text())
    index = collections.defaultdict(list)
    for key, body in re.findall(r"<div id='lib:([^']+)'><div class='indexitems'><span class='texttt'>[^<]*</span>((?:, <a href='[\w.]+#[^']*'>(?:<b\s*>)?\[[^\]]*\](?:</b>)?</a>)+)", lidx):
        for sec in re.findall(r"<a href='([\w.]+)#", body):
            if sec not in index[key]:
                index[key].append(sec)

    def from_index(key, hs):
        """The index entry that best fits: not a synopsis, near the header's synopsis."""
        cands = index.get(key, [])
        near = {headers[h].split('.')[0] for h in hs if h in headers}
        best = sorted(cands, key=lambda c: (bool(SYNOPSES.search(c)), c.split('.')[0] not in near))
        return best[0] if best else None

    ents = {}
    for q in sorted(set(declared_in) | set(defined)):
        parts = q.split('::')
        sec = defined.get(q)
        if not sec:
            name = parts[-1]
            key = f'{name},{parts[-2]}' if len(parts) > 2 and parts[-2] not in ('std', 'ranges', 'chrono', 'views', 'filesystem', 'pmr', 'execution', 'this_thread', 'linalg', 'simd', 'meta', 'numbers', 'regex_constants', 'placeholders') else name
            sec = from_index(key, declared_in.get(q, [])) or from_index(name, declared_in.get(q, []))
        hs = declared_in.get(q, [])
        if not sec and hs:
            sec = headers[hs[0]]
        ents[q] = [sec or '', hs]
    revision = re.search(r'github\.com/Eelis/draft/(?:tree|commit|blob)/([0-9a-f]{40})', _get(cache, 'support').read_text())
    revision = revision.group(1) if revision else 'unknown'
    data = {
        'source': DRAFT, 'revision': revision,
        'note': 'Generated by tools/gen-apidocs --refresh-draft; do not edit.',
        'headers': dict(sorted(headers.items())),
        'entities': ents,
    }
    out.write_text(json.dumps(data, indent=0, sort_keys=False, ensure_ascii=False) + '\n')
    print(f'draftmap: {len(headers)} headers, {len(ents)} entities, revision {revision}', file=sys.stderr)
