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
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2]))   # tools/headers.py

HERE = pathlib.Path(__file__).parent


# [zombie.names] Tables 38-41 (the names the draft lists; checked against the draft's text by
# gen.py --check-zombies)
ZOMBIE_STD = """auto_ptr auto_ptr_ref binary_function binary_negate bind1st bind2nd binder1st binder2nd
codecvt_mode codecvt_utf16 codecvt_utf8 codecvt_utf8_utf16 const_mem_fun1_ref_t const_mem_fun1_t
const_mem_fun_ref_t const_mem_fun_t consume_header declare_no_pointers declare_reachable generate_header
get_pointer_safety get_temporary_buffer get_unexpected gets is_literal_type is_literal_type_v istrstream
little_endian mem_fun1_ref_t mem_fun1_t mem_fun_ref_t mem_fun_ref mem_fun_t mem_fun not1 not2 ostrstream
pointer_safety pointer_to_binary_function pointer_to_unary_function ptr_fun random_shuffle
raw_storage_iterator result_of result_of_t return_temporary_buffer set_unexpected strstream strstreambuf
unary_function unary_negate uncaught_exception undeclare_no_pointers undeclare_reachable
unexpected_handler wbuffer_convert wstring_convert""".split()
ZOMBIE_MACROS = """argument_type first_argument_type io_state op open_mode preferred second_argument_type seek_dir
strict converted freeze from_bytes pcount stossc to_bytes""".split()
ZOMBIE_HEADERS = "ccomplex ciso646 codecvt cstdalign cstdbool ctgmath strstream".split()


# A macro of a subclause that names another header ([depr.c.macros]: <stdbool.h>).
MACRO_HEADER = {"__bool_true_false_are_defined": "stdbool.h"}


class Unprobeable(Exception):
    pass


IDENT = re.compile(r"[A-Za-z_]\w*$")


def ren(toks):
    """Tokens back to C++ (italics must have been substituted)."""
    out = []
    prev = ""
    for t in toks:
        t = str(t).replace("\x06", "")
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
        self.injected = {}                 # enclosing classes' injected-class-names


PACK = "\x06"
NS_NAMES = set()     # (namespace, name) declared at namespace scope by the synopses
CUR = {"ns": "std"}  # the namespace of the declaration being probed


def qualify(t):
    """A name the synopses declare in a namespace nested in std (chrono, pmr, execution, ...),
    seen from the declaration's namespace: its qualified name."""
    parts = CUR["ns"].split("::")
    for k in range(len(parts), 1, -1):
        ns = "::".join(parts[:k])
        if (ns, t) in NS_NAMES:
            return ns + "::" + t
    return None


def subst(toks, ctx, allow_italic=()):
    pack_seen = False
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
                v = ctx.env[t]
                if v.startswith(PACK):
                    pack_seen = True
                    v = v[1:]
                out.append(v)
                i += 1
                continue
            if t == ctx.selfname and not (i + 1 < n and toks[i + 1] == "<"):
                out.append(ctx.selftype)
                i += 1
                continue
            if t in ctx.injected and not (i + 1 < n and toks[i + 1] == "<"):
                out.append(ctx.injected[t])
                i += 1
                continue
            if t in ctx.members:
                out.append(ctx.members[t])
                i += 1
                continue
            q = qualify(t)
            if q:
                out.append(q)
                i += 1
                continue
        if t == "...":
            if pack_seen:
                i += 1
                continue
        if t in (",", "(", "<"):
            pack_seen = False
        if t in ("auto", "decltype"):
            raise Unprobeable("deduced type")
        out.append(t)
        i += 1
    s = ren(out)
    return s


def inj(new, old):
    new.injected = old.injected
    return new


def has_italic(toks):
    return any(D.is_italic(t) for t in toks)


