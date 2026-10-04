# libycxx design decisions

## 1. Preprocessor policy (strict)

C++26 gives us `if constexpr`, `consteval`, concepts, `requires`, pack indexing, alias and
variable templates. The preprocessor is used only where the language cannot do the job.

**Rules**

1. **One detection point.** `include/ycxx/config.hpp` is the only file that inspects compiler,
   target or language-mode macros (`__clang__`, `__GNUC__`, `__cpp_exceptions`,
   `__SIZEOF_INT128__`, `__has_builtin`, `__STDCPP_FLOAT16_T__`, ...).
2. **Convert once, then use the language.** Each answer becomes a `constexpr` value in
   `ycxx::detail::cfg` (`cfg::clang`, `cfg::exceptions`, `cfg::hardened`, ...) or a type alias or
   alias template in `ycxx::detail` (`int128`, `float16`, `remove_ref_t`, `make_integer_seq`, ...).
   Library code consumes them with `if constexpr`, `requires`, and templates.
   An unavailable type aliases a distinct incomplete type (`fp_unavailable<N>`,
   `int128_unavailable`), so type lists and overload sets stay well-formed without `#if`.
3. **No library macros.** The library defines no function-like macros, no attribute macros
   (write `[[gnu::always_inline]]`, `[[gnu::cold]]` directly; both compilers accept them), and no
   "shorthand" macros (no `FWD(x)`; write `static_cast<T&&>(x)`). The only macros the library
   defines are:
   - those the standard mandates (`NULL`, `offsetof`, `INT_MAX`, `INT32_C`, `__cpp_lib_*`,
     `assert`, `errno`, ...), and
   - the `YCXX_*` switches in `config.hpp` (user-settable `YCXX_HARDENED`; parse-level
     `YCXX_HAS_*` switches).
4. **`#if` outside `config.hpp` only when the code cannot be parsed or declared otherwise.**
   Examples: a declaration that needs a builtin only one compiler has
   (`std::is_within_lifetime` needs `__builtin_is_within_lifetime`), a `throw` expression under
   `-fno-exceptions`, or a standard macro whose value must be usable inside `#if`. Also
   `YCXX_HAS_RTTI`. A `typeid` expression is rejected under `-fno-rtti` even in an
   uninstantiated template or a discarded `if constexpr` branch (verified, GCC 16.2 and Clang
   23.1), so `any::type()` must not be parsed there. And a non-template class cannot choose
   between an inline constexpr and an out-of-line destructor (`exception_base.hpp`) with
   `if constexpr` or `requires`. Everywhere else the derived constant `cfg::rtti` is used: `typeid` is spelled once, in
   `ycxx::detail::type_id<T>` (`typeinfo.hpp`, nullptr without RTTI), and members of class
   templates that need it are gated with `requires cfg::rtti` (`function::target_type`). Such an `#if`
   tests a `YCXX_HAS_*` switch, never a compiler name. Prefer restructuring (an out-of-line
   function, a dependent expression) over adding an `#if`.
   **Probing builtins without the preprocessor.** A function-style builtin is detected with a
   concept over a dependent call, e.g.
   `template <class T> concept has_is_within_lifetime = requires(const T* p) { __builtin_is_within_lifetime(p); };`.
   If the builtin does not exist, the call is an ordinary failed lookup and the concept is false
   (verified on GCC 16.2 and Clang 23.1). These probes live in `ycxx::detail::builtin` in
   `config.hpp`. The library entity is then declared unconditionally with a `requires` clause,
   so no `#if` is needed. Type-taking builtins (`__is_integral(T)`, `__builtin_type_order(T, U)`)
   cannot be probed this way: an unknown one is a hard parse error. They still need
   `__has_builtin` in `config.hpp`. `__cpp_lib_*` macros still need a preprocessor switch
   because they must be usable in `#if`.
5. **Builtins stay inside trait definitions.** Compiler trait builtins (`__is_constructible`, ...)
   appear only in the definitions of `std::` traits, `ycxx::detail` variable templates, and
   concepts. Function signatures (return types, `requires`, `noexcept`, `explicit`) use the
   `_v` traits or concepts. GCC rejects builtins in mangled signatures, and named concepts are
   needed anyway for constraint subsumption. For the same reason the public `_t` aliases go
   through their class templates (`remove_cv_t<T> = typename remove_cv<T>::type`), because user
   code puts them in signatures.
6. **`#pragma once`** instead of include guards.
7. **Preconditions are a function, not a macro.** `ycxx::detail::precondition(cond, msg)` is
   `constexpr`. It always diagnoses a violation during constant evaluation, and checks at run
   time when `YCXX_HARDENED=1` (via `cfg::hardened`).

