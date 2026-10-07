#!/usr/bin/env python3
"""Regenerate an audit part's probes from the current working draft (docs/SPEC_COVERAGE.md).

    tools/spec_audit/regen.py --part part2 [--offline]

Downloads the part's clauses and [version.syn] from https://eel.is/c++draft/ into
build/spec_audit/draft/ (--offline: reuse what is there), extracts their declarations
(extract_draft.py) and writes the probes and the inventory (gen_probes.py). Then run
tools/spec_audit/run_probes.py --part <part> and review what changed with git diff.
"""
import argparse, importlib, os, subprocess, sys, urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--part', required=True)
    ap.add_argument('--offline', action='store_true')
    a = ap.parse_args()
    sys.path.insert(0, HERE)
    cfg = importlib.import_module(a.part + '_config')
    d = os.path.join(ROOT, 'build', 'spec_audit', 'draft')
    os.makedirs(d, exist_ok=True)
    pages = list(cfg.CLAUSES) + ['version.syn']
    for p in pages:
        path = os.path.join(d, p + '.html')
        if a.offline and os.path.exists(path):
            continue
        with urllib.request.urlopen('https://eel.is/c++draft/' + p) as r, open(path, 'wb') as o:
            o.write(r.read())
    blocks = os.path.join(d, a.part + '-blocks.json')
    version = os.path.join(d, 'version.json')
    py = sys.executable
    subprocess.run([py, os.path.join(HERE, 'extract_draft.py'), blocks] +
                   [os.path.join(d, c + '.html') for c in cfg.CLAUSES], check=True)
    subprocess.run([py, os.path.join(HERE, 'extract_draft.py'), version, os.path.join(d, 'version.syn.html')], check=True)
    subprocess.run([py, os.path.join(HERE, 'gen_probes.py'), '--part', a.part, blocks, '--version-json', version], check=True)


if __name__ == '__main__':
    main()
