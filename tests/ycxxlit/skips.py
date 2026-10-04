"""Skip lists shared by the conformance formats.

A line is  `<regex> | <category> | <reason>` (fields separated by " | ", with spaces, so
regexes may use "|" alternation). A regex prefixed with `content:` is matched
against the test source (re.search); otherwise against the test path (re.fullmatch).
tests/common/skip.txt applies to every suite; tests/<suite>/skip.txt to one suite.
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
