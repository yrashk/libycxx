"""lit test format for the libstdc++ testsuite (DejaGnu-style directives), run against libycxx.

Only the subset of DejaGnu that matters for conformance is interpreted:
  dg-do compile|run|link [{ target SEL }] [{ xfail SEL }]
  dg-options / dg-additional-options      (language-mode flags are normalised to C++26)
  dg-require-effective-target NAME
  dg-error ... [{ target SEL }]           (any applicable dg-error => compilation must fail)
Message matching (dg-error regexps, dg-warning) is not done: diagnostics text is
implementation-specific.
"""
import os, re, shutil, subprocess, tempfile
import lit.formats, lit.Test
from ycxxlit.skips import load_skips, match_skip

STD = 26
DG = re.compile(r'\{\s*dg-([a-z-]+)\s*(.*)\}\s*$')

# Effective targets we satisfy (besides c++NN selectors).
EFFECTIVE = {'hosted', 'cxx11_abi', 'gthreads', 'threads', 'pthread', 'std_allocator_new', 'tls',
             'tls_native', 'cstdint', 'string_conversions', 'c99_math', 'random_device',
             'x86_64-*-*', '*-*-linux*', 'linux', 'native', 'lp64', 'exceptions', 'rtti',
             'atomic_wait', 'net_ts_ip', 'fenv', 'little_endian', 'ieee_floats', 'size32plus'}


def eval_selector(sel):
    """Evaluate a DejaGnu target selector such as 'c++20', '{ ! c++23 }', 'c++17_only'."""
    toks = re.findall(r'\|\||&&|!|\{|\}|[^\s{}!|&]+', sel)
    pos = 0

    def atom(t):
        m = re.fullmatch(r'c\+\+(\d+)(_only|_down)?', t)
        if m:
            n = int(m.group(1)); n = 26 if n == 2 and False else n
            if n < 50:
                n = {98: -2, 3: -1}.get(n, n)
            if m.group(2) == '_only':
                return n == STD
            if m.group(2) == '_down':
                return STD <= n
            return STD >= n
        if t in ('*-*-*', 'native'):
            return True
        return t in EFFECTIVE

    def expr():
        nonlocal pos
        v = term()
        while pos < len(toks) and toks[pos] in ('||', '&&'):
            op = toks[pos]; pos += 1
            r = term()
            v = (v or r) if op == '||' else (v and r)
        return v

    def term():
        nonlocal pos
        t = toks[pos]; pos += 1
        if t == '!':
            return not term()
        if t == '{':
            v = expr()
            pos += 1  # '}'
            return v
        return atom(t)

    try:
        return expr() if toks else True
    except IndexError:
        return False


def braced(s):
    """Split the argument text of a directive into top-level tokens / brace groups."""
    out, depth, cur = [], 0, ''
    for ch in s:
        if ch == '{':
            if depth == 0 and cur.strip():
                out += cur.split(); cur = ''
            depth += 1
            if depth == 1:
                continue
        elif ch == '}':
            depth -= 1
            if depth == 0:
                out.append('{' + cur.strip() + '}'); cur = ''
                continue
        cur += ch
    out += cur.split()
    return out


def selector_of(args, key):
    for i, a in enumerate(args):
        if a.startswith('{') and a[1:].lstrip().startswith(key):
            return a[1:-1].strip()[len(key):].strip()
    return None


