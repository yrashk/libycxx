#!/usr/bin/env python3
"""libycxx test scripts: the report of one suite run.

    test_report.py RESULTS.json OUT_BASE [KEY=VALUE...]

RESULTS.json is lit's --output file; KEY=VALUE pairs describe the run (suite, compiler, commit,
command, ...; a key starting with '_' is used but not displayed: _root, the test root). Writes:
  OUT_BASE.html  a self-contained page: every test with its result, duration and transcript (the
                 commands it ran, their exit statuses and output; tests/ycxxlit/transcript.py),
                 with filters, search and copy buttons. The whole report is also embedded as
                 Markdown (<script type="text/markdown" id="report-md">), readable without a
                 browser, by a person or an AI assistant.
  OUT_BASE.md    the same Markdown.
  OUT_BASE.tsv   one line per test: result, seconds, test, steps, error.
  OUT_BASE.data.json  the tests and the run, for the composite report of tools/test.
"""
import html, json, os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import report_common  # noqa: E402

STEP = re.compile(r'^\[([a-z]+): (exit -?\d+|TIMEOUT[^,\]]*)(?:, ([\d.]+)s)?(?:; ([^\]]*))?\]$', re.M)
GOOD = {'PASS', 'XFAIL', 'FLAKYPASS'}
SKIP = {'UNSUPPORTED', 'SKIPPED', 'EXCLUDED'}


def first_error(output):
    """For a test that must not compile: the compiler's first error, which shows why it did not."""
    if '; must fail]' not in (output or ''):
        return ''
    for line in output.splitlines():
        i = line.find('error: ')
        if i >= 0:
            return line[i:i + 160]
    return ''


def steps_of(output):
    out = []
    for m in STEP.finditer(output or ''):
        s = f'{m.group(1)} {m.group(2)}'
        if m.group(3):
            s += f' {m.group(3)}s'
        if m.group(4):
            s += f' ({m.group(4)})'
        out.append(s)
    return out


def fence(text):
    f = '```'
    while f in text:
        f += '`'
    return f'{f}\n{text.rstrip()}\n{f}'


def markdown(title, meta, counts, tests, elapsed):
    """The whole report as Markdown: the run, the counts, every failure with its transcript, and
    every test with its steps."""
    shown = {k: v for k, v in meta.items() if not k.startswith('_')}
    root = meta.get('_root', '')
    bad = [t for t in tests if t['code'] not in GOOD and t['code'] not in SKIP]
    out = [f'# {title}', '']
    out += [f'- {k}: {v}' for k, v in shown.items()]
    out += [f'- lit time: {elapsed:.1f}s', '']
    out.append('Results: ' + ', '.join(f'{c} {n}' for c, n in counts.items()) + f' (of {len(tests)})')
    out.append('')
    if bad:
        out += [f'## Failures ({len(bad)})', '']
        for t in bad:
            out += [f'### {t["code"]}: {t["name"]}', '']
            if root:
                out.append(f'- source: {os.path.join(root, t["name"])}')
            out.append(f'- steps: {"; ".join(t["steps"]) or "none recorded"}')
            out += ['', fence(t['output'] or '(no output)'), '']
    else:
        out += ['No failures.', '']
    out += [f'## All tests ({len(tests)})', '',
            'Each line: result, test, the steps it ran (exit status and time), the compiler error '
            'of a test that must not compile.', '']
    for t in tests:
        line = f'- {t["code"]} {t["name"]}'
        if t['steps']:
            line += ' — ' + '; '.join(t['steps'])
        if t['error']:
            line += ' → ' + t['error']
        out.append(line)
    return '\n'.join(out) + '\n'


