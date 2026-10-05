#!/usr/bin/env python3
"""Generate the standard library modules std and std.compat ([std.modules]) from libycxx's headers.

    tools/gen_std_module.py           rewrite modules/std.cppm and modules/std.compat.cppm
    tools/gen_std_module.py --check   fail (exit 1, with a diff) if either file is out of date

The modules are module interface units whose global module fragment includes the headers; the
module purview only re-exports their declarations with using-declarations, so every entity stays
attached to the global module and `import std;` mixes with `#include` (DECISIONS §16). The
export lists are read from the headers by the compilers, never written by hand:

1. Clang (tools/ycxx-cxx clang) parses one translation unit that includes every importable C++
   library header and every C++ header for C library facilities (tools/headers.py) and dumps its
   AST (-ast-dump, text). Every declaration directly in namespace std and in its standard nested
   namespaces is exported: classes, enumerations (and the enumerators of unscoped ones),
   functions and operators, variables, typedefs and aliases, concepts, templates, and the
   using-declarations of the C wrappers (`using ::printf;`). Inline namespaces are redeclared
   inline, the implementation's too (std::ranges::cpo, which holds the customization point
   objects: their using-declarations cannot be placed in std::ranges itself, where the views'
   iterators declare hidden friends of the same names); a
   using-directive (std::chrono's of chrono_literals) is replaced by using-declarations of the
   nominated namespace's names; a namespace alias (std::views) is redeclared. Not exported:
   names reserved to the implementation (`_X`, `__x`), explicit and partial specializations,
   deduction guides, and anything outside namespace std (`ycxx::detail`, `ycxx::adl_free`),
   which stays reachable but invisible to an importer. A nested namespace of std that is neither
   in STD_NAMESPACES nor inline stops the generator: it would be an implementation name in std.
2. GCC (tools/ycxx-cxx gcc -freflection) compiles a probe that walks namespace std with
   reflection (members_of, source_location_of) and prints each named member with its
   declaration's file and line. Declarations that only one compiler or configuration declares
   are found through those locations and through Clang's: a declaration inside an
   `#if YCXX_HAS_<X>` region of a header (std::is_structural, <meta>) is exported under the same
   `#if` in the module, since a using-declaration of an undeclared name is an error. (The
   `#if` tests a YCXX_HAS_* switch, as DECISIONS §1 rule 4 allows.)
3. std.compat ([std.modules]/3) also exports, at global scope, the names that the C++ headers
   for C library facilities declare in std and that libycxx's or the C library's <name.h>
   headers declare in the global namespace (except [support.c.headers.other]/1's exclusions),
   and the declarations of <stdbit.h> and <stdckdint.h>.
4. std additionally exports the global replaceable allocation and deallocation functions.

Macros are not exported: a named module cannot export them ([module.import]/7).

The output is sorted, so it depends only on the headers (and the compilers that read them).
tools/check-all runs `--check` (policy stage) so a header change that adds or removes a name
fails until the modules are regenerated.
"""
import argparse, difflib, os, pathlib, re, subprocess, sys, tempfile

HERE = pathlib.Path(__file__).resolve().parent
REPO = HERE.parent
sys.path.insert(0, str(HERE))
from headers import CORE, HOSTED, ABI, FREESTANDING_SUBSET  # noqa: E402

OUT_DIR = REPO / "modules"
INCLUDE = REPO / "include"

# Table 25: the C++ headers for C library facilities.
C_HEADERS = ["cassert", "cctype", "cerrno", "cfenv", "cfloat", "cinttypes", "climits", "clocale", "cmath",
             "csetjmp", "csignal", "cstdarg", "cstddef", "cstdint", "cstdio", "cstdlib", "cstring", "ctime",
             "cuchar", "cwchar", "cwctype"]
# The importable C++ library headers (Table 24): every public header that is not a C header.
CXX_HEADERS = sorted(h for h in set(CORE + HOSTED + ABI + FREESTANDING_SUBSET)
                     if not h.endswith(".h") and h not in C_HEADERS)
