#!/usr/bin/env python3
"""A compiler that is neither GCC nor Clang gets libycxx's probe-driven paths (DECISIONS §5,
"Recognizing a compiler").

config.hpp recognizes ycxx, Clang and GCC by their identity macros, in that order (ycxx and Clang
define __GNUC__ too), sets exactly one _YCXX_COMPILER_* switch, and chooses everything else by
asking the compiler (__has_builtin, __has_cpp_attribute, __is_identifier, __cpp_* and the target
macros). This check plays a foreign compiler with Clang ($YCXX_CLANGXX): Clang's identity macros
are removed and ycxx's defined (-U__clang__ ... -D__ycxx__=1 ..., and __GNUC__ 4.2.1 as ycxx#229
has it), in two configurations:

  clang-probes  the probes answer as Clang does: config.hpp takes the builtin paths Clang's
                answers select, every public header preprocesses without an #error, and every
                core and freestanding header compiles freestanding (as tools/check_freestanding.sh
                compiles them: with no C library, whose facts a configured build would supply);
  zero-probes   the probes answer 0, as ycxx's do until it implements a feature. __has_builtin
                and the attribute and feature probes are redefined, and __is_identifier and every
                __cpp_* macro but __cpp_rtti are undefined. Clang ignores the renamed SIMD
                vector attribute. config.hpp takes its portable fallbacks, every public header
                preprocesses without an #error, and a test of those fallbacks compiles
                freestanding (tests/foreign/fallbacks.cpp).

It also pins the native builtin choices (GCC, $YCXX_GXX, and Clang), the vector layout of <simd>
when the attribute probe succeeds, the <atomic> paths with and without the GNU may-alias
attribute, the _YCXX_COMPILER_* switch each compiler gets (ycxx's, not GCC's, although it
defines __GNUC__), and that a compiler with no recognized identity still stops at config.hpp's
#error.

  tools/check_foreign_compiler.py [-v]
"""
import concurrent.futures, os, pathlib, re, subprocess, sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from headers import ABI, CORE, FREESTANDING_REQUIRED, FREESTANDING_SUBSET, HOSTED  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parent.parent
GXX = os.environ.get("YCXX_GXX", "g++-16")
CLANGXX = os.environ.get("YCXX_CLANGXX", "clang++-23")
HEADERS = sorted(set(CORE + HOSTED + ABI + FREESTANDING_SUBSET))
FREESTANDING = sorted(set(CORE + FREESTANDING_SUBSET + FREESTANDING_REQUIRED))
BASE = ["-std=c++26", "-nostdinc++", "-isystem", str(REPO / "include"), "-x", "c++"]
# No identity: Clang's macros removed, __GNUC__ and its family too.
ANONYMOUS = ["-U__clang__", "-U__clang_major__", "-U__clang_minor__", "-U__clang_patchlevel__",
             "-U__clang_version__", "-U__GNUC__", "-U__GNUG__", "-U__GNUC_MINOR__",
             "-U__GNUC_PATCHLEVEL__"]
# ycxx's identity (ycxx DECISIONS §1.5): __ycxx__, and __GNUC__ 4.2.1 as Clang has it (ycxx#229),
# without __clang__ or __GNUG__.
FOREIGN = ANONYMOUS + ["-D__GNUC__=4", "-D__GNUC_MINOR__=2", "-D__GNUC_PATCHLEVEL__=1",
                       "-D__ycxx__=1", "-D__ycxx_major__=0", "-D__ycxx_minor__=1",
                       "-D__ycxx_patchlevel__=0"]
PROBES = ["__has_builtin", "__has_attribute", "__has_cpp_attribute", "__has_feature",
          "__has_extension"]
verbose = "-v" in sys.argv[1:]
failures = []


def run(cmd, stdin=""):
    if verbose:
        print("  $", " ".join(cmd), flush=True)
    return subprocess.run(cmd, input=stdin, capture_output=True, text=True)


