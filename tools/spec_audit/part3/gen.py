#!/usr/bin/env python3
"""Generate the part-3 spec-audit probes (docs/SPEC_COVERAGE.md, part 3).

    tools/spec_audit/part3/gen.py [--regions cache/regions.json]

From the declarations of the synopses (inventory.py) it writes one probe file per subclause,
probes/<stable.name>.cpp, and checks.tsv (one line per check: id, subclause, header, entity, what
it checks, the declaration). A check is one line of C++ that is valid only when the library
declares the entity with the specified shape:

  presence   `using ns::name;` (namespace scope), `using C::name;` in a class derived from a
             sample specialization C (members), or the completeness of a specialization
  call       a call with arguments of the declared parameter types (template parameters and
             placeholders replaced by samples, samples.py) is valid, has the declared return
             type, is noexcept where declared so, is valid with the defaulted arguments left
             out, and is ill-formed for a deleted function
  ctor       is_constructible / is_nothrow_constructible / not convertible (explicit)
  type       a member type or alias names the declared type
  var        a variable or data member has the declared type
  base       a class derives publicly from the declared base
  enum       the enumerators exist; a scoped enumeration does not convert to int
  guide      class template argument deduction gives the declared type

Each check sits inside a concept over a dummy type Z, so a failed one is a false static_assert on
its own line (the runner maps diagnostics to lines). What cannot be probed (exposition-only types
in a signature, `see below`) gets a presence check only; `constexpr` is not probed here.
"""
import argparse, collections, json, pathlib, re, sys
sys.path.insert(0, str(pathlib.Path(__file__).parent))
import decls as D
import inventory as INV
import samples as SMP

HERE = pathlib.Path(__file__).parent


class Unprobeable(Exception):
    pass


IDENT = re.compile(r"[A-Za-z_]\w*$")


def ren(toks):
    """Tokens back to C++ (italics must have been substituted)."""
    out = []
    prev = ""
    for t in toks:
        t = str(t)
        if out and (re.match(r"\w", t) and re.search(r"\w$", prev)):
            out.append(" ")
        elif out and prev == ">" and t == ">":
            out.append(" ")
        elif out and prev == "<" and t.startswith("::"):
            out.append(" ")
        out.append(t)
        prev = t
    return "".join(out)


class Ctx:
    """What names mean in a declaration: template parameters and member types."""
    def __init__(self, env, members=None, selfname=None, selftype=None):
        self.env = dict(env)               # identifier -> replacement text
        self.members = members or {}       # member type name -> qualified text
        self.selfname, self.selftype = selfname, selftype


def subst(toks, ctx, allow_italic=()):
    out = []
    n = len(toks)
    i = 0
    while i < n:
        t = toks[i]
        prev = toks[i - 1] if i else ""
        if t.startswith("\x04"):
            i += 1
            continue
        if D.is_italic(t):
            name = D.italic_text(t)
            if name in ctx.env:
                out.append(ctx.env[name])
                i += 1
                continue
            if name == "deduced-vec-t" and i + 1 < n and toks[i + 1] == "<":
                # deduced-vec-t<V> is V for a vec sample
                j = D.match_angle(toks, i + 1)
                out.append(subst(toks[i + 2:j - 1], ctx))
                i = j
                continue
            raise Unprobeable(f"exposition-only or unspecified: {name}")
        if IDENT.match(t) and prev not in ("::", ".", "->"):
            if t in ctx.env:
                out.append(ctx.env[t])
                i += 1
                continue
            if t == ctx.selfname and not (i + 1 < n and toks[i + 1] == "<"):
                out.append(ctx.selftype)
                i += 1
                continue
            if t in ctx.members:
                out.append(ctx.members[t])
                i += 1
                continue
        if t in ("auto", "decltype"):
            raise Unprobeable("deduced type")
        out.append(t)
        i += 1
    s = ren(out)
    return s


def has_italic(toks):
    return any(D.is_italic(t) for t in toks)


