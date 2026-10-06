"""lit format for libycxx's own test suite (tests/ycxx).

  *.pass.cpp           compile, link, run; pass iff exit status 0
  *.compile.pass.cpp   must compile (-fsyntax-only)
  *.compile.fail.cpp   must fail to compile, for a reason other than a missing header

Optional directives:
  // FLAGS: <extra compiler flags>
  // FILES: <extra translation units, paths relative to the test file>  (*.pass.cpp only: compiled
                                          and linked into the test program)
  // ARCHIVE: <translation units>   (*.pass.cpp only) compiled to objects, put into a static
                                          archive (ar) and linked after the test's own objects
  // SHARED: <translation units>    (*.pass.cpp only) compiled and linked (-shared -fPIC, through
                                          the same wrapper, so with its own copy of a static
                                          library) into a shared library the program links to
  // UNSUPPORTED-SANITIZER: asan|ubsan|tsan[,...]  <reason>   not run under those sanitizers: for
                                          tests of what a sanitizer runtime replaces (asan's global
                                          allocation functions) or adds (its exported symbols)
  // XFAIL: gcc|clang|any  <reason>        an expected failure, for a reason outside the test's
                                          control that STATUS.md lists: a compiler bug, an ABI
                                          limit, a draft defect, a feature not implemented yet. The
                                          test is unchanged and reports XFAIL with the reason, or
                                          XPASS (a failure of the run) once it passes.
                                          (XFAIL-COMPILER: gcc|clang is the older spelling.) A
                                          suffix -linux or -darwin (`clang-darwin`) limits it to
                                          that OS (the OS feature of REQUIRES)
  // COUNTERPART: libcxx:<path> libstdcxx:<path> [...]   the external tests, skipped there as
                                          tied to that library's internals, extensions or modes,
                                          whose standard subject this test covers (paths relative
                                          to libcxx/test/std and to the libstdc++ testsuite;
                                          anchored regexes, so `libstdcxx:23_containers/x/.*`
                                          links a directory). Not read by this format: the
                                          external formats report the link (ycxxlit/counterparts.py)
  // EXPECT-ERROR: <Python regex>   (*.compile.fail.cpp only; repeatable) the compiler's output
                                          (its diagnostics, not the command line) must match
                                          every such regex (re.search, multi-line), or the test
                                          fails and names the regexes that did not match: the
                                          test fails to compile for the reason it is about
  // EXPECT-ERROR-GCC: <regex>, // EXPECT-ERROR-CLANG: <regex>   the same, for one compiler only
                                          (where the wording differs: GCC's "use of deleted
                                          function" is Clang's "call to deleted ...")
  // REQUIRES: <features>   (repeatable) run only when the boolean expression of lit features
                                          (&&, ||, !, parentheses; a comma is &&) holds, else
                                          UNSUPPORTED. Features: the compiler (gcc, clang), the
                                          OS (linux, darwin), the sanitizers (asan, ubsan, tsan),
                                          hardened (lit param hardened=1: -DYCXX_HARDENED=1),
                                          exceptions and rtti (unless the run's cxxflags have
                                          -fno-exceptions / -fno-rtti)
  // EXPECT-TERMINATE[: <regex>]   (*.pass.cpp only) the program must end abnormally, killed by
                                          SIGABRT, SIGTRAP or SIGILL (what abort() and
                                          __builtin_trap() raise: a hardened precondition's
                                          contract violation), not by a normal exit nor another
                                          signal; the optional regex must match its output
"""
import os, re, shutil, signal, tempfile
import lit.formats, lit.Test
from lit.BooleanExpression import BooleanExpression
from ycxxlit import transcript

FLAGS = re.compile(r'^//\s*FLAGS:(.*)$', re.M)
FILES = re.compile(r'^//\s*FILES:(.*)$', re.M)
ARCHIVE = re.compile(r'^//\s*ARCHIVE:(.*)$', re.M)
SHARED = re.compile(r'^//\s*SHARED:(.*)$', re.M)
XFAIL = re.compile(r'^//\s*XFAIL(?:-COMPILER)?:\s*(gcc|clang|any)(?:-(linux|darwin))?\b(.*)$', re.M)
UNSUPPORTED_SAN = re.compile(r'^//\s*UNSUPPORTED-SANITIZER:\s*([\w,]+)(.*)$', re.M)
EXPECT_ERROR = re.compile(r'^//\s*EXPECT-ERROR(?:-(GCC|CLANG))?:\s*(.*?)\s*$', re.M)
REQUIRES = re.compile(r'^//\s*REQUIRES:(.*)$', re.M)
EXPECT_TERMINATE = re.compile(r'^//\s*EXPECT-TERMINATE(?::\s*(.*?))?\s*$', re.M)
TERMINATING_SIGNALS = {signal.SIGABRT, signal.SIGTRAP, signal.SIGILL}