8. **Keep looking for replacements.** Every remaining preprocessor use is technical debt. When a
   new language feature or an in-language probe can replace one, replace it.

Rationale: `if constexpr` branches are type-checked, so both configurations stay compilable.
Templates and constants respect scope and namespaces, show up in diagnostics, and are visible to
tooling.

## 2. Namespaces and naming

- Standard entities are declared directly in `namespace std` (no inline versioning namespace).
  No ABI compatibility with libstdc++ or libc++ is attempted.
- Implementation details live in `ycxx::detail` with `snake_case` names. Detail namespaces
  avoid generic names that ADL could pick up (`swap`, `begin`, `get`); CPOs live in their own
  sub-namespaces (`ycxx::detail::swap_cpo`).
- **Base classes of std types live in `ycxx::adl_free`**, a namespace that declares no
  functions, only classes. A base's namespace is an associated namespace for ADL
  ([basic.lookup.argdep]/3), so a `ycxx::detail` base would expose every internal function to
  unqualified calls on the std type. Inside the library, internal function calls are qualified
  (`::ycxx::detail::f(...)`), so a user function with the same name in an argument's namespace is
  never picked up. Trait structs (`iterator_traits`, `pointer_traits`, `common_reference`) may
  still derive from `ycxx::detail` helpers, because they are never function arguments.
- Template parameters and locals use plain names (`T`, `first`), not reserved `_Ugly` names.
  Known deviation: a user macro that collides with such a name, defined before including a
  libycxx header, can break the header.

## 3. Freestanding layering

- `include/ycxx/core/**`: no OS, no libc headers, no heap unless an allocator is supplied, and
  no dependency on `<exception>`/`<typeinfo>`. C types and macros (`<cstddef>`, `<cstdint>`,
  `<climits>`) are defined from compiler-predefined macros only.
- `include/ycxx/hosted/**` plus `src/hosted`: anything needing the OS, reached only through the PAL.
- `include/ycxx/pal.h`: C-linkage platform hooks (`ycxx_pal_allocate`, `_write`, `_abort`,
  `_clock_now`, ...). `src/pal/posix` implements them on top of libc.

- **`std::mbstate_t` is core's own type.** The draft makes `mbstate_t` freestanding
  ([cwchar.syn]), and `char_traits::state_type` names it, so core defines it without the C
  library: an opaque, zero-initialisable struct with the C library's size and alignment (glibc
  and musl: 8 bytes, 4-byte alignment). The hosted `<cwchar>`/`<cuchar>` `static_assert` that
  layout and add `std::` overloads of the conversion functions taking `std::mbstate_t*`, which
  forward to the C functions; the `::mbstate_t*` versions are forwarding templates, so a null
  state pointer is not ambiguous. The cost: `std::mbstate_t` and `::mbstate_t` are distinct
  types, so a `std::mbstate_t` cannot be passed to the global `::mbrtowc` directly, and after
  `using namespace std;` the unqualified name `mbstate_t` is ambiguous once `<wchar.h>` is in.
- **The owning function wrappers are core.** Only `function_ref` is freestanding in the draft,
  but `function`, `move_only_function` and `copyable_function` need nothing hosted: small targets
  live in place, and larger ones use the replaceable `operator new`, which a freestanding program
  may define (the default reports `bad_alloc`). Keeping them in core avoids a preprocessor-
  selected include in `<functional>`.
- **Freestanding runtime archive.** `libycxx-freestanding.a` holds what a freestanding program
  may need defined but core headers must not define: the default replaceable allocation
  functions (no heap: `bad_alloc`/handler, `nullptr` for the nothrow forms) and `std::nothrow`.
  Every replaceable function lives in its own archive member, in the hosted `libycxx.a` too, so
  a program can replace any subset ([replacement.functions]); the defaults forward as
  [new.delete] specifies. (Weak definitions in headers were tried and rejected: they made
  replacement a redefinition error, and in hosted builds a weak definition keeps the linker from
  pulling the real `operator new` out of the archive.)

## 4. Error handling