class Gen:
    def __init__(self, ents):
        self.ents = ents
        self.nsec = {}
        self.checks = []          # (id, sec, header, entity, what, decl, code, ns)
        self.classes = collections.defaultdict(list)   # (ns, path) -> [classdef Decl]
        self.members = collections.defaultdict(dict)   # (ns, path) -> {name: kind}
        for d in ents:
            if d.kind == "classdef":
                path = tuple(c[1] for c in d.classes) + (d.name,)
                self.classes[(d.ns, path)].append(d)
        for d in ents:
            if d.classes:
                path = tuple(c[1] for c in d.classes)
                if d.kind in ("alias", "classdef", "classdecl", "enumdef", "enumdecl") and d.name:
                    self.members[(d.ns, path)][d.name] = d.kind
                if d.kind == "using-decl" and d.name:
                    self.members[(d.ns, path)][d.name] = "using"

    # ---- samples for class scopes ---------------------------------------------------------
    def class_samples(self, d):
        """For a member declaration: the sample instantiations of its enclosing classes, as a
        list of (ctx, selftype text, classdef) alternatives (one per sample)."""
        alts = [(Ctx({}), None, None, "")]
        for k, sc in enumerate(d.classes):
            cdecl = sc[3]
            new = []
            for ctx, outer, _, label in alts:
                for env, label2 in self.instantiations(cdecl, ctx):
                    base = (outer + "::" if outer else (d.ns + "::")) + cdecl.name
                    c2 = Ctx(env)
                    if cdecl.info.get("args"):
                        try:
                            args = subst(cdecl.info["args"], c2)
                        except Unprobeable:
                            continue
                        t = base + args
                    else:
                        outer_n = len(cdecl.classes[-1][3].heads) if cdecl.classes else 0
                        own = cdecl.heads[outer_n:]
                        own_params = own[-1] if own else []
                        if own_params:
                            t = base + "<" + ", ".join(x for x in (env[p.name] for p in own_params if p.name) if x != "") + ">"
                        else:
                            t = base
                    # member types of the class (and of the enclosing classes)
                    mem = dict(ctx.members)
                    path = tuple(c[1] for c in d.classes[:k + 1])
                    for nm in self.inherited_members(d.ns, path):
                        mem[nm] = t + "::" + nm
                    c3 = Ctx(env, mem, cdecl.name, t)
                    new.append((c3, t, cdecl, (label + " " + label2).strip()))
            alts = new
        return alts

    def instantiations(self, cdecl, outer_ctx):
        """The sample environments for a class's own template parameters."""
        heads = cdecl.heads
        outer_n = len(cdecl.classes[-1][3].heads) if cdecl.classes else 0
        own = heads[outer_n:]
        envs = [(dict(outer_ctx.env), "")]
        for params in own:
            nenv = []
            for env, label in envs:
                for e2, l2 in SMP.params_env(params, cdecl, env):
                    nenv.append((e2, (label + " " + l2).strip()))
            envs = nenv
        # placeholders in a specialization's arguments (atomic<integral-type>)
        if cdecl.info.get("args") and has_italic(cdecl.info["args"]):
            nenv = []
            for env, label in envs:
                for e2, l2 in SMP.placeholder_envs(cdecl.info["args"], env):
                    nenv.append((e2, (label + " " + l2).strip()))
            envs = nenv
        return envs

    def inherited_members(self, ns, path, seen=None):
        """The member type names of a class and of its base classes (by name)."""
        seen = seen or set()
        if (ns, path) in seen:
            return {}
        seen.add((ns, path))
        out = dict(self.members.get((ns, path), {}))
        for cdecl in self.classes.get((ns, path), [])[:1]:
            for b in D.split_top(cdecl.info.get("bases") or []):
                names = [str(x) for x in b if re.match(r"[A-Za-z_]\w*$", x) and x not in ("public", "virtual", "protected", "private")]
                if names:
                    for k, v in self.inherited_members(ns, (names[0],), seen).items():
                        out.setdefault(k, v)
        return out

    def macros(self, data):
        """The macros a header synopsis defines: #define NAME."""
        hdrs = INV.section_headers(data)
        for s in data["sections"]:
            if s["id"] not in hdrs:
                continue
            for kind, text in s["regions"]:
                for m in re.finditer(r"(?m)^[ \t]*#define\s+([A-Za-z_]\w*)", text):
                    d = D.Decl([], [], [("ns", "", False)], "public", False, None)
                    d.sec, d.header, d.name, d.kind = s["id"], hdrs[s["id"]], m.group(1), "macro"
                    self.add(d, "macro", "#" + m.group(1), ent=m.group(1))

    # ---- emit -------------------------------------------------------------------------------
    def add(self, d, what, code, ent=None):
        self.nsec[d.sec] = self.nsec.get(d.sec, 0) + 1
        cid = f"{d.sec}#{self.nsec[d.sec]}"
        ent = ent or self.entity(d)
        self.checks.append((cid, d.sec, d.header, ent, what, d.text().replace("\t", " "), code, d.ns))

    def entity(self, d):
        path = "::".join(c[1] for c in d.classes)
        return f"{d.ns}::{path + '::' if path else ''}{d.name}"

    def run(self):
        for d in self.ents:
            if d.expos or not d.name and d.kind not in ("classdef",):
                continue
            if any(c[3] is None or c[3].expos for c in d.classes):
                continue
            if d.access == "private":
                continue
            if d.kind in ("empty", "static_assert", "using-directive", "using-enum", "friend-class"):
                continue
            try:
                getattr(self, "do_" + d.kind.replace("-", "_"))(d)
            except Unprobeable as e:
                self.presence(d, str(e))

    def presence(self, d, why=""):
        """A name-only check."""
        if d.classes:
            for ctx, selft, cdecl, label in self.class_samples(d)[:1]:
                if cdecl.info.get("final"):
                    return
                nm = d.name
                if d.kind == "function" and nm == cdecl.name:
                    nm = None
                if nm is None or nm.startswith("~") or nm.startswith("operator "):
                    return
                code = f"struct D : {selft} {{ using {selft}::{nm}; }};"
                self.add(d, "presence" + (f" ({why})" if why else ""), code)
            return
        if d.kind in ("guide",) or d.info.get("args"):
            return
        nm = d.name
        if nm.startswith("operator"):
            # operators are found by ADL; presence of a namespace-scope operator
            code = f"using {d.ns}::{nm};"
        else:
            code = f"using {d.ns}::{nm};"
        self.add(d, "presence" + (f" ({why})" if why else ""), code)

    # concepts over Z
    def req(self, body):
        return "requires { " + body + " }"

    def do_using_decl(self, d):
        self.presence(d)

    def do_concept(self, d):
        if d.classes:
            return
        self.presence(d)

    def do_alias(self, d):
        if d.classes:
            for ctx, selft, cdecl, label in self.class_samples(d):
                try:
                    tgt = subst(d.info["target"], ctx)
                    self.add(d, f"type {label}".strip(), f"static_assert(spec_probe::same<{selft}::{d.name}, {tgt}>);")
                except Unprobeable as e:
                    self.add(d, f"type exists {label} ({e})".strip(), f"using T = {selft}::{d.name};")
            return
        heads = d.heads
        if heads:
            for env, label in SMP.heads_env(heads, d):
                ctx = Ctx(env)
                args = "<" + ", ".join(SMP.arg_text(p, env) for p in heads[-1]) + ">"
                try:
                    tgt = subst(d.info["target"], ctx)
                    self.add(d, f"type {label}".strip(), f"static_assert(spec_probe::same<{d.ns}::{d.name}{args}, {tgt}>);")
                except Unprobeable as e:
                    self.add(d, f"type exists {label} ({e})".strip(), f"using T = {d.ns}::{d.name}{args};")
            return
        try:
            tgt = subst(d.info["target"], Ctx({}))
            self.add(d, "type", f"static_assert(spec_probe::same<{d.ns}::{d.name}, {tgt}>);")
        except Unprobeable as e:
            self.add(d, f"type exists ({e})", f"using T = {d.ns}::{d.name};")

    def do_variable(self, d):
        typ = d.info["type"]
        specs = d.info["specs"]
        names = [d.name]
        if d.classes:
            for ctx, selft, cdecl, label in self.class_samples(d):
                try:
                    t = subst(typ, ctx)
                    if "constexpr" in specs and not t.startswith("const "):
                        t = "const " + t
                    self.add(d, f"var {label}".strip(), f"static_assert(spec_probe::same<decltype({selft}::{d.name}), {t}>);")
                except Unprobeable as e:
                    self.add(d, f"var exists {label} ({e})".strip(), f"using T = decltype({selft}::{d.name});")
            return
        if d.info.get("args"):
            # a partial specialization of a variable template: checked through the primary
            return
        if d.heads:
            for env, label in SMP.heads_env(d.heads, d):
                args = "<" + ", ".join(SMP.arg_text(p, env) for p in d.heads[-1]) + ">"
                try:
                    t = subst(typ, Ctx(env))
                    if "constexpr" in specs and not t.startswith("const "):
                        t = "const " + t
                    self.add(d, f"var {label}".strip(), f"static_assert(spec_probe::same<decltype({d.ns}::{d.name}{args}), {t}>);")
                except Unprobeable as e:
                    self.add(d, f"var exists {label} ({e})".strip(), f"using T = decltype({d.ns}::{d.name}{args});")
            return
        try:
            t = subst(typ, Ctx({}))
            if "constexpr" in specs and not t.startswith("const "):
                t = "const " + t
            self.add(d, "var", f"static_assert(spec_probe::same<decltype({d.ns}::{d.name}), {t}>);")
        except Unprobeable as e:
            self.add(d, f"var exists ({e})", f"using T = decltype({d.ns}::{d.name});")

    def do_enumdef(self, d):
        if d.classes:
            alts = self.class_samples(d)[:1]
            if not alts:
                return
            ctx, selft, cdecl, label = alts[0]
            q = selft + "::" + d.name
            scope_unscoped = selft
        else:
            q = d.ns + "::" + d.name
            scope_unscoped = d.ns
        if d.info.get("scoped"):
            self.add(d, "enum scoped", f"static_assert(std::is_enum_v<{q}> && !std::is_convertible_v<{q}, int>);")
        else:
            self.add(d, "enum", f"static_assert(std::is_enum_v<{q}>);")
        if d.info.get("underlying"):
            try:
                u = subst(d.info["underlying"], Ctx({}))
                self.add(d, "enum underlying type", f"static_assert(spec_probe::same<std::underlying_type_t<{q}>, {u}>);")
            except Unprobeable:
                pass
        for e in d.info.get("enumerators", []):
            if D.is_italic(e):
                continue
            if d.info.get("scoped"):
                self.add(d, f"enumerator {e}", f"static_assert(spec_probe::same<decltype({q}::{e}), {q}>);", ent=f"{q}::{e}")
            else:
                self.add(d, f"enumerator {e}", f"static_assert(spec_probe::same<decltype({scope_unscoped}::{e}), {q}>);", ent=f"{q}::{e}")

    def do_enumdecl(self, d):
        self.presence(d)

    def do_classdecl(self, d):
        args = d.info.get("args")
        if args is None:
            if not d.classes:
                self.presence(d)
            return
        self.specialization(d, args)

    def specialization(self, d, args):
        """A (partial or explicit) specialization: it is complete for a sample."""
        for env, label in SMP.heads_env(d.heads, d) if d.heads and d.heads[-1] else [({}, "")]:
            for env2, label2 in (SMP.placeholder_envs(args, env) if has_italic(args) else [(env, "")]):
                try:
                    a = subst(args, Ctx(env2))
                except Unprobeable as e:
                    continue
                prefix = d.ns + "::"
                if d.classes:
                    continue
                t = prefix + d.name + a
                lab = (label + " " + label2).strip()
                if d.name in ("formatter", "hash"):
                    self.add(d, f"specialization enabled {lab}".strip(),
                             f"static_assert(std::is_default_constructible_v<{t}>);", ent=t)
                else:
                    self.add(d, f"specialization complete {lab}".strip(),
                             f"template<class Z> concept c = requires {{ sizeof(spec_probe::dep<Z, {t}>); }}; static_assert(c<void>);", ent=t)

    def do_classdef(self, d):
        if d.info.get("args") is not None:
            if not d.classes:
                self.specialization(d, d.info["args"])
            return
        if d.classes:
            self.presence(d)
        else:
            self.add(d, "presence", f"using {d.ns}::{d.name};")
        # base classes and final, for the sample
        bases = d.info.get("bases")
        fake = D.Decl([], d.heads, d.scope + [("class", d.name, None, d)], "public", False, None)
        fake.sec, fake.header, fake.name, fake.kind = d.sec, d.header, d.name, "class"
        for ctx, selft, cdecl, label in self.class_samples(fake)[:2]:
            if d.info.get("final"):
                self.add(d, f"final {label}".strip(), f"static_assert(std::is_final_v<{selft}>);")
            if bases:
                for b in D.split_top(bases):
                    b = [x for x in b if x not in ("public", "virtual", "protected", "private")]
                    if not b or "private" in bases or "protected" in bases:
                        continue
                    try:
                        bt = subst(b, ctx)
                    except Unprobeable:
                        continue
                    self.add(d, f"base {bt} {label}".strip(),
                             f"static_assert(std::is_base_of_v<{bt}, {selft}> && std::is_convertible_v<{selft}*, {bt}*>);")

    def do_guide(self, d):
        params = d.info["params"]
        for env, label in SMP.heads_env(d.heads, d) if d.heads else [({}, "")]:
            ctx = Ctx(env)
            try:
                args = [self.arg(p, ctx) for p in params]
                res = subst(d.info["trailing"], ctx)
            except Unprobeable as e:
                continue
            if args is None:
                continue
            call = f"{d.ns}::{d.name}({', '.join(a for a in args if a)})"
            self.add(d, f"deduction guide {label}".strip(),
                     f"template<class Z> concept c = requires {{ requires spec_probe::same<decltype({call}), {res}>; }}; static_assert(c<void>);")

    def arg(self, p, ctx, z=True):
        t = subst(p["type"], ctx)
        if p["pack"]:
            # a pack: the sample pack is one element (or none for an empty sample)
            if t.strip() == "":
                return ""
        return f"spec_probe::dv<spec_probe::dep<Z, {t}>>()"

    def do_function(self, d):
        if d.name.startswith("~"):
            return self.destructor(d)
        if d.classes and d.name == d.classes[-1][1]:
            return self.constructor(d)
        if d.info.get("pure"):
            pass
        if d.classes and "friend" not in d.info["specs"]:
            return self.member_function(d)
        return self.free_function(d)

    def calls(self, d, ctx, own_params):
        """Argument lists: all parameters, and without the defaulted ones."""
        params = d.info["params"]
        full = []
        for p in params:
            full.append((p, self.arg(p, ctx)))
        out = [[a for p, a in full]]
        k = len(params)
        while k > 0 and params[k - 1]["default"] is not None:
            k -= 1
        if k < len(params):
            out.append([a for p, a in full[:k]])
        return out

    def ret_check(self, d, ctx):
        r = d.info["ret"]
        if d.info.get("trailing"):
            r = d.info["trailing"]
        r = [x for x in r if x not in ("typename",)]
        if not r or "auto" in r or "decltype" in r or has_italic(r):
            # deduced-vec-t<V> is V for the samples
            if r and has_italic(r) and all(not D.is_italic(x) or D.italic_text(x) == "deduced-vec-t" or D.italic_text(x) in ctx.env for x in r):
                try:
                    return subst(r, ctx)
                except Unprobeable:
                    return None
            return None
        try:
            return subst(r, ctx)
        except Unprobeable:
            return None

    def explicit_targs(self, d, own, env, params_text):
        """Explicit template arguments: the leading own template parameters up to the last one
        that the parameter types do not mention."""
        if not own:
            return ""
        used = set(re.findall(r"[A-Za-z_]\w*", " ".join(" ".join(map(str, p["type"])) for p in d.info["params"])))
        last = -1
        for k, p in enumerate(own):
            if p.name and p.name not in used and not (p.default is not None) and not p.pack:
                last = k
        if last < 0:
            return ""
        return "<" + ", ".join(SMP.arg_text(p, env) for p in own[:last + 1]) + ">"

    def call_checks(self, d, ctx, callee_fn, label, own, env):
        """callee_fn(args, targs) -> expression text."""
        rt = self.ret_check(d, ctx)
        targs = self.explicit_targs(d, own, env, None)
        variants = self.calls(d, ctx, own)
        for vi, args in enumerate(variants):
            expr = callee_fn(args, targs)
            what = "call" + (" (defaults)" if vi else "") + (f" {label}" if label else "")
            if d.info["deleted"]:
                if vi:
                    continue
                self.add(d, "deleted " + what, f"template<class Z> concept c = !requires {{ {expr}; }}; static_assert(c<void>);")
                continue
            parts = [f"{expr};"]
            if rt is not None and vi == 0:
                parts = [f"{{ {expr} }} -> spec_probe::same<{rt}>;"]
            if d.info["noexcept"] is True:
                parts.append(f"requires noexcept({expr});")
            self.add(d, what + (" ret" if rt is not None and vi == 0 else "") + (" noexcept" if d.info["noexcept"] is True else ""),
                     f"template<class Z> concept c = requires {{ {' '.join(parts)} }}; static_assert(c<void>);")

    def free_function(self, d):
        own = d.heads[-1] if d.heads and not d.classes else []
        if d.classes:
            # a friend function: the enclosing class's sample; its own template head if any
            alts = self.class_samples(d)
            outer_n = len(d.classes[-1][3].heads) if d.classes[-1][3] else 0
            own = d.heads[outer_n] if len(d.heads) > outer_n else []
        else:
            alts = [(Ctx({}), None, None, "")]
        for ctx0, selft, cdecl, label0 in alts:
            for env, label in (SMP.params_env(own, d, ctx0.env) if own else [(dict(ctx0.env), "")]):
                ctx = Ctx(env, ctx0.members, ctx0.selfname, ctx0.selftype)
                for env2, label2 in (SMP.placeholder_envs(d.toks2, env) if has_italic(d.toks2) else [(env, "")]):
                    ctx2 = Ctx(env2, ctx.members, ctx.selfname, ctx.selftype)
                    lab = " ".join(x for x in (label0, label, label2) if x)
                    friend = "friend" in d.info["specs"]
                    name = d.name
                    def callee(args, targs, name=name, friend=friend, d=d):
                        if name.startswith("operator") and not name.startswith("operator\"\"") and name not in ("operator new", "operator delete"):
                            op = name[len("operator"):]
                            if op in ("()", "[]"):
                                return f"{args[0]}{op[0]}{', '.join(args[1:])}{op[1]}"
                            if len(args) == 2:
                                return f"({args[0]} {op} {args[1]})"
                            if len(args) == 1:
                                return f"({op}{args[0]})"
                            raise Unprobeable("operator arity")
                        if name.startswith("operator\"\""):
                            return f"{d.ns}::{name}({', '.join(args)})"
                        q = name if friend else f"{d.ns}::{name}"
                        return f"{q}{targs}({', '.join(args)})"
                    self.call_checks(d, ctx2, callee, lab, own, env2)

    def member_function(self, d):
        cls = d.classes[-1][3]
        outer_n = len(cls.heads) if cls else 0
        own = d.heads[outer_n] if len(d.heads) > outer_n else []
        static = "static" in d.info["specs"]
        quals = d.info["quals"]
        for ctx0, selft, cdecl, label0 in self.class_samples(d):
            for env, label in (SMP.params_env(own, d, ctx0.env) if own else [(dict(ctx0.env), "")]):
                for env2, label2 in (SMP.placeholder_envs(d.toks2, env) if has_italic(d.toks2) else [(env, "")]):
                    ctx = Ctx(env2, ctx0.members, ctx0.selfname, ctx0.selftype)
                    lab = " ".join(x for x in (label0, label, label2) if x)
                    cv = " ".join(q for q in quals if q in ("const", "volatile"))
                    ref = "&&" if "&&" in quals else "&"
                    objt = f"{cv + ' ' if cv else ''}{selft}{ref}"
                    protected = d.access == "protected"
                    if protected:
                        if cdecl.info.get("final"):
                            continue
                        objt = f"{cv + ' ' if cv else ''}D{ref}"
                    obj = f"spec_probe::dv<spec_probe::dep<Z, {objt}>>()"
                    name = d.name

                    def callee(args, targs, name=name, obj=obj, static=static, selft=selft, protected=protected):
                        if "conv_type" in d.info:
                            # conversion function
                            t = subst(d.info["conv_type"], ctx)
                            return f"static_cast<{t}>({obj})"
                        if name == "operator()":
                            return f"{obj}({', '.join(args)})"
                        if static:
                            q = "D" if protected else selft
                            return f"{q}::{name}{(' template ' + '') if False else ''}{targs}({', '.join(args)})"
                        tk = "template " if targs else ""
                        return f"{obj}.{tk}{name}{targs}({', '.join(args)})"
                    before = len(self.checks)
                    self.call_checks(d, ctx, callee, lab, own, env2)
                    if protected:
                        # wrap: the concept inside a class derived from the sample
                        for k in range(before, len(self.checks)):
                            c = list(self.checks[k])
                            code = c[6].replace("template<class Z> concept c =", "template<class Z> static constexpr bool c =")
                            code = code.replace(" static_assert(c<void>);", "")
                            c[6] = f"struct D : {selft} {{ {code} }}; static_assert(D::c<void>);"
                            self.checks[k] = tuple(c)
                    if "conv_type" in d.info and "explicit" in d.info["specs"]:
                        try:
                            t = subst(d.info["conv_type"], ctx)
                            self.add(d, f"explicit {lab}".strip(), f"static_assert(!std::is_convertible_v<{objt}, {t}>);")
                        except Unprobeable:
                            pass

    def constructor(self, d):
        cls = d.classes[-1][3]
        outer_n = len(cls.heads) if cls else 0
        own = d.heads[outer_n] if len(d.heads) > outer_n else []
        if d.access == "protected":
            return
        for ctx0, selft, cdecl, label0 in self.class_samples(d):
            for env, label in (SMP.params_env(own, d, ctx0.env) if own else [(dict(ctx0.env), "")]):
                for env2, label2 in (SMP.placeholder_envs(d.toks2, env) if has_italic(d.toks2) else [(env, "")]):
                    ctx = Ctx(env2, ctx0.members, ctx0.selfname, ctx0.selftype)
                    lab = " ".join(x for x in (label0, label, label2) if x)
                    params = d.info["params"]
                    try:
                        types = [subst(p["type"], ctx) for p in params]
                    except Unprobeable as e:
                        self.add(d, f"constructor exists {lab} ({e})", "")
                        continue
                    k = len(params)
                    while k > 0 and params[k - 1]["default"] is not None:
                        k -= 1
                    variants = [types] + ([types[:k]] if k < len(params) else [])
                    for vi, ts in enumerate(variants):
                        targs = ", ".join([selft] + ts)
                        what = "ctor" + (" (defaults)" if vi else "") + (f" {lab}" if lab else "")
                        if d.info["deleted"]:
                            if vi == 0:
                                self.add(d, "deleted " + what, f"static_assert(!std::is_constructible_v<{targs}>);")
                            continue
                        conds = [f"std::is_constructible_v<{targs}>"]
                        if d.info["noexcept"] is True:
                            conds.append(f"std::is_nothrow_constructible_v<{targs}>")
                        expl = "explicit" in d.info["specs"]
                        if len(ts) == 1 and vi == 0:
                            if expl:
                                conds.append(f"!std::is_convertible_v<{ts[0]}, {selft}>")
                            elif not any(str(x) == "explicit" for x in d.toks2):
                                conds.append(f"std::is_convertible_v<{ts[0]}, {selft}>")
                        self.add(d, what + (" noexcept" if d.info["noexcept"] is True else "") + (" explicit" if expl and len(ts) == 1 else ""),
                                 f"static_assert({' && '.join(conds)});")

    def destructor(self, d):
        if d.access != "public":
            return
        for ctx0, selft, cdecl, label0 in self.class_samples(d)[:1]:
            conds = [f"std::is_nothrow_destructible_v<{selft}>"]
            if "virtual" in d.info["specs"]:
                conds.append(f"std::has_virtual_destructor_v<{selft}>")
            if d.info["deleted"]:
                conds = [f"!std::is_destructible_v<{selft}>"]
            self.add(d, "destructor" + (" virtual" if "virtual" in d.info["specs"] else ""), f"static_assert({' && '.join(conds)});")


