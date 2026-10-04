"""lit test format for libc++'s conformance tests (libcxx/test/std), run against libycxx."""
import os, re, shutil, subprocess, tempfile
import lit.formats, lit.Test, lit.TestRunner

COND_FLAGS = re.compile(r'//\s*ADDITIONAL_COMPILE_FLAGS(?:\(([^)]*)\))?:(.*)')
FILE_DEPS = re.compile(r'//\s*FILE_DEPENDENCIES:(.*)')


def load_skips(path):
    skips = []
    if os.path.exists(path):
        for line in open(path):
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            pat, cat, why = [x.strip() for x in line.split('|', 2)]
            skips.append((re.compile(pat), f'skipped ({cat}): {why}'))
    return skips


class LibcxxFormat(lit.formats.FileBasedTest):
    def __init__(self, wrapper, compiler, base_flags, features, skip_file):
        self.wrapper, self.compiler, self.base_flags, self.features = wrapper, compiler, base_flags, set(features)
        self.skips = load_skips(skip_file)

    def execute(self, test, lit_config):
        path = test.getSourcePath()
        name = os.path.basename(path)
        if name.endswith('.sh.cpp') or '.gen.' in name:
            return lit.Test.Result(lit.Test.UNSUPPORTED, 'shell/generated tests are not supported')
        rel = '/'.join(test.path_in_suite)
        for pat, why in self.skips:
            if pat.fullmatch(rel):
                return lit.Test.Result(lit.Test.UNSUPPORTED, why)
        script = lit.TestRunner.parseIntegratedTestScript(test, require_script=False)
        if isinstance(script, lit.Test.Result):
            return script
        missing = test.getMissingRequiredFeatures()
        if missing:
            return lit.Test.Result(lit.Test.UNSUPPORTED, 'missing features: ' + ', '.join(missing))
        unsupported = test.getUnsupportedFeatures()
        if unsupported:
            return lit.Test.Result(lit.Test.UNSUPPORTED, 'unsupported: ' + ', '.join(unsupported))
        if script:  # tests with explicit RUN lines rely on libc++'s substitutions
            return lit.Test.Result(lit.Test.UNSUPPORTED, 'RUN lines are not supported')

        src = open(path, encoding='utf-8', errors='replace').read()
        flags = list(self.base_flags)
        deps = []
        for line in src.splitlines():
            m = COND_FLAGS.search(line)
            if m and (m.group(1) is None or m.group(1).strip() in self.features):
                flags += [f for f in m.group(2).split() if not f.startswith('-D_LIBCPP')]
            m = FILE_DEPS.search(line)
            if m:
                deps += m.group(1).split()

        exec_dir = os.path.join(test.suite.exec_root, *test.path_in_suite[:-1])
        os.makedirs(exec_dir, exist_ok=True)
        tmp = tempfile.mkdtemp(prefix=name + '.', dir=exec_dir)
        try:
            for d in deps:
                s = os.path.join(os.path.dirname(path), d)
                if os.path.isdir(s):
                    shutil.copytree(s, os.path.join(tmp, os.path.basename(d)), dirs_exist_ok=True)
                elif os.path.exists(s):
                    shutil.copy(s, tmp)
            return self.run(name, path, src, flags, tmp)
        finally:
            shutil.rmtree(tmp, ignore_errors=True)

    def compile(self, args, cwd):
        p = subprocess.run([self.wrapper, self.compiler] + args, cwd=cwd, capture_output=True, text=True, timeout=300)
        return p.returncode, p.stdout + p.stderr

    def run(self, name, path, src, flags, tmp):
        exe = os.path.join(tmp, 't.exe')
        if name.endswith('.compile.pass.cpp'):
            rc, out = self.compile(['-fsyntax-only', path] + flags, tmp)
            return lit.Test.Result(lit.Test.PASS if rc == 0 else lit.Test.FAIL, out)
        if name.endswith('.verify.cpp'):
            if 'expected-error' not in src:
                return lit.Test.Result(lit.Test.UNSUPPORTED, 'warning-only verify test')
            rc, out = self.compile(['-fsyntax-only', path] + flags, tmp)
            return lit.Test.Result(lit.Test.PASS if rc != 0 else lit.Test.FAIL,
                                   out or 'expected a compile error')
        if name.endswith('.fail.cpp'):
            rc, out = self.compile(['-fsyntax-only', path] + flags, tmp)
            return lit.Test.Result(lit.Test.PASS if rc != 0 else lit.Test.FAIL, out or 'expected a compile error')
        if name.endswith('.link.pass.cpp') or name.endswith('.link.fail.cpp'):
            rc, out = self.compile([path, '-o', exe] + flags, tmp)
            ok = (rc == 0) == name.endswith('.link.pass.cpp')
            return lit.Test.Result(lit.Test.PASS if ok else lit.Test.FAIL, out)
        if name.endswith('.pass.cpp'):
            rc, out = self.compile([path, '-o', exe] + flags, tmp)
            if rc != 0:
                return lit.Test.Result(lit.Test.FAIL, 'COMPILE FAILED\n' + out)
            try:
                p = subprocess.run([exe], cwd=tmp, capture_output=True, text=True, timeout=120)
            except subprocess.TimeoutExpired:
                return lit.Test.Result(lit.Test.FAIL, 'TIMEOUT')
            return lit.Test.Result(lit.Test.PASS if p.returncode == 0 else lit.Test.FAIL,
                                   f'exit {p.returncode}\n' + p.stdout + p.stderr)
        return lit.Test.Result(lit.Test.UNSUPPORTED, 'unknown test kind')


