"""Exercise the performance gate with deterministic measurements in a real git worktree.

    python3 -m unittest discover -s bench -p 'test_*.py'
"""
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


CHECK = Path(__file__).with_name('check')
FAKE_RUN = '''import argparse, json
from pathlib import Path
p = argparse.ArgumentParser()
p.add_argument('--json')
a, _ = p.parse_known_args()
root = Path(__file__).resolve().parent.parent
ratio = float((root / 'ratio').read_text())
Path(a.json).write_text(json.dumps({'date': 'today', 'results': [
    {'cc': 'clang', 'prog': 'runtime', 'name': 'handoff',
     'ns': {'ycxx': ratio, 'libstdc++': 1}}]}))
'''


class ReferenceBaselineTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        (self.root / 'bench').mkdir()
        shutil.copyfile(CHECK, self.root / 'bench/check')
        (self.root / 'bench/run').write_text(FAKE_RUN)
        (self.root / 'ratio').write_text('1')
        self.git('init', '-q')
        self.git('add', '.')
        self.git('-c', 'user.name=Test', '-c', 'user.email=test@example.com',
                 'commit', '-qm', 'baseline')
        self.ref = self.git('rev-parse', 'HEAD').stdout.strip()
        (self.root / 'bench/baseline.json').write_text(json.dumps({
            'reference_commit': self.ref, 'ratios': {'clang: runtime: handoff': 0.1}}))

    def git(self, *args):
        return subprocess.run(['git', '-C', str(self.root), *args],
                              capture_output=True, text=True, check=True)

    def check(self, *args):
        return subprocess.run([sys.executable, str(self.root / 'bench/check'),
                               '--cc', 'clang', *args], capture_output=True, text=True)

    def assert_clean_worktrees(self):
        self.assertEqual(self.git('worktree', 'list', '--porcelain').stdout.count('worktree '), 1)

    def test_unchanged_code_passes_despite_other_machines_stored_ratio(self):
        self.assertEqual(self.check().returncode, 1)  # stored 0.1 is inappropriate here
        result = self.check('--reference-baseline')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        measured = json.loads((self.root / 'build/bench/check.baseline.json').read_text())
        self.assertEqual(measured['results'][0]['ns']['ycxx'], 1)
        self.assert_clean_worktrees()

    def test_regression_still_fails_every_confirmation(self):
        (self.root / 'ratio').write_text('2')
        result = self.check('--reference-baseline')
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn('1 suspect(s), 1 confirmed', result.stdout)
        self.assertIn('confirmations 2.00 2.00', result.stdout)
        self.assert_clean_worktrees()

    def test_noise_in_initial_reference_does_not_confirm_regression(self):
        counter = self.root / 'reference_calls'
        noisy = FAKE_RUN.replace('Path(a.json).write_text', f'''
if (root / '.git').is_file():
    counter = Path({str(counter)!r})
    calls = int(counter.read_text()) if counter.exists() else 0
    counter.write_text(str(calls + 1))
    if calls == 0:
        ratio = 0.1
Path(a.json).write_text''')
        (self.root / 'bench/run').write_text(noisy)
        self.git('add', 'bench/run')
        self.git('-c', 'user.name=Test', '-c', 'user.email=test@example.com',
                 'commit', '-qm', 'noisy reference')
        base = self.root / 'bench/baseline.json'
        doc = json.loads(base.read_text())
        doc['reference_commit'] = self.git('rev-parse', 'HEAD').stdout.strip()
        base.write_text(json.dumps(doc))
        result = self.check('--reference-baseline')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn('1 suspect(s), 0 confirmed', result.stdout)
        self.assertEqual(counter.read_text(), '3')
        self.assert_clean_worktrees()

    def test_failed_reference_measurement_removes_worktree(self):
        (self.root / 'bench/run').write_text('raise SystemExit(7)')
        self.git('add', 'bench/run')
        self.git('-c', 'user.name=Test', '-c', 'user.email=test@example.com',
                 'commit', '-qm', 'broken reference')
        base = self.root / 'bench/baseline.json'
        doc = json.loads(base.read_text())
        doc['reference_commit'] = self.git('rev-parse', 'HEAD').stdout.strip()
        base.write_text(json.dumps(doc))
        result = self.check('--reference-baseline')
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('bench/run failed (7)', result.stderr)
        self.assert_clean_worktrees()

    def test_update_pins_the_measured_revision(self):
        measurement = self.root / 'measured.json'
        measurement.write_text(json.dumps({'commit': self.ref, 'results': [
            {'cc': 'clang', 'prog': 'runtime', 'name': 'handoff',
             'ns': {'ycxx': 2, 'libstdc++': 1}}]}))
        result = self.check('--update', '--from', str(measurement))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        base = json.loads((self.root / 'bench/baseline.json').read_text())
        self.assertEqual(base['reference_commit'], self.ref)
        self.assertEqual(base['ratios']['clang: runtime: handoff'], 2)

    def test_update_without_revision_preserves_baseline(self):
        base = self.root / 'bench/baseline.json'
        before = base.read_text()
        result = self.check('--update')  # this measurement deliberately omits a commit
        self.assertEqual(result.returncode, 2)
        self.assertIn('measurement lacks a commit ID', result.stderr)
        self.assertEqual(base.read_text(), before)


if __name__ == '__main__':
    unittest.main()
