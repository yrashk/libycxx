"""The standard library modules std and std.compat for the lit formats.

A test that imports them (own suite: `// MODULES: std` or `// MODULES: std.compat`; libc++'s:
`// MODULE_DEPENDENCIES: std`) is compiled with `tools/ycxx-cxx <cc> --std-modules=<dir>`, whose
modules tools/ycxx-modules built with the test's own compile flags (a BMI is only valid with the
options it was built with). The modules are built once per compiler, set of flags and state of
the sources (modules/*.cppm, include/**), under a lock, in <exec root>/std-modules/<key>, and
reused by every test (and lit worker) that needs the same ones.

Before that, the compiler is asked to build a trivial module interface unit with the same flags:
if it cannot, the test is UNSUPPORTED with the compiler's reason (a compiler without C++20
modules). A compiler that builds modules but fails on std or std.compat is a FAIL of the test,
with the transcript of the module build.
"""
import fcntl, hashlib, os, shutil, tempfile
from ycxxlit import transcript

_fingerprint = None


def fingerprint(repo):
    """The state of the module sources and the headers they include (newest modification time and
    file count), so a changed header rebuilds the modules."""
    global _fingerprint
    if _fingerprint is None:
        newest, count = 0, 0
        for top in ('include', 'modules'):
            for d, _, files in os.walk(os.path.join(repo, top)):
                for f in files:
                    newest = max(newest, os.stat(os.path.join(d, f)).st_mtime_ns)
                    count += 1
        newest = max(newest, os.stat(os.path.join(repo, 'tools', 'ycxx-modules')).st_mtime_ns)
        _fingerprint = f'{newest}-{count}'
    return _fingerprint


def _locked(path):
    f = open(path, 'w')
    fcntl.flock(f, fcntl.LOCK_EX)
    return f


def probe(repo, compiler, flags, root):
    """None if `compiler` builds a module interface unit with `flags`; else the compiler's reason."""
    key = hashlib.sha1('\0'.join([compiler] + flags).encode()).hexdigest()[:16]
    done = os.path.join(root, f'probe-{key}')
    with _locked(done + '.lock'):
        if os.path.exists(done):
            reason = open(done).read()
            return reason or None
        tmp = tempfile.mkdtemp(prefix='probe.', dir=root)
        try:
            src = os.path.join(tmp, 'm.cppm')
            with open(src, 'w') as f:
                f.write('export module ycxx_probe;\nexport int ycxx_probe_f() { return 0; }\n')
            wrapper = os.path.join(repo, 'tools', 'ycxx-cxx')
            if compiler == 'clang':
                cmd = [wrapper, compiler] + flags + ['-fmodule-output=' + os.path.join(tmp, 'm.pcm'), '-c', src,
                                                     '-o', os.path.join(tmp, 'm.o')]
            else:
                cmd = [wrapper, compiler] + flags + ['-fmodules', '-c', src, '-o', os.path.join(tmp, 'm.o')]
            rc, out = transcript.run('module probe', cmd, tmp, 300)
            reason = '' if rc == 0 else ('the compiler cannot build a C++ module interface unit with these flags:\n'
                                         + out)
        finally:
            shutil.rmtree(tmp, ignore_errors=True)
        with open(done, 'w') as f:
            f.write(reason)
        return reason or None


def ensure(repo, compiler, flags, exec_root):
    """(status, directory or None, transcript): status 'ok', 'unsupported' or 'fail'."""
    root = os.path.join(exec_root, 'std-modules')
    os.makedirs(root, exist_ok=True)
    reason = probe(repo, compiler, flags, root)
    if reason:
        return 'unsupported', None, reason
    key = hashlib.sha1('\0'.join([compiler, fingerprint(repo)] + flags).encode()).hexdigest()[:16]
    d = os.path.join(root, key)
    with _locked(d + '.lock'):
        log = d + '.log'
        if os.path.exists(os.path.join(d, 'flags')):
            return 'ok', d, open(log).read() if os.path.exists(log) else ''
        if os.path.exists(log):  # built before, and failed: do not retry for every test
            return 'fail', None, open(log).read()
        rc, out = transcript.run('build std modules',
                                 [os.path.join(repo, 'tools', 'ycxx-modules'), compiler, '-o', d] + flags, root, 900)
        with open(log, 'w') as f:
            f.write(out)
        if rc != 0:
            shutil.rmtree(d, ignore_errors=True)
            return 'fail', None, out
        return 'ok', d, out
