#!/usr/bin/env python3
"""libycxx test scripts: the report of one tools/test run.

    run_report.py STEPS OUT_BASE [KEY=VALUE...]

STEPS is tools/test's record of the run: one line per step, fields separated by the ASCII unit
separator: status (ok, FAIL, skip), label, seconds (or, for a skipped step, why), log file,
command, and the HTML report of a suite step. KEY=VALUE pairs describe the run. Writes
OUT_BASE.html (every step with its command and duration; a failed step with the end of its log,
a suite step with a link to its report and its failing tests; copy buttons that put Markdown on
the clipboard; the whole report embedded as Markdown for reading without a browser) and
OUT_BASE.md (the same Markdown).
"""
import html, json, os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import report_common  # noqa: E402

ANSI = re.compile(r'\x1b\[[0-9;?]*[A-Za-z]')
LOG_TAIL = 300  # lines of a failed step's log in the report


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


def suite_failures(report):
    """The failures section of a suite report's Markdown (written next to its .html)."""
    md_path = report[:-5] + '.md' if report.endswith('.html') else ''
    try:
        with open(md_path, encoding='utf-8') as f:
            md = f.read()
    except OSError:
        return '', ''
    m = re.search(r'^Results: .*$', md, re.M)
    counts = m.group(0) if m else ''
    i = md.find('\n## Failures')
    j = md.find('\n## All tests')
    if i < 0:
        return counts, ''
    # One heading level down, under the step's own heading.
    part = md[i + 1:j if j > i else len(md)].strip()
    return counts, re.sub(r'^(#+) ', lambda h: '#' * (len(h.group(1)) + 1) + ' ', part, flags=re.M)


def main():
    steps_file, base = sys.argv[1], sys.argv[2]
    meta = dict(a.split('=', 1) for a in sys.argv[3:])
    steps = []
    with open(steps_file, encoding='utf-8') as f:
        for line in f:
            p = line.rstrip('\n').split('\x1f') + [''] * 6
            st, label, extra, log, cmd, report = p[:6]
            s = {'status': st, 'label': label, 'secs': '' if st == 'skip' else extra,
                 'reason': extra if st == 'skip' else '', 'log': log, 'command': cmd, 'report': report,
                 'tail': '', 'cut': 0, 'counts': '', 'failures': ''}
            if st == 'FAIL' and log:
                s['tail'], s['cut'] = read_tail(log)
            if report:
                s['counts'], s['failures'] = suite_failures(report)
            steps.append(s)
    nfail = sum(s['status'] == 'FAIL' for s in steps)
    nok = sum(s['status'] == 'ok' for s in steps)
    nskip = sum(s['status'] == 'skip' for s in steps)
    verdict = (f'{nfail} of {nok + nfail} steps failed' if nfail else f'all {nok} steps passed') + \
              (f', {nskip} skipped' if nskip else '')
    title = 'libycxx test run'

    # Markdown: the run, every step, and for each failure what is needed to act on it.
    md = [f'# {title}: {verdict}', '']
    md += [f'- {k}: {v}' for k, v in meta.items()]
    md += ['', '## Steps', '']
    for s in steps:
        line = f'- {s["status"].upper()} {s["label"]}'
        line += f' — {s["reason"]}' if s['status'] == 'skip' else f' ({dur(s["secs"])})'
        if s['command']:
            line += f' — `{s["command"]}`'
        if s['counts']:
            line += f' — {s["counts"]}'
        md.append(line)
    md.append('')
    for s in steps:
        if s['status'] != 'FAIL':
            continue
        md += [f'## FAIL: {s["label"]}', '']
        if s['command']:
            md.append(f'- command: `{s["command"]}`')
        if s['log']:
            md.append(f'- log: {s["log"]}')
        if s['report']:
            md.append(f'- report: {s["report"]} (Markdown: {s["report"][:-5]}.md)')
        md.append('')
        if s['tail']:
            omitted = f' (the first {s["cut"]} lines omitted)' if s['cut'] else ''
            md += [f'End of the log{omitted}:', '', fence(s['tail']), '']
        if s['failures']:
            md += [s['failures'], '']
    md = '\n'.join(md) + '\n'
    with open(base + '.md', 'w', encoding='utf-8') as f:
        f.write(md)

    rows = ''.join(f'<tr><th>{html.escape(k)}</th><td>{html.escape(v)}</td></tr>' for k, v in meta.items())
    for s in steps:
        if s['report']:
            s['report_href'] = os.path.relpath(s['report'], os.path.dirname(os.path.abspath(base)))
    page = PAGE.replace('@CSS@', report_common.CSS).replace('@COPYJS@', report_common.JS)
    page = page.replace('@TITLE@', html.escape(f'{title}: {verdict}')).replace('@META@', rows)
    page = page.replace('@VERDICT@', 'bad' if nfail else 'good')
    page = page.replace('@DATA@', json.dumps({'title': title, 'verdict': verdict, 'meta': meta, 'steps': steps},
                                             separators=(',', ':')).replace('</', '<\\/'))
    page = page.replace('@MD@', re.sub(r'</(script)', r'<\\/\1', md, flags=re.I))
    with open(base + '.html', 'w', encoding='utf-8') as f:
        f.write(page)