def fail(what, detail=""):
    failures.append(what)
    print(f"FAIL {what}")
    for line in detail.strip().splitlines()[:15]:
        print("    " + line)


def zero_probes():
    """Flags that zero Clang's feature probes, remove __is_identifier and undefine its __cpp_* macros
    (__cpp_rtti stays: RTTI is a language mode, not a feature ycxx lacks here)."""
    r = run([CLANGXX, "-std=c++26", "-x", "c++", "-E", "-dM", "-"])
    cpp = sorted(set(re.findall(r"^#define (__cpp_\w+)", r.stdout, re.M)) - {"__cpp_rtti"})
    if r.returncode != 0 or "__cpp_exceptions" not in cpp:
        sys.exit(f"{CLANGXX} -E -dM did not list its __cpp_* macros:\n{r.stderr}")
    return (["-Wno-builtin-macro-redefined", "-U__is_identifier"] + [f"-D{p}(x)=0" for p in PROBES] +
            [f"-U{m}" for m in cpp])


def config_text(cmd):
    """config.hpp preprocessed with cmd, or None (reported) if it fails."""
    r = run(cmd + ["-E", "-P", "-"], "#include <ycxx/config.hpp>\n")
    if r.returncode != 0:
        fail(f"{' '.join(cmd[:1])}: <ycxx/config.hpp> does not preprocess", r.stderr)
        return None
    return r.stdout


def expect(label, text, present, absent=()):
    if text is None:
        return
    missing = [p for p in present if p not in text]
    found = [a for a in absent if a in text]
    if missing or found:
        fail(f"{label}: config.hpp took the wrong paths",
             "".join(f"missing: {p}\n" for p in missing) + "".join(f"unexpected: {a}\n" for a in found))
    else:
        print(f"ok   {label}: config.hpp takes " + ", ".join(present))


def freestanding(cmd):
    """cmd compiling freestanding, with the compiler's own headers only (tools/check_freestanding.sh)."""
    r = run([cmd[0], "-print-file-name=include"])
    return cmd + ["-ffreestanding", "-nostdinc", "-isystem", r.stdout.strip(), "-fno-exceptions",
                  "-fno-rtti", "-Wall", "-Wextra", "-Werror"]


def each_header(label, cmd, headers, action):
    def one(h):
        return h, run(cmd + [action, "-o", os.devnull, "-"], f"#include <{h}>\n")
    with concurrent.futures.ThreadPoolExecutor(os.cpu_count() or 4) as pool:
        bad = [(h, r) for h, r in pool.map(one, headers) if r.returncode != 0]
    for h, r in bad:
        fail(f"{label}: <{h}>", "\n".join(l for l in r.stderr.splitlines() if "error" in l))
    if not bad:
        what = ("public headers preprocess" if action == "-E" else
                "core and freestanding headers compile freestanding")
        print(f"ok   {label}: all {len(headers)} {what}")


CLANG_BUILTINS = ["__remove_reference_t(_Tp)", "__make_integer_seq<_Seq, _Tp, _Np>"]
GCC_BUILTINS = ["__remove_reference(_Tp)", "__integer_pack(_Np)"]
FALLBACKS = ["struct __remove_ref_impl", "struct __int_pack_make"]
BITINT_SPECIALIZATIONS = ["__bitint_info<unsigned _BitInt(_Np)>", "__bitint_info<signed _BitInt(_Np)>"]

# No recognized identity: the #error (which also shows that ANONYMOUS removes Clang's identity and
# GCC's __GNUC__).
r = run([CLANGXX] + BASE + ANONYMOUS + ["-E", "-"], "#include <ycxx/config.hpp>\n")
if r.returncode == 0 or "libycxx supports GCC, Clang and ycxx" not in r.stderr:
    fail("unrecognized compiler: config.hpp does not stop at its #error", r.stderr)
else:
    print("ok   unrecognized compiler: config.hpp stops at its #error")


