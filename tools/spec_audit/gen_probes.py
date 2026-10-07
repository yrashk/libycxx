#!/usr/bin/env python3
"""Generate the spec-coverage probes of an audit part from the draft (docs/SPEC_COVERAGE.md).

    tools/spec_audit/gen_probes.py --part part2 BLOCKS.json

BLOCKS.json is extract_draft.py's output for the part's clauses. The part's configuration
(tools/spec_audit/<part>_config.py) says which clauses it covers, how to instantiate each class
and which template arguments to use. Writes tools/spec_audit/<part>/probes/<subclause>.cpp, one
compile-only probe per subclause that declares entities, and tools/spec_audit/<part>/entities.tsv,
the inventory: every entity with its probe checks. Each check is one line of a probe, ended by a
comment `// @<id> <aspect>`; tools/spec_audit/run_probes.py compiles the probes and maps each
diagnostic back to the check it is about.

Aspects: `name` (the entity is declared: a using-declaration of it, or of the member in a class
derived from the instantiation), `type` (a member type or alias is the specified type), `call`
(the function is callable with the specified parameter types; a constructor: is_constructible),
`ret` (the return type), `noexcept` (where the draft says noexcept, or its condition holds for
the instantiation), `implicit` (a non-explicit converting constructor converts), `deleted` (a
deleted overload is not viable), `guide` (a deduction guide deduces the specified type), `spec`
(a specialization is complete, a variable template specialization has its value).
"""
import argparse, importlib, json, os, re, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import decls, cxxdecl, inventory
from cxxdecl import Unresolved

ROOT = os.path.dirname(os.path.dirname(HERE))


def draft_names():
    names = set()
    with open(os.path.join(ROOT, 'tools', 'data', 'uglify', 'draft-names.txt'), encoding='utf-8') as f:
        for line in f:
            if line.startswith('#') or not line.strip():
                continue
            names.add(line.split()[0])
    return names


