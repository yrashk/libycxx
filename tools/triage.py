#!/usr/bin/env python3
"""Group conformance failures by cause.

  tools/triage.py libcxx|libstdcxx gcc|clang [dirs...]

Runs tools/run-conformance with lit's JSON output and prints failures grouped as
'missing header <x>' or by their first error message (normalised), most common first.
"""
import collections, json, os, re, subprocess, sys, tempfile

repo = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
suite, compiler, dirs = sys.argv[1], sys.argv[2], sys.argv[3:]
out = tempfile.mktemp(suffix='.json')
subprocess.run([os.path.join(repo, 'tools', 'run-conformance'), suite, compiler] + dirs +
               ['--', '-q', '-o', out], capture_output=True)
data = json.load(open(out))
groups = collections.defaultdict(list)
counts = collections.Counter()
for t in data['tests']:
    counts[t['code']] += 1
    if t['code'] != 'FAIL':
        continue
    name = t['name'].split(' :: ', 1)[1]
    text = t.get('output', '')
    m = re.search(r"fatal error: '?([\w./]+)'?:? (?:file not found|No such file or directory)", text)
    if m:
        key = f'missing header <{m.group(1)}>'
    else:
        m = re.search(r'error: (.*)', text)
        if m:
            key = re.sub(r"'[^']*'|‘[^’]*’|\d+", '_', m.group(1))[:110]
        elif 'exit ' in text:
            key = 'runtime failure: ' + text.split('\n')[0]
        else:
            key = text.strip().split('\n')[0][:110] or 'unknown'
    groups[key].append(name)
print(dict(counts))
for key, names in sorted(groups.items(), key=lambda kv: -len(kv[1])):
    print(f'{len(names):5d}  {key}')
    for n in names[:3]:
        print(f'         {n}')
