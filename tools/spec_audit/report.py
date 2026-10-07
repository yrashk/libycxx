#!/usr/bin/env python3
"""Per-subclause tables of the spec-coverage audit (docs/SPEC_COVERAGE.md).

    tools/spec_audit/report.py --part part2 [--results build/spec_audit/part2-results.tsv]

Reads the part's inventory (tools/spec_audit/<part>/entities.tsv), the failures of the last run of
run_probes.py over both compilers (build/spec_audit/<part>-results.tsv: one line per failed check
and compiler) and the known gaps (tools/spec_audit/<part>/gaps.tsv), and prints a Markdown table
per clause: for each subclause, the entities declared; those with a presence check (declared, in
its header, freestanding, callable or constructible, the macro defined) and how many pass on both
compilers; those with a shape check (member type, return type, noexcept, implicit or explicit,
deleted, deduction guide, specialization value, defaulted arguments) and how many pass; the gaps
by class (1-7, docs/SPEC_COVERAGE.md): open ones (gaps.tsv) and those the audit found and fixed
or found documented (found.tsv); and the probe files.
"""
import argparse, collections, os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
PRESENCE = {'name', 'call', 'header', 'include', 'spec', 'macro', 'freestanding'}


def load(a):
    ents = []
    with open(os.path.join(HERE, a.part, 'entities.tsv'), encoding='utf-8') as f:
        for line in f:
            if not line.startswith('#'):
                ents.append((line.rstrip('\n').split('\t') + [''] * 9)[:9])
    failed = collections.defaultdict(set)
    res = a.results or os.path.join(ROOT, 'build', 'spec_audit', f'{a.part}-results.tsv')
    if os.path.exists(res):
        with open(res, encoding='utf-8') as f:
            for line in f:
                c = line.rstrip('\n').split('\t')
                if c[4] in ('FAIL', 'XFAIL'):
                    failed[c[2]].add(c[3])
    gaps = collections.defaultdict(set)
    gp = os.path.join(HERE, a.part, 'gaps.tsv')
    if os.path.exists(gp):
        with open(gp, encoding='utf-8') as f:
            for line in f:
                if line.startswith('#') or not line.strip():
                    continue
                c = line.rstrip('\n').split('\t')
                gaps[c[0]].add(c[3])
    return ents, failed, gaps


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--part', required=True)
    ap.add_argument('--results')
    a = ap.parse_args()
    ents, failed, gaps = load(a)
    probes = sorted(os.listdir(os.path.join(HERE, a.part, 'probes')))
    rows = collections.OrderedDict()
    for i, sec, kind, name, decl, comment, checks, note, clause in ents:
        if kind == 'macro':
            sec, clause = 'version.syn', 'version.syn'
        r = rows.setdefault((clause, sec), collections.Counter())
        r['n'] += 1
        asp = set(x for x in checks.split(',') if x)
        bad = failed.get(i, set())
        if asp:
            r['pchk'] += 1
            r['pok'] += not (bad & PRESENCE)
        if asp - PRESENCE:
            r['schk'] += 1
            r['sok'] += not (bad & (asp - PRESENCE))
        for g in gaps.get(i, ()):
            r['gap' + g] += 1
    # the gaps the audit found, fixed or not (found.tsv: row subclause, class, status, reference, what)
    found = collections.defaultdict(collections.Counter)
    fp = os.path.join(HERE, a.part, 'found.tsv')
    if os.path.exists(fp):
        with open(fp, encoding='utf-8') as f:
            for line in f:
                if line.startswith('#') or not line.strip():
                    continue
                c = line.rstrip('\n').split('\t')
                found[c[0]][(c[1], c[2])] += 1
    tot = collections.Counter()
    by_clause = collections.OrderedDict()
    for (clause, sec), r in rows.items():
        by_clause.setdefault(clause, []).append((sec, r))
    for clause, secs in by_clause.items():
        ctot = collections.Counter()
        print(f'#### [{clause}]' + (' (feature-test macros of these headers)' if clause == 'version.syn' else ''))
        print()
        print('| Subclause | Declared | Presence checked / pass | Shape checked / pass | Gaps by class | Probes |')
        print('|---|---|---|---|---|---|')
        for sec, r in secs:
            if sec == 'version.syn':
                links = '`version.*.cpp` ({})'.format(sum(p.startswith('version.') for p in probes))
            else:
                ps = [p for p in probes if p.split('.cpp')[0].replace('.header', '').replace('.freestanding', '') == sec]
                links = ', '.join(f'[{p}](../tools/spec_audit/{a.part}/probes/{p})' for p in ps)
            g = [f'({k[3:]}) {v} open' for k, v in sorted(r.items()) if k.startswith('gap')]
            g += [f'({c}) {v} {st}' for (c, st), v in sorted(found.get(sec, {}).items())]
            g = ', '.join(g) or '-'
            print(f"| [{sec}] | {r['n']} | {r['pchk']} / {r['pok']} | {r['schk']} / {r['sok']} | {g} | {links} |")
            ctot.update(r)
        print(f"| **total** | {ctot['n']} | {ctot['pchk']} / {ctot['pok']} | {ctot['schk']} / {ctot['sok']} | | |")
        print()
        tot.update(ctot)
    print(f"All: {tot['n']} entities declared; presence checked {tot['pchk']}, passing {tot['pok']}; "
          f"shape checked {tot['schk']}, passing {tot['sok']}.")


if __name__ == '__main__':
    main()