- Every library "throw" goes through one of two hooks in `ycxx/core/error.hpp`. Both take an
  `ycxx_error_kind`. Without exceptions, both call the same handler.
  - `ycxx::detail::raise_with(kind, what, make)` is for exception classes defined inline in
    core headers (`bad_alloc`, `bad_optional_access`, `bad_variant_access`,
    `bad_expected_access<E>`, ...). With exceptions on, it throws `make()` from the header,
    which also works during constant evaluation (P3068, constexpr exceptions).
  - `ycxx::detail::raise(kind, what)` is for the `<stdexcept>` classes, whose message storage
    lives in the hosted runtime. With exceptions on, it calls `ycxx::detail::throw_std`, defined
    out of line there.
  - With `-fno-exceptions`, either hook calls `extern "C" ycxx_error_handler(kind, what)`; `make`
    is never called. The default definition is a weak symbol emitted from the header (PAL abort
    when hosted, `__builtin_trap()` when freestanding). A strong user definition replaces it at
    link time with no library rebuild.
- The language-support ABI (`__cxa_*`, `std::type_info`, unwinding) comes from the toolchain's
  ABI runtime (GCC's `libsupc++` plus `libgcc_s`/`libgcc_eh`), linked for both compilers. The
  exception classes are declared with Itanium layout and inline constexpr members (no key
  function). Their vtables and type_info are emitted where needed and merge with libsupc++'s
  copies; see `exception_base.hpp`. `std::nothrow` is `extern` and defined by libsupc++.
- **Unsupported: mixing translation units built with different `-fexceptions`/`-fno-exceptions`
  or `-frtti`/`-fno-rtti` settings in one program.** The inline error hooks differ between the
  modes, and the linker keeps one copy. The vtables of header-defined exception classes emitted
  without RTTI lack type_info. The one mitigation: without RTTI, the classes libsupc++ also
  defines declare an out-of-line destructor (their key function), so their vtables always come
  from libsupc++ (`YCXX_HAS_RTTI`). Whole-program `-fno-rtti` and whole-program
  `-fno-exceptions` are fully supported.

## 5. Compiler-builtin portability

Some trait builtins exist only on Clang (`__is_integral`, `__make_signed`, `__array_extent`,
...). These traits are implemented portably once in `ycxx/core/prim_traits.hpp` and
`ycxx/core/type_traits.hpp`. A builtin is used directly only when both compilers provide it
under the same name. Otherwise it gets one alias template in `config.hpp`.

| Feature | GCC 16.2 | Clang 23.1 | Result |
|---|---|---|---|
| `__builtin_is_within_lifetime` | no | yes | `std::is_within_lifetime` usable on Clang only (constraint) |
| `__builtin_is_corresponding_member` / `..._with_class` | yes | no | usable on GCC only (constraint) |
| `__builtin_type_order` | yes | no | `std::type_order` via a portable fallback (TBD) |

## 6. Development process

1. **Small commits.** One logical change per commit, with conformance test-count deltas in the
   message where relevant.
2. **Review every commit (P0–P2).** After committing, review that commit's diff:
   - **P0**: wrong behaviour, crashes, UB, build breaks, or a layering or policy violation
     (core reaching hosted/libc, preprocessor policy).
   - **P1**: conformance bugs (wrong constraints, `noexcept`, `explicit`, missing overloads),
     ODR or ABI hazards, missing `constexpr` where the standard requires it.
   - **P2**: notable quality issues: performance traps, dead code, misleading comments,
     missing tests.
3. **Fix, then commit again.** Review fixes go in a separate follow-up commit
   ("Review fixes for <commit>: ...") so history records what was wrong. Amend only for trivial
   fixes (typos, formatting), and only if the commit has not been pushed yet.
4. **Our own "pure" test suite (`tests/ycxx`).** Tests for new behaviour (C++23/26-era features
   and anything the two borrowed suites do not cover) are written by an independent author who
   may read only the working draft and cppreference.com. The author never sees libc++ or
   libstdc++ tests, or any implementation, libycxx's own included. A failing pure test is
   treated as a library bug until the draft shows otherwise; tests are never weakened to fit
   the library. Run with `tools/run-conformance ycxx gcc|clang`.
   - **The test author always runs.** One spec-only test-author agent is kept running in the
     background at all times, relaunched as soon as a batch finishes. It does two things:
     (a) widens coverage on its own, walking the draft for clauses of already-implemented
     headers that have no pure test yet; (b) takes nudges from current work. Whenever a header
     is being implemented or reworked, the agent is told which draft sections (stable names)
     that work covers, and those sections come first. The nudge names sections only, never
     files or code.
   - Each batch is committed on its own ("Own test suite: ..."), failures and all, before the
     library fixes, which go in a separate commit.
   - A failure caused by a missing compiler builtin is marked `// XFAIL-COMPILER: gcc|clang
     <reason>` and listed under compiler gaps in `STATUS.md`. The test body stays unchanged and
     reports XPASS once the compiler catches up.
5. **Gate before pushing:** `tools/check-all`, plus the affected conformance directories on
   both compilers and both suites.