# The C headers whose global declarations std.compat exports ([std.modules]/3): the <name.h>
# form of each Table 25 header, and <stdbit.h>, <stdckdint.h>.
COMPAT_HEADERS = [h[1:] + ".h" for h in C_HEADERS] + ["stdbit.h", "stdckdint.h"]

# The namespaces nested in std that the draft names (std::views is an alias of
# std::ranges::views). Inline namespaces not listed here are implementation details whose
# members are exported in the enclosing namespace.
STD_NAMESPACES = {
    "chrono", "chrono_literals", "complex_literals", "contracts", "execution", "filesystem", "linalg",
    "literals", "meta", "numbers", "placeholders", "pmr", "ranges", "regex_constants", "rel_ops", "simd",
    "string_literals", "string_view_literals", "this_thread", "views",
}
# Not placed in the global namespace by <name.h> ([support.c.headers.other]/1): the special
# mathematical functions, lerp, byte and its operations. Never exported by std.compat, should
# a global declaration of the name exist.
COMPAT_EXCLUDED = {
    "assoc_laguerre", "assoc_laguerref", "assoc_laguerrel", "assoc_legendre", "assoc_legendref",
    "assoc_legendrel", "beta", "betaf", "betal", "comp_ellint_1", "comp_ellint_1f", "comp_ellint_1l",
    "comp_ellint_2", "comp_ellint_2f", "comp_ellint_2l", "comp_ellint_3", "comp_ellint_3f",
    "comp_ellint_3l", "cyl_bessel_i", "cyl_bessel_if", "cyl_bessel_il", "cyl_bessel_j", "cyl_bessel_jf",
    "cyl_bessel_jl", "cyl_bessel_k", "cyl_bessel_kf", "cyl_bessel_kl", "cyl_neumann", "cyl_neumannf",
    "cyl_neumannl", "ellint_1", "ellint_1f", "ellint_1l", "ellint_2", "ellint_2f", "ellint_2l", "ellint_3",
    "ellint_3f", "ellint_3l", "expint", "expintf", "expintl", "hermite", "hermitef", "hermitel", "laguerre",
    "laguerref", "laguerrel", "legendre", "legendref", "legendrel", "riemann_zeta", "riemann_zetaf",
    "riemann_zetal", "sph_bessel", "sph_besself", "sph_bessell", "sph_legendre", "sph_legendref",
    "sph_legendrel", "sph_neumann", "sph_neumannf", "sph_neumannl",
    "lerp", "byte", "to_integer",
}
RESERVED = re.compile(r"^(__|_[A-Z])")
# The declarations of <stdbit.h> and <stdckdint.h> ([stdbit.h.syn], [stdckdint.h.syn]).
COMPAT_C23 = re.compile(r"^(stdc_|ckd_)")


def run(cmd, **kw):
    r = subprocess.run(cmd, capture_output=True, text=True, **kw)
    if r.returncode != 0:
        sys.exit(f"gen_std_module: command failed ({r.returncode}): {' '.join(map(str, cmd))}\n{r.stderr[-4000:]}")
    return r.stdout


# ---------------------------------------------------------------------------------------------
# The #if YCXX_HAS_* regions of the headers: (file, line) -> the condition that declares it.

COND = re.compile(r"^\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)")
HAS = re.compile(r"^\s*(!?)\s*(YCXX_HAS_\w+)\s*(//.*)?$")
_regions = {}