def namespaces_for(ns):
    parts = ns.split("::")
    out = []
    for k in range(1, len(parts) + 1):
        out.append("::".join(parts[:k]))
    return out


def write(gen, outdir, inline_ns):
    outdir.mkdir(parents=True, exist_ok=True)
    for f in outdir.glob("*.cpp"):
        f.unlink()
    by_sec = collections.OrderedDict()
    for c in gen.checks:
        by_sec.setdefault(c[1], []).append(c)
    for sec, cs in by_sec.items():
        headers = sorted({c[2] for c in cs if c[2]})
        lines = [f"// Spec-audit probes for [{sec}], generated by tools/spec_audit/part3/gen.py; do not edit.",
                 "// One check per line (see gen.py); the runner maps diagnostics to the check IDs below."]
        for h in headers:
            lines.append(f"#include <{h}>")
        lines.append('#include "../probe_support.hpp"')
        for k, c in enumerate(cs):
            cid, _, header, ent, what, decl, code, ns = c
            if not code:
                continue
            if code.startswith("#"):
                macro = code[1:]
                lines += [f"#ifndef {macro}", f"static_assert(false, \"{macro}\"); // {cid} {what}", "#endif"]
                continue
            uses = " ".join(f"using namespace {n};" for n in namespaces_for(ns) if n)
            lines.append(f"namespace p{k} {{ {uses} {code} }} // {cid} {what}")
        (outdir / f"{sec}.cpp").write_text("\n".join(lines) + "\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--regions", default=str(HERE / "cache" / "regions.json"))
    ap.add_argument("--out", default=str(HERE / "probes"))
    a = ap.parse_args()
    data = INV.load(a.regions)
    ents = INV.entities(data)
    g = Gen(ents)
    g.run()
    g.macros(data)
    write(g, pathlib.Path(a.out), None)
    with open(HERE / "checks.tsv", "w") as f:
        f.write("# id\tsubclause\theader\tentity\tcheck\tdeclaration\n")
        for c in g.checks:
            f.write("\t".join(c[:6]) + "\n")
    print(f"{len(g.checks)} checks in {len({c[1] for c in g.checks})} files")


if __name__ == "__main__":
    main()
