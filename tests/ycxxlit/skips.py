"""Skip lists shared by the conformance formats.

A line is  `<regex> | <category> | <reason>` (fields separated by " | ", with spaces, so
regexes may use "|" alternation). A regex prefixed with `content:` is matched
against the test source (re.search); otherwise against the test path (re.fullmatch).
tests/common/skip.txt applies to every suite; tests/<suite>/skip.txt to one suite.

Expected failures (tests/<suite>/xfail.txt) have a cause outside the test and the library (a
compiler gap or bug, a draft defect): the test still runs, and
reports XFAIL when it fails, XPASS (which fails the run) once it passes. A line is
`<path regex> | <gcc|clang|any> | <reason>`.

Tests that do not apply in one configuration only (tests/libcxx/unsupported.txt) are reported
UNSUPPORTED only while a lit feature names it: `root` when the tests run as root (permission
errors cannot happen), a compiler for a test that exercises an extension with that compiler only.
They still run everywhere else. A line is `<path regex> | <feature> | <reason>`.
"""
import os, re


def load_skips(*paths):
    skips = []
    for path in paths:
        if not os.path.exists(path):
            continue
        for line in open(path):
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            pat, cat, why = [x.strip() for x in line.split(' | ', 2)]
            on_content = pat.startswith('content:')
            skips.append((on_content, re.compile(pat[8:] if on_content else pat), f'skipped ({cat}): {why}'))
    return skips


def match_skip(skips, rel, src):
    for on_content, pat, why in skips:
        if pat.search(src) if on_content else pat.fullmatch(rel):
            return why
    return None


def load_unsupported(path):
    entries = []
    if os.path.exists(path):
        for line in open(path):
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            pat, feature, why = [x.strip() for x in line.split(' | ', 2)]
            entries.append((re.compile(pat), feature, f'unsupported ({feature}): {why}'))
    return entries


def match_unsupported(entries, rel, features):
    for pat, feature, why in entries:
        if feature in features and pat.fullmatch(rel):
            return why
    return None


def load_xfails(path):
    xfails = []
    if os.path.exists(path):
        for line in open(path):
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            pat, compiler, why = [x.strip() for x in line.split(' | ', 2)]
            xfails.append((re.compile(pat), compiler, why))
    return xfails


def apply_xfail(result, xfails, rel, compiler):
    """FAIL -> XFAIL and PASS -> XPASS for a test expected to fail with this compiler."""
    import lit.Test
    for pat, who, why in xfails:
        if who in (compiler, 'any') and pat.fullmatch(rel):
            if result.code == lit.Test.FAIL:
                result.code = lit.Test.XFAIL
                result.output = f'expected failure ({who}): {why}\n' + (result.output or '')
            elif result.code == lit.Test.PASS:
                result.code = lit.Test.XPASS
                result.output = f'listed as an expected failure ({who}: {why}) but passed: remove it from xfail.txt\n' + (result.output or '')
            break
    return result