def identity(label, cmd, want):
    """The _YCXX_COMPILER_* switches config.hpp sets: want is "YCXX CLANG GCC" as 0s and 1s."""
    r = run(cmd + ["-E", "-P", "-"],
            "#include <ycxx/config.hpp>\nidentity: _YCXX_COMPILER_YCXX _YCXX_COMPILER_CLANG _YCXX_COMPILER_GCC\n")
    got = next((l.split(":", 1)[1].split() for l in r.stdout.splitlines() if l.startswith("identity:")), None)
    if r.returncode != 0 or got != want.split():
        fail(f"{label}: config.hpp's _YCXX_COMPILER_* (YCXX CLANG GCC) are {got}, not {want}", r.stderr)
    else:
        print(f"ok   {label}: config.hpp's _YCXX_COMPILER_* (YCXX CLANG GCC) are {want}")

# Identity: ycxx first (it defines __GNUC__ too), then Clang, then GCC.
identity("gcc", [GXX] + BASE, "0 0 1")
identity("clang", [CLANGXX] + BASE, "0 1 0")
identity("foreign", [CLANGXX] + BASE + FOREIGN, "1 0 0")
identity("foreign with __clang__", [CLANGXX] + BASE + FOREIGN + ["-D__clang__=1"], "1 0 0")

# Native: the identity change selects nothing; each compiler keeps its builtins.
expect("gcc", config_text([GXX] + BASE), GCC_BUILTINS, CLANG_BUILTINS + FALLBACKS)
expect("clang", config_text([CLANGXX] + BASE), CLANG_BUILTINS, GCC_BUILTINS + FALLBACKS)

# A target without 128-bit integers still needs two distinct incomplete types for <type_traits>.
int128_fallback = """
#include <type_traits>
using namespace __ycxx::__detail;
template <class T>
concept complete = requires { sizeof(T); };
static_assert(!__cfg::__has_int128);
static_assert(!std::is_same_v<__y_int128, __uint128>);
static_assert(!complete<__y_int128>);
static_assert(!complete<__uint128>);
"""
for label, compiler in (("gcc", GXX), ("clang", CLANGXX)):
    cmd = freestanding([compiler] + BASE)
    r = run(cmd + ["-U__SIZEOF_INT128__", "-fsyntax-only", "-"], int128_fallback)
    if r.returncode != 0:
        fail(f"{label}: <type_traits> without 128-bit integers", r.stderr)
    else:
        print(f"ok   {label}: <type_traits> without 128-bit integers")

# Foreign, answering Clang's probes.
probes = [CLANGXX] + BASE + FOREIGN
text = config_text(probes)
expect("foreign, clang-probes", text, CLANG_BUILTINS + BITINT_SPECIALIZATIONS, GCC_BUILTINS + FALLBACKS)
if text is not None and "inline constexpr bool __constexpr_throw = false;" not in text:
    fail("foreign, clang-probes: __constexpr_throw is not false (Clang 23 cannot throw in constant evaluation)")
each_header("foreign, clang-probes", probes, HEADERS, "-E")
each_header("foreign, clang-probes", freestanding(probes), FREESTANDING, "-fsyntax-only")

# A compiler that reports the vector attribute uses the native chunk layout.
simd = """
#include <simd>
using storage = __ycxx::__detail::__simd_storage<int, 4>;
static_assert(storage::__is_vector);
static_assert(sizeof(storage::__chunk_type) == 4 * sizeof(int));
static_assert(std::simd::vec<int, 4>::size() == 4);
"""
for label, cmd in (("gcc", [GXX] + BASE), ("clang", [CLANGXX] + BASE), ("foreign, clang-probes", probes)):
    r = run(freestanding(cmd) + ["-fsyntax-only", "-"], simd)
    if r.returncode != 0:
        fail(f"{label}: <simd> uses vector chunks", r.stderr)
    else:
        print(f"ok   {label}: <simd> uses vector chunks")