PAGE = r'''<!doctype html>
<!--
  libycxx test run report. Reading this file without a browser (a person or an AI assistant): the
  whole report is in Markdown in the <script type="text/markdown" id="report-md"> element below
  (also written next to this file as a .md): the run (commit, platform, compilers, command), every
  step with its command and duration, and for each failed step the end of its log and, for a
  suite, its failing tests with their transcripts. Data as JSON: <script id="report-data">.
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
table.meta { border-collapse: collapse; margin: 0 0 12px; font-size: 13px; width: 100%; }
table.meta th { text-align: left; color: var(--muted); font-weight: 500; padding: 2px 16px 2px 0;
  white-space: nowrap; vertical-align: top; width: 1%; }
table.meta td { padding: 2px 0; font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
  font-size: 12px; overflow-wrap: anywhere; }
.actions { display: flex; flex-wrap: wrap; gap: 8px; margin: 0 0 16px; }
.step { border: 1px solid var(--line); border-radius: 10px; background: var(--card); margin: 0 0 8px; }
.head { display: grid; grid-template-columns: 4.5em minmax(0, 1fr) auto auto; gap: 12px; padding: 8px 12px;
  align-items: baseline; }
.st { font-weight: 600; font-size: 12px; }
.st.ok { color: var(--good); } .st.FAIL { color: var(--bad); } .st.skip { color: var(--skip); }
.label { font-weight: 500; }
.cmd { display: block; color: var(--muted); font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
  font-size: 12px; overflow-wrap: anywhere; font-weight: 400; }
.time { color: var(--muted); font-size: 12px; font-variant-numeric: tabular-nums; }
.body { border-top: 1px solid var(--line); padding: 8px 12px; font-size: 13px; }
.body a { color: inherit; }
pre { margin: 8px 0 0; padding: 10px 12px; background: var(--code-bg); border-radius: 6px; font-size: 12px;
  overflow-x: auto; white-space: pre-wrap; overflow-wrap: anywhere; max-height: 480px; overflow-y: auto; }
.note { color: var(--muted); font-size: 12px; }
@media (max-width: 640px) {
  .head { grid-template-columns: 4em minmax(0, 1fr) auto; }
  .time { grid-column: 2; grid-row: 2; }
}
</style>
</head>
<body>
<main>
<h1 class="@VERDICT@">@TITLE@</h1>
<table class="meta">@META@</table>
<div class="actions"></div>
<div class="steps"></div>
</main>
<script type="text/markdown" id="report-md">
@MD@
</script>
<script type="application/json" id="report-data">@DATA@</script>
<script>
@COPYJS@
const data = JSON.parse(document.getElementById('report-data').textContent);
const reportMd = document.getElementById('report-md').textContent.replace(/<\\\/(script)/gi, '</$1').trim() + '\n';
function esc(s) { return s.replace(/[&<>"]/g, c => ({'&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;'}[c])); }
function dur(s) { s = Math.floor(+s || 0); return s >= 60 ? Math.floor(s / 60) + 'm' + String(s % 60).padStart(2, '0') + 's' : s + 's'; }
const runLines = () => Object.entries(data.meta).map(([k, v]) => '- ' + k + ': ' + v).join('\n');
// One failed step as Markdown, with the run it belongs to.
function stepMd(s) {
  let md = '## FAIL: ' + s.label + ' (' + data.title + ')\n\n' + runLines() + '\n';
  if (s.command) md += '- command: `' + s.command + '`\n';
  if (s.log) md += '- log: ' + s.log + '\n';
  if (s.report) md += '- report: ' + s.report + '\n';
  if (s.tail) md += '\nEnd of the log' + (s.cut ? ' (the first ' + s.cut + ' lines omitted)' : '') + ':\n\n' + fence(s.tail) + '\n';
  if (s.failures) md += '\n' + s.failures + '\n';
  return md;
}
const actions = document.querySelector('.actions');
actions.appendChild(copyButton('Copy whole report', () => reportMd,
  'The run, every step, and every failure with its log or failing tests, as Markdown'));
const failed = data.steps.filter(s => s.status === 'FAIL');
if (failed.length)
  actions.appendChild(copyButton('Copy failures (' + failed.length + ')', () =>
    '# ' + data.title + ': failures\n\n' + runLines() + '\n\n' + failed.map(stepMd).join('\n')));
const box = document.querySelector('.steps');
for (const s of data.steps) {
  const el = document.createElement('div');
  el.className = 'step';
  const right = s.status === 'skip' ? esc(s.reason) : dur(s.secs);
  el.innerHTML = '<div class="head"><span class="st ' + s.status + '">' + s.status.toUpperCase() + '</span>' +
    '<span class="label">' + esc(s.label) + (s.command ? '<span class="cmd">$ ' + esc(s.command) + '</span>' : '') +
    '</span><span class="time">' + right + '</span><span></span></div>';
  if (s.status === 'FAIL') el.querySelector('.head > span:last-child').appendChild(copyButton('', () => stepMd(s),
    'Copy this failure (the run, the command, the end of the log or the failing tests) as Markdown'));
  let body = '';
  if (s.report) body += '<div>Report: <a href="' + esc(s.report_href) + '">' + esc(s.report) + '</a>' +
    (s.counts ? ' <span class="note">' + esc(s.counts) + '</span>' : '') + '</div>';
  const failing = (s.failures.match(/^#{3,} [A-Z]+: .*$/gm) || []).map(h => h.replace(/^#+ /, ''));
  if (failing.length) body += '<div>Failing tests:</div><pre>' + esc(failing.join('\n')) + '</pre>';
  if (s.status === 'FAIL' && s.log) body += '<div class="note">Log: ' + esc(s.log) +
    (s.cut ? ' (the end: the first ' + s.cut + ' lines are omitted here)' : '') + '</div>';
  if (s.status === 'FAIL' && s.tail) body += '<pre>' + esc(s.tail) + '</pre>';
  if (body) {
    const b = document.createElement('div');
    b.className = 'body';
    b.innerHTML = body;
    el.appendChild(b);
  }
  box.appendChild(el);
}
</script>
</body>
</html>
'''

if __name__ == '__main__':
    main()
