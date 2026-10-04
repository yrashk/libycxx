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

STD = 26
DG = re.compile(r'\{\s*dg-([a-z-]+)\s*(.*)\}\s*$')

# Effective targets we satisfy (besides c++NN selectors).
EFFECTIVE = {'hosted', 'cxx11_abi', 'gthreads', 'threads', 'pthread', 'std_allocator_new', 'tls',
             'tls_native', 'cstdint', 'string_conversions', 'c99_math', 'random_device',
             'x86_64-*-*', '*-*-linux*', 'linux', 'native', 'lp64', 'exceptions', 'rtti',
             'atomic_wait', 'net_ts_ip', 'fenv', 'little_endian'}


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


def load_skips(path):
    skips = []
    if os.path.exists(path):
        for line in open(path):
            line = line.strip()
            if line and not line.startswith('#'):
                pat, cat, why = [x.strip() for x in line.split('|', 2)]
                on_content = pat.startswith('content:')
                skips.append((on_content, re.compile(pat[8:] if on_content else pat), f'skipped ({cat}): {why}'))
    return skips


class LibstdcxxFormat(lit.formats.FileBasedTest):
    def __init__(self, wrapper, compiler, base_flags, skip_file):
        self.wrapper, self.compiler, self.base_flags = wrapper, compiler, base_flags
        self.skips = load_skips(skip_file)

    def execute(self, test, lit_config):
        path = test.getSourcePath()
        rel = '/'.join(test.path_in_suite)
        src = open(path, encoding='utf-8', errors='replace').read()
        for on_content, pat, why in self.skips:
            if (pat.search(src) if on_content else pat.fullmatch(rel)):
                return lit.Test.Result(lit.Test.UNSUPPORTED, why)
        if re.search(r'#\s*include\s*<(ext|bits|tr1|tr2|backward|debug|parallel|profile)/', src) or '__gnu_' in src:  # extension
            return lit.Test.Result(lit.Test.UNSUPPORTED, 'skipped (extension): uses libstdc++ extensions')

        action, expect_fail_run, flags, errors = 'run', False, list(self.base_flags), False
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
                if tsel is not None and not eval_selector(tsel):
                    return lit.Test.Result(lit.Test.UNSUPPORTED, f'dg-do target {tsel!r} not selected')
                xsel = selector_of(args, 'xfail')
                if xsel is not None and eval_selector(xsel):
                    expect_fail_run = True
            elif kind in ('options', 'additional-options'):
                opts = re.findall(r'"([^"]*)"', rest)
                if not opts:
                    continue
                tsel = selector_of(args, 'target')
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
                    if o.startswith('-D_GLIBCXX') or o in ('-fno-inline',):
                        continue
                    flags.append(o)
            elif kind == 'require-effective-target':
                t = args[0] if args else ''
                if not eval_selector(t):
                    return lit.Test.Result(lit.Test.UNSUPPORTED, f'effective target {t} not provided')
            elif kind.startswith('require-'):
                if kind not in ('require-gthreads', 'require-cstdint', 'require-string-conversions',
                                'require-normal-namespace', 'require-normal-mode', 'require-effective-target'):
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

    def compile(self, args, cwd):
        p = subprocess.run([self.wrapper, self.compiler] + args, cwd=cwd, capture_output=True, text=True, timeout=300)
        return p.returncode, p.stdout + p.stderr

    def run(self, action, path, flags, errors, expect_fail_run, tmp):
        if errors:
            rc, out = self.compile(['-fsyntax-only', path] + flags, tmp)
            return lit.Test.Result(lit.Test.PASS if rc != 0 else lit.Test.FAIL, out or 'expected a compile error')
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