class LibstdcxxFormat(lit.formats.FileBasedTest):
    def __init__(self, wrapper, compiler, base_flags, skip_file):
        self.wrapper, self.compiler, self.base_flags = wrapper, compiler, base_flags
        self.skips = load_skips(os.path.join(os.path.dirname(os.path.dirname(skip_file)), 'common', 'skip.txt'), skip_file)

    def execute(self, test, lit_config):
        path = test.getSourcePath()
        rel = '/'.join(test.path_in_suite)
        src = open(path, encoding='utf-8', errors='replace').read()
        why = match_skip(self.skips, rel, src)
        if why:
            return lit.Test.Result(lit.Test.UNSUPPORTED, why)
        if re.search(r'#\s*include\s*<(ext|bits|tr1|tr2|backward|debug|parallel|profile)/', src) or '__gnu_' in src:  # extension
            return lit.Test.Result(lit.Test.UNSUPPORTED, 'skipped (extension): uses libstdc++ extensions')

        action, expect_fail_run, flags, errors = 'run', False, list(self.base_flags), False
        # libstdc++'s hardened mode, requested in the source itself, maps to ours.
        if re.search(r'^\s*#\s*define\s+_GLIBCXX_ASSERTIONS\b', src, re.M):
            flags.append('-DYCXX_HARDENED=1')
        saw_do = False
        for line in src.splitlines():
            m = DG.search(line)
            if not m:
                continue
            kind, rest = m.group(1), m.group(2)
            args = braced(rest)
            if kind == 'do':
                saw_do = True
                action = args[0] if args else 'run'
                tsel = selector_of(args, 'target')
                xsel = selector_of(args, 'xfail')
                # DejaGnu also accepts both in one group: { target c++14 xfail *-*-* }.
                if tsel is not None and ' xfail ' in f' {tsel} ':
                    tsel, _, xsel = f' {tsel} '.partition(' xfail ')
                    tsel, xsel = tsel.strip(), xsel.strip()
                if tsel is not None and not eval_selector(tsel):
                    return lit.Test.Result(lit.Test.UNSUPPORTED, f'dg-do target {tsel!r} not selected')
                if xsel is not None and eval_selector(xsel):
                    expect_fail_run = True
            elif kind in ('options', 'additional-options'):
                opts = re.findall(r'"([^"]*)"', rest)
                if not opts:
                    continue
                tsel = selector_of(args, 'target')
                if tsel is not None and ' xfail ' in f' {tsel} ':  # xfail does not apply to options
                    tsel = f' {tsel} '.partition(' xfail ')[0].strip()
                if tsel is not None and not eval_selector(tsel):
                    continue
                for o in opts[0].split():
                    sm = re.fullmatch(r'-std=(?:gnu|c)\+\+(\w+)', o)
                    if sm:
                        v = sm.group(1)
                        n = {'98': -2, '03': -1, '0x': 11, '1y': 14, '1z': 17, '2a': 20, '2b': 23, '2c': 26}.get(v)
                        n = int(v) if n is None else n
                        if n != STD:
                            return lit.Test.Result(lit.Test.UNSUPPORTED, f'skipped (pre-c++26): needs {o}')
                        continue
                    if o == '-D_GLIBCXX_ASSERTIONS':
                        flags.append('-DYCXX_HARDENED=1')  # libstdc++'s hardened mode -> ours
                        continue
                    if o.startswith('-D_GLIBCXX'):
                        continue
                    flags.append(o)
            elif kind == 'require-effective-target':
                t = args[0] if args else ''
                if not eval_selector(t):
                    return lit.Test.Result(lit.Test.UNSUPPORTED, f'effective target {t} not provided')
            elif kind.startswith('require-'):
                if kind not in ('require-gthreads', 'require-cstdint', 'require-string-conversions',
                                'require-normal-namespace', 'require-normal-mode', 'require-effective-target',
                                # <filesystem> (POSIX): symlinks, space, last_write_time, mkfifo
                                'require-filesystem-ts', 'require-target-fs-symlinks',
                                'require-target-fs-space', 'require-target-fs-lwt', 'require-mkfifo'):
                    return lit.Test.Result(lit.Test.UNSUPPORTED, f'dg-{kind} not provided')
            elif kind == 'error':
                tsel = selector_of(args, 'target')
                if tsel is None or eval_selector(tsel):
                    errors = True
            elif kind == 'add-options':
                if args and args[0] == 'libatomic':
                    flags.append('-latomic')
        if not saw_do:
            action = 'compile'

        exec_dir = os.path.join(test.suite.exec_root, *test.path_in_suite[:-1])
        os.makedirs(exec_dir, exist_ok=True)
        tmp = tempfile.mkdtemp(prefix=os.path.basename(path) + '.', dir=exec_dir)
        try:
            return self.run(action, path, flags, errors, expect_fail_run, tmp)
        finally:
            shutil.rmtree(tmp, ignore_errors=True)

    MISSING = re.compile(r"fatal error: '?[\w./]+'?:? (file not found|No such file or directory)")

    def expect_error(self, args, cwd):
        """Compile expecting failure. A missing header is not the failure the test wants."""
        rc, out = self.compile(args, cwd)
        if rc != 0 and self.MISSING.search(out):
            return lit.Test.Result(lit.Test.FAIL, 'expected a compile error, but a header is missing\n' + out)
        return lit.Test.Result(lit.Test.PASS if rc != 0 else lit.Test.FAIL, out or 'expected a compile error')

    def compile(self, args, cwd):
        p = subprocess.run([self.wrapper, self.compiler] + args, cwd=cwd, capture_output=True, text=True, timeout=300)
        return p.returncode, p.stdout + p.stderr

    def run(self, action, path, flags, errors, expect_fail_run, tmp):
        if errors:
            return self.expect_error(['-fsyntax-only', path] + flags, tmp)
        if action in ('compile', 'preprocess', 'assemble'):
            rc, out = self.compile(['-fsyntax-only', path] + flags, tmp)
            return lit.Test.Result(lit.Test.PASS if rc == 0 else lit.Test.FAIL, out)
        exe = os.path.join(tmp, 't.exe')
        rc, out = self.compile([path, '-o', exe] + flags, tmp)
        if rc != 0:
            return lit.Test.Result(lit.Test.FAIL, 'COMPILE FAILED\n' + out)
        if action == 'link':
            return lit.Test.Result(lit.Test.PASS, out)
        try:
            p = subprocess.run([exe], cwd=tmp, capture_output=True, text=True, timeout=120)
        except subprocess.TimeoutExpired:
            return lit.Test.Result(lit.Test.FAIL, 'TIMEOUT')
        ok = (p.returncode == 0) != expect_fail_run
        return lit.Test.Result(lit.Test.PASS if ok else lit.Test.FAIL, f'exit {p.returncode}\n' + p.stdout + p.stderr)