class Gen:
    def __init__(self, ents):
        self.ents = ents
        self.nsec = {}
        self.inventory = []   # [decl id, subclause, header, entity, kind, checks, note, declaration]
        self.decl_id = None
        self.freestanding = set()   # IDs of the checks of freestanding declarations
        # headers whose synopsis is all freestanding (`// all freestanding`)
        syn = collections.defaultdict(set)
        for d in ents:
            if d.sec.endswith(".syn") and d.kind not in ("empty",) and not d.expos:
                syn[d.header].add(d.fs)
        self.fs_headers = {h for h, f in syn.items() if f == {"freestanding"}}
        self.fs_partial = {h for h, f in syn.items() if "freestanding" in f}
        self.checks = []          # (id, sec, header, entity, what, decl, code, ns)
        self.classes = collections.defaultdict(list)   # (ns, path) -> [classdef Decl]
        self.members = collections.defaultdict(dict)   # (ns, path) -> {name: kind}
        for d in ents:
            if d.kind == "classdef":
                path = tuple(c[1] for c in d.classes) + (d.name,)
                self.classes[(d.ns, path)].append(d)
        for d in ents:
            if not d.classes and d.name and not d.expos:
                NS_NAMES.add((d.ns, d.name))
        NS_NAMES.update({("std::pmr", "string"), ("std::pmr", "wstring")})
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
                over = SMP.MEMBER_CLASS.get((d.sec, d.name)) if k == len(d.classes) - 1 else None
                insts = self.instantiations(cdecl, ctx)
                if over and insts:
                    own = cdecl.heads[len(cdecl.classes[-1][3].heads) if cdecl.classes else 0:]
                    env0 = dict(insts[0][0])
                    for p, v in zip(own[-1] if own else [], over):
                        if p.name:
                            env0[p.name] = v
                    insts = [(env0, "")]
                for env, label2 in insts:
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
                    known = self.inherited_members(d.ns, path)
                    if not known:
                        known = dict.fromkeys(("pointer", "reference", "const_reference", "value_type", "size_type",
                                               "difference_type", "iterator", "const_iterator", "allocator_type"))
                    for nm in known:
                        mem[nm] = "typename " + t + "::" + nm
                    c3 = Ctx(env, mem, cdecl.name, t)
                    c3.injected = dict(ctx.injected)
                    if ctx.selfname:
                        c3.injected[ctx.selfname] = ctx.selftype
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
        # Annex D's macros: the header of the subclause (INV.HEADER_OF)
        for s in data["sections"]:
            if s["id"].startswith("depr.") and s["id"] != "depr.atomics.types.operations":
                h = next((hh for pre, hh in INV.HEADER_OF if s["id"].startswith(pre)), None)
                if h and any("#define" in t for k, t in s["regions"]):
                    hdrs[s["id"]] = h
        for s in data["sections"]:
            if s["id"] not in hdrs or s["id"] == "version.syn":
                continue
            for kind, text in s["regions"]:
                for m in re.finditer(r"(?m)^[ \t]*#define\s+([A-Za-z_]\w*)(\x01N\x02)?(.*)$", text):
                    if "\x04optional" in m.group(3):
                        continue
                    names = [m.group(1) + n for n in ("8", "16", "32", "64")] if m.group(2) else [m.group(1)]
                    for nm in names:
                        if (s["id"], nm) in SMP.SKIP:
                            continue
                        d = D.Decl([], [], [("ns", "", False)], "public", False, None)
                        d.sec, d.header, d.name, d.kind = s["id"], MACRO_HEADER.get(nm, hdrs[s["id"]]), nm, "macro"
                        self.add(d, "macro", "#" + nm, ent=nm)

    def feature_macros(self, data):
        """[version.syn]: each __cpp_lib macro that one of these clauses' headers also defines has
        the specified value in <version> and in each of those headers (freestanding too, where
        its comment says so)."""
        hdrs = INV.section_headers(data)
        mine = {h for sid, h in hdrs.items() if sid != "version.syn"}
        text = "".join(t for s in data["sections"] if s["id"] == "version.syn" for k, t in s["regions"])
        for m in re.finditer(r"#define\s+(__cpp_lib_\w+)\s+(\d+L)\s*(\x04[^\x05]*\x05)?", text):
            name, value, comment = m.group(1), m.group(2), m.group(3) or ""
            also = re.findall(r"<([\w./]+)>", comment)
            ours = [h for h in also if h in mine]
            if not ours:
                continue
            if name.startswith("__cpp_lib_hardened_"):
                continue   # [version.syn]/3: hardened implementations only (YCXX_HARDENED=1)
            fs = "freestanding" in comment
            for h in ["version"] + ours:
                # in freestanding: <version> and the headers with freestanding declarations
                hfs = fs and (h == "version" or h in self.fs_partial)
                d = D.Decl([], [], [("ns", "", False)], "public", False, "freestanding" if hfs else None)
                d.sec, d.header, d.name, d.kind = f"version.syn@{h}", h, name, "macro"
                self.add(d, f"macro value {value}", f"#if:!defined({name}) || ({name}) != {value}", ent=name)

    def zombie_names(self, data):
        """[zombie.names]: with every header included, no zombie name of Table 38 is declared in
        std, none of Tables 39-40 is a macro, and the zombie headers of Table 41 do not exist.
        (Reserved names: an implementation may declare them; libycxx does not, STATUS "Deliberate
        omissions".) The tables are read from the draft's text."""
        sec = next((s for s in data["sections"] if s["id"] == "zombie.names"), None)
        if sec is None:
            return
        import headers as H
        names = ZOMBIE_STD
        def mk(kind, name, what, code):
            d = D.Decl([], [], [("ns", "", False)], "public", False, None)
            d.sec, d.header, d.name, d.kind = "zombie.names", "", name, kind
            self.add(d, what, code, ent=name)
        for n in names:
            mk("zombie", n, "not declared in std",
               f"#zombie:namespace zombie_{n} {{ struct {n}; }} namespace zp_{n} {{ using namespace std; using namespace zombie_{n}; using t = {n}*; }}")
        for n in ZOMBIE_MACROS:
            mk("zombie", n, "not a macro", f"#if:defined({n})")
        for h in ZOMBIE_HEADERS:
            mk("zombie", h, "header absent", f"#if:__has_include(<{h}>)")
        self.zombie_headers = [h for h in dict.fromkeys(H.CORE + H.FREESTANDING_SUBSET + H.HOSTED)
                               if h not in ("meta", "contracts")]

    # ---- emit -------------------------------------------------------------------------------
    def add(self, d, what, code, ent=None):
        if any(d.sec == sec and sub in d.text() for sec, sub in SMP.SKIP_DECLS):
            return
        self.nsec[d.sec] = self.nsec.get(d.sec, 0) + 1
        cid = f"{d.sec}#{self.nsec[d.sec]}"
        ent = ent or self.entity(d)
        fs = d.fs or next((c[3].fs for c in reversed(d.classes) if c[3] is not None and c[3].fs), None)
        if fs is None and d.header in self.fs_headers:
            fs = "freestanding"
        self.checks.append((cid, d.sec, d.header, ent, what, d.text().replace("\t", " "), code, d.ns,
                            self.decl_id if self.decl_id is not None else ""))
        if fs == "freestanding":
            self.freestanding.add(cid)

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
            self.decl_id = len(self.inventory)
            entry = [self.decl_id, d.sec, d.header or "", self.entity(d), d.kind, 0, "", d.text().replace("\t", " ")]
            self.inventory.append(entry)
            if (d.sec, d.name) in SMP.SKIP or d.fs == "optional":
                entry[6] = "not probed: " + (SMP.SKIP.get((d.sec, d.name)) or "optional ([version.syn] comment // optional)")
                continue
            CUR["ns"] = d.ns
            before = len(self.checks)
            try:
                if (d.sec, d.name) in SMP.PRESENCE_ONLY:
                    raise Unprobeable("signature not probed")
                getattr(self, "do_" + d.kind.replace("-", "_"))(d)
            except Unprobeable as e:
                entry[6] = "presence only: " + str(e)
                self.presence(d, str(e))
            entry[5] = len(self.checks) - before
            if entry[5] and all(c[4].startswith("presence") for c in self.checks[before:]) and not entry[6]:
                entry[6] = "presence only: " + self.checks[before][4]
            if not entry[5] and not entry[6]:
                entry[6] = "not probed (no sample, or a hidden friend whose signature is exposition-only)"
        self.decl_id = None

    def presence(self, d, why=""):
        """A name-only check."""
        if d.classes and "friend" in (d.info.get("specs") or []):
            return   # a hidden friend is no member: not probed
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
            cls = d.classes[-1][3]
            own = d.heads[len(cls.heads):] if cls else []
            for ctx, selft, cdecl, label in self.class_samples(d):
                targs = ""
                if own:
                    envs = SMP.params_env(own[-1], d, ctx.env)
                    if not envs:
                        continue
                    ctx = Ctx(envs[0][0], ctx.members, ctx.selfname, ctx.selftype)
                    targs = "<" + ", ".join(SMP.arg_text(p, ctx.env) for p in own[-1]) + ">"
                try:
                    tgt = subst(d.info["target"], ctx)
                    tm = "template " if targs else ""
                    self.add(d, f"type {label}".strip(), f"static_assert(spec_probe::same<typename {selft}::{tm}{d.name}{targs}, {tgt}>);")
                except Unprobeable as e:
                    if own:
                        self.add(d, f"presence {label} ({e})".strip(), f"struct D : {selft} {{ using {selft}::{d.name}; }};")
                    else:
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
            # a specialization of a variable template: its value, when the draft gives it
            # (`enable_view<filesystem::directory_iterator> = true`); a partial one with a sample
            init = [str(x) for x in d.info.get("init", [])]
            if init[:1] == ["="] and init[1:] in (["true"], ["false"]) and not d.heads[-1:] or \
                    init[:1] == ["="] and init[1:] in (["true"], ["false"]):
                for env, label in (SMP.heads_env(d.heads, d) if d.heads and d.heads[-1] else [({}, "")]):
                    try:
                        a = subst(d.info["args"], Ctx(env))
                    except Unprobeable:
                        continue
                    neg = "" if init[1] == "true" else "!"
                    self.add(d, f"value {init[1]} {label}".strip(), f"static_assert({neg}{d.ns}::{d.name}{a});")
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
            # (a nested class: `using C::name;` in a class derived from the sample)
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
        mem = {}
        if d.classes:
            alts = self.class_samples(d)
            if not alts:
                return
            mem = alts[0][0].members
            mem[d.name] = alts[0][1] + "::" + d.name
        for env, label in SMP.heads_env(d.heads, d) if d.heads else [({}, "")]:
            ctx = Ctx(env, mem)
            try:
                args = [self.arg(p, ctx) for p in params]
                res = subst(d.info["trailing"], ctx)
            except Unprobeable as e:
                continue
            if args is None:
                continue
            path = "".join(c[1] + "::" for c in d.classes)
            call = f"{d.ns}::{path}{d.name}({', '.join(a for a in args if a)})"
            self.add(d, f"deduction guide {label}".strip(),
                     f"template<class Z> concept c = requires {{ requires spec_probe::same<decltype({call}), {res}>; }}; static_assert(c<void>);")

    def arg(self, p, ctx, z=True):
        if p["pack"] and any(str(x) in ctx.env and ctx.env[str(x)] in (PACK, "") for x in p["type"]) and \
                all(str(x) in ctx.env or x in ("&", "&&", "const") for x in p["type"]):
            return ""
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
        out = [[a for p, a in full if a]]
        k = len(params)
        while k > 0 and params[k - 1]["default"] is not None:
            k -= 1
        if k < len(params):
            out.append([a for p, a in full[:k] if a])
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
        if not d.info["deleted"] and ({"constexpr", "consteval"} & set(d.info["specs"])):
            self.constexpr_check(d, ctx, callee_fn, targs, label)

    def constexpr_check(self, d, ctx, callee_fn, targs, label):
        """A constexpr (consteval) function can be called in a constant expression: with sample
        arguments (1 for an arithmetic type, value-initialized otherwise; spec_probe::sample) and
        a sample object. Not for pointer parameters, volatile or protected members."""
        obj = getattr(self, "_obj", None)
        if obj == "protected" or "volatile" in d.info["quals"] or (d.sec, d.name) in SMP.NO_CONSTEXPR_PROBE:
            return
        decls, args = [], []
        for k, p in enumerate(d.info["params"]):
            try:
                t = subst(p["type"], ctx)
            except Unprobeable:
                return
            if "*" in t or p["pack"] and not t:
                return
            if p["pack"] and any(str(x) in ctx.env and ctx.env[str(x)] in (PACK, "") for x in p["type"]):
                continue
            base = t[:-2] if t.endswith("&&") else t
            decls.append(f"auto a{k} = spec_probe::sample<{base}>();")
            args.append(f"static_cast<{t}>(a{k})" if t.endswith("&&") else f"a{k}")
        expr = callee_fn(args, targs)
        if obj:
            objt = obj[len("spec_probe::dv<spec_probe::dep<Z, "):-len(">>()")]
            cls = objt.rstrip("&").replace("const ", "", 1) if objt.startswith("const ") else objt.rstrip("&")
            if cls.startswith("std::atomic_ref<"):
                # no default constructor: refer to a local object
                decls.insert(0, f"typename {cls}::value_type v = spec_probe::sample<typename {cls}::value_type>(); {cls} o(v);")
            else:
                decls.insert(0, f"auto o = spec_probe::sample<{cls}>();")
            ref = "static_cast<" + objt + ">(o)"
            expr = expr.replace(obj, ref)
        if "spec_probe::dv<" in expr:
            return
        self.add(d, "constexpr" + (f" {label}" if label else ""),
                 f"static_assert([]() consteval {{ {' '.join(decls)} (void)({expr}); return true; }}());")

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
                ctx = inj(Ctx(env, ctx0.members, ctx0.selfname, ctx0.selftype), ctx0)
                for env2, label2 in (SMP.placeholder_envs(d.toks2, env) if has_italic(d.toks2) else [(env, "")]):
                    ctx2 = inj(Ctx(env2, ctx.members, ctx.selfname, ctx.selftype), ctx)
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
                    ctx = inj(Ctx(env2, ctx0.members, ctx0.selfname, ctx0.selftype), ctx0)
                    lab = " ".join(x for x in (label0, label, label2) if x)
                    cv = " ".join(q for q in quals if q in ("const", "volatile"))
                    ref = "&&" if "&&" in quals else "&"
                    objt = f"{cv + ' ' if cv else ''}{selft}{ref}"
                    ps = d.info["params"]
                    if ps and ps[0]["type"] and ps[0]["type"][0] == "this":
                        try:
                            objt = subst(ps[0]["type"][1:], ctx)
                        except Unprobeable:
                            continue
                        if not objt.endswith("&"):
                            objt += "&&"
                        d.info = dict(d.info)
                        d.info["params"] = ps[1:]
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
                        if protected:
                            return f"{obj}.B::{tk}{name}{targs}({', '.join(args)})"
                        return f"{obj}.{tk}{name}{targs}({', '.join(args)})"
                    before = len(self.checks)
                    self._obj = "protected" if protected else (None if static else obj)
                    self.call_checks(d, ctx, callee, lab, own, env2)
                    self._obj = None
                    if protected:
                        # wrap: the concept inside a class derived from the sample
                        for k in range(before, len(self.checks)):
                            c = list(self.checks[k])
                            code = c[6].replace("template<class Z> concept c =", "template<class Z> static constexpr bool c =")
                            code = code.replace(" static_assert(c<void>);", "")
                            c[6] = f"struct D : {selft} {{ using B = {selft}; {code} }}; static_assert(D::c<void>);"
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
                    ctx = inj(Ctx(env2, ctx0.members, ctx0.selfname, ctx0.selftype), ctx0)
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
                        # a new-expression: constructible whatever the destructor's access (facets)
                        newx = f"::new {selft}({', '.join(f'spec_probe::dv<spec_probe::dep<Z, {t}>>()' for t in ts)})"
                        conds = [f"requires {{ {newx}; }}"]
                        if d.info["noexcept"] is True:
                            conds.append(f"std::is_nothrow_constructible_v<{targs}>")
                        expl = "explicit" in d.info["specs"]
                        if len(ts) == 1 and vi == 0:
                            if expl:
                                conds.append(f"!std::is_convertible_v<{ts[0]}, {selft}>")
                            elif not any(str(x) == "explicit" for x in d.toks2):
                                conds.append(f"std::is_convertible_v<{ts[0]}, {selft}>")
                        self.add(d, what + (" noexcept" if d.info["noexcept"] is True else "") + (" explicit" if expl and len(ts) == 1 else ""),
                                 f"template<class Z> concept c = {' && '.join(conds)}; static_assert(c<void>);")

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