class Gen:
    def __init__(self, cfg, ents):
        self.cfg = cfg
        self.ents = ents
        self.n = 0
        self.rows = []            # (id, sec, kind, qualified name, decl text, aspects)
        self.ns_names = {}        # ns -> {name}
        self.members = {}         # (ns, class path) -> {name}
        for clause, sec, d in ents:
            if d.cls:
                self.members.setdefault((d.ns, '::'.join(d.cls)), set()).add(d.name)
            elif d.name and d.kind not in ('pp', 'deduction-guide', 'class-spec', 'variable-spec'):
                self.ns_names.setdefault(d.ns, set()).add(d.name.split('<')[0])
        self.std_names = draft_names()

    # -- name lookup ---------------------------------------------------------------------------
    def lookup_for(self, ns):
        """name -> qualified name, as unqualified lookup from namespace ns finds it."""
        table = {}
        for n in self.std_names:
            table[n] = 'std::' + n
        table.update(self.cfg.EXTRA_LOOKUP)
        parts = ns.split('::')
        for k in range(1, len(parts) + 1):
            q = '::'.join(parts[:k])
            for n in self.ns_names.get(q, ()):
                table[n] = q + '::' + n
        return table

    def new_id(self):
        self.n += 1
        return f'E{self.n}'

    # -- class configuration -------------------------------------------------------------------
    def class_cfg(self, sec, d):
        path = '::'.join(d.cls)
        for key in (sec + '|' + path, path):
            if key in self.cfg.CLASSES:
                return self.cfg.CLASSES[key]
        return None

    def subst_for(self, f, base):
        """Template arguments for a function's own template parameters, by name."""
        s = dict(base)
        for tp in f.tparams:
            if tp.name is None or tp.name in s:
                continue
            v = self.cfg.default_targ(tp, f)
            if v is not None:
                s[tp.name] = v
        return s

    # -- generation ----------------------------------------------------------------------------
    def run(self, outdir):
        by_sec = {}
        for clause, sec, d in self.ents:
            by_sec.setdefault(sec, []).append(d)
        os.makedirs(outdir, exist_ok=True)
        for f in os.listdir(outdir):
            if f.endswith('.cpp'):
                os.remove(os.path.join(outdir, f))
        for sec, ds in by_sec.items():
            lines = []
            for d in ds:
                if d.kind == 'pp':
                    lines += self.pp(sec, d)
                    continue
                try:
                    lines += self.entity(sec, d)
                except Unresolved as e:
                    raise
            if not lines:
                continue
            with open(os.path.join(outdir, sec + '.cpp'), 'w', encoding='utf-8') as o:
                o.write(self.cfg.prelude(sec))
                o.write('\n'.join(lines) + '\n')

    def record(self, sec, d, aspects, note=''):
        i = self.new_id()
        q = d.ns + '::' + ('::'.join(d.cls) + '::' if d.cls else '') + d.name
        self.rows.append([i, sec, d.kind, q, ' '.join(d.text.split()), d.comment.strip(), ','.join(aspects), note])
        return i

    def pp(self, sec, d):
        m = re.match(r'#\s*include\s*<(\w+)>', d.toks[0].text)
        if m:
            i = self.record(sec, d, ['include'])
            hdr = self.cfg.SECTION_HEADER.get(sec)
            rep = self.cfg.HEADER_REPRESENTATIVE.get(m.group(1))
            if not hdr or not rep:
                self.rows[-1][-1] = 'no check: ' + ('no representative name' if hdr else 'no header')
                self.rows[-1][-2] = ''
                return []
            # checked in the per-header probe (gen header probes)
            self.header_includes.setdefault(hdr, []).append((i, m.group(1), rep))
            return []
        m = re.match(r'#\s*define\s+(\w+)', d.toks[0].text)
        if m:
            i = self.record(sec, d, ['name'])
            return [f'#ifndef {m.group(1)}', f'#error "{m.group(1)} is not defined" // @{i} name', '#endif']
        return []

    def entity(self, sec, d):
        k = d.kind
        if k.startswith('friend-'):
            k = k[len('friend-'):]
            friend = True
        else:
            friend = False
        cfg = self.class_cfg(sec, d) if d.cls else None
        skip = self.cfg.skip(sec, d)
        if skip:
            self.record(sec, d, [], 'no check: ' + skip)
            return []
        if d.cls and cfg is None:
            self.record(sec, d, [], 'no check: class not configured')
            return []
        lookup = self.lookup_for(d.ns)
        members = self.members.get((d.ns, '::'.join(d.cls)), set()) | set(cfg.get('members', ())) if cfg else set()
        inst = cfg['inst'] if cfg else None
        base_subst = dict(self.cfg.NS_SUBST.get(d.ns, {}))
        if cfg:
            base_subst.update(cfg.get('subst', {}))
        rw = cxxdecl.Rewriter(base_subst, members, inst, lookup, self.cfg.EXPO, d.ns)
        if d.cls:
            rw.cls_name = re.sub(r'<.*', '', d.cls[-1].split('::')[-1]).strip('@')
            members.discard(rw.cls_name)
        out = []
        if k in ('alias', 'class', 'enum', 'concept', 'variable', 'namespace-alias', 'using-decl'):
            if d.cls:
                return self.member_name(sec, d, cfg, rw)
            return self.ns_name(sec, d, rw)
        if k in ('class-spec',):
            return self.class_spec(sec, d, rw, cfg)
        if k == 'variable-spec':
            return self.var_spec(sec, d, rw)
        if k in ('function', 'qualified-function', 'conversion', 'constructor', 'destructor', 'deduction-guide'):
            f = cxxdecl.parse(d)
            if f is None:
                self.record(sec, d, [], 'no check: not parsed')
                return []
            if k == 'constructor':
                return self.ctor(sec, d, f, cfg, rw)
            if k == 'destructor':
                i = self.record(sec, d, ['noexcept'])
                return [f'static_assert(std::is_nothrow_destructible_v<{inst}>); // @{i} noexcept']
            if k == 'deduction-guide':
                return self.guide(sec, d, f, rw)
            if k == 'conversion':
                return self.conversion(sec, d, f, cfg, rw)
            return self.function(sec, d, f, cfg, rw, friend)
        self.record(sec, d, [], 'no check: kind ' + k)
        return []

    # names ------------------------------------------------------------------------------------
    def ns_name(self, sec, d, rw):
        if d.name in ('', '{') or d.name.startswith('@'):
            self.record(sec, d, [], 'no check: unnamed')
            return []
        i = self.record(sec, d, ['name'])
        if d.kind == 'namespace-alias':
            return [f'namespace {i} = {d.ns}::{d.name}; // @{i} name']
        lines = [f'namespace {i} {{ using {d.ns}::{d.name}; }} // @{i} name']
        if d.kind == 'alias':
            lines += self.alias_value(i, d, rw, f'{d.ns}::{d.name}')
        return lines

    def alias_value(self, i, d, rw, spelled):
        body = decls.strip_template_heads(d.toks)
        heads = cxxdecl.template_heads(d.toks)
        if heads:
            return []      # an alias template: its value needs arguments
        if len(body) < 4 or body[2].text != '=':
            return []
        rhs = body[3:]
        try:
            want = rw.type(rhs)
        except Unresolved:
            return []
        self.rows[-1][6] += ',type'
        return [f'static_assert(std::is_same_v<{spelled}, {want}>); // @{i} type']

    def member_name(self, sec, d, cfg, rw):
        inst = cfg['inst']
        if d.name in ('', '{') or d.name.startswith('@'):
            self.record(sec, d, [], 'no check: unnamed')
            return []
        i = self.record(sec, d, ['name'])
        if cfg.get('final') or cfg.get('nonclass'):
            line = f'static_assert(requires {{ typename {inst}::{d.name}; }}); // @{i} name' if d.kind in ('alias', 'class') \
                else f'static_assert(requires {{ &{inst}::{d.name}; }}); // @{i} name'
            lines = [line]
        else:
            lines = [f'struct {i} : {inst} {{ using {inst}::{d.name}; }}; // @{i} name']
        if d.kind == 'alias':
            lines += self.alias_value(i, d, rw, f'{inst}::{d.name}')
        return lines

    def class_spec(self, sec, d, rw, cfg):
        f_heads = cxxdecl.template_heads(d.toks)
        body = decls.strip_template_heads(d.toks)
        # struct name<args>
        j = 1
        name = []
        while j < len(body) and body[j].text not in ('{', ':', 'final'):
            name.append(body[j])
            j += 1
        subst = dict(self.cfg.NS_SUBST.get(d.ns, {}))
        if cfg:
            subst.update(cfg.get('subst', {}))
        for h in f_heads:
            for tp in h:
                if tp.name and tp.name not in subst:
                    v = self.cfg.default_targ(tp, None)
                    if v is not None:
                        subst[tp.name] = v
        subst.update(self.cfg.SPEC_SUBST.get(' '.join(d.text.split()), {}))
        rw2 = cxxdecl.Rewriter(subst, rw.members, rw.inst, rw.lookup, rw.expo, rw.ns)
        try:
            t = rw2.type([x for x in name])
        except Unresolved as e:
            self.record(sec, d, [], f'no check: {e}')
            return []
        if not t.startswith('std::'):
            t = d.ns + '::' + t
        i = self.record(sec, d, ['spec'])
        return [f'static_assert(sizeof({t}) > 0); // @{i} spec']

    def var_spec(self, sec, d, rw):
        body = decls.strip_template_heads(d.toks)
        eq = decls.top_level_index(body, '=')
        if eq < 0:
            self.record(sec, d, [], 'no check: no value')
            return []
        lhs = body[:eq]
        # drop specifiers and the type
        k = len(lhs) - 1
        depth = 0
        while k >= 0:
            if lhs[k].text == '>':
                depth += 1
            elif lhs[k].text == '<':
                depth -= 1
                if depth == 0:
                    break
            k -= 1
        start = k - 1
        while start >= 2 and lhs[start - 1].text == '::':
            start -= 2
        subst = dict(self.cfg.NS_SUBST.get(d.ns, {}))
        for h in cxxdecl.template_heads(d.toks):
            for tp in h:
                if tp.name and tp.name not in subst:
                    v = self.cfg.default_targ(tp, None)
                    if v is not None:
                        subst[tp.name] = v
        subst.update(self.cfg.SPEC_SUBST.get(' '.join(d.text.split()), {}))
        rw2 = cxxdecl.Rewriter(subst, rw.members, rw.inst, rw.lookup, rw.expo, rw.ns)
        try:
            v = rw2.type(lhs[start:])
            val = rw2.type(body[eq + 1:])
        except Unresolved as e:
            self.record(sec, d, [], f'no check: {e}')
            return []
        if not v.startswith('std::'):
            v = d.ns + '::' + v
        i = self.record(sec, d, ['spec'])
        return [f'static_assert({v} == {val}); // @{i} spec']

    # functions --------------------------------------------------------------------------------
    def args_of(self, f, rw, upto=None):
        """The argument expressions and the parameter types of a call."""
        args, types = [], []
        params = f.params if upto is None else f.params[:upto]
        for p in params:
            if p.pack:
                names = [t.text for t in p.type if t.kind in ('id', 'expo')]
                packv = None
                for n in names:
                    v = rw.subst.get(n, rw.subst.get('@' + n + '@'))
                    if isinstance(v, list):
                        packv = v
                        break
                if packv is None:
                    raise Unresolved('pack')
                for k in range(len(packv)):
                    t = rw.type(p.type, k)
                    args.append(f'std::declval<{t}>()')
                    types.append(t)
                continue
            t = rw.type(p.type)
            args.append(f'std::declval<{t}>()')
            types.append(t)
        return args, types

    def explicit_targs(self, f, rw):
        """Explicit template arguments for template parameters no parameter type deduces."""
        if not f.tparams:
            return ''
        used = set()
        for p in f.params:
            used |= {t.text for t in p.type if t.kind in ('id', 'expo')}
        need = -1
        for k, tp in enumerate(f.tparams):
            if tp.name and tp.name not in used and tp.default is None and not tp.pack:
                need = k
        if need < 0:
            return ''
        vals = []
        for tp in f.tparams[:need + 1]:
            v = rw.subst.get(tp.name)
            if v is None:
                raise Unresolved(tp.name or '?')
            vals.append(', '.join(v) if isinstance(v, list) else v)
        return '<' + ', '.join(vals) + '>'

    def min_params(self, f):
        n = len(f.params)
        while n and f.params[n - 1].default is not None:
            n -= 1
        return n

    def function(self, sec, d, f, cfg, rw, friend):
        rw.subst = self.subst_for(f, rw.subst)
        inst = cfg['inst'] if cfg else None
        static = 'static' in f.specs
        friend = friend or f.friend or 'friend' in f.specs
        try:
            targs = self.explicit_targs(f, rw)
            args, types = self.args_of(f, rw)
        except Unresolved as e:
            return self.presence_only(sec, d, f, cfg, f'parameter {e}')
        if f.deleted:
            i = self.record(sec, d, ['deleted'])
            expr = self.call_expr(d, f, cfg, rw, targs, args, friend, static)
            return [f'static_assert(!requires {{ {expr}; }}); // @{i} deleted']
        aspects = ['call']
        expr = self.call_expr(d, f, cfg, rw, targs, args, friend, static)
        lines_after = []
        ret = None
        rtoks = f.trailing if f.trailing is not None else [t for t in f.ret if t.text not in cxxdecl.SPECIFIERS and t.text != 'typename']
        if rtoks and not any(t.text in ('auto', 'decltype') for t in rtoks) and not any(t.kind == 'expo' and t.text not in self.cfg.EXPO and '@' + t.text + '@' not in rw.subst for t in rtoks):
            try:
                ret = rw.type(rtoks)
            except Unresolved:
                ret = None
        i = self.record(sec, d, aspects)
        lines = [f'namespace {i} {{ using t = decltype({expr}); }} // @{i} call']
        if ret is not None:
            self.rows[-1][6] += ',ret'
            lines.append(f'static_assert(std::is_same_v<{i}::t, {ret}>); // @{i} ret')
        cond = self.noexcept_cond(f, rw)
        if cond is not None:
            self.rows[-1][6] += ',noexcept'
            lines.append(f'static_assert(!({cond}) || noexcept({expr})); // @{i} noexcept')
        # with the defaulted parameters left out
        m = self.min_params(f)
        if m < len(f.params) and not any(p.pack for p in f.params):
            try:
                a2, _ = self.args_of(f, rw, m)
                e2 = self.call_expr(d, f, cfg, rw, targs, a2, friend, static)
                self.rows[-1][6] += ',defaults'
                lines.append(f'namespace {i} {{ using t2 = decltype({e2}); }} // @{i} defaults')
            except Unresolved:
                pass
        return lines

    def noexcept_cond(self, f, rw):
        if f.noexcept is True:
            return 'true'
        if f.noexcept is None:
            return None
        try:
            return rw.type(f.noexcept)
        except Unresolved:
            return None

    def call_expr(self, d, f, cfg, rw, targs, args, friend, static):
        a = ', '.join(args)
        if friend or (not d.cls and f.operator):
            if f.operator and not friend:
                return f'{d.ns}::{f.name}({a})'
            return f'{f.name}({a})'
        if d.cls:
            inst = cfg['inst']
            if static:
                return f'{inst}::{f.name}{targs}({a})'
            cv = 'const ' if f.const else ''
            ref = '&&' if f.ref == '&&' else '&'
            obj = f'std::declval<{cv}{inst}{ref}>()'
            tmpl = 'template ' if targs else ''
            return f'{obj}.{tmpl}{f.name}{targs}({a})'
        q = d.ns
        if f.qualifier:
            q = q + '::' + '::'.join(f.qualifier)
        return f'{q}::{f.name}{targs}({a})'

    def presence_only(self, sec, d, f, cfg, why):
        if d.cls and not f.operator and not d.kind.startswith('friend') and not cfg.get('final') and not cfg.get('nonclass') and not f.name.startswith('~'):
            inst = cfg['inst']
            i = self.record(sec, d, ['name'], f'presence only: {why}')
            return [f'struct {i} : {inst} {{ using {inst}::{f.name}; }}; // @{i} name']
        if not d.cls and not f.operator:
            i = self.record(sec, d, ['name'], f'presence only: {why}')
            q = d.ns + ('::' + '::'.join(f.qualifier) if f.qualifier else '')
            return [f'namespace {i} {{ using {q}::{f.name}; }} // @{i} name']
        self.record(sec, d, [], f'no check: {why}')
        return []

    def ctor(self, sec, d, f, cfg, rw):
        rw.subst = self.subst_for(f, rw.subst)
        inst = cfg['inst']
        try:
            args, types = self.args_of(f, rw)
        except Unresolved as e:
            self.record(sec, d, [], f'no check: parameter {e}')
            return []
        if f.deleted:
            i = self.record(sec, d, ['deleted'])
            return [f'static_assert(!std::is_constructible_v<{", ".join([inst] + types)}>); // @{i} deleted']
        i = self.record(sec, d, ['call'])
        tl = ', '.join([inst] + types)
        lines = [f'static_assert(std::is_constructible_v<{tl}>); // @{i} call']
        cond = self.noexcept_cond(f, rw)
        if cond is not None:
            self.rows[-1][6] += ',noexcept'
            lines.append(f'static_assert(!({cond}) || std::is_nothrow_constructible_v<{tl}>); // @{i} noexcept')
        m = self.min_params(f)
        if m < len(f.params):
            try:
                _, t2 = self.args_of(f, rw, m)
                self.rows[-1][6] += ',defaults'
                lines.append(f'static_assert(std::is_constructible_v<{", ".join([inst] + t2)}>); // @{i} defaults')
                if 'explicit' not in f.specs and len(t2) == 1:
                    self.rows[-1][6] += ',implicit'
                    lines.append(f'static_assert(std::is_convertible_v<{t2[0]}, {inst}>); // @{i} implicit')
                if 'explicit' in f.specs and 'explicit(cond)' not in f.specs and len(t2) == 1:
                    self.rows[-1][6] += ',explicit'
                    lines.append(f'static_assert(!std::is_convertible_v<{t2[0]}, {inst}>); // @{i} explicit')
            except Unresolved:
                pass
        if len(types) == 1 and m == 1 and len(f.params) == 1:
            if 'explicit' not in f.specs:
                self.rows[-1][6] += ',implicit'
                lines.append(f'static_assert(std::is_convertible_v<{types[0]}, {inst}>); // @{i} implicit')
            elif 'explicit(cond)' not in f.specs:
                self.rows[-1][6] += ',explicit'
                lines.append(f'static_assert(!std::is_convertible_v<{types[0]}, {inst}>); // @{i} explicit')
        return lines

    def guide(self, sec, d, f, rw):
        cfg = self.cfg.CLASSES.get(f.name) or {}
        subst = dict(self.cfg.NS_SUBST.get(d.ns, {}))
        subst.update(cfg.get('guide_subst', cfg.get('subst', {})))
        subst.update(self.cfg.SPEC_SUBST.get(' '.join(d.text.split()), {}))
        rw2 = cxxdecl.Rewriter(subst, set(), None, rw.lookup, rw.expo, rw.ns)
        rw2.subst = self.subst_for(f, rw2.subst)
        rw2.subst.update(self.cfg.SPEC_SUBST.get(' '.join(d.text.split()), {}))
        try:
            args, types = self.args_of(f, rw2)
            want = rw2.type(f.trailing)
        except Unresolved as e:
            self.record(sec, d, [], f'no check: {e}')
            return []
        if not want.startswith('std::'):
            want = d.ns + '::' + want
        i = self.record(sec, d, ['guide'])
        return [f'static_assert(std::is_same_v<decltype({d.ns}::{f.name}({", ".join(args)})), {want}>); // @{i} guide']

    def conversion(self, sec, d, f, cfg, rw):
        if f.tparams:
            self.record(sec, d, [], 'no check: conversion template')
            return []
        inst = cfg['inst']
        try:
            to = rw.type(cxxdecl.decls.tokenize(f.name[len('operator '):]))
        except Unresolved as e:
            self.record(sec, d, [], f'no check: {e}')
            return []
        cv = 'const ' if f.const else ''
        ref = '&&' if f.ref == '&&' else '&'
        i = self.record(sec, d, ['call'])
        lines = [f'namespace {i} {{ using t = decltype(static_cast<{to}>(std::declval<{cv}{inst}{ref}>())); }} // @{i} call']
        if 'explicit' in f.specs:
            self.rows[-1][6] += ',explicit'
            lines.append(f'static_assert(!std::is_convertible_v<{cv}{inst}{ref}, {to}>); // @{i} explicit')
        cond = self.noexcept_cond(f, rw)
        if cond is not None:
            self.rows[-1][6] += ',noexcept'
            lines.append(f'static_assert(!({cond}) || noexcept(static_cast<{to}>(std::declval<{cv}{inst}{ref}>()))); // @{i} noexcept')
        return lines