# Give the exact scoped attribute an unknown name so the probe and any declaration spelling
# both see the same unavailable attribute. A probe-only override would leave aliasing enabled.
atomic_types = """
#include <atomic>
struct word { unsigned int value; };
struct byte { unsigned char value; };
struct bulk { unsigned char value[3]; };
"""
atomic_unaffected = """
static_assert(std::atomic<int>::is_always_lock_free);
static_assert(std::atomic_ref<const volatile int>::is_always_lock_free);
static_assert(std::atomic<byte>::is_always_lock_free);
static_assert(!std::atomic<bulk>::is_always_lock_free);
static_assert(!std::atomic_ref<bulk>::is_always_lock_free);
static_assert(std::atomic_ref<word>::required_alignment == sizeof(word));
void unaffected(word& w, volatile std::atomic<word>& a, int& i, bulk& b) {
  std::atomic_ref<word>(w).notify_all();
  std::atomic_notify_one(&a);
  std::atomic_ref<int>(i).fetch_add(1);
  std::atomic_ref<bulk>(b).exchange({});
  std::atomic<byte> value;
  value.store(value.load());
}
"""
atomic_affected = {
    "atomic CAS": "bool f(std::atomic<word>& a, word& e) { return a.compare_exchange_strong(e, {}); }",
    "atomic_ref wait": "void f(word& w) { std::atomic_ref<word>(w).wait({}); }",
    "atomic pointer RMW": "void f(std::atomic<int*>& a) { a.fetch_add(1); }",
    "atomic distinct character type": "char32_t f(std::atomic<char32_t>& a) { return a.load(); }",
    "atomic lock-free query": "static_assert(std::atomic<word>::is_always_lock_free);",
    "atomic_ref lock-free query": "bool f(const word& w) { return std::atomic_ref(w).is_lock_free(); }",
}
for label, compiler in (("gcc", GXX), ("clang", CLANGXX)):
    cmd = freestanding([compiler] + BASE)
    missing = cmd + ["-D__may_alias__=__ycxx_missing_may_alias__"]
    syntax = ["-fsyntax-only", "-"]
    r = run(missing + syntax, atomic_types + atomic_unaffected)
    if r.returncode != 0:
        fail(f"{label}: <atomic> operations available without may_alias", r.stderr)
    else:
        print(f"ok   {label}: <atomic> operations that do not require may_alias compile")
    for check, use in atomic_affected.items():
        r = run(cmd + syntax, atomic_types + use)
        if r.returncode != 0:
            fail(f"{label}: {check} with may_alias", r.stderr)
        r = run(missing + syntax, atomic_types + use)
        if r.returncode == 0 or "lock-free atomic access for this type requires" not in r.stderr:
            fail(f"{label}: {check} diagnoses missing may_alias", r.stderr)
        else:
            print(f"ok   {label}: {check} diagnoses missing may_alias")

# Foreign, without the keyword operator and answering 0 to feature probes (ycxx today).
zero = [CLANGXX] + BASE + FOREIGN + zero_probes() + ["-D__vector_size__=__ycxx_missing_vector_size__"]
text = config_text(zero)
expect("foreign, zero-probes", text, FALLBACKS, GCC_BUILTINS + CLANG_BUILTINS + BITINT_SPECIALIZATIONS)
if text is not None and "inline constexpr bool __constexpr_throw = false;" not in text:
    fail("foreign, zero-probes: __constexpr_throw is not false (__cpp_constexpr_exceptions undefined)")
each_header("foreign, zero-probes", zero, HEADERS, "-E")
test = REPO / "tests/foreign/fallbacks.cpp"
r = run(freestanding(zero) + ["-fsyntax-only", str(test)])
if r.returncode != 0:
    fail(f"foreign, zero-probes: {test.relative_to(REPO)}", r.stderr)
else:
    print(f"ok   foreign, zero-probes: {test.relative_to(REPO)} compiles")

if failures:
    print(f"{len(failures)} failure(s)")
    sys.exit(1)
