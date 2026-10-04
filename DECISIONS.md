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
- **`<string>` is core; its C-library parts live in the hosted runtime.** `basic_string` needs
  only an allocator, so it is defined in core (`ycxx/core/basic_string.hpp`), constexpr, with
  the integral `to_string`/`to_wstring`. The functions that need the C library (`sto*` through
  `strto*`/`wcsto*`; the floating-point `to_string`/`to_wstring`, which use `<charconv>`'s
  out-of-line code) are declared there and defined
  out of line in `src/hosted/string.cpp` (libycxx.a), as the `<stdexcept>` members are; a
  freestanding program that calls them gets a link error. So `<string>` includes no C header
  (unlike the C wrappers, it does not provide `errno`, `EOF`, `::uint32_t`, ...).
- **`<memory_resource>`: the classes are core, their definitions hosted.** `memory_resource`
  and `polymorphic_allocator` are defined in core (`ycxx/core/memory_resource.hpp`), so `<string>`
  (and the other containers, whose `pmr::` aliases name `polymorphic_allocator` through
  `ycxx/core/memory_resource_fwd.hpp`, which carries the one default template argument) can
  make `pmr::string` complete without a hosted include. Everything that needs a single
  definition is in the hosted runtime (`src/hosted/memory_resource.cpp`, libycxx.a): the
  destructor of `memory_resource` (its key function, so its vtable and type_info are emitted
  there, with RTTI), `new_delete_resource`/`null_memory_resource` (constant-initialized
  objects; their destructors run at exit and do nothing), the default-resource pointer
  (`__atomic` load/exchange), and all members of the pool resources and
  `monotonic_buffer_resource` (declared in `ycxx/hosted/memory_resource.hpp`).
  `synchronized_pool_resource` is the unsynchronized pool behind a three-state lock built on
  the PAL's `ycxx_pal_wait`/`ycxx_pal_wake_all` (no `<mutex>` dependency, no per-thread pools).
  The `<memory_resource>` header is hosted; a freestanding program can name the core types but
  gets a link error if it uses them.
- **`<system_error>`: the value types are core, the categories hosted.** `error_category`,
  `error_code`, `error_condition`, `system_error`, the comparisons, `hash` and the
  `is_error_*_enum` traits are defined in core (`ycxx/core/system_error.hpp`; they need only
  `<string>`). The hosted runtime (`src/hosted/system_error.cpp`) holds what needs one definition
  or the OS: `generic_category()`/`system_category()` (constant-initialized objects in a union
  that never destroys them, so they work in any static initializer or destructor), their
  messages (the new PAL hook `ycxx_pal_error_message`, `strerror_r` on POSIX, thread-safe and
  leaving `errno` alone), the destructors of `error_category` and `system_error` (key functions:
  one vtable and type_info, built with RTTI) and `system_error`'s constructors, which compose
  `what()` as `what_arg + ": " + message()` (the message alone for an empty or absent
  `what_arg`). `system_category().default_error_condition(ev)` maps 0 and every `errc` value to
  the generic category. The converting constructors find `make_error_code`/`make_error_condition`
  by argument-dependent lookup only ([contents]/3: a zero-argument deleted declaration hides the
  `std::` ones), so `<future>`/`<ios>` only need to specialize `is_error_code_enum` and declare
  their overloads. The `<system_error>` header itself stays hosted (it checks `errc` against
  `<errno.h>`).
- **Freestanding runtime archive.** `libycxx-freestanding.a` holds what a freestanding program
  may need defined but core headers must not define: the default replaceable allocation
  functions (no heap: `bad_alloc`/handler, `nullptr` for the nothrow forms) and `std::nothrow`.
  Every replaceable function lives in its own archive member, in the hosted `libycxx.a` too, so
  a program can replace any subset ([replacement.functions]); the defaults forward as
  [new.delete] specifies. (Weak definitions in headers were tried and rejected: they made
  replacement a redefinition error, and in hosted builds a weak definition keeps the linker from
  pulling the real `operator new` out of the archive.)
- **Floating-point `<charconv>` is out of line, in both archives** (`src/runtime/charconv`).
  The draft makes it freestanding-deleted, but nothing in it needs the OS: it works on
  stack-allocated big integers, so libycxx provides it freestanding too. The header passes the
  value's bits and a format tag (`fp_kind`) to one entry point per direction, so the extended
  floating-point types need neither `#if` nor per-type symbols, and the 128-bit power-of-ten
  table is computed once, at the library's compile time, instead of in every user TU. Integer
  conversions stay in the header (they are constexpr).

## 4. Error handling

