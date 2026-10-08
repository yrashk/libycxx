// libycxx.org API reference: the sidebar's index, the search box and the cppreference pane.
// No dependencies; everything degrades to plain links without JavaScript.
(function () {
  'use strict';
  var css = document.querySelector('link[href$="assets/api.css"]');
  if (!css) return;
  var ROOT = css.href.replace(/assets\/api\.css$/, '');       // the site's root, absolute
  var API = ROOT + 'api/';
  var here = location.href.split('#')[0];

  function el(tag, attrs, text) {
    var e = document.createElement(tag);
    if (attrs) for (var k in attrs) if (attrs[k] != null) e.setAttribute(k, attrs[k]);
    if (text != null) e.textContent = text;
    return e;
  }
  function store(key, val) {
    try {
      if (val === undefined) return localStorage.getItem(key);
      localStorage.setItem(key, val);
    } catch (e) { return null; }
  }
  var cache = {};
  function getJSON(name) {
    if (!cache[name]) cache[name] = fetch(API + name).then(function (r) {
      if (!r.ok) throw new Error(name + ': ' + r.status);
      return r.json();
    });
    return cache[name];
  }

  // ---- the index: by header and by namespace ----------------------------------------------
  var idx = document.querySelector('.idx');
  var GROUPS = { N: 'Namespaces', C: 'Concepts and classes', E: 'Enumerations', T: 'Type aliases', F: 'Functions', V: 'Variables and constants', O: 'Other' };
  function entityLink(q, rel, short) {
    var a = el('a', { href: API + rel });
    if (API + rel === here) a.setAttribute('aria-current', 'page');
    var leaf = short ? q.split('::').pop() : q;
    a.textContent = leaf;
    return a;
  }
  function fillHeaders(nav) {
    var box = document.getElementById('idx-h');
    var cur = idx.getAttribute('data-current');
    box.textContent = '';
    box.appendChild(Object.assign(el('a', { href: API + 'index.html', class: 'top' }, 'All headers')));
    Object.keys(nav.headers).forEach(function (h) {
      var items = nav.headers[h];
      var d = el('details');
      var s = el('summary');
      s.appendChild(el('span', null, '<' + h + '>'));
      s.appendChild(el('small', null, String(items.length)));
      d.appendChild(s);
      var filled = false;
      function fill() {
        if (filled) return; filled = true;
        var ul = el('ul');
        var hl = el('li'); hl.appendChild(Object.assign(el('a', { href: API + 'headers/' + h + '.html' }), { textContent: 'header <' + h + '>' }));
        if (API + 'headers/' + h + '.html' === here) hl.firstChild.setAttribute('aria-current', 'page');
        ul.appendChild(hl);
        items.forEach(function (it) { var li = el('li'); li.appendChild(entityLink(it[0], it[1], false)); ul.appendChild(li); });
        d.appendChild(ul);
      }
      d.addEventListener('toggle', fill);
      if (h === cur) { d.className = 'cur'; d.open = true; fill(); }
      box.appendChild(d);
    });
    var on = box.querySelector('[aria-current=page]') || box.querySelector('details.cur');
    if (on) on.scrollIntoView({ block: 'center' });
  }
  function fillNamespaces(nav) {
    var box = document.getElementById('idx-n');
    box.textContent = '';
    var members = {};
    Object.keys(nav.headers).forEach(function (h) {
      nav.headers[h].forEach(function (it) {
        var ns = it[0].split('::').slice(0, -1).join('::');
        (members[ns] = members[ns] || {})[it[0]] = it;
      });
    });
    nav.namespaces.forEach(function (n) {
      var q = n[0], d = el('details'), s = el('summary');
      var list = members[q] ? Object.keys(members[q]).sort().map(function (k) { return members[q][k]; }) : [];
      s.appendChild(el('span', null, q));
      s.appendChild(el('small', null, String(list.length)));
      d.appendChild(s);
      var filled = false;
      d.addEventListener('toggle', function () {
        if (filled) return; filled = true;
        var ul = el('ul');
        var top = el('li'); top.appendChild(Object.assign(el('a', { href: API + n[1] }), { textContent: 'namespace ' + q })); ul.appendChild(top);
        list.forEach(function (it) { var li = el('li'); li.appendChild(entityLink(it[0], it[1], true)); ul.appendChild(li); });
        d.appendChild(ul);
      });
      box.appendChild(d);
    });
  }
  if (idx) {
    var tabs = idx.querySelectorAll('[role=tab]');
    var which = store('ycxx-idx') === 'n' ? 1 : 0;
    function select(i, focus) {
      tabs.forEach(function (t, j) {
        t.setAttribute('aria-selected', i === j ? 'true' : 'false');
        t.tabIndex = i === j ? 0 : -1;
        document.getElementById(t.getAttribute('aria-controls')).hidden = i !== j;
      });
      if (focus) tabs[i].focus();
      store('ycxx-idx', i ? 'n' : 'h');
    }
    tabs.forEach(function (t, i) {
      t.addEventListener('click', function () { select(i); });
      t.addEventListener('keydown', function (e) {
        if (e.key === 'ArrowRight' || e.key === 'ArrowLeft') { select(1 - i, true); e.preventDefault(); }
      });
    });
    ['idx-h', 'idx-n'].forEach(function (id) { document.getElementById(id).appendChild(el('p', { class: 'loading' }, 'Loading the index…')); });
    getJSON('nav.json').then(function (nav) { fillHeaders(nav); fillNamespaces(nav); }).catch(function () {
      ['idx-h', 'idx-n'].forEach(function (id) {
        var b = document.getElementById(id); b.textContent = '';
        b.appendChild(Object.assign(el('a', { href: API + 'index.html', class: 'top' }), { textContent: 'The index of headers and namespaces' }));
      });
    });
    select(which);
  }

  // ---- search ----------------------------------------------------------------------------
  var q = document.getElementById('q'), out = document.getElementById('search-results');
  var data = null, sel = -1, shown = [];
  function load() { if (!data) getJSON('search.json').then(function (d) { data = d; run(); }); }
  function score(e, t, tl) {
    var full = e[0].toLowerCase(), leaf = full.slice(full.lastIndexOf('::') + 2);
    var bare = full.replace(/^std::/, '');
    var s = -1;
    if (leaf === tl || bare === tl || full === tl) s = 1000;
    else if (bare.indexOf(tl) === 0 || full.indexOf(tl) === 0) s = 700;
    else if (leaf.indexOf(tl) === 0) s = 600;
    else if (full.indexOf(tl) >= 0) s = 300;
    else return -1;
    return s - full.split('::').length * 10 - full.length * 0.5;
  }
  function mark(text, t) {
    var frag = document.createDocumentFragment();
    var i = text.toLowerCase().lastIndexOf(t.toLowerCase());
    if (!t || i < 0) { frag.appendChild(document.createTextNode(text)); return frag; }
    frag.appendChild(document.createTextNode(text.slice(0, i)));
    frag.appendChild(el('b', null, text.slice(i, i + t.length)));
    frag.appendChild(document.createTextNode(text.slice(i + t.length)));
    return frag;
  }
  function run() {
    var t = q.value.trim();
    out.textContent = ''; sel = -1; shown = [];
    if (!t) { out.hidden = true; q.setAttribute('aria-expanded', 'false'); return; }
    out.hidden = false; q.setAttribute('aria-expanded', 'true');
    if (!data) { out.appendChild(el('li', { class: 'r-none' }, 'Loading…')); return; }
    var tl = t.toLowerCase().replace(/\s+/g, '');
    var res = [];
    for (var i = 0; i < data.length; i++) {
      var s = score(data[i], t, tl);
      if (s >= 0) res.push([s, data[i]]);
    }
    res.sort(function (a, b) { return b[0] - a[0]; });
    shown = res.slice(0, 40).map(function (r) { return r[1]; });
    if (!shown.length) { out.appendChild(el('li', { class: 'r-none' }, 'No entity matches “' + t + '”.')); return; }
    shown.forEach(function (e, i) {
      var li = el('li', { role: 'option', id: 'r' + i, 'aria-selected': 'false' });
      var a = el('a', { href: API + e[2], tabindex: '-1' });
      var rq = el('span', { class: 'r-q' }); rq.appendChild(mark(e[0], t));
      a.appendChild(rq);
      a.appendChild(el('span', { class: 'r-meta' }, e[1] + (e[3] ? ' · <' + e[3] + '>' : '')));
      li.appendChild(a); out.appendChild(li);
    });
  }
  function move(d) {
    if (!shown.length) return;
    var items = out.querySelectorAll('[role=option]');
    if (sel >= 0) items[sel].setAttribute('aria-selected', 'false');
    sel = (sel + d + items.length) % items.length;
    items[sel].setAttribute('aria-selected', 'true');
    q.setAttribute('aria-activedescendant', items[sel].id);
    items[sel].scrollIntoView({ block: 'nearest' });
  }
  if (q) {
    q.setAttribute('role', 'combobox'); q.setAttribute('aria-expanded', 'false'); q.setAttribute('aria-autocomplete', 'list');
    q.addEventListener('focus', load);
    q.addEventListener('input', function () { load(); run(); });
    q.addEventListener('keydown', function (e) {
      if (e.key === 'ArrowDown') { move(1); e.preventDefault(); }
      else if (e.key === 'ArrowUp') { move(-1); e.preventDefault(); }
      else if (e.key === 'Enter') { var i = sel >= 0 ? sel : 0; if (shown[i]) location.href = API + shown[i][2]; e.preventDefault(); }
      else if (e.key === 'Escape') { q.value = ''; run(); q.blur(); }
    });
    document.addEventListener('click', function (e) { if (!e.target.closest('.search')) { out.hidden = true; } });
    document.addEventListener('keydown', function (e) {
      if (e.key === '/' && !/^(INPUT|TEXTAREA|SELECT)$/.test(document.activeElement.tagName) && !e.ctrlKey && !e.metaKey) {
        e.preventDefault(); openMenu(true); q.focus();
      }
    });
    var p = new URLSearchParams(location.search).get('q');
    if (p) { q.value = p; load(); run(); }
  }

  // ---- the narrow-screen menu --------------------------------------------------------------
  var menu = document.querySelector('.menu-btn');
  function openMenu(on) {
    if (!menu || getComputedStyle(menu).display === 'none') return;
    document.body.classList.toggle('menu-open', on);
    menu.setAttribute('aria-expanded', on ? 'true' : 'false');
  }
  if (menu) menu.addEventListener('click', function () { openMenu(!document.body.classList.contains('menu-open')); });

  // ---- the cppreference pane ---------------------------------------------------------------
  var pane = document.getElementById('cpp-pane');
  if (!pane) return;
  var body = pane.querySelector('.pane-body'), nameEl = pane.querySelector('.pane-name');
  var live = pane.querySelector('.pane-live'), src = pane.querySelector('.pane-src');
  var grip = pane.querySelector('.pane-grip'), closeBtn = pane.querySelector('.pane-close');
  var opener = null;
  var narrow = window.matchMedia('(max-width: 860px)');
  function setSize(px) {
    if (narrow.matches) {
      px = Math.max(200, Math.min(window.innerHeight - 60, px));
      document.documentElement.style.setProperty('--pane-h', px + 'px');
      store('ycxx-pane-h', String(Math.round(px)));
    } else {
      px = Math.max(320, Math.min(window.innerWidth - 360, px));
      document.documentElement.style.setProperty('--pane-w', px + 'px');
      store('ycxx-pane-w', String(Math.round(px)));
    }
  }
  var w = parseInt(store('ycxx-pane-w'), 10), h = parseInt(store('ycxx-pane-h'), 10);
  if (w > 0) document.documentElement.style.setProperty('--pane-w', Math.min(w, window.innerWidth - 360) + 'px');
  if (h > 0) document.documentElement.style.setProperty('--pane-h', h + 'px');

  function summary(link) {
    var art = document.querySelector('article.ref');
    var box = el('div', { class: 'pane-summary' });
    box.appendChild(el('h3', null, link.getAttribute('data-title')));
    var label = art ? art.getAttribute('data-label') : '';
    var q = art ? art.getAttribute('data-q') : '';
    box.appendChild(el('p', null, (q && q !== link.getAttribute('data-title') ? q + ' is described on the page of ' + link.getAttribute('data-title') + '. ' : '') +
      'This page is not in the copy of cppreference.com hosted with this reference (the offline archive of 2025-02-09 predates it, or it is new in C++26); the live site has the current text.'));
    var dl = el('dl', { class: 'facts' });
    document.querySelectorAll('.ref-facts > div').forEach(function (d) { if (!/cppreference/i.test(d.textContent)) dl.appendChild(d.cloneNode(true)); });
    if (label) { var row = el('div'); row.appendChild(el('dt', null, 'Kind')); row.appendChild(el('dd', null, label)); dl.insertBefore(row, dl.firstChild); }
    box.appendChild(dl);
    var a = el('a', { class: 'big', href: link.href, target: '_blank', rel: 'noopener' }, 'Open on cppreference.com ↗');
    box.appendChild(a);
    return box;
  }
  function open(link) {
    opener = link;
    var path = link.getAttribute('data-cppref'), hosted = link.getAttribute('data-hosted') === '1';
    nameEl.textContent = link.getAttribute('data-title');
    live.href = 'https://en.cppreference.com/w/' + path;
    src.textContent = '';
    body.textContent = '';
    if (hosted) {
      var f = el('iframe', { src: ROOT + 'cppref/' + path + '.html', title: 'cppreference: ' + link.getAttribute('data-title'), loading: 'eager' });
      body.appendChild(f);
      src.appendChild(document.createTextNode('Source: cppreference.com, CC-BY-SA 3.0 and GFDL, a copy hosted here · '));
      src.appendChild(el('a', { href: ROOT + 'cppref/LICENSE.txt', target: '_blank' }, 'licence'));
      if (link.getAttribute('data-exact') !== '1') src.appendChild(document.createTextNode(' · the nearest page; no page of its own'));
    } else {
      body.appendChild(summary(link));
      src.textContent = 'cppreference.com: not in the hosted copy; open the live page.';
    }
    pane.hidden = false; pane.setAttribute('aria-hidden', 'false');
    document.body.classList.add('pane-open');
    pane.focus();
  }
  function close() {
    if (pane.hidden) return;
    pane.hidden = true; pane.setAttribute('aria-hidden', 'true');
    document.body.classList.remove('pane-open');
    body.textContent = '';
    if (opener) opener.focus();
  }
  document.addEventListener('click', function (e) {
    var a = e.target.closest && e.target.closest('a.cppref-link');
    if (!a || e.button !== 0 || e.metaKey || e.ctrlKey || e.shiftKey || e.altKey) return;
    e.preventDefault();
    open(a);
  });
  closeBtn.addEventListener('click', close);
  document.addEventListener('keydown', function (e) {
    if (e.key === 'Escape' && !pane.hidden) { close(); }
    else if (e.key === 'Escape') openMenu(false);
  });
  // Esc inside the hosted page (same origin) closes the pane too.
  pane.addEventListener('load', function (e) {
    var f = e.target;
    if (f.tagName !== 'IFRAME') return;
    try { f.contentWindow.document.addEventListener('keydown', function (k) { if (k.key === 'Escape') close(); }); } catch (x) { /* not same-origin */ }
  }, true);
  // Resizing: drag the grip, or focus it and use the arrow keys.
  grip.addEventListener('pointerdown', function (e) {
    e.preventDefault();
    grip.setPointerCapture(e.pointerId);
    pane.classList.add('resizing');
    function mv(ev) { setSize(narrow.matches ? window.innerHeight - ev.clientY : window.innerWidth - ev.clientX); }
    function up() { pane.classList.remove('resizing'); grip.removeEventListener('pointermove', mv); grip.removeEventListener('pointerup', up); grip.removeEventListener('pointercancel', up); }
    grip.addEventListener('pointermove', mv); grip.addEventListener('pointerup', up); grip.addEventListener('pointercancel', up);
  });
  grip.addEventListener('keydown', function (e) {
    var step = e.shiftKey ? 96 : 32, r = pane.getBoundingClientRect();
    if (narrow.matches) {
      if (e.key === 'ArrowUp') setSize(r.height + step); else if (e.key === 'ArrowDown') setSize(r.height - step); else return;
    } else {
      if (e.key === 'ArrowLeft') setSize(r.width + step); else if (e.key === 'ArrowRight') setSize(r.width - step); else return;
    }
    e.preventDefault();
  });
})();