def condition_at(path, line):
    """The YCXX_HAS_* condition (e.g. 'YCXX_HAS_REFLECTION', '!YCXX_HAS_RTTI', or a conjunction
    'A && B') under which line `line` of `path` is compiled, or '' when it is unconditional
    (other conditionals, such as YCXX_HOSTED, are not modelled: the module is hosted)."""
    path = os.path.realpath(path)
    if not path.startswith(str(INCLUDE) + os.sep):
        return ""
    if path not in _regions:
        conds, stack = [], []
        for text in pathlib.Path(path).read_text().splitlines():
            m = COND.match(text)
            if m:
                kw, arg = m.group(1), m.group(2)
                if kw == "if":
                    h = HAS.match(arg)
                    stack.append((h.group(1) + h.group(2)) if h else None)
                elif kw in ("ifdef", "ifndef"):
                    stack.append(None)
                elif kw == "elif" and stack:
                    stack[-1] = None
                elif kw == "else" and stack and stack[-1]:
                    c = stack[-1]
                    stack[-1] = c[1:] if c.startswith("!") else "!" + c
                elif kw == "endif" and stack:
                    stack.pop()
            conds.append(" && ".join(c for c in stack if c))
        _regions[path] = conds
    conds = _regions[path]
    return conds[line - 1] if 0 < line <= len(conds) else ""


# ---------------------------------------------------------------------------------------------
# Clang's AST dump.

NODE = re.compile(r"^([| `]*)[|`]-(\w+) 0x([0-9a-f]+) (.*)$")
LOC = re.compile(r"(<built-in>|<scratch space>|/[^\s:<>,']+):(\d+):(\d+)|\bline:(\d+):(\d+)|\bcol:(\d+)")
FLAGS = {"implicit", "referenced", "used", "constexpr", "consteval", "invalid", "inline", "hidden",
         "imported", "constinit", "static", "extern", "friend", "nested"}
NAMED = {"CXXRecordDecl", "ClassTemplateDecl", "FunctionDecl", "FunctionTemplateDecl", "VarDecl",
         "VarTemplateDecl", "TypedefDecl", "TypeAliasDecl", "TypeAliasTemplateDecl", "EnumDecl",
         "ConceptDecl", "UsingDecl"}


class Namespace:
    def __init__(self, name, parent, inline):
        self.name, self.parent, self.inline = name, parent, inline
        self.names = {}        # name -> set of conditions of its declarations ('' unconditional)
        self.children = {}     # name -> Namespace
        self.aliases = {}      # alias name -> address of the namespace it names
        self.directives = []   # addresses of namespaces nominated by using-directives

    def path(self):
        return (self.parent.path() + "::" if self.parent and self.parent.name else "") + self.name

    def child(self, name, inline):
        ns = self.children.get(name)
        if ns is None:
            ns = self.children[name] = Namespace(name, self, inline)
        ns.inline = ns.inline or inline
        return ns


def split_head(rest):
    """Split the text after a node's address into (range, location, remainder)."""
    rest = re.sub(r"^(parent 0x[0-9a-f]+ )?(prev 0x[0-9a-f]+ )?", "", rest)
    if not rest.startswith("<"):
        return "", "", rest
    depth = 0
    for i, ch in enumerate(rest):
        depth += ch == "<"
        depth -= ch == ">"
        if depth == 0:
            break
    rng, rest = rest[:i + 1], rest[i + 1:].lstrip()
    if rest.startswith("<invalid sloc>"):
        return rng, "", rest[len("<invalid sloc>"):].lstrip()
    m = re.match(r"(\S+)(?: <Spelling=[^>]*>)?\s*", rest)
    return rng, m.group(1) if m else "", rest[m.end():] if m else rest


def decl_name(kind, rest):
    """The declared name from the remainder of a node line (after its location)."""
    words = rest.split(" ")
    while words and words[0] in FLAGS:
        words.pop(0)
    text = " ".join(words)
    if kind == "UsingDecl":
        return text.split()[0].rsplit("::", 1)[-1]
    if kind == "CXXRecordDecl":
        m = re.match(r"(?:struct|class|union) (\w+)", text)
        return m.group(1) if m else None
    if kind == "EnumDecl":
        m = re.match(r"(?:(?:class|struct) )?(\w+)", text)
        return m.group(1) if m else None
    if text.startswith("operator"):
        # operator<=>, operator new[], operator""s, operator() ... up to the type or the linkage.
        m = re.match(r"(operator(?:\"\"\s*\w+|\s*new\[\]|\s*delete\[\]|\s*new|\s*delete|\(\)|\[\]|[^\s']+))", text)
        return m.group(1).replace(" ", "") if m else None
    m = re.match(r"(\w+)", text)
    return m.group(1) if m else None