- Every library "throw" goes through one of two hooks in `ycxx/core/error.hpp`. Both take an
  `ycxx_error_kind`. Without exceptions, both call the same handler.
  - `ycxx::detail::raise_with(kind, what, make)` is for exception classes defined inline in
    core headers (`bad_alloc`, `bad_optional_access`, `bad_variant_access`,
    `bad_expected_access<E>`, ...). With exceptions on, it throws `make()` from the header,
    which also works during constant evaluation (P3068, constexpr exceptions).
  - `ycxx::detail::raise(kind, what)` is the run-time path for the `<stdexcept>` classes. With
    exceptions on, it calls `ycxx::detail::throw_std`, defined out of line in the hosted runtime,
    which keeps the many call sites small. The helpers (`throw_out_of_range`, ...) are
    constexpr: during constant evaluation they throw the class from the header instead
    (`raise_std`, through `raise_with`), so `string::at` and friends throw catchable exceptions
    there.
  - With `-fno-exceptions`, either hook calls `extern "C" ycxx_error_handler(kind, what)`; `make`
    is never called. The default definition is a weak symbol emitted from the header (PAL abort
    when hosted, `__builtin_trap()` when freestanding). A strong user definition replaces it at
    link time with no library rebuild.
- **libycxx has its own Itanium C++ ABI runtime** (`src/abi`, `libycxx-abi.a`), written from the
  published Itanium C++ ABI documents (the base ABI's RTTI layout and dynamic_cast, 2.9; the
  exception-handling ABI, Levels I-II) and DWARF's pointer encodings; no runtime's source is used.
  It provides exception allocation, throw/catch/rethrow, the personality routine and LSDA
  parsing, catch matching, `std::type_info` and the `__cxxabiv1` RTTI classes, `__dynamic_cast`,
  static-local guards, terminate/new handlers, `std::nothrow`, and the reference counting that
  `exception_ptr` needs (libsupc++ exposes that only through libstdc++-internal symbols, so it
  cannot serve `exception_ptr` from the draft alone). Stack unwinding itself (Level I,
  `_Unwind_*`) still comes from the toolchain's unwinder, `libgcc_s`/`libgcc_eh`.
  Exception objects use the vendor class "YCXXC++\0" ("…\1" for the dependent exceptions
  rethrow_exception creates). The exception classes are declared with Itanium layout and inline
  constexpr members (no key function); their vtables and type_info are emitted where needed.
- **Constexpr `<stdexcept>` (P3068/P3378).** The nine classes keep one pointer to their
  message. At run time it points into a reference-counted heap block of the hosted runtime
  (`message_create`/`_retain`/`_release`, out of line as before, so a copy never throws and never
  allocates). During constant evaluation (`if consteval`) it points to a `new[]` array, and a
  copy duplicates it: an allocation cannot outlive constant evaluation, so the two forms never
  meet, and a failed allocation there is not a constant expression, so the copy constructor is
  still noexcept. The classes are therefore inline and constexpr throughout (no key function,
  as for `exception`). `ycxx/core/stdexcept.hpp` needs only `exception_base.hpp`, so `error.hpp`
  includes it; the constructors taking `const string&` are declared there and defined after
  `basic_string` (`stdexcept_string.hpp`, included at the end of `basic_string.hpp`; any caller
  holding a string has it). Under -fno-rtti in hosted builds they get the same out-of-line
  destructors as the runtime's own classes (defined in `src/hosted/stdexcept.cpp`, built with
  RTTI and `YCXX_EXCEPTION_KEY_FUNCTIONS`), because the runtime throws them; there they are not
  constexpr-destructible. `system_error` is not constexpr in the draft and keeps its key function.
  `__cpp_lib_constexpr_exceptions` stays undefined: Clang 23 cannot throw during constant
  evaluation, and GCC 16 offers no way to make a non-null `exception_ptr`
  (`current_exception`/`rethrow_exception`) work there for libycxx's `exception_ptr`.
- **Unsupported: mixing translation units built with different `-fexceptions`/`-fno-exceptions`
  or `-frtti`/`-fno-rtti` settings in one program.** The inline error hooks differ between the
  modes, and the linker keeps one copy. The vtables of header-defined exception classes emitted
  without RTTI lack type_info. The one mitigation: without RTTI, the classes the runtime throws
  itself declare an out-of-line destructor (their key function), so their vtables always come
  from the runtime, built with RTTI (`src/abi/exception_classes.cpp`, `YCXX_HAS_RTTI`). Whole-program `-fno-rtti` and whole-program
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

## 7. Iostreams and localization (hosted)

- **Layering.** Every stream and locale header is hosted (`ycxx/hosted/*.hpp`), with the
  non-template code in the hosted runtime (`src/hosted/{ios,iostream,locale,num,time,fstream,
  syncstream}.cpp`). Core headers that the draft makes declare stream operators for their types
  (`<string>`, `<string_view>`, `<bitset>`, `<memory>`, `<system_error>`, `<complex>`) declare or
  define them against declarations of the stream templates (`ycxx/core/iosfwd.hpp`, or a local
  declaration in `<complex>`) and get the definitions from `<istream>`/`<ostream>`, so they include
  no stream header. `ycxx/hosted/iosfwd.hpp` is the single declaration carrying the default
  template arguments.