def write(gen, outdir, inline_ns, only=None):
    outdir.mkdir(parents=True, exist_ok=True)
    for f in outdir.glob("*.cpp"):
        f.unlink()
    by_sec = collections.OrderedDict()
    for c in gen.checks:
        if only is None or c[0] in only:
            by_sec.setdefault(c[1], []).append(c)
    for sec, cs in by_sec.items():
        headers = sorted({c[2] for c in cs if c[2]})
        if sec == "zombie.names":
            headers = gen.zombie_headers
        lines = [f"// Spec-audit probes for [{sec}], generated by tools/spec_audit/part3/gen.py; do not edit.",
                 "// One check per line (see gen.py); the runner maps diagnostics to the check IDs below."]
        for h in headers:
            lines.append(f"#include <{h}>")
        for h in headers:
            lines.append("#define SPEC_PROBE_" + re.sub(r'\W', '_', h))
        lines.append('#include "' + ('../' * (len(outdir.relative_to(HERE).parts))) + 'probe_support.hpp"')
        for k, c in enumerate(cs):
            cid, _, header, ent, what, decl, code, ns = c[:8]
            if not code:
                continue
            code = code.replace(PACK, "")
            if code.startswith("#zombie:"):
                lines.append(f"{code[8:]} // {cid} {what}")
                continue
            if code.startswith("#if:"):
                lines += ["#if " + code[4:], f"static_assert(false, \"{ent}\"); // {cid} {what}", "#endif"]
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
    g.feature_macros(data)
    g.zombie_names(data)
    write(g, pathlib.Path(a.out), None)
    # the freestanding declarations, compiled with -ffreestanding (run.py --freestanding)
    write(g, pathlib.Path(a.out) / "freestanding", None, g.freestanding)
    with open(HERE / "checks.tsv", "w") as f:
        f.write("# id\tsubclause\theader\tentity\tcheck\tdeclaration\tdecl (inventory.tsv)\t[freestanding]\n")
        for c in g.checks:
            f.write("\t".join(c[:6]) + "\t" + str(c[8]) + ("\tfreestanding" if c[0] in g.freestanding else "") + "\n")
    with open(HERE / "inventory.tsv", "w") as f:
        f.write("# decl\tsubclause\theader\tentity\tkind\tchecks\tnote\tdeclaration\n")
        for e in g.inventory:
            f.write("\t".join(str(x) for x in e) + "\n")
    print(f"{len(g.checks)} checks in {len({c[1] for c in g.checks})} files")


if __name__ == "__main__":
    main()
