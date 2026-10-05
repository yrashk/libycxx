"""lit test format for the libstdc++ testsuite (DejaGnu-style directives), run against libycxx.

Only the subset of DejaGnu that matters for conformance is interpreted:
  dg-do compile|run|link [{ target SEL }] [{ xfail SEL }]
  dg-options / dg-additional-options      (language-mode flags are normalised to C++26)
  dg-require-effective-target NAME
  dg-require-namedlocale NAME             (the C library has it and libycxx accepts it:
                                           tests/ycxxlit/locales.py)
  dg-error ... [{ target SEL }]           (any applicable dg-error => compilation must fail;
                                           one marked { xfail SEL } is a known-missing error)
  dg-xfail-run-if COMMENT { SEL } [{ INCLUDE-OPTS } [{ EXCLUDE-OPTS }]]
Message matching (dg-error regexps, dg-warning) is not done: diagnostics text is
implementation-specific.
"""
import os, re, shutil, tempfile
import lit.formats, lit.Test
from ycxxlit import transcript
from ycxxlit.skips import load_skips, match_skip, load_xfails, apply_xfail
from ycxxlit import locales

STD = 26
DG = re.compile(r'\{\s*dg-([a-z-]+)\s*(.*)\}\s*$')

# Effective targets we satisfy (besides c++NN selectors).
EFFECTIVE = {'hosted', 'cxx11_abi', 'gthreads', 'threads', 'pthread', 'std_allocator_new', 'tls',
             'tls_native', 'cstdint', 'string_conversions', 'c99_math', 'random_device',
             'x86_64-*-*', '*-*-linux*', 'linux', 'native', 'lp64', 'exceptions', 'rtti',
             'atomic_wait', 'net_ts_ip', 'fenv', 'little_endian', 'ieee_floats', 'size32plus',
             # <atomic> needs no libatomic (DECISIONS: lock-based atomics in the runtime archive).
             'libatomic_available'}
# dg-require-* checks the target passes (beyond the effective targets above).
REQUIRES = {'require-gthreads', 'require-cstdint', 'require-string-conversions', 'require-normal-namespace',
            'require-normal-mode', 'require-effective-target', 'require-atomic-builtins',
            'require-atomic-cmpxchg-word', 'require-thread-fence', 'require-sleep', 'require-gthreads-timed',
            'require-sched-yield', 'require-time',
            # <filesystem> (POSIX): symlinks, space, last_write_time, mkfifo
            'require-filesystem-ts', 'require-target-fs-symlinks', 'require-target-fs-space',
            'require-target-fs-lwt', 'require-mkfifo',
            # file streams work (the target has a file system)
            'require-fileio'}


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


# Options of GCC's dg-options that Clang rejects ("unknown argument") and that only steer GCC's
# optimiser or dumps, so dropping them for Clang keeps the test's meaning.
GCC_ONLY_DROP = ('-fno-assume-sane-operators-new-delete', '-fvtable-verify=', '-fdump-tree-')
# Options that enable a language feature Clang 23 does not have: such a test cannot run on Clang.
GCC_ONLY_FEATURE = {'-fcontracts': 'contracts (P2900)', '-fcontract-evaluation-semantic=': 'contracts (P2900)'}


