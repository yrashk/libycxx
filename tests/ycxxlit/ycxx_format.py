"""lit format for libycxx's own test suite (tests/ycxx).

  *.pass.cpp           compile, link, run; pass iff exit status 0
  *.compile.pass.cpp   must compile (-fsyntax-only)
  *.compile.fail.cpp   must fail to compile, for a reason other than a missing header

Optional directives:
  // FLAGS: <extra compiler flags>
  // FILES: <extra translation units, paths relative to the test file>  (*.pass.cpp only: compiled
                                          and linked into the test program)
  // XFAIL-COMPILER: gcc|clang  <reason>   known compiler gap (listed in STATUS.md); the test is
                                          unchanged and reports XFAIL, or XPASS once the gap closes
"""
import os, re, shutil, tempfile
import lit.formats, lit.Test
from ycxxlit import transcript

FLAGS = re.compile(r'^//\s*FLAGS:(.*)$', re.M)
FILES = re.compile(r'^//\s*FILES:(.*)$', re.M)
XFAIL = re.compile(r'^//\s*XFAIL-COMPILER:\s*(\w+)', re.M)
MISSING = re.compile(r"fatal error: '?[\w./]+'?:? (file not found|No such file or directory)")


class YcxxFormat(lit.formats.FileBasedTest):
    def __init__(self, wrapper, compiler, base_flags):
        self.wrapper, self.compiler, self.base_flags = wrapper, compiler, base_flags

    def compile(self, args, cwd, expect=''):
        return transcript.run('compile', [self.wrapper, self.compiler] + args, cwd, 300, expect)

    def execute(self, test, lit_config):
        result = self.run(test)
        src = open(test.getSourcePath(), encoding='utf-8').read()
        if any(m.group(1) == self.compiler for m in XFAIL.finditer(src)):
            if result.code == lit.Test.PASS:
                result.code = lit.Test.XPASS
            elif result.code == lit.Test.FAIL:
                result.code = lit.Test.XFAIL
        return result

    def run(self, test):
        path = test.getSourcePath()
        name = os.path.basename(path)
        src = open(path, encoding='utf-8').read()
        flags = list(self.base_flags)
        for m in FLAGS.finditer(src):
            flags += m.group(1).split()
        exec_dir = os.path.join(test.suite.exec_root, *test.path_in_suite[:-1])
        os.makedirs(exec_dir, exist_ok=True)
        tmp = tempfile.mkdtemp(prefix=name + '.', dir=exec_dir)
        try:
            if name.endswith('.compile.pass.cpp'):
                rc, out = self.compile(['-fsyntax-only', path] + flags, tmp)
                return lit.Test.Result(lit.Test.PASS if rc == 0 else lit.Test.FAIL, out)
            if name.endswith('.compile.fail.cpp'):
                rc, out = self.compile(['-fsyntax-only', path] + flags, tmp, '; must fail')
                if rc != 0 and MISSING.search(out):
                    return lit.Test.Result(lit.Test.FAIL, 'failed only because a header is missing\n' + out)
                return lit.Test.Result(lit.Test.PASS if rc not in (0, None) else lit.Test.FAIL, out or 'expected a compile error')
            if name.endswith('.pass.cpp'):
                exe = os.path.join(tmp, 't.exe')
                extra = [os.path.normpath(os.path.join(os.path.dirname(path), f))
                         for m in FILES.finditer(src) for f in m.group(1).split()]
                rc, out = self.compile([path] + extra + ['-o', exe] + flags, tmp)
                if rc != 0:
                    return lit.Test.Result(lit.Test.FAIL, 'COMPILE FAILED\n' + out)
                rc, ran = transcript.run('run', [exe], tmp, 60)
                return lit.Test.Result(lit.Test.PASS if rc == 0 else lit.Test.FAIL, out + ran)
            return lit.Test.Result(lit.Test.UNSUPPORTED, 'not a test file')
        finally:
            shutil.rmtree(tmp, ignore_errors=True)