def parse_clang_dump(text):
    """The namespace tree of std (and the global namespace's direct names) from an AST dump."""
    root = Namespace("", None, False)
    namespaces = {}            # address -> Namespace (std's and its nested ones)
    enums = {}                 # address -> (Namespace, [enumerator names], scoped, condition)
    using_enums = []           # (Namespace, enum address, condition)
    stack = [(-1, root, "ns")]  # (depth, owner, what): what is 'ns' or 'enum' or 'skip'
    cur_file, cur_line = "", 0
    pending_alias = None
    for raw in text.splitlines():
        m = NODE.match(raw)
        # Every location printed advances the dumper's "last location" (a location is printed in
        # full only when its file or line differs from that one): follow them all, in order.
        if not m:
            for lm in LOC.finditer(raw):
                cur_file, cur_line = update(lm, cur_file, cur_line)
            continue
        prefix, kind, addr, rest = m.groups()
        depth = len(prefix) // 2
        # `parent 0x...`: a member of a class defined outside it (`void C::f() {}`), not a
        # namespace member.
        out_of_line = rest.startswith("parent ")
        rng, loc, remainder = split_head(rest)
        for lm in LOC.finditer(rng):
            cur_file, cur_line = update(lm, cur_file, cur_line)
        for lm in LOC.finditer(loc):
            cur_file, cur_line = update(lm, cur_file, cur_line)
        decl_file, decl_line = cur_file, cur_line
        for lm in LOC.finditer(remainder):
            cur_file, cur_line = update(lm, cur_file, cur_line)
        while stack and stack[-1][0] >= depth:
            stack.pop()
        _, owner, what = stack[-1]
        if pending_alias and kind == "Namespace" and depth == pending_alias[2] + 1:
            alias_ns, alias_name, _ = pending_alias
            alias_ns.aliases[alias_name] = addr  # the line is `Namespace 0x<target> 'name'`
            pending_alias = None
            continue
        if out_of_line:
            stack.append((depth, None, "skip"))
            continue
        if kind == "Namespace" and what == "ns":
            continue
        if what == "enum":
            if kind == "EnumConstantDecl":
                enums[owner][1].append(decl_name("VarDecl", remainder))
            continue
        if what != "ns":
            continue
        ns = owner
        cond = condition_at(decl_file, decl_line) if decl_file.startswith("/") else ""
        implicit = re.match(r"(\w+ )*implicit\b", remainder) is not None
        if kind == "LinkageSpecDecl" or kind == "ExportDecl":
            stack.append((depth, ns, "ns"))
            continue
        if kind == "NamespaceDecl":
            words = remainder.split()
            name = next((w for w in words if w not in FLAGS and w != "external-linkage"), None)
            if ns is root and name != "std":
                stack.append((depth, None, "skip"))
                continue
            child = ns.child(name, " inline " in f" {remainder} ")
            namespaces[addr] = child
            stack.append((depth, child, "ns"))
            continue
        if kind == "NamespaceAliasDecl":
            pending_alias = (ns, decl_name("VarDecl", remainder), depth)
            continue
        if kind == "UsingDirectiveDecl":
            ns.directives.append(addr_of(remainder))
            continue
        if kind == "UsingEnumDecl":
            using_enums.append((ns, addr_of(remainder), cond))
            continue
        if kind == "EnumDecl":
            name = decl_name(kind, remainder)
            scoped = re.match(r"(\w+ )*(class|struct) ", remainder) is not None
            enums[addr] = (ns, [], scoped, cond)
            stack.append((depth, addr, "enum"))
            if name and not implicit:
                ns.names.setdefault(name, set()).add(cond)
            continue
        if kind in NAMED and not implicit:
            name = decl_name(kind, remainder)
            if name:
                ns.names.setdefault(name, set()).add(cond)
        stack.append((depth, None, "skip"))
    # Enumerators of unscoped enumerations are members of the enclosing namespace; so are those of
    # an enumeration named by a using-enum-declaration.
    for owner, names, scoped, cond in enums.values():
        if not scoped:
            for n in names:
                owner.names.setdefault(n, set()).add(cond)
    for ns, addr, cond in using_enums:
        if addr in enums:
            for n in enums[addr][1]:
                ns.names.setdefault(n, set()).add(cond)
    return root, namespaces


