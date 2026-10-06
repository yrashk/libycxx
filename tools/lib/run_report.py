#!/usr/bin/env python3
"""libycxx test scripts: the composite report of one tools/test run.

    run_report.py STEPS OUT_BASE [KEY=VALUE...]

STEPS is tools/test's record of the run: one line per step, fields separated by the ASCII unit
separator: status (ok, FAIL, skip, interrupted: cut short by Ctrl-C), label, seconds (or, for a skipped step, why), log file,
command, and the HTML report of a suite step (whose <run>.data.json, written by test_report.py,
holds the suite's tests). KEY=VALUE pairs describe the run.

Writes OUT_BASE.html, one self-contained page to share (with a person or an AI assistant):
  - the run (command, commit, platform, compilers) and every stage with its command and time;
  - every failure: a failed stage with the end of its log (a build error, say) and the end of
    every log file that output refers to (a sub-check's "(see /path/x.log)"), a failed test with
    its full transcript (commands, exit statuses, output);
  - every suite (own suite, libc++, libstdc++, per compiler): its run, its counts, why tests
    were unsupported, one passing test's transcript as an example of how its tests run, and
    every test with its result and steps (filter and search);
  - copy buttons: for an agent (everything but the list of every test), the failures, the whole
    report, one stage or one test.
The whole report is also embedded as Markdown (<script type="text/markdown" id="report-md">)
and written as OUT_BASE.md: an overview first, then an appendix with every test. Passing tests'
full transcripts stay in the suites' own reports (linked), which keeps this page small.
"""
import html, json, os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import report_common  # noqa: E402

ANSI = re.compile(r'\x1b\[[0-9;?]*[A-Za-z]')
LOG_TAIL = 300  # lines of a failed step's log in the report
GOOD = {'PASS', 'XFAIL', 'FLAKYPASS'}
SKIP = {'UNSUPPORTED', 'SKIPPED', 'EXCLUDED'}
APPENDIX = '## Appendix: every test'


def fence(text):
    f = '```'
    while f in text:
        f += '`'
    return f'{f}\n{text.rstrip()}\n{f}'


def dur(s):
    s = int(float(s or 0))
    return f'{s // 60}m{s % 60:02d}s' if s >= 60 else f'{s}s'


def read_tail(path):
    try:
        with open(path, encoding='utf-8', errors='replace') as f:
            lines = ANSI.sub('', f.read()).replace('\r', '\n').splitlines()
    except OSError:
        return '', 0
    lines = [l for l in lines if l.strip()]
    return '\n'.join(lines[-LOG_TAIL:]), max(0, len(lines) - LOG_TAIL)


REF_LOG = re.compile(r'(/[^\s()\'"`<>]+?\.(?:log|txt))\b')
REF_TAIL = 200    # lines of each log a failed stage's output refers to
REF_MAX = 12      # logs embedded per stage


def referenced_logs(text, own):
    """The log files a failed stage's output names (a sub-check's "(see /path/x.log)"), each with
    its end, so that the report carries them instead of pointing at files on another machine."""
    out, seen = [], {own}
    for m in REF_LOG.finditer(text or ''):
        path = m.group(1)
        if path in seen or not os.path.isfile(path):
            continue
        seen.add(path)
        try:
            with open(path, encoding='utf-8', errors='replace') as f:
                lines = [l for l in ANSI.sub('', f.read()).replace('\r', '\n').splitlines() if l.strip()]
        except OSError:
            continue
        out.append({'path': path, 'tail': '\n'.join(lines[-REF_TAIL:]) or '(empty)',
                    'cut': max(0, len(lines) - REF_TAIL)})
        if len(out) == REF_MAX:
            break
    return out