def main():
    results, base = sys.argv[1], sys.argv[2]
    meta = dict(a.split('=', 1) for a in sys.argv[3:])
    with open(results, encoding='utf-8') as f:
        data = json.load(f)
    tests = []
    for t in data.get('tests', []):
        name = t['name'].split(' :: ', 1)[-1]
        out = t.get('output', '') or ''
        steps = steps_of(out)
        if not steps and out.strip():  # e.g. why a test is unsupported
            steps = [out.strip().splitlines()[0][:120]]
        tests.append({'name': name, 'code': t['code'], 'secs': round(t.get('elapsed') or 0.0, 3),
                      'steps': steps, 'error': first_error(out), 'output': out})
    # Failures first, then by name.
    tests.sort(key=lambda t: (t['code'] in GOOD or t['code'] in SKIP, t['name']))
    order = ['PASS', 'XFAIL', 'FAIL', 'XPASS', 'UNRESOLVED', 'TIMEOUT', 'UNSUPPORTED']
    counts = {}
    for t in tests:
        counts[t['code']] = counts.get(t['code'], 0) + 1
    counts = dict(sorted(counts.items(), key=lambda kv: order.index(kv[0]) if kv[0] in order else len(order)))

    with open(base + '.tsv', 'w', encoding='utf-8') as f:
        f.write('result\tseconds\ttest\tsteps\terror\n')
        for t in tests:
            f.write(f"{t['code']}\t{t['secs']:.3f}\t{t['name']}\t{'; '.join(t['steps'])}\t{t['error']}\n")

    title = f"libycxx {meta.get('suite', '')} suite, {meta.get('compiler', '')}"
    md = markdown(title, meta, counts, tests, data.get('elapsed', 0))
    with open(base + '.md', 'w', encoding='utf-8') as f:
        f.write(md)
    # For the composite report of a tools/test run (tools/lib/run_report.py).
    with open(base + '.data.json', 'w', encoding='utf-8') as f:
        json.dump({'title': title, 'meta': {k: v for k, v in meta.items() if not k.startswith('_')},
                   'root': meta.get('_root', ''), 'counts': counts, 'elapsed': data.get('elapsed', 0),
                   'tests': tests}, f)

    shown = {k: v for k, v in meta.items() if not k.startswith('_')}
    rows = ''.join(f'<tr><th>{html.escape(k)}</th><td>{html.escape(v)}</td></tr>' for k, v in shown.items())
    rows += f"<tr><th>lit time</th><td>{data.get('elapsed', 0):.1f}s</td></tr>"
    chips = ''.join(
        f'<button class="chip {"good" if c in GOOD else "skip" if c in SKIP else "bad"}" data-code="{c}">'
        f'{c} <b>{n}</b></button>' for c, n in counts.items())

    def embed(value):
        return json.dumps(value, separators=(',', ':')).replace('</', '<\\/')

    page = PAGE.replace('@CSS@', report_common.CSS).replace('@COPYJS@', report_common.JS)
    page = page.replace('@TITLE@', html.escape(title)).replace('@META@', rows).replace('@CHIPS@', chips)
    page = page.replace('@TOTAL@', str(len(tests))).replace('@DATA@', embed(tests))
    page = page.replace('@RUN@', embed({'title': title, 'meta': shown, 'root': meta.get('_root', '')}))
    # Only "</script" could end the element early; the page undoes this escape when copying.
    page = page.replace('@MD@', re.sub(r'</(script)', r'<\\/\1', md, flags=re.I))
    with open(base + '.html', 'w', encoding='utf-8') as f:
        f.write(page)