def signal_name(n):
    try:
        return signal.Signals(n).name
    except ValueError:
        return f'signal {n}'
MISSING = re.compile(r"fatal error: '?[\w./]+'?:? (file not found|No such file or directory)")


class YcxxFormat(lit.formats.FileBasedTest):
    def __init__(self, wrapper, compiler, base_flags, sanitizers=(), features=()):
        self.wrapper, self.compiler, self.base_flags = wrapper, compiler, base_flags
        self.sanitizers = set(sanitizers)
        self.features = set(features)

    def compile(self, args, cwd, expect=''):
        return transcript.run('compile', [self.wrapper, self.compiler] + args, cwd, 300, expect)

    def execute(self, test, lit_config):
        src = open(test.getSourcePath(), encoding='utf-8').read()
        for m in UNSUPPORTED_SAN.finditer(src):
            if self.sanitizers & set(m.group(1).split(',')):
                return lit.Test.Result(lit.Test.UNSUPPORTED,
                                       f'not run with -fsanitize ({m.group(1)}):{m.group(2)}')
        for m in REQUIRES.finditer(src):
            expr = ' && '.join(f'({e.strip()})' for e in m.group(1).split(','))
            try:
                ok = BooleanExpression.evaluate(expr, self.features)
            except ValueError as e:
                return lit.Test.Result(lit.Test.FAIL, f'REQUIRES:{m.group(1)}: {e}')
            if not ok:
                return lit.Test.Result(lit.Test.UNSUPPORTED, f'REQUIRES:{m.group(1)} (features of this run: '
                                       f'{", ".join(sorted(self.features))})')
        result = self.run(test)
        xf = [m for m in XFAIL.finditer(src)
              if m.group(1) in (self.compiler, 'any') and m.group(2) in (None, *self.features)]
        if xf:
            why = '; '.join(m.group(3).strip() for m in xf)
            if result.code == lit.Test.PASS:
                result.code = lit.Test.XPASS
                result.output = (result.output or '') + f'\nexpected to fail ({why}), but passed\n'
            elif result.code == lit.Test.FAIL:
                result.code = lit.Test.XFAIL
                result.output = (result.output or '') + f'\nexpected failure: {why}\n'
        return result

    def check_expected_errors(self, src, out):
        """A failed compile passes when its diagnostics match every EXPECT-ERROR regex that applies
        to this compiler. Matched against what the compiler printed: the transcript's first two
        lines (the command and its exit status) are left out, since the command names the test."""
        expected = [m.group(2) for m in EXPECT_ERROR.finditer(src)
                    if m.group(1) in (None, self.compiler.upper())]
        diagnostics = out.split('\n', 2)[2] if out.count('\n') >= 2 else ''
        missed = []
        for rx in expected:
            try:
                if not re.search(rx, diagnostics, re.M):
                    missed.append(rx)
            except re.error as e:
                return lit.Test.Result(lit.Test.FAIL, f'EXPECT-ERROR: invalid regex {rx!r}: {e}\n' + out)
        if missed:
            return lit.Test.Result(lit.Test.FAIL, 'failed to compile, but not with the expected diagnostics; '
                                   'no match for:\n' + ''.join(f'  EXPECT-ERROR: {rx}\n' for rx in missed) + out)
        note = ''.join(f'[matched EXPECT-ERROR: {rx}]\n' for rx in expected)
        return lit.Test.Result(lit.Test.PASS, note + out)

    def check_terminated(self, rc, rx, out, ran):
        """EXPECT-TERMINATE: the program was killed by one of TERMINATING_SIGNALS (subprocess reports
        death by signal N as -N), and printed what the optional regex asks for."""
        if rc is None or rc >= 0 or -rc not in TERMINATING_SIGNALS:
            how = ('timed out' if rc is None else f'exited with status {rc}' if rc >= 0
                   else f'was killed by {signal_name(-rc)}')
            return lit.Test.Result(lit.Test.FAIL, 'expected abnormal termination (SIGABRT, SIGTRAP or SIGILL); '
                                   f'the program {how}\n' + out)
        printed = ran.split('\n', 2)[2] if ran.count('\n') >= 2 else ''
        if rx and not re.search(rx, printed, re.M):
            return lit.Test.Result(lit.Test.FAIL, 'terminated, but its output does not match '
                                   f'EXPECT-TERMINATE: {rx}\n' + out)
        return lit.Test.Result(lit.Test.PASS, f'[terminated by {signal_name(-rc)}, as expected]\n' + out)

    def run(self, test):
        path = test.getSourcePath()
        name = os.path.basename(path)
        src = open(path, encoding='utf-8').read()
        flags = list(self.base_flags)
        for m in FLAGS.finditer(src):
            flags += m.group(1).split()
        exec_dir = os.path.join(test.suite.exec_root, *test.path_in_suite[:-1])
        os.makedirs(exec_dir, exist_ok=True)
        if EXPECT_ERROR.search(src) and not name.endswith('.compile.fail.cpp'):
            return lit.Test.Result(lit.Test.FAIL, 'EXPECT-ERROR applies only to *.compile.fail.cpp tests')
        terminate = EXPECT_TERMINATE.search(src)
        if terminate and (not name.endswith('.pass.cpp') or name.endswith('.compile.pass.cpp')):
            return lit.Test.Result(lit.Test.FAIL, 'EXPECT-TERMINATE applies only to *.pass.cpp tests')
        tmp = tempfile.mkdtemp(prefix=name + '.', dir=exec_dir)
        try:
            if name.endswith('.compile.pass.cpp'):
                rc, out = self.compile(['-fsyntax-only', path] + flags, tmp)
                return lit.Test.Result(lit.Test.PASS if rc == 0 else lit.Test.FAIL, out)
            if name.endswith('.compile.fail.cpp'):
                rc, out = self.compile(['-fsyntax-only', path] + flags, tmp, '; must fail')
                if rc != 0 and MISSING.search(out):
                    return lit.Test.Result(lit.Test.FAIL, 'failed only because a header is missing\n' + out)
                if rc in (0, None):
                    return lit.Test.Result(lit.Test.FAIL, out or 'expected a compile error')
                return self.check_expected_errors(src, out)
            if name.endswith('.pass.cpp'):
                exe = os.path.join(tmp, 't.exe')
                def listed(rx):
                    return [os.path.normpath(os.path.join(os.path.dirname(path), f))
                            for m in rx.finditer(src) for f in m.group(1).split()]
                extra, out = listed(FILES), ''
                archived, shared = listed(ARCHIVE), listed(SHARED)
                if archived:
                    objs = []
                    for i, f in enumerate(archived):
                        obj = os.path.join(tmp, f'a{i}.o')
                        rc, o = self.compile(['-c', f, '-o', obj] + flags, tmp)
                        out += o
                        if rc != 0:
                            return lit.Test.Result(lit.Test.FAIL, 'COMPILE FAILED\n' + out)
                        objs.append(obj)
                    lib = os.path.join(tmp, 'libtest.a')
                    rc, o = transcript.run('archive', ['ar', 'rcs', lib] + objs, tmp, 60)
                    out += o
                    if rc != 0:
                        return lit.Test.Result(lit.Test.FAIL, 'ARCHIVE FAILED\n' + out)
                    extra.append(lib)
                if shared:
                    so = os.path.join(tmp, 'libtestshared.so')
                    rc, o = self.compile(['-shared', '-fPIC'] + shared + ['-o', so] + flags, tmp)
                    out += o
                    if rc != 0:
                        return lit.Test.Result(lit.Test.FAIL, 'COMPILE FAILED\n' + out)
                    extra += [so, '-Wl,-rpath,' + tmp]
                rc, o = self.compile([path] + extra + ['-o', exe] + flags, tmp)
                out += o
                if rc != 0:
                    return lit.Test.Result(lit.Test.FAIL, 'COMPILE FAILED\n' + out)
                rc, ran = transcript.run('run', [exe], tmp, 60, '; must terminate' if terminate else '')
                if terminate:
                    return self.check_terminated(rc, terminate.group(1), out + ran, ran)
                return lit.Test.Result(lit.Test.PASS if rc == 0 else lit.Test.FAIL, out + ran)
            return lit.Test.Result(lit.Test.UNSUPPORTED, 'not a test file')
        finally:
            shutil.rmtree(tmp, ignore_errors=True)