def addr_of(text):
    m = re.search(r"0x([0-9a-f]+)", text)
    return m.group(1) if m else None


def update(lm, cur_file, cur_line):
    if lm.group(1):
        return lm.group(1), int(lm.group(2))
    if lm.group(4):
        return cur_file, int(lm.group(4))
    return cur_file, cur_line


def tu(headers):
    return "".join(f"#include <{h}>\n" for h in headers)


def clang_dump(source, workdir):
    src = pathlib.Path(workdir) / "dump.cpp"
    src.write_text(source)
    return run([str(HERE / "ycxx-cxx"), "clang", "-fsyntax-only", "-fno-color-diagnostics", "-Xclang", "-ast-dump",
                str(src)], cwd=workdir)


# ---------------------------------------------------------------------------------------------
# GCC's view through reflection: the named members of std with their locations.

PROBE = r"""
#include <meta>
#include <cstdio>
#include <string>
// The headers come first (-include). One line per named member: its qualified name, the file of
// its declaration (empty when it is the previous line's) and the line.
consteval void walk(std::meta::info ns, const std::string& prefix, std::string& out, std::string_view& file) {
  for (auto m : std::meta::members_of(ns, std::meta::access_context::unchecked())) {
    if (!std::meta::has_identifier(m))
      continue;
    std::string_view name = std::meta::identifier_of(m);
    auto where = std::meta::source_location_of(m);
    std::string_view f = where.file_name();
    out.append(prefix).append(name).append("\t").append(f == file ? std::string_view() : f).append("\t");
    out.append(std::to_string(where.line()));
    file = f;
    if (std::meta::is_type(m) && std::meta::is_enum_type(m)) {
      // An enumeration: its kind, then its enumerators.
      out.append(std::meta::is_scoped_enum_type(m) ? "\tscoped-enum:" : "\tenum:");
      for (auto e : std::meta::enumerators_of(m))
        out.append(" ").append(std::meta::identifier_of(e));
    }
    out.append("\n");
    if (std::meta::is_namespace(m) && !std::meta::is_namespace_alias(m))
      walk(m, prefix + std::string(name) + "::", out, file);
  }
}
consteval std::string members() {
  std::string out, prefix;
  std::string_view file;
  walk(^^std, prefix, out, file);
  return out;
}
// The header-defined default error handler calls the PAL; this program links without libycxx.
extern "C" void ycxx_pal_abort(const char*) noexcept { __builtin_trap(); }
int main() { std::fputs(std::define_static_string(members()), stdout); }
"""