PAGE = r'''<!doctype html>
<!--
  libycxx test report. Reading this file without a browser (a person or an AI assistant): the
  whole report is in Markdown in the <script type="text/markdown" id="report-md"> element below
  (also written next to this file as a .md): the run (commit, compiler, command), the counts,
  every failing test with its full transcript (commands, exit statuses, output), and every test
  with the steps it ran. Per-test data as JSON: <script id="report-data">.
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
table.meta { border-collapse: collapse; margin: 0 0 12px; font-size: 13px; width: 100%; }
table.meta th { text-align: left; color: var(--muted); font-weight: 500; padding: 2px 16px 2px 0;
  white-space: nowrap; vertical-align: top; width: 1%; }
table.meta td { padding: 2px 0; font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
  font-size: 12px; overflow-wrap: anywhere; }
.actions { display: flex; flex-wrap: wrap; gap: 8px; margin: 0 0 12px; }
.bar { display: flex; flex-wrap: wrap; gap: 8px; align-items: center; margin: 0 0 12px;
  position: sticky; top: 0; background: var(--bg); padding: 8px 0; z-index: 1; }
.chip { border: 1px solid var(--line); background: var(--card); color: var(--fg); border-radius: 999px;
  padding: 4px 12px; font: inherit; cursor: pointer; }
.chip b { font-variant-numeric: tabular-nums; }
.chip.good b { color: var(--good); } .chip.bad b { color: var(--bad); } .chip.skip b { color: var(--skip); }
.chip[aria-pressed="true"] { border-color: var(--fg); }
input[type=search] { flex: 1 1 220px; min-width: 0; padding: 6px 10px; border-radius: 8px;
  border: 1px solid var(--line); background: var(--card); color: var(--fg); font: inherit; }
.shown { color: var(--muted); font-size: 13px; }
.list { border: 1px solid var(--line); border-radius: 10px; background: var(--card); overflow: hidden; }
details { border-top: 1px solid var(--line); }
details:first-child { border-top: 0; }
summary { display: grid; grid-template-columns: 7.5em minmax(0, 1fr) auto auto; gap: 12px; padding: 6px 12px;
  cursor: pointer; list-style: none; align-items: baseline; }
summary::-webkit-details-marker { display: none; }
summary:hover { background: var(--hover); }
.code { font-weight: 600; font-size: 12px; }
.code.good { color: var(--good); } .code.bad { color: var(--bad); } .code.skip { color: var(--skip); }
.name { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 12.5px; overflow-wrap: anywhere; }
.err { color: var(--muted); font-size: 11.5px; }
.steps { color: var(--muted); font-size: 12px; text-align: right; font-variant-numeric: tabular-nums; }
.out { position: relative; border-top: 1px solid var(--line); background: var(--code-bg); }
.out .copy { position: absolute; top: 8px; right: 8px; }
pre { margin: 0; padding: 10px 12px; padding-right: 96px; font-size: 12px; overflow-x: auto;
  white-space: pre-wrap; overflow-wrap: anywhere; }
pre .cmd { color: var(--fg); font-weight: 600; } pre .st { color: var(--muted); }
.more { padding: 12px; text-align: center; }
@media (max-width: 640px) {
  summary { grid-template-columns: 6.5em minmax(0, 1fr) auto; }
  .steps { grid-column: 2; grid-row: 2; text-align: left; }
}
</style>
</head>
<body>
<main>
<h1>@TITLE@</h1>
<table class="meta">@META@</table>
<div class="actions"></div>
<div class="bar">
  <button class="chip" data-code="" aria-pressed="true">all <b>@TOTAL@</b></button>
  @CHIPS@
  <input type="search" placeholder="Filter by test name" aria-label="Filter by test name">
  <span class="shown"></span>
</div>
<div class="list"></div>
<div class="more"><button class="chip" hidden>Show more</button></div>
</main>
<script type="text/markdown" id="report-md">
@MD@
</script>
<script type="application/json" id="report-data">@DATA@</script>
<script>
@COPYJS@
const tests = JSON.parse(document.getElementById('report-data').textContent);
const run = @RUN@;
const reportMd = document.getElementById('report-md').textContent.replace(/<\\\/(script)/gi, '</$1').trim() + '\n';
const GOOD = new Set(['PASS', 'XFAIL', 'FLAKYPASS']), SKIP = new Set(['UNSUPPORTED', 'SKIPPED', 'EXCLUDED']);
const kind = c => GOOD.has(c) ? 'good' : SKIP.has(c) ? 'skip' : 'bad';
const list = document.querySelector('.list'), more = document.querySelector('.more button');
const search = document.querySelector('input[type=search]'), shown = document.querySelector('.shown');
let code = '', query = '', matches = [], limit = 0;
const PAGE = 400;
function esc(s) { return s.replace(/[&<>]/g, c => ({'&': '&amp;', '<': '&lt;', '>': '&gt;'}[c])); }
function runLines() {
  return Object.entries(run.meta).map(([k, v]) => '- ' + k + ': ' + v).join('\n');
}
// One test as Markdown, with the run it belongs to.
function testMd(t) {
  return '## ' + t.code + ': ' + t.name + ' (' + run.title + ')\n\n' + runLines() + '\n' +
    (run.root ? '- source: ' + run.root + '/' + t.name + '\n' : '') +
    '- steps: ' + (t.steps.join('; ') || 'none recorded') + '\n\n' + fence(t.output || '(no output)') + '\n';
}
const failing = tests.filter(t => kind(t.code) === 'bad');
const actions = document.querySelector('.actions');
actions.appendChild(copyButton('Copy whole report', () => reportMd,
  'The run, the counts, every failure with its transcript and every test with its steps, as Markdown'));
if (failing.length)
  actions.appendChild(copyButton('Copy failures (' + failing.length + ')', () =>
    '# ' + run.title + ': failures\n\n' + runLines() + '\n\n' + failing.map(testMd).join('\n'),
    'Every failing test with the run and its full transcript, as Markdown'));
function transcript(out) {
  return esc(out || '(no output)').split('\n').map(l =>
    l.startsWith('$ ') ? '<span class="cmd">' + l + '</span>' :
    /^\[[a-z]+: /.test(l) ? '<span class="st">' + l + '</span>' : l).join('\n');
}
function row(t) {
  const d = document.createElement('details');
  d.innerHTML = '<summary><span class="code ' + kind(t.code) + '">' + t.code + '</span><span class="name">' +
    esc(t.name) + (t.error ? '<br><span class="err">→ ' + esc(t.error) + '</span>' : '') +
    '</span><span class="steps">' + esc(t.steps.join(' · ') || (t.secs.toFixed(2) + 's')) + '</span></summary>';
  d.querySelector('summary').appendChild(copyButton('', () => testMd(t),
    'Copy this test (the run, the steps and the full transcript) as Markdown'));
  d.addEventListener('toggle', () => {
    if (d.open && !d.querySelector('.out')) {
      const box = document.createElement('div');
      box.className = 'out';
      const p = document.createElement('pre');
      p.innerHTML = transcript(t.output);
      box.appendChild(p);
      box.appendChild(copyButton('Copy', () => testMd(t)));
      d.appendChild(box);
    }
  });
  return d;
}
function render(reset) {
  if (reset) {
    matches = tests.filter(t => (!code || t.code === code) && t.name.toLowerCase().includes(query));
    limit = 0;
    list.textContent = '';
  }
  const next = matches.slice(limit, limit + PAGE);
  const frag = document.createDocumentFragment();
  next.forEach(t => frag.appendChild(row(t)));
  list.appendChild(frag);
  limit += next.length;
  more.hidden = limit >= matches.length;
  shown.textContent = matches.length + ' shown';
}
document.querySelectorAll('.bar .chip').forEach(b => b.addEventListener('click', () => {
  document.querySelectorAll('.bar .chip').forEach(x => x.setAttribute('aria-pressed', 'false'));
  b.setAttribute('aria-pressed', 'true');
  code = b.dataset.code;
  render(true);
}));
search.addEventListener('input', () => { query = search.value.trim().toLowerCase(); render(true); });
more.addEventListener('click', () => render(false));
render(true);
</script>
</body>
</html>
'''

if __name__ == '__main__':
    main()