- **locale** is a pointer to a reference-counted, immutable implementation object (facet array
  indexed by `locale::id`, name). `locale::id` gets its index on first use, so facets work during
  static initialization; the classic locale is built on first use and never destroyed. Named
  locales: `"C"`, `"POSIX"`, `"C.UTF-8"` and `""` (the environment, which must name one of
  those, else the classic locale); any other name, and the `_byname` facets with such a name,
  throw `runtime_error`. The environment's own conventions (other languages, money, dates) are
  not supported: no C-library locale is consulted.
- **Classic-locale choices** where the draft leaves them implementation-defined:
  `codecvt<wchar_t, char, mbstate_t>` converts UTF-32 to and from UTF-8 (so `encoding()` is 0
  and `max_length()` 4), `ctype<charT>` for character types other than `char` classifies ASCII
  only, `widen`/`narrow` map 0-255 one to one, `time_get`/`time_put` use the "C" conventions,
  `messages` has no catalogs. The deprecated `codecvt<char16_t/char32_t, char, mbstate_t>` are
  provided (Annex D) without `[[deprecated]]`.
- **num_get / num_put** convert with `<charconv>` (stage 3 of num_get, stage 1 of num_put), not
  with the C library: the results are correctly rounded and independent of the C locale.
- **The standard stream objects** are raw storage in the runtime, constructed by a runtime
  `ios_base::Init` object with `init_priority(100)` (before any program static object, after the
  runtime's own), never destroyed; that object's destructor flushes them. Their buffers work on
  the C streams through C stdio (unbuffered while synchronized), so `sync_with_stdio(true)` needs
  no extra coordination.
- **filebuf** works on a C stdio `FILE` with stdio buffering off (the filebuf buffers); the
  open-mode table maps to `fopen` modes including `"x"` for `noreplace`; `native_handle_type` is
  the POSIX file descriptor. `<fstream>` also includes `<cstdio>`.
- **syncbuf** keeps its output in a `basic_string` with its allocator; `emit()` takes a lock that
  belongs to the wrapped buffer alone (a slot table keyed by address, on the PAL's wait/wake), so
  no `<mutex>` dependency and no false sharing of locks between buffers.
- **Bitmask types** `fmtflags`, `iostate`, `openmode` are unscoped enumerations nested in
  `ios_base` with hidden-friend operators (an integer literal such as `0` does not convert to
  them, as with libstdc++).
- **Small conformance-preserving additions:** `fpos` has `operator==(const fpos&, I)` for integral `I`, so
  `pos == 0` is not ambiguous; `istream::ignore(streamsize, char_type)` is a constrained template
  (exactly `char_type` is deduced), so `ignore(n, -1L)` is not ambiguous; the rvalue stream
  operators exclude `ios_base` itself ("derived from" in the core-language sense).

## 8. `<random>`

- **Layering.** `<random>` is a core header (the draft makes most of it freestanding). Its stream
  operators are hidden-friend templates whose every use of the stream is dependent, so they are
  defined in core against `ycxx/core/iosfwd.hpp` (as in §7) and need `<istream>`/`<ostream>` only
  where they are used. `random_device` is declared in core and defined in the hosted runtime
  (`src/hosted/random.cpp`), like the C-library parts of `<string>`: it reads the PAL's random
  sources (`ycxx_pal_random_open/_read/_close`; tokens `"default"`/`"getrandom"` for the system
  generator, `"/dev/urandom"`, `"/dev/random"`), buffering 64 bytes per object.
- **Engines** follow the draft's state sequence X literally (ring buffers for the lagged engines,
  compared and printed in logical order), with all arithmetic in `unsigned long long` reduced
  modulo 2^w or m, so narrow `UIntType`s never meet integral promotion. `discard` is O(log z) for
  `linear_congruential_engine` (affine-map squaring) and O(1) for `philox_engine` (counter
  arithmetic). `default_random_engine` is `mt19937`.
- **generate_canonical** is the C++26 algorithm exactly: a power-of-two range takes the top d
  bits of k draws; any other range uses compile-time R^k and x with 192-bit integers (64-bit when
  R^k fits). It also accepts the extended floating-point types; the distributions accept the
  three standard floating-point types and the standard integer types only (the
  implementation-defined subsets of [rand.req.genl] are otherwise empty).
- **Distributions** are unbiased (uniform integers by Lemire's method, or rejection), and every
  inserter writes all internal state (normal's cached variate) with max_digits10 digits, so an
  extracted distribution continues the same sequence. The sampling distributions derive their
  cumulative tables from the stored parameters alone for the same reason.
