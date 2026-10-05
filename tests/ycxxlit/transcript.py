"""Command transcripts for the lit formats of libycxx's test harness.

Every command a test runs (the compile, the program) is recorded in the test's output, whether
the test passes or not, as

    $ <the command, quoted for a shell>
    [<step>: exit <status>, <seconds>s]
    <what it printed>

so that a passing test shows exactly how it was run and what it produced (tools/run-conformance
keeps every test's output in its log and HTML report), and a failing one can be rerun by hand.
"""
import shlex, subprocess, time


def run(step, cmd, cwd, timeout, expect=''):
    """Runs cmd; returns (exit status, or None on timeout, transcript)."""
    t0 = time.monotonic()
    try:
        p = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, errors='replace', timeout=timeout)
    except subprocess.TimeoutExpired:
        return None, f'$ {shlex.join(cmd)}\n[{step}: TIMEOUT after {timeout}s{expect}]\n'
    secs = time.monotonic() - t0
    return p.returncode, f'$ {shlex.join(cmd)}\n[{step}: exit {p.returncode}, {secs:.2f}s{expect}]\n' + p.stdout + p.stderr
