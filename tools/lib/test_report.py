#!/usr/bin/env python3
"""libycxx test scripts: the report of one suite run.

    test_report.py RESULTS.json OUT_BASE [KEY=VALUE...]

RESULTS.json is lit's --output file; KEY=VALUE pairs describe the run (suite, compiler, commit,
command, ...). Writes OUT_BASE.html, a self-contained page listing every test with its result,
duration and transcript (the commands it ran, their exit statuses and output, see
tests/ycxxlit/transcript.py), and OUT_BASE.tsv, one line per test: result, seconds, test, and the
steps it ran ("compile exit 0 0.06s; run exit 0 0.00s").
"""
import html, json, re, sys

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
    counts = {}
    for t in tests:
        counts[t['code']] = counts.get(t['code'], 0) + 1

    with open(base + '.tsv', 'w', encoding='utf-8') as f:
        f.write('result\tseconds\ttest\tsteps\terror\n')
        for t in tests:
            f.write(f"{t['code']}\t{t['secs']:.3f}\t{t['name']}\t{'; '.join(t['steps'])}\t{t['error']}\n")

    title = f"libycxx {meta.get('suite', '')} suite, {meta.get('compiler', '')}"
    rows = ''.join(f'<tr><th>{html.escape(k)}</th><td>{html.escape(v)}</td></tr>' for k, v in meta.items())
    rows += f"<tr><th>lit time</th><td>{data.get('elapsed', 0):.1f}s</td></tr>"
    order = ['PASS', 'XFAIL', 'FAIL', 'XPASS', 'UNRESOLVED', 'TIMEOUT', 'UNSUPPORTED']
    codes = sorted(counts, key=lambda c: order.index(c) if c in order else len(order))
    chips = ''.join(
        f'<button class="chip {"good" if c in GOOD else "skip" if c in SKIP else "bad"}" data-code="{c}">'
        f'{c} <b>{counts[c]}</b></button>' for c in codes)
    payload = json.dumps(tests, separators=(',', ':')).replace('</', '<\\/')
    page = PAGE.replace('@TITLE@', html.escape(title)).replace('@META@', rows).replace('@CHIPS@', chips)
    page = page.replace('@TOTAL@', str(len(tests))).replace('@DATA@', payload)
    with open(base + '.html', 'w', encoding='utf-8') as f:
        f.write(page)


PAGE = r'''<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>@TITLE@</title>
<style>
:root {
  --bg: #fbfbfa; --fg: #1d1d1f; --muted: #6b6b70; --line: #e3e3e0; --card: #ffffff;
  --good: #1a7f37; --bad: #c62828; --skip: #9a6700; --code-bg: #f3f3f1; --hover: #f1f4f8;
}
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    --bg: #141416; --fg: #e8e8ea; --muted: #9a9aa2; --line: #2c2c31; --card: #1b1b1f;
    --good: #4cc26a; --bad: #ff6b6b; --skip: #e3b341; --code-bg: #101012; --hover: #22252c;
  }
}
:root[data-theme="dark"] {
  --bg: #141416; --fg: #e8e8ea; --muted: #9a9aa2; --line: #2c2c31; --card: #1b1b1f;
  --good: #4cc26a; --bad: #ff6b6b; --skip: #e3b341; --code-bg: #101012; --hover: #22252c;
}
* { box-sizing: border-box; }
body { margin: 0; background: var(--bg); color: var(--fg);
  font: 14px/1.45 -apple-system, BlinkMacSystemFont, "Segoe UI", system-ui, sans-serif; }
main { max-width: 1100px; margin: 0 auto; padding: 24px 16px 64px; }
h1 { font-size: 20px; margin: 0 0 12px; }
table.meta { border-collapse: collapse; margin: 0 0 16px; font-size: 13px; width: 100%; }
table.meta th { text-align: left; color: var(--muted); font-weight: 500; padding: 2px 16px 2px 0;
  white-space: nowrap; vertical-align: top; width: 1%; }
table.meta td { padding: 2px 0; font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
  font-size: 12px; overflow-wrap: anywhere; }
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
summary { display: grid; grid-template-columns: 7.5em minmax(0, 1fr) auto; gap: 12px; padding: 6px 12px;
  cursor: pointer; list-style: none; align-items: baseline; }
summary::-webkit-details-marker { display: none; }
summary:hover { background: var(--hover); }
.code { font-weight: 600; font-size: 12px; }
.code.good { color: var(--good); } .code.bad { color: var(--bad); } .code.skip { color: var(--skip); }
.err { color: var(--muted); font-size: 11.5px; }
.name { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 12.5px; overflow-wrap: anywhere; }
.steps { color: var(--muted); font-size: 12px; text-align: right; font-variant-numeric: tabular-nums; }
pre { margin: 0; padding: 10px 12px; background: var(--code-bg); font-size: 12px; overflow-x: auto;
  white-space: pre-wrap; overflow-wrap: anywhere; border-top: 1px solid var(--line); }
pre .cmd { color: var(--fg); font-weight: 600; } pre .st { color: var(--muted); }
.more { padding: 12px; text-align: center; }
@media (max-width: 640px) {
  summary { grid-template-columns: 6.5em minmax(0, 1fr); }
  .steps { grid-column: 2; text-align: left; }
}
</style>
</head>
<body>
<main>
<h1>@TITLE@</h1>
<table class="meta">@META@</table>
<div class="bar">
  <button class="chip" data-code="" aria-pressed="true">all <b>@TOTAL@</b></button>
  @CHIPS@
  <input type="search" placeholder="Filter by test name" aria-label="Filter by test name">
  <span class="shown"></span>
</div>
<div class="list"></div>
<div class="more"><button class="chip" hidden>Show more</button></div>
</main>
<script>
const tests = @DATA@;
const GOOD = new Set(['PASS', 'XFAIL', 'FLAKYPASS']), SKIP = new Set(['UNSUPPORTED', 'SKIPPED', 'EXCLUDED']);
const kind = c => GOOD.has(c) ? 'good' : SKIP.has(c) ? 'skip' : 'bad';
const list = document.querySelector('.list'), more = document.querySelector('.more button');
const search = document.querySelector('input[type=search]'), shown = document.querySelector('.shown');
let code = '', query = '', matches = [], limit = 0;
const PAGE = 400;
function esc(s) { return s.replace(/[&<>]/g, c => ({'&': '&amp;', '<': '&lt;', '>': '&gt;'}[c])); }
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
  d.addEventListener('toggle', () => {
    if (d.open && !d.querySelector('pre')) {
      const p = document.createElement('pre');
      p.innerHTML = transcript(t.output);
      d.appendChild(p);
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