def load_suite(report):
    """A suite step's tests and run, from the <run>.data.json next to its report."""
    path = report[:-5] + '.data.json' if report.endswith('.html') else ''
    try:
        with open(path, encoding='utf-8') as f:
            d = json.load(f)
    except (OSError, ValueError):
        return None
    tests = d['tests']
    bad = [t for t in tests if t['code'] not in GOOD and t['code'] not in SKIP]
    # One passing test that was compiled and run, as an example of how this suite runs tests.
    example = next((t for t in tests if t['code'] == 'PASS' and any(s.startswith('run ') for s in t['steps'])),
                   next((t for t in tests if t['code'] == 'PASS'), None))
    reasons = {}
    for t in tests:
        if t['code'] in SKIP:
            r = (t['steps'] or ['(no reason given)'])[0]
            reasons[r] = reasons.get(r, 0) + 1
    # Only failures keep their transcript here; the suite's own report has every one.
    slim = [{'name': t['name'], 'code': t['code'], 'secs': t['secs'], 'steps': t['steps'], 'error': t['error'],
             'link': t.get('link', ''),
             'output': t['output'] if t['code'] not in GOOD and t['code'] not in SKIP else ''} for t in tests]
    return {'title': d['title'], 'meta': d['meta'], 'root': d.get('root', ''), 'counts': d['counts'],
            'elapsed': d.get('elapsed', 0), 'tests': slim, 'bad': len(bad), 'coverage': d.get('coverage', {}),
            'example': {'name': example['name'], 'output': example['output']} if example else None,
            'reasons': sorted(reasons.items(), key=lambda kv: -kv[1])}


def test_md(suite, t, level='###'):
    lines = [f'{level} {t["code"]}: {t["name"]} ({suite["title"]})', '']
    if suite['root']:
        lines.append(f'- source: {os.path.join(suite["root"], t["name"])}')
    lines.append(f'- steps: {"; ".join(t["steps"]) or "none recorded"}')
    lines += ['', fence(t['output'] or '(no output)'), '']
    return lines


def step_md(s, level='###'):
    lines = [f'{level} FAIL: {s["label"]}', '']
    if s['command']:
        lines.append(f'- command: `{s["command"]}`')
    if s['log']:
        lines.append(f'- log: {s["log"]}')
    if s['report']:
        lines.append(f'- report: {s["report"]}')
    lines.append('')
    if s['tail']:
        omitted = f' (the first {s["cut"]} lines omitted)' if s['cut'] else ''
        lines += [f'End of the log{omitted}:', '', fence(s['tail']), '']
    for f in s.get('files', []):
        omitted = f' (the first {f["cut"]} lines omitted)' if f['cut'] else ''
        lines += [f'Log it refers to, {f["path"]}{omitted}:', '', fence(f['tail']), '']
    return lines


