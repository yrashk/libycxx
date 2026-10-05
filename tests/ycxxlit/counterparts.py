"""Links from skipped external tests to libycxx's own tests (tests/ycxx).

An external test skipped because it is about the other library's internals, extensions or modes
(a skip-list category below, or a lit feature / dg-require that names a library mode) usually
still has a standard subject: a behaviour the draft specifies. An own test covering that subject
names the external test in a directive (tests/ycxxlit/ycxx_format.py):

    // COUNTERPART: libcxx:<path under libcxx/test/std> libstdcxx:<path under the testsuite>

A path is a regex, anchored like the skip lists' (re.fullmatch), so one directive can link a
whole directory: `libstdcxx:23_containers/vector/debug/.*`. The index is built once per lit run
(by the format's constructor, which lit pickles to its workers), and a skipped test's result
output ends with one line:

    covered by libycxx: tests/ycxx/<path>[, ...]
    no libycxx counterpart: <why the draft gives it no subject, from TRIAGE.md>
    no libycxx counterpart

Why one has none (no standard subject: internals, ABI layout, an extension API) is recorded in
the suite's TRIAGE.md, section "Skipped tests without a counterpart" (a table of `pattern` and
reason between the counterparts:begin / counterparts:end markers; `\|` stands for `|`); a test
that matches no row there and has no own test has not been triaged yet.
"""
import os, re

DIRECTIVE = re.compile(r'^//\s*COUNTERPART:(.*)$', re.M)
SUITES = ('libcxx', 'libstdcxx')
# Skip-list categories (skipped (<category>): ...) whose tests are linked.
CATEGORIES = ('implementation-specific', 'extension', 'divergence', 'removed')
# Reasons that are a library mode or a libc++-specific directory, not a missing platform feature.
MODES = re.compile(r'libcpp-hardening-mode|dg-require-debug-mode|warning-only verify test')
SKIPPED = re.compile(r'skipped \(([^)]*)\)')
COVERED, NONE = 'covered by libycxx: ', 'no libycxx counterpart'
ROW = re.compile(r'^\| `([^`]*)` \| (.*) \|$', re.M)
BEGIN, END = '<!-- counterparts:begin', '<!-- counterparts:end -->'


def load_none(triage):
    """The TRIAGE.md rows of tests without a counterpart: [(regex, reason)]."""
    try:
        text = open(triage, encoding='utf-8').read()
    except OSError:
        return []
    if BEGIN not in text or END not in text:
        return []
    out = []
    for m in ROW.finditer(text[text.index(BEGIN):text.index(END)]):
        try:
            out.append((re.compile(m.group(1).replace('\\|', '|')), m.group(2).strip()))
        except re.error:
            pass
    return out


class Index:
    """suite + external path -> the own tests (paths from the repository root) that cover it."""

    def __init__(self, root):
        self.exact = {s: {} for s in SUITES}
        self.patterns = {s: [] for s in SUITES}
        tests = os.path.dirname(os.path.abspath(root))
        self.none = {s: load_none(os.path.join(tests, s, 'TRIAGE.md')) for s in SUITES}
        top = os.path.dirname(tests)
        for d, _, files in os.walk(root):
            for name in sorted(files):
                if not name.endswith('.cpp'):
                    continue
                path = os.path.join(d, name)
                try:
                    src = open(path, encoding='utf-8', errors='replace').read()
                except OSError:
                    continue
                own = os.path.relpath(path, top)
                for m in DIRECTIVE.finditer(src):
                    for item in m.group(1).split():
                        suite, _, pat = item.partition(':')
                        if suite not in SUITES or not pat:
                            continue
                        self.exact[suite].setdefault(pat, []).append(own)
                        try:
                            self.patterns[suite].append((re.compile(pat), own))
                        except re.error:
                            pass

    def lookup(self, suite, rel):
        own = set(self.exact.get(suite, {}).get(rel, ()))
        own.update(o for p, o in self.patterns.get(suite, ()) if p.fullmatch(rel))
        return sorted(own)

    def why_none(self, suite, rel):
        return next((why for p, why in self.none.get(suite, ()) if p.fullmatch(rel)), '')


def linked(reason, rel):
    """Whether a test UNSUPPORTED for this reason gets the counterpart line."""
    m = SKIPPED.match(reason)
    if m:
        return m.group(1) in CATEGORIES
    if reason.startswith('unsupported by lit.local.cfg'):
        return rel.startswith('experimental/')  # libc++'s TS directory (c++experimental)
    return bool(MODES.search(reason))


def annotate(result, index, suite, rel):
    """Appends the counterpart line to a skipped / UNSUPPORTED result of a linked reason."""
    import lit.Test
    if result.code != lit.Test.UNSUPPORTED or index is None:
        return result
    out = result.output or ''
    if not linked(out.lstrip().split('\n', 1)[0], rel):
        return result
    own = index.lookup(suite, rel)
    why = '' if own else index.why_none(suite, rel)
    line = COVERED + ', '.join(own) if own else NONE + (': ' + why if why else '')
    result.output = out.rstrip('\n') + '\n' + line + '\n'
    return result
