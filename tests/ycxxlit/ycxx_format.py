"""lit format for libycxx's own test suite (tests/ycxx).

  *.pass.cpp           compile, link, run; pass iff exit status 0
  *.compile.pass.cpp   must compile (-fsyntax-only)
  *.compile.fail.cpp   must fail to compile, for a reason other than a missing header

Optional directives:
  // FLAGS: <extra compiler flags>
  // XFAIL-COMPILER: gcc|clang  <reason>   known compiler gap (listed in STATUS.md); the test is
                                          unchanged and reports XFAIL, or XPASS once the gap closes
"""
import os, re, shutil, subprocess, tempfile
import lit.formats, lit.Test

FLAGS = re.compile(r'^//\s*FLAGS:(.*)$', re.M)
XFAIL = re.compile(r'^//\s*XFAIL-COMPILER:\s*(\w+)', re.M)
MISSING = re.compile(r"fatal error: '?[\w./]+'?:? (file not found|No such file or directory)")


class YcxxFormat(lit.formats.FileBasedTest):
    def __init__(self, wrapper, compiler, base_flags):
        self.wrapper, self.compiler, self.base_flags = wrapper, compiler, base_flags

    def compile(self, args, cwd):
        p = subprocess.run([self.wrapper, self.compiler] + args, cwd=cwd, capture_output=True, text=True,
                           timeout=300)
        return p.returncode, p.stdout + p.stderr

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
                rc, out = self.compile(['-fsyntax-only', path] + flags, tmp)
                if rc != 0 and MISSING.search(out):
                    return lit.Test.Result(lit.Test.FAIL, 'failed only because a header is missing\n' + out)
                return lit.Test.Result(lit.Test.PASS if rc != 0 else lit.Test.FAIL, out or 'expected a compile error')
            if name.endswith('.pass.cpp'):
                exe = os.path.join(tmp, 't.exe')
                rc, out = self.compile([path, '-o', exe] + flags, tmp)
                if rc != 0:
                    return lit.Test.Result(lit.Test.FAIL, 'COMPILE FAILED\n' + out)
                try:
                    p = subprocess.run([exe], cwd=tmp, capture_output=True, text=True, timeout=60)
                except subprocess.TimeoutExpired:
                    return lit.Test.Result(lit.Test.FAIL, 'TIMEOUT')
                return lit.Test.Result(lit.Test.PASS if p.returncode == 0 else lit.Test.FAIL,
                                       f'exit {p.returncode}\n' + p.stdout + p.stderr)
            return lit.Test.Result(lit.Test.UNSUPPORTED, 'not a test file')
        finally:
            shutil.rmtree(tmp, ignore_errors=True)
