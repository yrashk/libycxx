#!/usr/bin/env python3
"""Compares a suite run with its recorded baseline: the tests known not to pass.

    baseline.py <run>.tsv <baseline file> <candidate file>

The external suites (libc++'s, libstdc++'s) have tests that libycxx does not pass yet (their
TRIAGE.md files say why). A run with a baseline fails only on a test that does not pass and is
not in the baseline: a regression, or a new test. A test in the baseline that now passes is
listed, to be removed from the baseline. Tests the run did not include (a filtered run) are not
compared.

The candidate file gets the run's own list of tests that did not pass, in the baseline's
format: to record a baseline, or to update one, copy it over the baseline file.

Exit status: 0 when nothing regressed, 1 when a test outside the baseline did not pass (with no
baseline file every test that did not pass counts: a platform or configuration without a
recorded baseline is never green while tests fail), 2 on a usage error.

Baseline format: one test per line (its path in the suite, as the .tsv has it), sorted; blank
lines and lines starting with # are ignored.
"""
import os
import sys

OK = {'PASS', 'XFAIL', 'FLAKYPASS', 'UNSUPPORTED', 'SKIPPED', 'EXCLUDED'}


def read_run(tsv):
    results = {}
    with open(tsv, encoding='utf-8') as f:
        header = f.readline().rstrip('\n').split('\t')
        ri, ti = header.index('result'), header.index('test')
        for line in f:
            cols = line.rstrip('\n').split('\t')
            if len(cols) > max(ri, ti):
                results[cols[ti]] = cols[ri]
    return results


def read_baseline(path):
    with open(path, encoding='utf-8') as f:
        return {s for s in (line.strip() for line in f) if s and not s.startswith('#')}


def main(argv):
    if len(argv) != 4:
        print(__doc__, file=sys.stderr)
        return 2
    tsv, base_path, cand_path = argv[1:]
    results = read_run(tsv)
    failing = sorted(t for t, r in results.items() if r not in OK)
    with open(cand_path, 'w', encoding='utf-8') as f:
        f.write(f'# Tests that did not pass ({len(failing)} of {len(results)}); tools/lib/baseline.py\n')
        f.writelines(t + '\n' for t in failing)
    if os.path.exists(base_path):
        base = read_baseline(base_path)
    else:
        base = set()
        print(f'no baseline ({base_path}): every test that did not pass counts. To record the '
              f'known failures: cp {cand_path} {base_path}')
    new = [t for t in failing if t not in base]
    fixed = sorted(t for t in base if t in results and results[t] in OK)
    print(f'baseline {base_path}: {len(base)} known; this run: {len(failing)} did not pass, '
          f'{len(new)} outside the baseline, {len(fixed)} of the baseline now pass')
    for t in new:
        print(f'  NEW  {results[t]:<11} {t}')
    for t in fixed:
        print(f'  FIXED            {t}  (remove it from the baseline)')
    return 1 if new else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
