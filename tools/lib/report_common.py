"""libycxx test scripts: what the HTML reports share (tools/lib/test_report.py, run_report.py).

Copy buttons put Markdown on the clipboard that stands on its own, for a person or an AI
assistant: what ran (commit, platform, compiler, command), what failed, and the full transcript
or log. They work from file:// pages, falling back to execCommand where the Clipboard API is
unavailable.
"""

# Colour tokens, light and dark, plus the copy button.
CSS = r'''
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
.copy { display: inline-flex; align-items: center; gap: 6px; border: 1px solid var(--line);
  background: var(--card); color: var(--muted); border-radius: 6px; padding: 3px 8px; font: inherit;
  font-size: 12px; cursor: pointer; line-height: 1.2; white-space: nowrap; }
.copy:hover { color: var(--fg); border-color: var(--muted); }
.copy svg { width: 14px; height: 14px; flex: none; }
.copy.done { color: var(--good); border-color: var(--good); }
.copy.icon { padding: 3px 5px; }
'''

ICON = ('<svg viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.5" aria-hidden="true">'
        '<rect x="5.5" y="5.5" width="8" height="8" rx="1.5"/>'
        '<path d="M10.5 5.5V3.5a1 1 0 0 0-1-1h-6a1 1 0 0 0-1 1v6a1 1 0 0 0 1 1h2"/></svg>')

# copyButton(label, getText): a button that copies getText() and says so for a moment.
JS = r'''
const COPY_ICON = '@ICON@';
async function copyText(text) {
  try {
    if (navigator.clipboard && window.isSecureContext !== false) {
      await navigator.clipboard.writeText(text);
      return true;
    }
  } catch (e) {}
  const ta = document.createElement('textarea');
  ta.value = text;
  ta.setAttribute('readonly', '');
  ta.style.position = 'fixed';
  ta.style.opacity = '0';
  document.body.appendChild(ta);
  ta.select();
  let ok = false;
  try { ok = document.execCommand('copy'); } catch (e) {}
  ta.remove();
  return ok;
}
function copyButton(label, getText, title) {
  const b = document.createElement('button');
  b.type = 'button';
  b.className = 'copy' + (label ? '' : ' icon');
  b.title = title || 'Copy as Markdown, for a person or an AI assistant';
  b.setAttribute('aria-label', label || b.title);
  b.innerHTML = COPY_ICON + (label ? '<span>' + label + '</span>' : '');
  b.addEventListener('click', async ev => {
    ev.preventDefault();
    ev.stopPropagation();
    const ok = await copyText(getText());
    const span = b.querySelector('span');
    const old = span ? span.textContent : '';
    b.classList.add('done');
    if (span) span.textContent = ok ? 'Copied' : 'Copy failed';
    else b.title = ok ? 'Copied' : 'Copy failed';
    setTimeout(() => { b.classList.remove('done'); if (span) span.textContent = old; }, 1400);
  });
  return b;
}
// A fenced block that its content cannot close.
function fence(text) {
  let f = '```';
  while (text.includes(f)) f += '`';
  return f + '\n' + text.replace(/\s+$/, '') + '\n' + f;
}
'''.replace('@ICON@', ICON)
