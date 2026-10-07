#!/usr/bin/env python3
"""The inventory of declared entities of draft clauses (tools/spec_audit, docs/SPEC_COVERAGE.md).

    tools/spec_audit/inventory.py BLOCKS.json [--clause NAME...] [--tsv OUT] [--summary]

BLOCKS.json is extract_draft.py's output. Every declaration of a synopsis or class definition is
one entity (a class, a member, a free function overload, a variable or variable template, a
concept, a CPO, a deduction guide, a specialization, a macro); item declarations of the
subclauses that their synopsis does not show (members declared only in an itemdecl) are added.
--tsv writes one line per entity: subclause, namespace, class, kind, name, declaration, comment.
"""
import argparse, collections, json, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import decls

SKIP_KINDS = {'access', 'empty', 'static_assert', 'using-directive', 'other'}


def entities(blocks, clauses=None):
    out = []
    for b in blocks:
        if clauses and b['clause'] not in clauses:
            continue
        if b['kind'] != 'code':
            continue
        for d in decls.declarations(b['text']):
            if d.kind in SKIP_KINDS:
                continue
            if not d.ns and d.kind != 'pp':
                continue    # the code of a requirements table or of a description, not a declaration
            # exposition-only declarations: a name in italics, or a comment saying so
            if 'exposition only' in d.comment or (d.name and d.name.startswith('@')):
                continue
            body = decls.strip_template_heads(d.toks)
            if d.kind not in ('pp',) and _expo_name(d):
                continue
            out.append((b['clause'], b['sec'], d))
    return out


def _expo_name(d):
    """True when the declared name is exposition-only (an italic token where the name is)."""
    if d.kind in ('class', 'enum', 'alias', 'concept', 'variable', 'function', 'friend-function'):
        if not d.name:
            return True
        # classify() returns ids only; italic names come out empty or as a neighbour
        toks = decls.strip_template_heads(d.toks)
        for i, t in enumerate(toks):
            if t.kind == 'expo' and i + 1 < len(toks) and toks[i + 1].text in ('(', '=', ';', '{') and d.kind != 'alias':
                if t.text.strip() not in ('see below', 'unspecified', 'implementation-defined'):
                    return True
            if d.kind in ('class', 'concept', 'alias') and i in (1,) and t.kind == 'expo':
                return True
    return False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('blocks')
    ap.add_argument('--clause', nargs='*')
    ap.add_argument('--tsv')
    ap.add_argument('--summary', action='store_true')
    a = ap.parse_args()
    blocks = json.load(open(a.blocks, encoding='utf-8'))
    ents = entities(blocks, a.clause)
    if a.tsv:
        with open(a.tsv, 'w', encoding='utf-8') as o:
            for clause, sec, d in ents:
                o.write('\t'.join([sec, d.ns, '::'.join(d.cls), d.kind, d.name, d.text.replace('\t', ' ').replace('\n', ' '),
                                   d.comment.strip()]) + '\n')
    if a.summary:
        c = collections.Counter((clause, d.kind) for clause, sec, d in ents)
        for k, v in sorted(c.items()):
            print(k, v)
        print('total', len(ents))


if __name__ == '__main__':
    main()