def version_probes(g, cfg, blocks, outdir):
    """[version.syn]: each feature-test macro of the part's headers, with its value, in each of
    those headers and in <version> (aspect `macro`)."""
    text = '\n'.join(b['text'] for b in blocks if b['kind'] == 'code')
    macros = []
    for m in re.finditer(r'#define\s+(__cpp_lib_\w+)\s+(\d+L)((?:\s*//[^\n]*)*)', text):
        hdrs = re.findall(r'<(\w+)>', m.group(3))
        macros.append((m.group(1), m.group(2), hdrs, 'freestanding' in m.group(3)))
    per_header = {}
    for name, val, hdrs, fs in macros:
        mine = [h for h in hdrs if h in cfg.MACRO_HEADERS]
        if not mine:
            continue
        for h in mine + ['version']:
            i = g.new_id()
            g.rows.append([i, 'version.syn', 'macro', name, f'#define {name} {val} // in <{h}>',
                           'freestanding' if fs else '', 'macro', ''])
            per_header.setdefault(h, []).append((i, name, val))
    for h, ms in per_header.items():
        lines = [f'// Generated by tools/spec_audit/gen_probes.py; do not edit. [version.syn] macros of <{h}>.',
                 f'#include <{h}>']
        for i, name, val in ms:
            lines += [f'#if !defined({name})', f'#error "{name} is not defined" // @{i} macro',
                      f'#elif {name} != {val}', f'#error "{name} is not {val}" // @{i} macro', '#endif']
        with open(os.path.join(outdir, f'version.{h}.cpp'), 'w', encoding='utf-8') as o:
            o.write('\n'.join(lines) + '\n')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--part', required=True)
    ap.add_argument('blocks')
    ap.add_argument('--version-json', help="extract_draft.py's output for [version.syn]")
    a = ap.parse_args()
    cfg = importlib.import_module(a.part + '_config')
    blocks = json.load(open(a.blocks, encoding='utf-8'))
    ents = inventory.entities(blocks, cfg.CLAUSES)
    g = Gen(cfg, ents)
    g.header_includes = {}
    outdir = os.path.join(HERE, a.part, 'probes')
    g.run(outdir)
    cfg.header_probes(g, ents, outdir)
    if a.version_json:
        version_probes(g, cfg, json.load(open(a.version_json, encoding='utf-8')), outdir)
    with open(os.path.join(HERE, a.part, 'entities.tsv'), 'w', encoding='utf-8') as o:
        o.write('# id\tsubclause\tkind\tentity\tdeclaration\tdraft comment\tchecks\tnote\n')
        for r in g.rows:
            o.write('\t'.join(r) + '\n')
    print(len(g.rows), 'entities,', sum(1 for r in g.rows if r[6]), 'checked')


if __name__ == '__main__':
    main()