def option_sets_match(groups, flags):
    """DejaGnu's include/exclude option lists: each group is a string of options that must all
    be present; the list matches when any group does ('*' matches everything, '' nothing)."""
    for g in groups:
        opts = g.split()
        if g.strip() == '*' or (opts and all(o in flags for o in opts)):
            return True
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
    def __init__(self, wrapper, compiler, base_flags, skip_file, locale_probe=None):
        self.wrapper, self.compiler, self.base_flags = wrapper, compiler, base_flags
        self.locale_probe = locale_probe  # locales.build_probe
        self.skips = load_skips(os.path.join(os.path.dirname(os.path.dirname(skip_file)), 'common', 'skip.txt'), skip_file)
        self.xfails = load_xfails(os.path.join(os.path.dirname(skip_file), 'xfail.txt'))

    def execute(self, test, lit_config):
        return apply_xfail(self.execute_test(test, lit_config), self.xfails, '/'.join(test.path_in_suite), self.compiler)

    def execute_test(self, test, lit_config):
        path = test.getSourcePath()
        rel = '/'.join(test.path_in_suite)
        src = open(path, encoding='utf-8', errors='replace').read()
        why = match_skip(self.skips, rel, src)
        if why:
            return lit.Test.Result(lit.Test.UNSUPPORTED, why)
        if re.search(r'#\s*include\s*<(ext|bits|tr1|tr2|backward|debug|parallel|profile)/', src) or '__gnu_' in src:  # extension
            return lit.Test.Result(lit.Test.UNSUPPORTED, 'skipped (extension): uses libstdc++ extensions')

        action, expect_fail_run, flags, errors = 'run', False, list(self.base_flags), False
        xfail_run_if = []
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
                # Tcl command substitutions ([atomic_link_flags [get_multilibs]]) name DejaGnu
                # helpers for libatomic, which libycxx does not need; -latomic likewise.
                text = opts[0]
                while re.search(r'\[[^\[\]]*\]', text):
                    text = re.sub(r'\[[^\[\]]*\]', '', text)
                for o in text.split():
                    if o in ('-latomic', '-lstdc++exp'):  # libstdc++'s own extra libraries
                        continue
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
                    if self.compiler == 'clang':
                        # GCC-only options from dg-options used to reach Clang ("unknown argument").
                        if o.startswith(GCC_ONLY_DROP):
                            continue
                        feat = next((v for k, v in GCC_ONLY_FEATURE.items() if o.startswith(k)), None)
                        if feat:
                            return lit.Test.Result(lit.Test.UNSUPPORTED, f'needs {o}: Clang 23 has no {feat}')
                    flags.append(o)
            elif kind == 'require-effective-target':
                t = args[0] if args else ''
                if not eval_selector(t):
                    return lit.Test.Result(lit.Test.UNSUPPORTED, f'effective target {t} not provided')
            elif kind == 'require-cpp-feature-test':
                # The feature-test macro must be defined by <version> (as libstdc++'s harness checks).
                macro = (args[0] if args else rest).strip().strip('"').strip()
                if not self.has_macro(macro, flags):
                    return lit.Test.Result(lit.Test.UNSUPPORTED, f'{macro} not defined')
            elif kind == 'require-namedlocale':
                name = (args[0] if args else rest).strip().strip('"').strip()
                why = locales.usable(name, self.locale_probe)
                if why:
                    return lit.Test.Result(lit.Test.UNSUPPORTED, why)
            elif kind.startswith('require-'):
                if kind not in REQUIRES:
                    return lit.Test.Result(lit.Test.UNSUPPORTED, f'dg-{kind} not provided')
            elif kind == 'error':
                tsel = selector_of(args, 'target')
                xsel = selector_of(args, 'xfail')
                if tsel is not None and ' xfail ' in f' {tsel} ':
                    tsel, _, xsel = f' {tsel} '.partition(' xfail ')
                    tsel, xsel = tsel.strip(), xsel.strip()
                # { xfail SEL }: DejaGnu expects this error NOT to be issued (a known missing
                # diagnostic), so it does not make the test a compile-fail test.
                if xsel is not None and eval_selector(xsel):
                    continue
                if tsel is None or eval_selector(tsel):
                    errors = True
            elif kind == 'xfail-run-if':
                # dg-xfail-run-if COMMENT { SEL } [{ INCLUDE-OPTS } [{ EXCLUDE-OPTS }]]: the program
                # is expected to fail at run time (e.g. an assertion that must fire). Previously
                # ignored, so a test whose program aborted as intended was reported as failing.
                groups = [a[1:-1].strip() for a in args if a.startswith('{')]
                incl = re.findall(r'"([^"]*)"', groups[1]) if len(groups) > 1 else ['*']
                excl = re.findall(r'"([^"]*)"', groups[2]) if len(groups) > 2 else ['']
                xfail_run_if.append((groups[0] if groups else '*-*-*', incl or ['*'], excl or ['']))
            elif kind == 'add-options':
                if args and args[0] == 'libatomic':
                    pass  # libycxx's atomics need no libatomic
        if not saw_do:
            action = 'compile'
        for sel, incl, excl in xfail_run_if:
            if eval_selector(sel) and option_sets_match(incl, flags) and not option_sets_match(excl, flags):
                expect_fail_run = True
        # DejaGnu compiles libstdc++'s tests with -g (the testsuite's default CXXFLAGS are
        # "-g -O2"); the <stacktrace> tests check source file names and lines, which need it.
        # Only those tests get it here, to keep the run time down; a later -g0 still wins.
        if re.search(r'#\s*include\s*<stacktrace>', src):
            flags.insert(len(self.base_flags), '-g')

        exec_dir = os.path.join(test.suite.exec_root, *test.path_in_suite[:-1])
        os.makedirs(exec_dir, exist_ok=True)
        tmp = tempfile.mkdtemp(prefix=os.path.basename(path) + '.', dir=exec_dir)
        # DejaGnu copies testsuite/data/* into the directory the tests run in, and tests open
        # those files by their plain names (e.g. "filebuf_members-1.txt"). Copy the data files a
        # test names (copies, not links: some tests write to them).
        data_dir = os.path.join(test.suite.source_root, 'data')
        if os.path.isdir(data_dir):
            for name in os.listdir(data_dir):
                if name in src:
                    shutil.copy(os.path.join(data_dir, name), tmp)
        try:
            return self.run(action, path, flags, errors, expect_fail_run, tmp)
        finally:
            shutil.rmtree(tmp, ignore_errors=True)

    MISSING = re.compile(r"fatal error: '?[\w./]+'?:? (file not found|No such file or directory)")

    def expect_error(self, args, cwd):
        """Compile expecting failure. A missing header is not the failure the test wants."""
        rc, out = self.compile(args, cwd, '; must fail')
        if rc != 0 and self.MISSING.search(out):
            return lit.Test.Result(lit.Test.FAIL, 'expected a compile error, but a header is missing\n' + out)
        return lit.Test.Result(lit.Test.PASS if rc not in (0, None) else lit.Test.FAIL, out or 'expected a compile error')

    def has_macro(self, macro, flags):
        with tempfile.TemporaryDirectory() as d:
            src = os.path.join(d, 'ftm.cc')
            with open(src, 'w') as f:
                f.write(f'#include <version>\n#ifndef {macro}\n#error missing\n#endif\n')
            return self.compile(['-fsyntax-only', src] + flags, d)[0] == 0

    def compile(self, args, cwd, expect=''):
        return transcript.run('compile', [self.wrapper, self.compiler] + args, cwd, 300, expect)

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
        rc, ran = transcript.run('run', [exe], tmp, 120, '; must fail' if expect_fail_run else '')
        ok = rc is not None and (rc == 0) != expect_fail_run
        return lit.Test.Result(lit.Test.PASS if ok else lit.Test.FAIL, out + ran)
