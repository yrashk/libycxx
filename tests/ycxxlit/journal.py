"""A journal of finished tests, for runs stopped with Ctrl-C.

lit applies its workers' results in the order the tests were submitted, and on Ctrl-C it drops
those it has not applied yet: tests that did finish, out of order, then appear as SKIPPED in its
JSON report although they ran. Journaled(format) records every test's result as it finishes, one
JSON line per test in $YCXX_LIT_JOURNAL (tools/run-conformance sets it), with the fields of lit's
JSON report (name, code, elapsed, output); tools/lib/test_report.py merges the journal into the
report of an interrupted run.

Ctrl-C also reaches the commands of the tests that are running (they share the terminal's process
group; lit's workers ignore it): a test that fails because one of its commands was killed by
SIGINT did not fail, it was cut short, and is reported SKIPPED ("interrupted").
"""
import fcntl, json, os, re, signal, time

import lit.Test

KILLED_BY_SIGINT = re.compile(r'^\[\w+: exit %d,' % -signal.SIGINT, re.M)


class Journaled:
    def __init__(self, inner):
        self.inner = inner
        self.path = os.environ.get('YCXX_LIT_JOURNAL', '')

    def __getattr__(self, name):  # getTestsInDirectory and the rest: the format's own
        if name.startswith('__') or name == 'inner':  # (pickling, before inner is set)
            raise AttributeError(name)
        return getattr(self.inner, name)

    def execute(self, test, lit_config):
        t0 = time.monotonic()
        result = self.inner.execute(test, lit_config)
        if result.code.isFailure and KILLED_BY_SIGINT.search(result.output or ''):
            result = lit.Test.Result(lit.Test.SKIPPED, (result.output or '') +
                                     '\ninterrupted (Ctrl-C): a command of the test was stopped\n')
        if self.path:
            line = json.dumps({'name': test.getFullName(), 'code': result.code.name,
                               'elapsed': time.monotonic() - t0, 'output': result.output or ''}) + '\n'
            with open(self.path, 'a', encoding='utf-8') as f:
                fcntl.flock(f, fcntl.LOCK_EX)
                f.write(line)
        return result