def gcc_probe(headers, workdir):
    """[(namespace path, name, file, line, enum)] of the named members of std (recursively) as GCC
    with reflection sees them; enum is None or (scoped, [enumerators])."""
    work = pathlib.Path(workdir)
    (work / "all.hpp").write_text(tu(headers))
    (work / "probe.cpp").write_text(PROBE)
    exe, obj = work / "probe", work / "probe.o"
    # Constant evaluation of the whole walk: lift GCC's operation and loop limits. The program
    # needs only the C library (fputs), so it is linked without libycxx: the generator runs before
    # the library is built (tools/test's policy stage).
    r = subprocess.run([str(HERE / "ycxx-cxx"), "gcc", "-freflection", "-fconstexpr-ops-limit=2147483647",
                        "-fconstexpr-loop-limit=2147483647", "-include", str(work / "all.hpp"),
                        "-c", str(work / "probe.cpp"), "-o", str(obj)], capture_output=True, text=True)
    if r.returncode == 0:
        r = subprocess.run([os.environ.get("YCXX_GCC", "gcc-16"), str(obj), "-o", str(exe)],
                           capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit("gen_std_module: the GCC reflection probe failed to build:\n" + r.stderr[-4000:])
    out = run([str(exe)])
    entries = []
    file = ""
    for line in out.splitlines():
        qual, f, ln, *rest = line.split("\t")
        file = f or file
        path, _, name = qual.rpartition("::")
        enum = None
        if rest:
            kind, _, enumerators = rest[0].partition(":")
            enum = (kind == "scoped-enum", enumerators.split())
        entries.append((path, name, file, int(ln), enum))
    return entries


# ---------------------------------------------------------------------------------------------
# Building the export lists.

def collect(root, namespaces):
    """{namespace path ('' is std): {name: condition}} of the names std exports, and
    {path: {alias: target path}}."""
    std = root.children.get("std")
    if std is None:
        sys.exit("gen_std_module: no namespace std in the AST dump")
    exports, aliases, errors = {}, {}, []

    def add(path, name, conds):
        if RESERVED.match(name) or not (name.startswith("operator") or re.match(r"^\w+$", name)):
            return
        cur = exports.setdefault(path, {})
        conds = set(conds) | ({cur[name]} if name in cur else set())
        cur[name] = "" if "" in conds or len(conds) > 1 else next(iter(conds))

    def visit(ns, path):
        for name, conds in ns.names.items():
            add(path, name, conds)
        for alias, target in ns.aliases.items():
            if RESERVED.match(alias):
                continue
            if alias not in STD_NAMESPACES:
                errors.append(f"namespace alias std::{path + '::' if path else ''}{alias} is not a standard namespace")
                continue
            aliases.setdefault(path, {})[alias] = namespaces[target].path()[len("std::"):] if target in namespaces else None
        for target in ns.directives:
            nominated = namespaces.get(target)
            if nominated is not None:
                for name, conds in nominated.names.items():
                    add(path, name, conds)
        for name, child in ns.children.items():
            # An inline namespace of the implementation (std::ranges::cpo) is redeclared too, rather
            # than its members being exported from the parent: a using-declaration of the CPO
            # std::ranges::iter_move directly in std::ranges would conflict with the hidden friends
            # iter_move that the views' iterators declare there (GCC 16 rejects an instantiation in
            # the importer: "redeclared as different kind of entity").
            if name in STD_NAMESPACES or child.inline:
                exports.setdefault(f"{path}::{name}" if path else name, {})
                visit(child, f"{path}::{name}" if path else name)
            elif RESERVED.match(name):
                continue
            else:
                errors.append(f"std::{path + '::' if path else ''}{name}: a namespace in std that is neither "
                              "standard (STD_NAMESPACES) nor inline")
    visit(std, "")
    if errors:
        sys.exit("gen_std_module:\n  " + "\n  ".join(errors))
    return exports, aliases


def inline_path(root_std, path):
    """For each component of `path`, whether that namespace is inline (to redeclare it so)."""
    ns, out = root_std, []
    for part in path.split("::") if path else []:
        ns = ns.children.get(part) if ns else None
        out.append(bool(ns and ns.inline))
    return out


def add_gcc_only(exports, entries, known_namespaces):
    """The names GCC's probe finds in a YCXX_HAS_* region and Clang did not export: declared only
    where that switch is set. A namespace declared in such a region contributes all its names."""
    # Standard namespaces first declared in such a region (std::meta): their members inherit it.
    ns_cond = {}
    for path, name, file, line, _ in entries:
        full = f"{path}::{name}" if path else name
        cond = condition_at(file, line)
        if (cond and name in STD_NAMESPACES and full not in known_namespaces
                and (path == "" or path in exports or path in ns_cond)):
            ns_cond[full] = cond

    def add(path, name, cond):
        cur = exports.setdefault(path, {})
        if cur.get(name) != "":
            cur[name] = cond if name not in cur or cur[name] == cond else ""

    for path, name, file, line, enum in entries:
        if RESERVED.match(name) or (f"{path}::{name}" if path else name) in ns_cond:
            continue
        cond = condition_at(file, line) or ns_cond.get(path, "")
        if not cond:
            continue  # declared unconditionally: Clang's dump has it
        if path and path not in exports and path not in ns_cond:
            continue  # not a standard namespace
        add(path, name, cond)
        # The enumerators of an unscoped enumeration, or of one that a using-enum-declaration in
        # the same region names (std::meta's `using enum operators;`), are namespace members.
        if enum and (not enum[0] or using_enum_in(file, name, cond)):
            for e in enum[1]:
                add(path, e, cond)
    return ns_cond


def using_enum_in(path, name, cond):
    """Whether `path` has a namespace-scope `using enum name;` under condition cond."""
    lines = pathlib.Path(path).read_text().splitlines()
    return any(re.match(rf"^using enum (\w+::)*{re.escape(name)}\s*;", text) and condition_at(path, n) == cond
               for n, text in enumerate(lines, 1))


def compat_names(dump_root, std_names):
    """Global names std.compat exports: those of the C headers' std declarations that the global
    namespace also has, and <stdbit.h>'s and <stdckdint.h>'s."""
    glob = dump_root.names
    out = {}
    for name, conds in glob.items():
        if RESERVED.match(name) or name in COMPAT_EXCLUDED:
            continue
        if name in std_names or COMPAT_C23.match(name):
            out[name] = "" if "" in conds else sorted(conds)[0]
    return out


# ---------------------------------------------------------------------------------------------
# Output.

HEADER = """\
// libycxx: the named module {module} ([std.modules]).
// GENERATED by tools/gen_std_module.py from libycxx's headers; do not edit. Regenerate after a
// header adds or removes a declaration (tools/check-all fails while this file is stale).
//
// The global module fragment includes the headers; the purview only re-exports their
// declarations, so each entity stays attached to the global module and is the same entity
// whether it is reached through `import {module};` or an #include ([std.modules]/5).
// Macros are not exported ([module.import]/7). Build and use: README.md, "Modules".
"""


def using(path, name):
    return f"using {('std::' + path + '::') if path else 'std::'}{name};"


def emit_namespace_block(lines, std_ns, exports, aliases, cond, indent="  "):
    """The `export namespace std { ... }` body of the names whose condition is `cond`."""
    def body(path, depth):
        out = []
        names = sorted(n for n, c in exports.get(path, {}).items() if c == cond)
        for n in names:
            out.append(indent * depth + using(path, n))
        for alias, target in sorted(aliases.get(path, {}).items()):
            if cond == "" and target:
                out.append(indent * depth + f"namespace {alias} = std::{target};")
        kids = sorted({p[len(path) + 2:].split("::")[0] if path else p.split("::")[0]
                       for p in exports if p and (p.startswith(path + "::") if path else True) and p != path})
        for kid in kids:
            kp = f"{path}::{kid}" if path else kid
            inner = body(kp, depth + 1)
            if inner:
                inl = inline_path(std_ns, kp)[-1] if std_ns else False
                out.append(indent * depth + f"{'inline ' if inl else ''}namespace {kid} {{")
                out += inner
                out.append(indent * depth + "}")
        return out
    return body("", 1)


def generate_std(exports, aliases, std_ns):
    lines = [HEADER.format(module="std"), "module;", ""]
    lines += ["#include <" + h + ">" for h in CXX_HEADERS + C_HEADERS]
    lines += ["", "export module std;", ""]
    conds = sorted({c for names in exports.values() for c in names.values()})
    for cond in conds:
        block = emit_namespace_block(lines, std_ns, exports, aliases, cond)
        if not block:
            continue
        if cond:
            lines.append(f"#if {cond}")
        lines.append("export namespace std {")
        lines += block
        lines.append("} // namespace std")
        if cond:
            lines.append("#endif")
        lines.append("")
    lines += ["// [new.delete]: the replaceable allocation and deallocation functions ([std.modules]/2).",
              "export using ::operator new;", "export using ::operator new[];",
              "export using ::operator delete;", "export using ::operator delete[];", ""]
    return "\n".join(lines)


def generate_compat(compat):
    lines = [HEADER.format(module="std.compat"), "module;", ""]
    lines += ["#include <" + h + ">" for h in COMPAT_HEADERS]
    lines += ["", "export module std.compat;", "", "export import std;", ""]
    for cond in sorted(set(compat.values())):
        names = sorted(n for n, c in compat.items() if c == cond)
        if cond:
            lines.append(f"#if {cond}")
        lines.append("// [std.modules]/3: the C library's names in the global namespace.")
        lines.append("export {")
        lines += [f"  using ::{n};" for n in names]
        lines.append("}")
        if cond:
            lines.append("#endif")
        lines.append("")
    return "\n".join(lines)


def generate_all_headers():
    """include/bits/stdc++.h: GCC's -fmodules looks it up on the include path for every #include of
    a standard header (an #include becomes an import of its header unit when one has been built
    for it, gcc.info "C++ Modules"); without the file, the #include is a fatal error. It is the
    whole library, so such a header unit is a correct replacement for any standard header."""
    lines = ["// -*- C++ -*-  libycxx: <bits/stdc++.h>   [hosted]  (generated by tools/gen_std_module.py)",
             "//",
             "// Not a standard header: every importable C++ library header and C++ header for C library",
             "// facilities. GCC with -fmodules looks for this file whenever a standard header is",
             "// #included, to translate the #include into an import of this header's header unit if one",
             "// was built (g++ -fmodules -x c++-system-header bits/stdc++.h). Without a header unit it",
             "// is only looked up.",
             "#pragma once", ""]
    lines += ["#include <" + h + ">" for h in CXX_HEADERS + C_HEADERS]
    return "\n".join(lines) + "\n"


def build():
    with tempfile.TemporaryDirectory(prefix="gen_std_module.") as work:
        text = clang_dump(tu(CXX_HEADERS + C_HEADERS), work)
        root, namespaces = parse_clang_dump(text)
        del text
        exports, aliases = collect(root, namespaces)
        std_ns = root.children["std"]
        known = {p for p in exports if p}
        add_gcc_only(exports, gcc_probe(CXX_HEADERS + C_HEADERS, work), known)
        # std.compat: the global names of the <name.h> headers that the C++ C headers put in std.
        croot, _ = parse_clang_dump(clang_dump(tu(C_HEADERS), work))
        c_std = {n for n in croot.children["std"].names} if "std" in croot.children else set()
        groot, _ = parse_clang_dump(clang_dump(tu(COMPAT_HEADERS), work))
        compat = compat_names(groot, c_std)
    return generate_std(exports, aliases, std_ns), generate_compat(compat)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--check", action="store_true", help="fail if the committed modules are out of date")
    args = ap.parse_args()
    std_text, compat_text = build()
    outputs = {OUT_DIR / "std.cppm": std_text, OUT_DIR / "std.compat.cppm": compat_text,
               INCLUDE / "bits" / "stdc++.h": generate_all_headers()}
    stale = []
    for path, text in outputs.items():
        old = path.read_text() if path.exists() else ""
        if old != text:
            stale.append(path)
            if args.check:
                sys.stdout.writelines(difflib.unified_diff(old.splitlines(True), text.splitlines(True),
                                                           str(path.relative_to(REPO)), "generated", n=1))
            else:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(text)
    if args.check and stale:
        print("\ngen_std_module: stale: " + ", ".join(str(p.relative_to(REPO)) for p in stale) +
              " (run tools/gen_std_module.py)")
        return 1
    if not args.check:
        print("gen_std_module: " + (", ".join(str(p.relative_to(REPO)) for p in stale) + " updated" if stale
                                    else "up to date"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