def markdown(title, verdict, meta, steps, suites):
    md = [f'# {title}: {verdict}', '',
          'An overview (the run, every stage, every failure in full, every suite), then an appendix '
          'listing every test.', '']
    md += [f'- {k}: {v}' for k, v in meta.items()]
    md += ['', '## Stages', '']
    for s in steps:
        line = f'- {s["status"].upper()} {s["label"]}'
        line += f' — {s["reason"]}' if s['status'] == 'skip' else f' ({dur(s["secs"])})'
        if s['command']:
            line += f' — `{s["command"]}`'
        if s['suite'] is not None:
            su = suites[s['suite']]
            line += ' — ' + ', '.join(f'{c} {n}' for c, n in su['counts'].items())
        md.append(line)
    md.append('')
    failed = [s for s in steps if s['status'] == 'FAIL' and (s['suite'] is None or s['tail'])]
    nbad = sum(su['bad'] for su in suites)
    if failed or nbad:
        md += ['## Failures', '']
        for s in failed:
            md += step_md(s)
        for su in suites:
            for t in su['tests']:
                if t['code'] not in GOOD and t['code'] not in SKIP:
                    md += test_md(su, t)
    else:
        md += ['No failures.', '']
    if suites:
        md += ['## Suites', '']
        for su in suites:
            md += [f'### {su["title"]}', '']
            md += [f'- {k}: {v}' for k, v in su['meta'].items()]
            md.append(f'- results: {", ".join(f"{c} {n}" for c, n in su["counts"].items())} '
                      f'(of {len(su["tests"])}, lit time {su["elapsed"]:.1f}s)')
            md.append(f'- full report (every transcript): {su["report"]}')
            cov = su['coverage']
            if cov:
                n, k = sum(v[0] for v in cov.values()), sum(v[1] for v in cov.values())
                md += [f'- counterparts: {n} tests skipped as tied to the other library\'s internals, extensions '
                       f'or modes; {k} covered by a libycxx test, {n - k} without a libycxx counterpart:', '',
                       '  | category | skipped | covered | uncovered |', '  |---|---:|---:|---:|']
                md += [f'  | {c} | {v[0]} | {v[1]} | {v[0] - v[1]} |' for c, v in cov.items()]
                md.append('')
            if su['reasons']:
                md.append('- why tests were unsupported (count: first reason line):')
                md += [f'  - {n}: {r}' for r, n in su['reasons'][:15]]
                if len(su['reasons']) > 15:
                    md.append(f'  - … {len(su["reasons"]) - 15} more reasons')
            if su['example']:
                md += ['', f'How a test runs in this suite (a passing one, {su["example"]["name"]}):', '',
                       fence(su['example']['output'] or '(no output)')]
            md.append('')
    md += [APPENDIX, '',
           'Each line: result, test, the steps it ran (exit status and time), the compiler error of a '
           'test that must not compile. Full transcripts: the suite\'s own report.', '']
    for su in suites:
        md += [f'### {su["title"]}', '']
        for t in su['tests']:
            line = f'- {t["code"]} {t["name"]}'
            if t['steps']:
                line += ' — ' + '; '.join(t['steps'])
            if t['error']:
                line += ' → ' + t['error']
            if t.get('link'):
                line += ' ⇒ ' + t['link']
            md.append(line)
        md.append('')
    return '\n'.join(md) + '\n'


def main():
    steps_file, base = sys.argv[1], sys.argv[2]
    meta = dict(a.split('=', 1) for a in sys.argv[3:])
    here = os.path.dirname(os.path.abspath(base))
    steps, suites = [], []
    with open(steps_file, encoding='utf-8') as f:
        for line in f:
            p = line.rstrip('\n').split('\x1f') + [''] * 6
            st, label, extra, log, cmd, report = p[:6]
            s = {'status': st, 'label': label, 'secs': '' if st == 'skip' else extra,
                 'reason': extra if st == 'skip' else '', 'log': log, 'command': cmd, 'report': report,
                 'report_href': os.path.relpath(report, here) if report else '', 'tail': '', 'cut': 0,
                 'files': [], 'suite': None}
            if report:
                su = load_suite(report)
                if su is not None:
                    su['report'] = report
                    su['report_href'] = s['report_href']
                    s['suite'] = len(suites)
                    suites.append(su)
            # A failed stage's log, unless it is a suite whose failures are listed with their tests.
            # A suite that failed without a report, or without a failing test (lit or the harness
            # broke), shows what its run printed (<run>.console.log, written by run-conformance).
            if st == 'FAIL' and report and (s['suite'] is None or suites[s['suite']]['bad'] == 0):
                log = s['log'] = report[:-5] + '.console.log'
            if st == 'FAIL' and log and (s['suite'] is None or suites[s['suite']]['bad'] == 0):
                s['tail'], s['cut'] = read_tail(log)
                if not s['tail']:
                    s['tail'] = '(nothing was recorded: see the terminal or CI output of this step)'
            if st == 'FAIL' and s['tail']:
                s['files'] = referenced_logs(s['tail'], log)
            steps.append(s)
    nfail = sum(s['status'] == 'FAIL' for s in steps)
    nok = sum(s['status'] == 'ok' for s in steps)
    nskip = sum(s['status'] == 'skip' for s in steps)
    nint = sum(s['status'] == 'interrupted' for s in steps)
    ntests = sum(len(su['tests']) for su in suites)
    nbad = sum(su['bad'] for su in suites)
    if nint or meta.get('interrupted'):
        verdict = 'interrupted (Ctrl-C); stages: ' + \
                  ', '.join(f'{n} {what}' for n, what in ((nok, 'passed'), (nfail, 'failed'), (nint, 'cut short'),
                                                            (nskip, 'skipped or not run')) if n) + \
                  (f'; {ntests} tests ran, ' + (f'{nbad} failed' if nbad else 'all passed') if ntests else '')
    else:
        verdict = (f'{nfail} of {nok + nfail} stages failed' if nfail else f'all {nok} stages passed') + \
                  (f', {nskip} skipped' if nskip else '') + \
                  (f'; {nbad} of {ntests} tests failed' if nbad else f'; all {ntests} tests passed' if ntests else '')
    title = 'libycxx test run'
    md = markdown(title, verdict, meta, steps, suites)
    with open(base + '.md', 'w', encoding='utf-8') as f:
        f.write(md)

    rows = ''.join(f'<tr><th>{html.escape(k)}</th><td>{html.escape(v)}</td></tr>' for k, v in meta.items())
    page = PAGE.replace('@CSS@', report_common.CSS).replace('@THEMEHEAD@', report_common.THEME_HEAD).replace('@COPYJS@', report_common.JS)
    page = page.replace('@TITLE@', html.escape(f'{title}: {verdict}')).replace('@META@', rows)
    page = page.replace('@VERDICT@', 'bad' if nfail or nbad else 'good')
    payload = {'title': title, 'verdict': verdict, 'meta': meta, 'steps': steps, 'suites': suites,
               'appendix': APPENDIX}
    page = page.replace('@DATA@', json.dumps(payload, separators=(',', ':')).replace('</', '<\\/'))
    page = page.replace('@MD@', re.sub(r'</(script)', r'<\\/\1', md, flags=re.I))
    with open(base + '.html', 'w', encoding='utf-8') as f:
        f.write(page)


PAGE = r'''<!doctype html>
<!--
  libycxx test run: the composite report of one tools/test run. Reading this file without a
  browser (a person or an AI assistant): the whole report is in Markdown in the
  <script type="text/markdown" id="report-md"> element below (also written next to this file as
  a .md). It starts with an overview: the run (commit, platform, compilers, command), every
  stage, every failure in full (a stage's log tail, a test's transcript), and every suite (counts,
  why tests were unsupported, an example of how a test runs). An appendix then lists every test.
  Data as JSON: <script id="report-data">.
-->
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>@TITLE@</title>
<style>
@CSS@
* { box-sizing: border-box; }
body { margin: 0; background: var(--bg); color: var(--fg);
  font: 14px/1.45 -apple-system, BlinkMacSystemFont, "Segoe UI", system-ui, sans-serif; }
main { max-width: 1100px; margin: 0 auto; padding: 24px 16px 64px; }
h1 { font-size: 20px; margin: 0 0 12px; }
h1.good { color: var(--good); } h1.bad { color: var(--bad); }
h2 { font-size: 16px; margin: 28px 0 10px; }
table.meta { border-collapse: collapse; margin: 0 0 12px; font-size: 13px; width: 100%; }
table.meta th { text-align: left; color: var(--muted); font-weight: 500; padding: 2px 16px 2px 0;
  white-space: nowrap; vertical-align: top; width: 1%; }
table.meta td { padding: 2px 0; font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
  font-size: 12px; overflow-wrap: anywhere; }
.actions { display: flex; flex-wrap: wrap; gap: 8px; margin: 0 0 8px; }
.card { border: 1px solid var(--line); border-radius: 10px; background: var(--card); margin: 0 0 8px; }
.head { display: grid; grid-template-columns: 4.5em minmax(0, 1fr) auto auto; gap: 12px; padding: 8px 12px;
  align-items: baseline; }
.st { font-weight: 600; font-size: 12px; }
.st.ok, .st.good { color: var(--good); } .st.FAIL, .st.bad { color: var(--bad); }
.st.skip, .st.interrupted { color: var(--skip); }
.label { font-weight: 500; overflow-wrap: anywhere; }
.cmd { display: block; color: var(--muted); font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
  font-size: 12px; overflow-wrap: anywhere; font-weight: 400; }
.time { color: var(--muted); font-size: 12px; font-variant-numeric: tabular-nums; white-space: nowrap; }
.body { border-top: 1px solid var(--line); padding: 8px 12px; font-size: 13px; }
.body a { color: inherit; }
.note { color: var(--muted); font-size: 12px; }
details.ref { margin-top: 8px; }
details.ref > summary { cursor: pointer; font-size: 12.5px; overflow-wrap: anywhere; }
pre { margin: 8px 0 0; padding: 10px 12px; background: var(--code-bg); border-radius: 6px; font-size: 12px;
  overflow-x: auto; white-space: pre-wrap; overflow-wrap: anywhere; max-height: 480px; overflow-y: auto; }
pre .c { color: var(--fg); font-weight: 600; } pre .s { color: var(--muted); }
details.suite > summary { list-style: none; cursor: pointer; }
details.suite > summary::-webkit-details-marker { display: none; }
.counts { display: flex; flex-wrap: wrap; gap: 6px; align-items: baseline; }
.chip { border: 1px solid var(--line); background: var(--card); color: var(--fg); border-radius: 999px;
  padding: 2px 10px; font: inherit; font-size: 12.5px; cursor: pointer; }
.chip b { font-variant-numeric: tabular-nums; }
.chip.good b { color: var(--good); } .chip.bad b { color: var(--bad); } .chip.skip b { color: var(--skip); }
.chip[aria-pressed="true"] { border-color: var(--fg); }
.tools { display: flex; flex-wrap: wrap; gap: 8px; align-items: center; margin: 8px 0; }
input[type=search] { flex: 1 1 200px; min-width: 0; padding: 5px 10px; border-radius: 8px;
  border: 1px solid var(--line); background: var(--card); color: var(--fg); font: inherit; font-size: 13px; }
.list { border: 1px solid var(--line); border-radius: 8px; overflow: hidden; }
.t { border-top: 1px solid var(--line); }
.t:first-child { border-top: 0; }
.t > summary { display: grid; grid-template-columns: 7em minmax(0, 1fr) auto auto; gap: 10px; padding: 4px 10px;
  cursor: pointer; list-style: none; align-items: baseline; font-size: 12.5px; }
.t > summary::-webkit-details-marker { display: none; }
.t > summary:hover { background: var(--hover); }
.name { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; overflow-wrap: anywhere; }
.err { color: var(--muted); font-size: 11.5px; }
.steps { color: var(--muted); font-size: 12px; text-align: right; }
.t .out { padding: 0 10px 8px; }
.more { padding: 8px; text-align: center; }
@media (max-width: 640px) {
  .head { grid-template-columns: 4em minmax(0, 1fr) auto; }
  .head .time { grid-column: 2; grid-row: 2; }
  .t > summary { grid-template-columns: 6em minmax(0, 1fr) auto; }
  .t .steps { grid-column: 2; grid-row: 2; text-align: left; }
}
</style>
@THEMEHEAD@
</head>
<body>
<main>
<h1 class="@VERDICT@">@TITLE@</h1>
<table class="meta">@META@</table>
<div class="actions"></div>
<h2>Stages</h2>
<div id="stages"></div>
<div id="failures"></div>
<div id="suites"></div>
</main>
<script type="text/markdown" id="report-md">
@MD@
</script>
<script type="application/json" id="report-data">@DATA@</script>
<script>
@COPYJS@
const data = JSON.parse(document.getElementById('report-data').textContent);
const reportMd = document.getElementById('report-md').textContent.replace(/<\\\/(script)/gi, '</$1').trim() + '\n';
const GOOD = new Set(['PASS', 'XFAIL', 'FLAKYPASS']), SKIP = new Set(['UNSUPPORTED', 'SKIPPED', 'EXCLUDED']);
const kind = c => GOOD.has(c) ? 'good' : SKIP.has(c) ? 'skip' : 'bad';
function esc(s) { return String(s).replace(/[&<>"]/g, c => ({'&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;'}[c])); }
function dur(s) { s = Math.floor(+s || 0); return s >= 60 ? Math.floor(s / 60) + 'm' + String(s % 60).padStart(2, '0') + 's' : s + 's'; }
function el(tag, cls, inner) { const e = document.createElement(tag); if (cls) e.className = cls; if (inner != null) e.innerHTML = inner; return e; }
const runLines = () => Object.entries(data.meta).map(([k, v]) => '- ' + k + ': ' + v).join('\n');
function stepMd(s) {
  let md = '### FAIL: ' + s.label + '\n\n';
  if (s.command) md += '- command: `' + s.command + '`\n';
  if (s.log) md += '- log: ' + s.log + '\n';
  if (s.report) md += '- report: ' + s.report + '\n';
  if (s.tail) md += '\nEnd of the log' + (s.cut ? ' (the first ' + s.cut + ' lines omitted)' : '') + ':\n\n' + fence(s.tail) + '\n';
  for (const f of s.files || [])
    md += '\nLog it refers to, ' + f.path + (f.cut ? ' (the first ' + f.cut + ' lines omitted)' : '') + ':\n\n' + fence(f.tail) + '\n';
  return md;
}
function testMd(su, t) {
  return '### ' + t.code + ': ' + t.name + ' (' + su.title + ')\n\n' +
    Object.entries(su.meta).map(([k, v]) => '- ' + k + ': ' + v).join('\n') + '\n' +
    (su.root ? '- source: ' + su.root + '/' + t.name + '\n' : '') +
    '- steps: ' + (t.steps.join('; ') || 'none recorded') + '\n\n' + fence(t.output || '(no output)') + '\n';
}
const failedSteps = data.steps.filter(s => s.status === 'FAIL' && (s.suite === null || s.tail));
const failedTests = [];
data.suites.forEach(su => su.tests.forEach(t => { if (kind(t.code) === 'bad') failedTests.push([su, t]); }));
const failuresMd = () => '# ' + data.title + ': failures\n\n' + runLines() + '\n\n' +
  failedSteps.map(stepMd).join('\n') + failedTests.map(([su, t]) => testMd(su, t)).join('\n');
const forAgent = () => reportMd.split('\n' + data.appendix)[0].trim() + '\n\n(Not included: the appendix ' +
  'listing every test; it is in ' + (data.meta.logs || 'the logs') + '/run.md.)\n';

const actions = document.querySelector('.actions');
actions.appendChild(copyButton('Copy for an agent', forAgent,
  'The run, every stage, every failure in full and every suite, as Markdown (everything but the list of every test)'));
if (failedSteps.length + failedTests.length)
  actions.appendChild(copyButton('Copy failures (' + (failedSteps.length + failedTests.length) + ')', failuresMd,
    'Every failed stage with the end of its log and every failed test with its transcript, as Markdown'));
actions.appendChild(copyButton('Copy whole report', () => reportMd, 'Everything, including the list of every test, as Markdown'));

function transcript(out) {
  return esc(out || '(no output)').split('\n').map(l =>
    l.startsWith('$ ') ? '<span class="c">' + l + '</span>' :
    /^\[[a-z]+: /.test(l) ? '<span class="s">' + l + '</span>' : l).join('\n');
}
// Stages.
const stages = document.getElementById('stages');
for (const s of data.steps) {
  const c = el('div', 'card');
  const su = s.suite === null ? null : data.suites[s.suite];
  const right = s.status === 'skip' ? esc(s.reason) : dur(s.secs);
  c.innerHTML = '<div class="head"><span class="st ' + s.status + '">' + s.status.toUpperCase() + '</span>' +
    '<span class="label">' + esc(s.label) + (s.command ? '<span class="cmd">$ ' + esc(s.command) + '</span>' : '') +
    '</span><span class="time">' + right + '</span><span></span></div>';
  if (s.status === 'FAIL' && (su === null || s.tail))
    c.querySelector('.head > span:last-child').appendChild(copyButton('',
      () => '# ' + data.title + '\n\n' + runLines() + '\n\n' + stepMd(s)));
  let body = '';
  if (su) body += '<div class="counts">' + Object.entries(su.counts).map(([k, n]) =>
    '<span class="chip ' + kind(k) + '">' + k + ' <b>' + n + '</b></span>').join('') +
    ' <a class="note" href="#suite-' + s.suite + '">details below</a> · <a class="note" href="' +
    esc(s.report_href) + '">suite report</a></div>';
  if (s.status === 'FAIL' && s.log && s.tail) body += '<div class="note">Log: ' + esc(s.log) +
    (s.cut ? ' (the end: the first ' + s.cut + ' lines are omitted here)' : '') + '</div>';
  if (s.tail) body += '<pre>' + esc(s.tail) + '</pre>';
  for (const f of s.files || [])
    body += '<details class="ref"><summary>Log it refers to: ' + esc(f.path) +
      (f.cut ? ' <span class="note">(the end: the first ' + f.cut + ' lines omitted)</span>' : '') +
      '</summary><pre>' + esc(f.tail) + '</pre></details>';
  if (body) c.appendChild(el('div', 'body', body));
  stages.appendChild(c);
}
// Failed tests, in full.
if (failedTests.length) {
  const f = document.getElementById('failures');
  f.appendChild(el('h2', '', 'Failed tests (' + failedTests.length + ')'));
  for (const [su, t] of failedTests) {
    const c = el('div', 'card');
    c.innerHTML = '<div class="head"><span class="st bad">' + esc(t.code) + '</span><span class="label">' + esc(t.name) +
      '<span class="cmd">' + esc(su.title) + ' · ' + esc(t.steps.join(' · ')) + '</span></span><span></span><span></span></div>';
    c.querySelector('.head > span:last-child').appendChild(copyButton('', () => testMd(su, t)));
    c.appendChild(el('div', 'body', '<pre>' + transcript(t.output) + '</pre>'));
    f.appendChild(c);
  }
}
// Suites.
const suites = document.getElementById('suites');
if (data.suites.length) suites.appendChild(el('h2', '', 'Suites'));
data.suites.forEach((su, i) => {
  const d = el('details', 'card suite');
  d.id = 'suite-' + i;
  d.innerHTML = '<summary><div class="head"><span class="st ' + (su.bad ? 'bad' : 'good') + '">' + (su.bad ? 'FAIL' : 'OK') +
    '</span><span class="label">' + esc(su.title) + '<span class="cmd">' +
    Object.entries(su.counts).map(([k, n]) => k + ' ' + n).join(' · ') + ' (of ' + su.tests.length + ')</span></span>' +
    '<span class="time">' + dur(su.elapsed) + '</span><span></span></div></summary>';
  const body = el('div', 'body');
  const meta = el('table', 'meta');
  meta.innerHTML = Object.entries(su.meta).map(([k, v]) => '<tr><th>' + esc(k) + '</th><td>' + esc(v) + '</td></tr>').join('') +
    '<tr><th>full report</th><td><a href="' + esc(su.report_href) + '">' + esc(su.report) + '</a></td></tr>';
  body.appendChild(meta);
  if (su.example) {
    body.appendChild(el('div', 'note', 'How a test runs in this suite (a passing one, ' + esc(su.example.name) + '):'));
    body.appendChild(el('pre', '', transcript(su.example.output)));
  }
  const cov = Object.entries(su.coverage || {});
  if (cov.length) {
    const n = cov.reduce((a, [, v]) => a + v[0], 0), k = cov.reduce((a, [, v]) => a + v[1], 0);
    body.appendChild(el('div', 'note', 'Counterparts: ' + n + ' tests skipped as tied to the other library, ' + k +
      ' covered by a libycxx test, ' + (n - k) + ' without (filter: "covered by" or "no libycxx"). Per category (skipped / covered / uncovered):'));
    body.appendChild(el('pre', '', esc(cov.map(([c, v]) => c + ': ' + v[0] + ' / ' + v[1] + ' / ' + (v[0] - v[1])).join('\n'))));
  }
  if (su.reasons.length) {
    body.appendChild(el('div', 'note', 'Why tests were unsupported (count: reason):'));
    body.appendChild(el('pre', '', esc(su.reasons.slice(0, 30).map(([r, n]) => n + ': ' + r).join('\n') +
      (su.reasons.length > 30 ? '\n… ' + (su.reasons.length - 30) + ' more' : ''))));
  }
  const tools = el('div', 'tools');
  const all = el('button', 'chip', 'all <b>' + su.tests.length + '</b>');
  all.dataset.code = '';
  all.setAttribute('aria-pressed', 'true');
  tools.appendChild(all);
  Object.entries(su.counts).forEach(([k, n]) => {
    const b = el('button', 'chip ' + kind(k), k + ' <b>' + n + '</b>');
    b.dataset.code = k;
    tools.appendChild(b);
  });
  const search = el('input');
  search.type = 'search';
  search.placeholder = 'Filter by test name';
  search.setAttribute('aria-label', 'Filter ' + su.title + ' by test name');
  tools.appendChild(search);
  body.appendChild(tools);
  const list = el('div', 'list');
  const more = el('div', 'more');
  const moreBtn = el('button', 'chip', 'Show more');
  more.appendChild(moreBtn);
  body.appendChild(list);
  body.appendChild(more);
  d.appendChild(body);
  let code = '', query = '', matches = [], limit = 0;
  function row(t) {
    const r = el('details', 't');
    r.innerHTML = '<summary><span class="st ' + kind(t.code) + '">' + t.code + '</span><span class="name">' + esc(t.name) +
      (t.error ? '<br><span class="err">→ ' + esc(t.error) + '</span>' : '') +
      (t.link ? '<br><span class="err">⇒ ' + esc(t.link) + '</span>' : '') + '</span><span class="steps">' +
      esc(t.steps.join(' · ') || t.secs.toFixed(2) + 's') + '</span><span></span></summary>';
    if (t.output) r.querySelector('summary > span:last-child').appendChild(copyButton('', () => testMd(su, t)));
    r.addEventListener('toggle', () => {
      if (r.open && !r.querySelector('.out')) {
        const o = el('div', 'out');
        o.innerHTML = t.output ? '<pre>' + transcript(t.output) + '</pre>' :
          '<div class="note">Its full transcript is in the <a href="' + esc(su.report_href) + '">suite report</a>.</div>';
        r.appendChild(o);
      }
    });
    return r;
  }
  function render(reset) {
    if (reset) {
      matches = su.tests.filter(t => (!code || t.code === code) && (t.name.toLowerCase().includes(query) || (t.link || "").toLowerCase().includes(query)));
      limit = 0;
      list.textContent = '';
    }
    const frag = document.createDocumentFragment();
    matches.slice(limit, limit + 300).forEach(t => frag.appendChild(row(t)));
    list.appendChild(frag);
    limit = Math.min(matches.length, limit + 300);
    more.hidden = limit >= matches.length;
  }
  tools.querySelectorAll('.chip').forEach(b => b.addEventListener('click', () => {
    tools.querySelectorAll('.chip').forEach(x => x.setAttribute('aria-pressed', 'false'));
    b.setAttribute('aria-pressed', 'true');
    code = b.dataset.code;
    render(true);
  }));
  search.addEventListener('input', () => { query = search.value.trim().toLowerCase(); render(true); });
  moreBtn.addEventListener('click', () => render(false));
  // The list is built when the suite is first opened: a run of every suite holds tens of
  // thousands of tests.
  d.addEventListener('toggle', () => { if (d.open && !list.firstChild) render(true); });
  suites.appendChild(d);
});
document.querySelectorAll('a[href^="#suite-"]').forEach(a => a.addEventListener('click', () => {
  const t = document.querySelector(a.getAttribute('href'));
  if (t) t.open = true;
}));
</script>
</body>
</html>
'''

if __name__ == '__main__':
    main()
