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
   - the `YCXX_*` switches in `config.hpp` (user-settable `YCXX_HARDENED` and `YCXX_NO_TRANSITIVE_INCLUDES`, §19; parse-level
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

8. **Facts about the target are discovered, not written down.** What a C library, an assembler
   or a compiler's code generation provides is never encoded as knowledge about a platform
   (`#if defined(__APPLE__)` for "has no strfromd", a hand-made list of symbols). A fact the
   preprocessor can see is tested where it is used: a macro (`#ifndef _PRINTF_NAN_LEN_MAX`,
   `!defined(PRIb8)`) or a header (`__has_include_next(<uchar.h>)`). Anything else is found
   when libycxx is configured, by CMake probes compiled against the real toolchain on every
   target, and handed to the code as generated files: `cmake/ycxx-c-library.cmake` writes the
   C library's `YCXX_C_HAS_*` switches to `<ycxx/generated/c_library.hpp>` (included by
   `config.hpp` when found; without it a C23 C library is assumed, so a gap is a compile error
   naming the function), and the fundamental type_info probe (`CMakeLists.txt`) writes the
   symbols `src/abi/rtti.cpp` hides (§2). Each probe is documented where it is defined.
9. **Keep looking for replacements.** Every remaining preprocessor use is technical debt. When a
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
- **libycxx's symbols have hidden visibility: a program or shared object exports none of them**
  (one table excepted, below).
  Another C++ library in the same process must neither take over libycxx's definitions nor be
  taken over by them. On Darwin, libSystem loads Apple's libc++ and libc++abi into every process,
  and dyld coalesces each exported weak definition with a non-weak one of the same name in any
  loaded image: `std::current_exception` (inline in a header) became libc++'s, and with GCC the
  exception classes' type_info, so `catch (const std::exception&)` stopped matching. In the
  other direction an exported definition of a name libc++abi defines weakly (`operator delete`,
  the `<stdexcept>` type_info objects) is patched into the shared cache, for every system
  library. On ELF the same happens when libycxx and libstdc++ or libc++ meet in one process
  (a shared library built with either, in a program built with the other). One model on every
  target:
  - **Headers.** Every file-scope opening of `std` and `ycxx` is
    `namespace [[gnu::visibility("hidden")]] std {`. The attribute is written on each block:
    it applies to that block only (both compilers export what a reopening without it declares),
    and a nested namespace definition cannot carry attributes, so `namespace std::ranges {` is
    `namespace [[gnu::visibility("hidden")]] std { namespace ranges {` ... `}}`. The attribute
    reaches everything declared in the block, nested namespaces, classes and their members,
    templates and their instantiations (explicit ones and those with program types), type_info
    objects and vtables, inline variables (verified, GCC 16.2 and Clang 23.1).
    `tools/check_visibility.py` enforces it (`--fix` rewrites new openings). Rejected:
    `#pragma GCC visibility push(hidden)`/`pop` in each header. It needs no restructuring, but
    it is preprocessor where an attribute does the job (§1), and it also hides what a header
    declares at global scope or includes: the C library's functions (a hidden reference cannot
    bind to a shared libc), the replaceable functions; every header would have to keep its
    C-library includes outside the region.
  - **Archives.** The sources of `libycxx.a`, `libycxx-abi.a` (and the freestanding runtime
    archive) say what is hidden themselves, as the headers do: every file-scope opening of `std`,
    `ycxx` and `__cxxabiv1` in `src/` carries the attribute (`tools/check_visibility.py` covers
    `src/` too), and what they define outside those namespaces carries
    `[[gnu::visibility("hidden")]]` on its declaration: the ABI entry points (`__cxa_*`,
    `__dynamic_cast`, `__gxx_personality_v0`), the PAL (`ycxx/pal.h`), the replaceable hooks'
    defaults (`handle_contract_violation`, `ycxx_error_handler`) and the runtime's markers. No
    `-fvisibility=hidden`: a flag changes what a declaration means without the source saying so
    (it would also narrow the allocation table's declaration, which must stay default), and a
    build of the sources by other means gets the same result. Only `ycxx_allocation_functions`
    is exported (checked: `nm` of both archives, both compilers). Users need no flag to get
    hidden symbols, but GCC warns
    (`-Wattributes`) about each program class with a member or base of a library class type: it
    gives such a class the lower visibility and says so. The warning says nothing about the
    program, so the CMake package (`ycxx::headers`) and `tools/ycxx-cxx` pass `-Wno-attributes`
    to GCC; other build systems add it themselves (STATUS, known limitations). What the compilers
    keep default despite an attribute is hidden with assembler directives (`asm((constant-expression))`, `.hidden` on ELF,
    `.private_extern` on Mach-O): GCC's seven predeclared `__cxa_*` entry points (GCC ignores an attribute on them with a warning),
    and the fundamental type_info objects GCC emits with `__fundamental_type_info`'s key function.
    Which of those exist depends on the target (AArch64 adds `__bf16`, `__mfp8` and the SVE types),
    so their list is not written down: CMake compiles a probe defining that key function with the
    runtime's flags at configure time, lists its `_ZTI`/`_ZTS` symbols with `nm`, and generates
    `abi/fundamental_type_infos.hpp` in the build tree, which `src/abi/rtti.cpp` turns into
    `.hidden`/`.private_extern` directives.
  - **Default visibility** stays only for what is not libycxx's to hide: the C library
    functions `ycxx/core/c_stdlib.hpp` declares by assembler name, and a program's own
    definitions. That includes a program's replacement `operator new`: it is linked instead of
    the archive member holding the hidden default ([replacement.functions]), and is exported as
    the program's other functions are (the nothrow forms are declared
    `[[gnu::visibility("default")]]` in `<new>` so that a replacement of them is too: a function
    otherwise takes the hidden visibility of its parameter type `std::nothrow_t`).
  - **The allocation table.** libycxx's default allocation functions are hidden (assembler
    directives, `src/runtime/new/hidden.hpp`), so they never take over, and are never taken
    over by, another runtime's (Apple's libc++abi, libstdc++). Yet the images of a process that
    link libycxx must share one set: an object built in a shared library and destroyed in the
    program is allocated by one image and freed by the other, which pairs only while both reach
    the same functions (a program that replaces `operator delete`, AddressSanitizer's
    alloc-dealloc-mismatch). Each image therefore holds `ycxx_allocation_functions`
    (`src/runtime/new/allocation_table.{hpp,cpp}`), a weak, exported, constant-initialized table
    under a name only libycxx uses, whose entries call that image's `::operator new` ...
    `::operator delete[]` (thunks with `size_t`/`void*` signatures). The dynamic linker binds
    every image to the first image's table: the program's, which the CMake package and
    `tools/ycxx-cxx` keep in it (`-u` of a hidden anchor in the table's archive member, since a
    libycxx shared library on the link line would satisfy `-u` of the table itself, and
    `--export-dynamic-symbol` where the linker has it; both probed, `cmake/ycxx-link.cmake`); in a host that does not link libycxx, the first
    libycxx library's. Each default first looks up its entry and forwards when the entry is not
    its own image's; otherwise it is the process's default. So a program's replacement serves
    every libycxx image ([replacement.functions]/2), objects cross libycxx images, and libycxx
    and another runtime keep their own allocation functions, `new_handler` and `bad_alloc`
    (`tests/cmake/visibility`: "mine 7 other 7", with libycxx's library in a libycxx program
    and in a host built with the toolchain's library). The cost: one indirect call per
    allocation in a shared library, one comparison in the program. A program's own replacement
    is exported and also serves the other runtime's code, the platform's ordinary rule for a
    program that replaces `operator new`. Rejected: libycxx's defaults with default visibility
    (as libstdc++ and libc++): on Darwin dyld coalesces them with libc++abi's by name, taking
    each from the first image in load order that defines it (observed, macOS 26), so a libycxx
    program's default served Apple's libc++ (whose failure then threw libycxx's `bad_alloc` into
    code built with libc++abi), and a libycxx shared library in a host without libycxx got
    libc++abi's `operator new` (libc++abi's `bad_alloc`, foreign to libycxx's runtime, and
    libycxx's `new_handler` never called); ELF interposition does the same when libycxx and
    libstdc++ meet. Per-image hidden defaults without the table (libycxx before 2026-10-05)
    could not serve a program's replacement to its shared libraries.
  - **The ABI runtime is per image.** `__cxa_*`, `__gxx_personality_v0`, the `__cxxabiv1`
    type_info classes and their vtables, `std::type_info` and the classes the compiler looks
    up (`std::initializer_list`, `std::align_val_t`, `std::bad_alloc`, the comparison
    categories, `std::coroutine_handle`, `source_location::__impl`, ...) are hidden like the
    rest: the compilers reference them by name, and the reference binds to the definition in
    the same image. Exporting them would make a process's other runtime bind half of its names
    to libycxx's and half to its own (observed with GCC's predeclared entry points: the
    exception of a libstdc++ shared library aborted). A program and a shared library each
    linking libycxx therefore have separate runtimes, and exceptions still cross between them:
    both recognise the exception class, type_info objects are compared by name (§4), the
    runtime classifies a type_info object (class, pointer, ...) by the name of its ABI class
    when the address is another image's, and an exception is removed from the uncaught count
    of the runtime that added it (`exception_header::counted_in`). What is not shared: while
    an exception of one image unwinds through the other's frames, `uncaught_exceptions()`
    there does not count it, and `current_exception()`/`throw;` see only the handlers of their
    own image.
  - **Cost.** Nothing of libycxx can be exported for plugins to share. GCC warns
    (`-Wattributes`, "declared with greater visibility than the type of its field/its base")
    for every class of a program, outside libycxx's namespaces, that has a member or base of a
    libycxx class type: GCC has no way to hide a class's members, type_info and vtable without
    hiding the class's type (Clang's `type_visibility` attribute), and hidden class types are
    what keeps the exception classes from being coalesced on Darwin. Clang does not warn.
  Tested by `tests/ycxx/linkage/no_exported_library_symbols` and `tests/cmake/run.sh` (exports
  of the example programs; `tests/cmake/visibility`, libycxx and libstdc++ in one process).
- **Every name of the headers' own is reserved** ([macro.names]/1: a program that includes a
  standard header may `#define` any identifier the standard library does not declare and that
  [lex.name]/4 does not reserve: `#define C char`, `#define first`, `#define detail`). So the
  headers (include/ and the runtime's src/**/*.hpp) spell template parameters, function
  parameters, locals, members that are not standard, helpers, internal namespaces and internal
  macros as reserved identifiers; standard names (`first`, `value_type`, `size`) stay as they
  are, also where a local reuses one. The scheme depends only on the spelling:
  - lowercase-initial `x` -> `__x`: `ycxx` -> `__ycxx`, `detail` -> `__detail`,
    `adl_free` -> `__adl_free`, `first1` -> `__first1`, `size_` -> `__size_`, the platform
    layer `ycxx_pal_wait` -> `ycxx_pal_wait`, the allocation table
    `__ycxx_allocation_functions` (its `-u` anchor `__ycxx_allocation_table_anchor`);
  - one capital letter `X` -> `_Xp` (`T` -> `_Tp`, `C` -> `_Cp`: `_C`, `_L`, `_N`, ... are
    macros of some C libraries' `<ctype.h>`); a name that is already a capital and `p` gets a
    trailing `_` (`Ep` -> `_Ep_`); any other uppercase-initial `X` -> `_X` (`Alloc` -> `_Alloc`,
    `T1` -> `_T1`, `YCXX_HAS_RTTI` -> `_YCXX_HAS_RTTI`, `YCXX_HOSTED` -> `_YCXX_HOSTED`);
  - attributes take their reserved spellings: `[[__gnu__::__visibility__("hidden")]]`,
    `[[__gnu__::__cold__]]`, `__attribute__((__unused__))` (the standard attribute-tokens are
    reserved already, [cpp.replace.general]/9);
  - a spelling that the compilers or some platform already use (a keyword or builtin such as
    `__int128`, `__make_integer_seq`, a macro such as BSD's `__unused`, Darwin's `__weak` and
    `__block`, glibc's `__always_inline`; `tools/data/uglify/avoid.txt`) becomes `__y_x` /
    `_Y_X` (`unused` -> `__y_unused`, `int128` -> `__y_int128`).
  Not renamed: the names the standard library declares (the draft's index of library names,
  the names the std and std.compat modules export, and `tools/data/uglify/allowed.txt`'s
  [standard] section for those the index misses: `npos`, `failbit`, `param_type`, struct tm's
  members, `INT8_C` ...), and libycxx's documented user-facing names: `YCXX_HARDENED`, `YCXX_NO_TRANSITIVE_INCLUDES` (§19), and the
  `-fno-exceptions` hook `ycxx_error_handler`, `ycxx_error_kind` and its `ycxx_error_*`
  enumerators (§4), and the hosted layers' interface that integrators implement (§18): the
  primitives `ycxx_pal_*` with `YCXX_PAL_NOEXCEPT`/`YCXX_PAL_NORETURN` of `<ycxx/pal.h>`, and the
  CMake names `YCXX_PAL`, `YCXX_HOSTED_LAYERS`, `YCXX_PAL_<LAYER>_PROVIDER`,
  `ycxx_add_hosted_layer`. In the runtime's sources the names the C library and the system declare
  stay (`exception_class` of `_Unwind_Exception`, `link`, `unlink`, `truncate`; [src-platform]).
  Comments keep their text, but code in them follows (backquoted code, `ycxx::`-qualified names,
  `ycxx_`/`YCXX_` words); prose in DECISIONS, STATUS and the docs names internals by their plain
  spelling (`ycxx::detail::precondition` is `__ycxx::__detail::__precondition`).
  **`tools/uglify.py`** does the renaming on the token level (string literals, header names,
  `#pragma` lines and the C library's assembler names untouched), in include/, src/, the C++ and
  symbol names of the CMake files and tools/ycxx-cxx; it is idempotent, and records every name it
  renamed (`tools/data/uglify/renamed.txt`) so that a merged source written with the old names is
  fixed by running it again. The header generators (tools/gen_*.py) keep plain-name templates
  and pass their output through it. `tools/uglify.py --check` (policy stage of `tools/test`, so
  `tools/check-all`) fails when a header spells a non-reserved name the standard does not
  declare. `tests/ycxx/conformance/nasty_macros*` define the 5874 identifiers the headers used
  before the renaming as macros expanding to invalid tokens, then include every public header,
  together, one by one, and after `import std;`; every name a later run renames in include/
  joins that list (`tools/data/uglify/nasty-macros.txt`) and the tests. The draft's index is a snapshot
  (`tools/uglify.py --fetch-index` refreshes it).
  **The index misses names**, mostly members a program provides and the library looks up
  (`Rcvr::make_receiver_for`, `Environment::template env_type<...>`) and members a program
  names (`member_offset::bytes`, the designator `.annotations`): renamed, they break programs
  silently. So `--check` also compares the renaming with **`tools/data/uglify/draft-names.txt`**,
  the names the draft's library code spells for programs, which `tools/gen_draft_names.py`
  extracts from https://eel.is/c++draft/full and which records the draft's revision
  (Eelis/draft's commit). Read are the library's clauses ([library] to [exec], and [depr]),
  without examples and notes (a program's own names), and in them the code blocks, the item
  declarations and the code in sentences and tables. A name counts when it is declared at
  namespace or class scope (a declarator-id outside parentheses and template argument lists, a
  `using` alias, a class, a concept, an enumerator), used as a member or a qualified name (after
  `.`, `->`, `::`, also `::template`: this covers the requirements' `X::type`, the designators
  and the protocols of program-defined types), or alone in a table's first column (the
  enumerators of `path::format`). Exposition-only names are told apart by their typesetting:
  eel.is renders the draft's italics as `<i>`, so an italic name never counts, nor does a
  declaration with a comment "exposition only" (after it, or on the line before it); the members
  of a class whose name is italic do count (programs read `insert-return-type`'s `inserted` and
  `node`). Parameters and template
  parameters are inside parentheses or template argument lists (eel.is marks the latter's angle
  brackets, unlike the `<` operator), and locals are in a function's body: a code block of an
  item's description, or one whose top level has statements, is a body. A capital letter (`T`,
  `X`) is a placeholder. What this cannot tell apart, exposition-only members that the draft
  does not set in italics (`regex_iterator`'s `match`, `basic_stringbuf`'s `mode`, when_all's
  `disp`) and a few table entries and slips, is listed in `allowed.txt`'s `[draft-internal]`
  section with the reason, and stays renamed. `--check` fails when a name of `draft-names.txt`
  is renamed (spelled reserved in the headers, or recorded in `renamed.txt`) and is neither
  allowed nor in `[draft-internal]`, or when a `[draft-internal]` entry excuses nothing; the
  renaming run warns about the same names. The fix is the `[standard]` section, and a test in
  `tests/ycxx` (`conformance/standard_member_names/` or the area's directory) that spells the
  name from a program-defined type. Names the core language looks up in a program's types
  (`get_return_object_on_allocation_failure`, `::handle_contract_violation`) are outside the
  library's clauses and are listed in `[standard]` by hand. To refresh the list for a new
  draft: `python3 tools/gen_draft_names.py && python3 tools/uglify.py --check`. After a merge:
  `python3 tools/uglify.py && python3 tools/gen_std_module.py && python3 tools/uglify.py --check`.

## 3. Freestanding layering

- `include/ycxx/core/**`: no OS, no libc headers, no heap unless an allocator is supplied, and
  no dependency on `<exception>`/`<typeinfo>`. C types and macros (`<cstddef>`, `<cstdint>`,
  `<climits>`) are defined from compiler-predefined macros only. The one exception is the
  compiler's own `<stddef.h>` (GCC and Clang ship it for freestanding environments; it is not the
  C library's): `std::max_align_t` must be `::max_align_t` ([support.c.headers.other]/1), a class
  only that header can name, so core reads it (`#include_next`, past libycxx's own `<stddef.h>`,
  which is `<cstddef>` in C++: Clang's header lacks `::nullptr_t` there; partial `__need_*`
  requests of C library headers and C go to the compiler's) and declares `using ::max_align_t;`. The
  freestanding builds keep `-nostdinc` (no C library) and add the compiler's header directory
  back (`-isystem $(cc -print-file-name=include)`); `tools/check_includes.py` does not
  follow `#include_next`.
- `include/ycxx/hosted/**` plus `src/hosted`: anything needing the OS, reached only through the PAL.
- `include/ycxx/pal.h`: C-linkage platform hooks (`ycxx_pal_allocate`, `_write`, `_abort`,
  `_clock_now`, ...). `src/pal/posix` implements them on top of libc.

- **`std::mbstate_t` is the C library's `::mbstate_t` when hosted, core's own type freestanding.**
  [support.c.headers.other]/1 makes `std::mbstate_t` and `<wchar.h>`'s `::mbstate_t` one type,
  and `char_traits::state_type` (core) names it, so in hosted builds core's `char_traits.hpp`
  reads the C library's `<wchar.h>` (`ycxx/hosted/c_wchar.hpp`, under `#if YCXX_HOSTED`, which
  `tools/check_includes.py` does not follow) and declares `using ::mbstate_t;`. So `<string>` and
  every header with `char_traits` also declare the C library's `<wchar.h>` names in the global
  namespace when hosted. The draft makes `mbstate_t` freestanding ([cwchar.syn]); without a C
  library core defines it (`ycxx/core/mbstate.hpp`): an opaque, zero-initialisable struct with the
  target C library's size and alignment (glibc and musl: 8 bytes, 4-byte alignment; Darwin: 128
  bytes, 8-byte alignment; `cfg::mbstate_size`/`_align`, which `c_wchar.hpp` checks against
  `::mbstate_t`), so both configurations agree on the layout of everything holding one. The
  library's own conversion state fits either form: the `codecvt` facets keep only a pending
  UTF-16 surrogate (a `char32_t`, 4 bytes) in the object representation, which glibc's and musl's
  8 bytes hold, no side table needed; the `<cuchar>` fallbacks (Darwin) use the last 16 of
  Darwin's 128 bytes. Rejected: keeping core's own type in hosted builds with `std::` overloads
  of the conversion functions (the previous design): `std::mbstate_t` and `::mbstate_t` were
  distinct, so unqualified `mbstate_t` was ambiguous under `using namespace std;` with
  `<wchar.h>`, and a `std::mbstate_t` could not be passed to `::mbrtowc`.
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
  but the C library's `<wchar.h>` for `mbstate_t` (unlike the C wrappers, it does not provide
  `errno`, `EOF`, `::uint32_t`, ...).
- **`<memory_resource>`: the classes are core, their definitions hosted.** `memory_resource`
  and `polymorphic_allocator` are defined in core (`ycxx/core/memory_resource.hpp`), so `<string>`
  (and the other containers, whose `pmr::` aliases name `polymorphic_allocator` through
  `ycxx/core/memory_resource_fwd.hpp`, which carries the one default template argument) can
  make `pmr::string` complete without a hosted include. Everything that needs a single
  definition is in the hosted runtime (`src/hosted/memory_resource.cpp`, libycxx.a): the
  destructor of `memory_resource` (its key function, so its vtable and type_info are emitted
  there, with RTTI), `new_delete_resource`/`null_memory_resource` (constant-initialized
  objects that are never destroyed, so they stay usable during termination), the default-resource pointer
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
  their overloads. The `<system_error>` header itself is hosted (it checks `errc` against
  `<errno.h>`); freestanding it skips that check and provides `errc` (its freestanding part) and
  the classes, whose categories then fail to link (see the C library headers below).
- **Freestanding runtime archive.** `libycxx-freestanding.a` holds what a freestanding program
  may need defined but core headers must not define: the default replaceable allocation
  functions (no heap: `bad_alloc`/handler, `nullptr` for the nothrow forms) and `std::nothrow`.
  Every replaceable function lives in its own archive member, in the hosted `libycxx.a` too, so
  a program can replace any subset ([replacement.functions]); the defaults forward as
  [new.delete] specifies. (Weak definitions in headers were tried and rejected: they made
  replacement a redefinition error, and in hosted builds a weak definition keeps the linker from
  pulling the real `operator new` out of the archive.)
- **The C library headers with freestanding parts compile without the C library.** `<cstdlib>`,
  `<cstring>`, `<cwchar>`, `<cerrno>` and `<system_error>` stay hosted wrappers
  (`tools/gen_cheaders.py`), but with `YCXX_HOSTED` 0 (`-ffreestanding`) they include core headers
  instead of the C library's (`#if YCXX_HOSTED`, which `tools/check_includes.py` understands):
  `ycxx/core/c_stdlib.hpp` (div_t & co., EXIT_*, bsearch, a heapsort qsort, and the start and
  termination functions as forwarders to the environment's `abort`/`exit`/... through
  declarations with assembler names, so a C header the program may still include declares
  distinct entities), `ycxx/core/c_string.hpp` (the freestanding string and wide-string functions,
  written out; memcpy/memmove/memset/memcmp through the builtins, which both compilers already
  require of a freestanding environment), `ycxx/core/cerrno_macros.hpp` (the E* values of
  `errc`), and `<system_error>` skips only its errno cross-check. Each function is a
  `template <class = void>`, so a same-named C function declared by the program wins ties under
  `using namespace std;`. What both modes share (constexpr div, memalignment, memset_explicit)
  is outside the conditional. `<cstdarg>` is core in both modes: va_start takes one argument
  ([cstdarg.syn]), which Clang 23 supports only through `__builtin_va_start(V, 0)` with its
  -Wvarargs check silenced (`YCXX_HAS_C23_VA_START` picks GCC's C23 builtin). `<stdbit.h>` and
  `<stdckdint.h>` are core headers of templates and inline functions in the global namespace; the
  C library's versions (type-generic macros) are not included.
- **The `.h` forms of the C headers with C++ additions are libycxx's own** (generated by
  `tools/gen_cheaders.py`; [support.c.headers.other]/1 places each name of `<cname>` in the
  global namespace). In C++, `<stdlib.h>`, `<inttypes.h>`, `<string.h>` and `<wchar.h>` include
  `<cstdlib>`, `<cinttypes>`, `<cstring>`, `<cwchar>` and add the names those declare themselves (`using std::abs;`,
  `div`, the const-correct `bsearch` pair, `memalignment`, `free_sized`, `imaxabs`,
  `memset_explicit`, the const-correct `strchr` and `wcschr` pairs, ...); `<time.h>` and `<uchar.h>`
  likewise wrap `<ctime>` and `<cuchar>`; in C they are the C library's (`#include_next`), as `<math.h>` is.
  `<complex.h>` and `<tgmath.h>` include `<complex>` (and `<cmath>`) in C++ and never the C
  library's, whose `complex`/`I` and type-generic macros would break C++ code. The `<cname>`
  headers read the C library's header with `#include_next`. A C function that is an exact match
  for one of libycxx's (`int abs(int)`, `div_t div(int, int)`, `labs`, `ldiv`, `imaxabs`, ...)
  would win overload resolution against libycxx's constexpr templates, cannot be redeclared
  constexpr, and C's `bsearch` conflicts with the const pair. The same holds for the searching
  functions whose C declaration is `char* strchr(const char*, int)` (`memchr`, `strchr`,
  `strpbrk`, `strrchr`, `strstr`, and the wide `wcschr`, `wcspbrk`, `wcsrchr`, `wcsstr`,
  `wmemchr`): [cstring.syn] and [cwchar.syn] replace it with a const/non-const pair, which
  cannot coexist with it; and for `atexit`/`at_quick_exit`, which [support.start.term] declares
  noexcept. So `<cstdlib>`, `<cinttypes>`, `<cstring>` and `<cwchar>` rename those C
  declarations on every C library (glibc's own C++ pairs too, so nothing depends on which C
  library it is) while they read the C library's header (`#define abs ycxx_c_abs`,
  `#include_next`, `#undef`; the renamed declarations are never used) and then place
  libycxx's functions under the C names in the global namespace, so code that calls `::abs` or an
  unqualified `abs` after `<cstdlib>` keeps working. This is preprocessor use the language cannot
  replace (rule 4 of §1). Should the C library's header have been read before `<cstdlib>`
  (through a path that bypasses libycxx's include directory), its functions stay and win ties
  against libycxx's templates: the global names are then the C library's, not constexpr. `<wchar.h>`
  works the same way for `wcschr`, `wcspbrk`, `wcsrchr`, `wcsstr` and `wmemchr` (glibc declares
  the const-correct pairs only for GCC, Darwin never): its declarations are renamed while
  `ycxx/hosted/c_wchar.hpp` reads it, the one place that does, since core's `char_traits.hpp`
  reads it too and must not get there first without the renames; libycxx's pairs (templates)
  call the C functions through declarations with their assembler names
  (`ycxx::detail::c_wchar`, as `c_stdlib.hpp` does) and are placed in the global namespace by
  `<cwchar>` and `<wchar.h>`. The
  wrappers wrap `<cname>` in `extern "C++"`, as a C header may include them inside `extern "C"`. `tools/check_freestanding.sh`
  compiles every header of [compliance]'s Table 27 as well as the core ones.
- **C-library values that core spells out are per C library family.** Core cannot include the
  C headers, yet `errc`, the freestanding `<cerrno>` and `<cmath>` macros and `mbstate_t`'s
  layout must equal the C library's. `config.hpp` names the family once (`YCXX_TARGET_DARWIN`
  for the macros, which must be literals usable in `#if`; `cfg::darwin` for everything else):
  the Linux values (glibc, musl; also the bare-metal default) or Darwin's (BSD errno numbers,
  `FP_NAN` 1 .. `FP_SUBNORMAL` 5, a 128-byte `mbstate_t` for the freestanding definition). (These
  are freestanding values that core must spell out without the C library's headers, each checked
  against them; what a hosted wrapper needs to know about the C library is probed instead, §1
  rule 8.) `errc` gives each value as
  `errno_number(Linux, Darwin)`; the freestanding macro lists are checked against `errc`, and
  every value against the C library's headers wherever those are included (`<system_error>`,
  `<cwchar>`, `<cuchar>`, `src/hosted/cmath_check.cpp`). Where the C library lacks a C23 function
  of a wrapper (`strfromd/f/l`, `mbrtoc8`/`c8rtomb`, `timespec_getres`, or the whole of
  `<uchar.h>`; found by `cmake/ycxx-c-library.cmake`, §1 rule 8), a `YCXX_C_HAS_*` switch replaces
  the using-declaration with libycxx's own,
  defined in the hosted runtime (`src/hosted/strfrom.cpp`, `src/hosted/uchar.cpp`, built on every
  platform). `strfrom*` are templates (as `free_sized` is), so a C library that gains them wins
  unqualified calls; the `<cuchar>` fallbacks are plain functions, as the C library's would be.
- **Floating-point `<charconv>` is out of line, in both archives** (`src/runtime/charconv`).
  The draft makes it freestanding-deleted, but nothing in it needs the OS: it works on
  stack-allocated big integers, so libycxx provides it freestanding too. The header passes the
  value's bits and a format tag (`fp_kind`) to one entry point per direction, so the extended
  floating-point types need neither `#if` nor per-type symbols, and the 128-bit power-of-ten
  table is computed once, at the library's compile time, instead of in every user TU. Integer
  conversions stay in the header (they are constexpr).

- **`<atomic>` needs no libatomic.** Types whose size is 1, 2, 4, 8 or (with `__int128`) 16
  bytes and for which `__atomic_always_lock_free` holds are lock-free: every operation works on
  the object as an unsigned integer of that size through the `__atomic` builtins, with padding
  bits cleared before a value is stored or compared (compare-and-exchange compares value
  representations; a failure caused only by differing stored padding, possible through
  `atomic_ref`, is retried), so `atomic<long double>` RMW loops terminate. Every other type is
  lock-based: each operation holds one lock of a striped table of 256 locks in the runtime
  archive (`src/runtime/atomic`, in both `libycxx.a` and `libycxx-freestanding.a`), selected by the
  object's address. `atomic<T>` aligns its object to its size whenever the size is one of those
  representation sizes, so its layout does not depend on `-mcx16`. Waiting and notifying use a
  second address-keyed table of 256 slots (a waiter registers, re-checks the value and blocks on
  the slot's version counter through the PAL's `ycxx_pal_wait`; notify bumps the version and
  wakes only when the slot has waiters, so notify_one wakes the whole slot). The freestanding
  archive's default PAL wait returns at once (waits become spins); a freestanding program can
  supply blocking `ycxx_pal_wait`/`ycxx_pal_wake_*`. The deprecated parts of [depr.atomics]
  (`memory_order::consume`, which `<stdatomic.h>` names, `kill_dependency`, `atomic_init`,
  `ATOMIC_VAR_INIT`, volatile members for types that are not always lock-free) are provided,
  marked `[[deprecated]]` where the language allows (§6, Annex D). A volatile member is two
  overloads: one constrained on `is_always_lock_free`, and a `[[deprecated]]` one on its negation;
  the volatile non-member functions are split the same way.
- **The thread support library is built on the PAL's address wait, not on pthread objects.**
  Mutexes are three-state futex locks, condition variables sequence counters, call_once a
  four-state word; all are constexpr-constructible (where the draft allows); the mutexes are
  trivially destructible, and a condition variable's destructor only waits for notified waiters
  to stop touching it ([thread.condition.condvar]/5 allows destroying it while they return). Threads, sleeping, the thread-end list (`notify_all_at_thread_exit`, the
  `*_at_thread_exit` results) and timed waits are PAL hooks (`ycxx_pal_thread_*`,
  `ycxx_pal_wait_until`, `ycxx_pal_at_thread_end`). A timed wait on system_clock waits on the
  realtime clock, one on any other clock on the monotonic clock for the remaining time and then
  re-checks `Clock::now()` (whose exceptions propagate). No `native_handle` is provided for
  mutexes and condition variables (`thread::native_handle()` is the pthread handle). A thread's
  entry function lets a foreign exception (the forced unwind of `pthread_exit` or cancellation)
  pass through instead of calling terminate. The POSIX PAL's address wait is the futex on Linux
  and the kernel's ulock compare-and-wait on Darwin (`__ulock_wait`/`__ulock_wake`: not in the
  SDK's headers, but the interface libSystem's `os_unfair_lock` and Apple's own libc++ use since
  macOS 10.12; the public `os_sync_wait_on_address` needs macOS 14.4 and is not usable from GCC,
  which has no `__builtin_available`), with relative timeouts in microseconds.
- **The thread-end actions run for the thread that ends the program too.** The `*_at_thread_exit`
  results ([futures.promise]/23, /26, [futures.task.members]) and `notify_all_at_thread_exit`
  ([thread.condition.nonmember]/2-3) act "when the current thread exits, after all objects with
  thread storage duration associated with the current thread have been destroyed". A thread that
  calls `exit` (returning from `main` does, [basic.start.main]/5) destroys its thread_local
  objects as part of `exit` ([support.start.term]/9.1, [basic.start.term]/2), and only then are
  static objects destroyed and the `atexit` functions called: that is where its actions belong,
  so a static `future`, condition variable or mutex sees them done before it is destroyed.
  `quick_exit`, `_Exit` and `abort` destroy no thread_local objects and run none; threads still
  running when the program ends never exit and run none. A pthread key destructor (the previous
  design) runs only when a thread ends on its own, so the actions of the thread calling `exit`
  never ran. Now the POSIX PAL keeps a thread's list in a thread_local pointer and runs it from a
  thread_local destructor of its own, the *sentinel*, registered with the C library's list of
  thread_local destructors (`__cxa_thread_atexit_impl`, Darwin's `_tlv_atexit`) before every other
  destructor of the thread. The C library runs that list in reverse order of registration, also
  the destructors registered while it runs, both when the thread ends and in `exit` (glibc's
  `__call_tls_dtors`, Darwin's `_tlv_exit`, both before the static destructors), so the sentinel
  runs after every other thread_local destructor of the thread. An action registering another
  action (or constructing a thread_local) while the list runs is run too: the list is drained, and
  a new registration re-arms the sentinel. Where a destructor cannot be registered (a C library
  without `__cxa_thread_atexit_impl`) the list falls back to the pthread key, as before.
- **The sentinel is registered first, early (G7, Darwin).** "Before every other destructor" cannot
  rely on libycxx seeing the others: Clang on Darwin (Darwin's TLV ABI; `clang -S` shows
  `bl __tlv_atexit`) registers a program's thread_local destructors with `_tlv_atexit` itself,
  while on ELF Clang and GCC, and GCC on Darwin (emulated TLS), call `__cxa_thread_atexit`,
  libycxx's, which goes through `ycxx_pal_thread_atexit`. Arming the sentinel only there and at
  the first action (the first G7 design) made a thread_local constructed before a thread's first
  `*_at_thread_exit` call outlive the actions on macOS with Clang, and only there (the macOS CI
  failures of `at_thread_exit_by_*`, `review_at_thread_exit_retry` and
  `thread/many_at_thread_exit_registrations`, Clang only: CI run 240, where GCC passed them all).
  So the POSIX PAL arms it where no thread_local of the thread can have been constructed yet:
  - a thread `ycxx_pal_thread_create` starts (`thread`, `jthread`, `async`, the parallel
    scheduler) arms it before its initial function runs (the PAL's start routine wraps the
    caller's; one more allocation and one registration per thread, about 0.2 µs on Linux against
    9 µs for a start and join, below the noise);
  - the main thread arms it in an initializer of the PAL: on ELF with priority 100 (reserved to
    the implementation), before every static initializer of the program; on Mach-O, which has no
    priorities, in link order, after the program's object files (libycxx's archives come last);
  - `ycxx_pal_thread_atexit` and the first `ycxx_pal_at_thread_end` still arm it, on any thread.
  The order relied on is then one and the same everywhere: the sentinel is the first registration
  of the thread's list, and the C library runs that list last-in first-out, in `exit` too. On
  glibc, `__cxa_thread_atexit_impl` is the C library's side of the C++ ABI's
  `__cxa_thread_atexit`, through which both compilers destroy thread_local objects in reverse
  order of construction ([basic.start.term]/4), with no code of their own to order them. On
  Darwin, `_tlv_atexit` is not in Apple's public documentation (no SDK header declares it; the
  PAL declares it, with the signature Clang calls); libycxx relies on what Clang's code generation
  requires of it in the same way (`clang -S`: one `_tlv_atexit` call per object, nothing else
  orders the destructors), and on the macOS runs with GCC, where every registration, the
  sentinel's included, has gone to `_tlv_atexit` through the PAL in program order, and every
  order check passed, at thread end and in `exit`. What the next macOS run with Clang tells:
  everything passing confirms the design. Failures only in `at_thread_exit_by_main_return` and
  `_by_exit` (the main thread) point at the initializer (it ran after the program's first
  thread_local, or `_tlv_atexit` before `main` does not reach the list `exit` runs); failures in
  `at_thread_exit_by_thread_exit` (a std::thread calling `exit`), `at_thread_exit`,
  `review_at_thread_exit_retry`, `thread/many_at_thread_exit_registrations` (300 thread_local
  destructors per thread, so also the LIFO order at length), `linkage/thread_local_at_thread_exit`
  or `condition_variable/notify_all_at_thread_exit` point at the start routine's registration, and
  failures everywhere, GCC included, at the LIFO premise itself.
  What remains ordered by first use only, on Darwin with Clang: a thread the program starts
  itself (`pthread_create`) and a thread_local of the main thread constructed by the program's
  own static initialization, both before their first `*_at_thread_exit` call. Their actions still
  run after every thread_local constructed after that call
  (`future/at_thread_exit_foreign_thread`). Everywhere else every order holds (ELF: libycxx's
  `__cxa_thread_atexit` arms it first anyway).
  The simulation on Linux: `tests/ycxx/support/tlv_bypass.hpp` and the `*_tlv_bypass` tests,
  linked with `-Wl,--wrap=__cxa_thread_atexit`, send every thread_local destructor of the program
  (and of libycxx's C++ code) straight to `__cxa_thread_atexit_impl`, as Clang does with
  `_tlv_atexit` on Darwin, so that only the PAL's own registrations order the list; the
  first G7 design fails them with the macOS CI messages, this one passes. The
  `ycxx_pal_at_thread_end` contract (`pal.h`) says what other providers of the `threads` layer
  must do.
- **`<stop_token>` is core.** Its stop state needs only atomics and three PAL hooks: the address
  wait (through `<atomic>`'s tables), `ycxx_pal_thread_self` (a callback deregistered while
  `request_stop` runs it: on the requesting thread it is not waited for) and
  `ycxx_pal_thread_yield` (the list lock's backoff). The freestanding runtime archive defaults to
  one thread of execution (identity 1, yield does nothing); a freestanding program with threads
  supplies its own, as for the wait. `stop_source`'s shared state uses the replaceable
  `operator new`, as the function wrappers do. Core so that `<execution>`'s senders, which use
  `inplace_stop_source`, are freestanding-capable.
- **`<rcu>` and `<hazard_pointer>` are hosted, with their state in the runtime.** One RCU domain
  with epochs: a global counter advances with every retire and every `rcu_synchronize`; each
  thread that enters a region owns a reader record (released at thread end) where its outermost
  lock stores the epoch it read, followed by a fence. A retired object (queued without
  allocation through `rcu_obj_base`, in epoch order) may be evaluated once every record is clear
  or holds a later epoch, so only regions that began before the retire hold it back (the proof
  is in `src/hosted/rcu.cpp`). Evaluations run by `rcu_barrier`, or by an outermost unlock or a
  retire outside any region once 1000 are queued, one batch at a time; `rcu_barrier` inside a
  region evaluates what was retired before the region began. **`rcu_barrier` in the two
  situations [saferecl.rcu.domain.func]/4 cannot satisfy** (it has no precondition and no
  exception): (1) Inside a region R, an evaluation scheduled after R began can only be evaluated
  after R ends ([saferecl.rcu.general]/5), so if its scheduling happens before the call the
  barrier must block for ever. For the caller's own retires (sequenced before the call) libycxx
  does exactly that, and checks it as a hardened precondition, as it does for `rcu_synchronize`
  inside a region (the draft's Effects block for ever there too): a certain self-deadlock
  becomes a diagnosed termination with `YCXX_HARDENED`. Both checks are in `<rcu>` (a runtime
  query, then `precondition`), since the runtime itself is not built with `YCXX_HARDENED`
  (`rcu_synchronize`'s check used to be in the runtime, where it was never active). Another thread's retire after R began is
  taken as not happening before the call (the barrier cannot tell whether other synchronization
  ordered it), so it is not waited for. (2) Inside a scheduled evaluation E, /4 would have the
  barrier wait for E itself, whose evaluation includes the call: impossible (a draft defect,
  STATUS). libycxx's barrier there evaluates the rest of the batch E belongs to, then waits for
  the readers and evaluates the queue up to its bound like any barrier, keeping the evaluation
  lock (other barriers must not see E's batch as done while it runs): when it returns, everything
  scheduled before the call has been evaluated except the evaluations in progress on the calling
  thread. (Before, it returned at once.) Rejected: releasing the evaluation lock while waiting
  (a barrier on another thread would then return before E finished). Rejected: two phase counters
  flipped by `rcu_synchronize` (the previous design), which cannot tell a region that began
  before a retire from one that began after it, so a barrier inside a region waited for itself.
  The cost is a third word in `rcu_obj_base` (the node's epoch: a barrier inside a region must
  find exactly the queued prefix retired before the region began). Hazard pointers are records
  of a push-only list; retiring links the object into a retired list through its
  `hazard_pointer_obj_base`, and the retiring thread reclaims the unprotected ones once the list
  exceeds twice the number of records plus 64.

- **`<regex>` is hosted; one syntax tree, two matchers.** regex_traits needs `<locale>`, so the
  header is hosted; the name tables (class names, POSIX collating symbols) and regex_error's
  members are in the runtime (`src/hosted/regex.cpp`), everything else is templates
  (`ycxx/hosted/regex_{base,compile,engine}.hpp`, `regex.hpp`). All six grammars parse into one
  tree. ECMAScript (and POSIX with back-references) runs on a backtracking matcher with an
  explicit stack, never native recursion over the input; it remembers failed (pc, position)
  pairs when the program allows it, and otherwise has a step budget (error_complexity) and a
  frame budget (error_stack). The POSIX grammars otherwise compile to a Thompson NFA: an NFA
  simulation finds the leftmost-longest match, and the subexpressions are assigned afterwards by
  the POSIX rule from the tree (each subpattern, left to right, the longest that still lets the
  match complete), so leftmost-longest needs no exhaustive search.

- **POSIX matching with back-references: two phases** ([re.synopt]/1 basic, extended, awk, grep,
  egrep; [re.alg.match], [re.alg.search]; IEEE Std 1003.1 XBD 9.1, 9.3.6 and regexec()). A POSIX
  program that cannot run on the NFA (back-references; bounded repetitions beyond 256 copies or
  65536 expanded nodes; a non-matching list holding a multi-character collating element) is
  matched in two phases:
  1. *The match.* The backtracker explores every path from each start position in turn and keeps
     the longest end (leftmost-longest); it stops early at a path reaching the end of the input.
  2. *The subexpressions.* POSIX's rule is a lexicographic order: "each subpattern, from left to
     right, shall match the longest possible string" (XBD 9.1), a subpattern's length being
     decided before what is inside it. A second, guided search over the syntax tree with the
     match's span fixed decides each node's end *on entry*: a concatenation tries its first
     element's end from the largest down, then that element's inside, then the next element's
     end; an alternation tries its alternatives in order (the first that fits the span, as the
     NFA resolver does); a repetition tries each iteration's end from the largest down. The
     first complete path in this order is the POSIX answer. It backtracks on an explicit stack
     with continuations (no recursion over the input); length bounds per node prune the ends;
     a node no back-reference outside it refers to commits to its first way of matching a span
     (what follows cannot depend on its inside), and a node without back-references remembers
     the spans it cannot match.
  Rules shared by both phases, from XBD 9.3.6: a back-reference to a subexpression that did not
  participate fails ("\(a\)*\1" does not match "a"); a repeated subexpression reports, and is
  referred to by, its last iteration, and the subexpressions inside an iteration are reset at its
  start ("\(a\(b\)*\)*\2" does not match "abab"); an iteration matches the empty string only
  when it is needed for the minimum count or is the only iteration ("\(a*\)*" against "bc":
  \1 is the empty string at 0).
  Limits (implementation-defined; regex_error): phase 1 keeps the backtracker's step budget
  (error_complexity beyond 2*10^7 + 32 x (input reached) x (program size) steps) and frame
  budget (error_stack beyond 2^22 frames); phase 2 has the same step budget over the match and
  its node count, and error_stack beyond 2^22 pending goals or choice points. Patterns without
  back-references that fit the NFA keep the NFA path unchanged.

- **Collating elements and primary keys** ([re.traits]/7-8, [re.grammar]/8, /10, /14.3; XBD
  9.3.5). `transform_primary` returns the primary key for a `collate_byname` facet (exact type)
  whose key form is known: glibc's multi-level keys (the weights before the first level
  separator), or keys that are a copy of the string (a locale without collation rules: every
  character its own class, the whole key is primary). *Deliberate divergence:* for the classic
  locale's own facet (exactly `collate<charT>`) it returns the whole key too (code point order,
  each character its own class), where the letter of [re.traits]/7 gives an empty string and so
  makes every `[=x=]` invalid in the default locale ([re.grammar]/10): portable code uses
  `[[=a=]]` there, and libc++ and libstdc++ both accept it (STATUS "Deliberate divergences",
  "Draft issues noticed"). Other facets (a user's collate, Darwin's undocumented keys) give an
  empty string and `[=x=]` is invalid (error_collate). `lookup_collatename` accepts one character, the POSIX collating-symbol
  names, and, for a `collate_byname` locale, a multi-character collating element of that
  locale: the C library's own `regcomp` is asked, under that locale (`uselocale`), whether
  `[[.xy.]]` is valid (cs_CZ defines "ch"; glibc exposes no other public interface to the
  elements). In a bracket expression a multi-character element is an alternative of its own:
  a matching list matches it as one element (2 or more characters, tried before the single
  characters); a non-matching list does not match where one of its listed elements begins
  (XBD 9.3.5 leaves both unspecified). A range end that is a multi-character element is valid
  only with `collate` (its sort key bounds the range). With `[=x=]`, a multi-character `x` adds
  itself and the characters of its primary class.

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
- **The ABI runtime on Darwin.** Unwinding is libSystem's (LLVM libunwind) for both compilers.
  Its `<unwind.h>` returns the LSDA as `uintptr_t` (accepted through an overload pair) and has no
  text- or data-relative bases (asked for only when an encoding needs them, which never happens
  there). Clang's Apple arm64 ABI marks a `type_info` that may be duplicated across images by
  setting bit 63 of its name pointer; `std::type_info` and the runtime clear it before reading
  (`ycxx::detail::rtti_name`) and compare such names as strings, as they compare every name not
  marked `*`. libSystem's processes may also hold Apple's libc++abi: libycxx's runtime is linked
  statically into the executable and bound there (two-level namespace), so the two runtimes
  coexist with separate exception state; Apple's exceptions are foreign to libycxx's (only
  `catch (...)` catches them) and vice versa. Thread-local destructors go to `_tlv_atexit`
  (Clang calls it directly; GCC, with emulated TLS, through `__cxa_thread_atexit` and the PAL),
  so the thread-end sentinel is registered at thread start and before `main` (§3).
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
  `__cpp_lib_constexpr_exceptions` is defined only where the whole feature works: constant
  evaluation can throw (`YCXX_HAS_CONSTEXPR_EXCEPTIONS`, from `__cpp_constexpr_exceptions`) and
  hold exceptions in `exception_ptr` (below), i.e. on GCC 16; Clang 23 cannot throw during
  constant evaluation, so there it is undefined.
- **What is not constexpr.** `current_exception`, `uncaught_exceptions`, `nested_exception`,
  `throw_with_nested` and `rethrow_if_nested` are not constexpr in the draft: P3068 made them
  so, P3818 took it back (a `const` local initialized from them would be constant-initialized
  in a context without exceptions), leaving the exposition-only current-exception. They stay
  run-time functions ([constexpr.functions]/1 forbids adding constexpr), although GCC 16 could
  evaluate them: it also has `__builtin_uncaught_exceptions()` (found among the compiler's
  strings; it counts the evaluation's uncaught exceptions). Own test `exception/not_constexpr`.
- **`exception_ptr` during constant evaluation (GCC).** GCC 16 keeps a constant evaluation's
  exceptions itself and has two builtins for them, documented nowhere (not in its manual) but
  reported by `__has_builtin` (`YCXX_HAS_CONSTEXPR_EXCEPTION_PTR`; the names were found among the
  compiler binary's strings, as for §13): `__builtin_current_exception()` returns a
  `std::exception_ptr` whose one data member is set to the handled exception's object (it
  requires `std::exception_ptr` to be declared), and `__builtin_eh_ptr_adjust_ref(p, n)` adds n
  to that object's reference count; both are rejected outside constant evaluation (at run time
  the first gives a null `exception_ptr`). Its evaluator also implements `__cxa_throw` (and the
  other entry points its throw expressions call), and throwing an object that is already
  referenced is a rethrow of it. So, in `if consteval` branches: the copy constructor and the
  destructor adjust the count, current-exception ([exception.syn], used by `make_exception_ptr`'s
  `try { throw e; } catch (...)`) is the builtin, `rethrow_exception` is
  `__cxa_throw(p, nullptr, nullptr)`, and `exception_ptr_cast<E>` lets a `catch (const E&)` of
  such a rethrow decide (the reference stays valid while the `exception_ptr` holds the object).
  These are GCC's private interface to its own library, verified by experiment only
  (own test `exception/exception_ptr_constexpr`); should GCC change them, the probe turns the
  feature off and the test fails rather than the library.
- **`make_exception_ptr` without exceptions** creates the primary exception object directly
  through the runtime (`ycxx::abi::exception_object_create`: the header `__cxa_throw` would
  fill in, one reference owned by the exception_ptr) and copy-constructs `e` into it, so
  [propagation]/12 holds in `-fno-exceptions` code: `exception_ptr_cast` observes the copy and code
  built with exceptions can rethrow it. It needs `typeid(E)` (the object's type must be
  recorded for handler matching), so under `-fno-exceptions -fno-rtti` it still returns a null
  exception_ptr (nothing in such a program could match the object anyway).
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
| `__builtin_is_structural` | yes | no | `std::is_structural` declared on GCC only (`#if`, type-taking) |
| reflection (`^^`, metafunctions) | yes (`-freflection`) | no | `<meta>` empty on Clang (§13) |

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
5. **Annex D (deprecated features) is implemented.** Every deprecated library feature the current
   draft still specifies is provided exactly as [depr] specifies it; removed features (no longer
   in the draft) are not. Each Annex D entity carries `[[deprecated("<hint>")]]` wherever the
   language allows the attribute (classes, functions, variables, alias templates, enumerators,
   explicit and partial specializations; since GCC ignores the attribute on a partial
   specialization, its `value`/`type` member repeats it); macros cannot carry it (STATUS: Annex D).
   The library never uses a deprecated entity itself: its own sources build with `-Werror`, and
   only `src/hosted/locale.cpp`, which must install the Annex D codecvt facets in the classic
   locale, disables `-Wdeprecated-declarations`.
6. **Gate before pushing:** `tools/check-all`, plus the affected conformance directories on
   both compilers and both suites.
7. **Test scripts are traceable.** `tools/test` is the single driver. Each stage prints the
   commands it runs, shows live progress and ends with a summary; the full logs go to
   `build/test-logs/`. The scripts are POSIX sh and POSIX awk, so they run unchanged on Linux
   and macOS. lit runs through `uvx` with a pinned version, so a checkout needs only uv, not a
   Python environment. `tools/run-conformance` brings `build/<compiler>` up to date before it
   tests, so a run can never test a stale library.
   A passing test must show evidence, not just a verdict. The lit formats record every command
   a test runs, with its exit status, duration and output (`tests/ycxxlit/transcript.py`), for
   passing tests as well as failing ones. The terminal lists every test together with its
   steps. Each run's HTML report (`build/test-logs/<run>.html`) holds every test's transcript
   and the run's provenance: commit, compiler version, command and host. `tools/test` adds
   `run.html`, one composite page for the whole run. It shows every stage, every failure in
   full (a stage's log tail, a test's transcript) and every suite with every test. Passing
   tests' full transcripts stay in the suites' own reports, which keeps the page shareable. Every report can be copied, or
   read without a browser, as self-contained Markdown that a person or an AI assistant can act
   on directly.
8. **Sanitizer runs test an instrumented libycxx.** A sanitizer checks only the code compiled
   with it. ThreadSanitizer derives happens-before from the atomic operations it sees and from
   the functions it intercepts (pthread, malloc); libycxx's mutexes, condition variables,
   `call_once`, the parallel scheduler's queue and the exception and `shared_ptr` reference
   counts are atomics and futex calls, so in an uninstrumented archive every hand-off through
   them is invisible and reported as a race (observed: the 307 reports of the six `execution/`
   failures of 2026-10-06 all crossed `src/hosted/parallel_scheduler.cpp`'s queue or the
   exception reference counts of `src/abi/exception.cpp`; none remained with the instrumented
   library). So `SANITIZER=<list>` (`tools/test -s`) compiles the tests with the sanitizers and
   links them with libycxx built with the same ones:
   - `-DYCXX_SANITIZE=address,undefined,thread` (any of them, the compilers' names) compiles
     `libycxx.a` and `libycxx-abi.a` with `-fsanitize=<list> -fno-sanitize-recover=all
     -fno-omit-frame-pointer`, and `ycxx::ycxx` adds `-fsanitize=<list>` to the program's link.
     Empty by default: the default build, and the runs without a sanitizer, are unchanged.
   - `tools/run-conformance` configures `build/<cc>-<sanitizers>` (`asan` -> `address`, `ubsan`
     -> `undefined`, `tsan` -> `thread`; `build/clang-asan-ubsan`, `build/clang-tsan`) on first
     use, RelWithDebInfo with the same compilers as `build/<cc>`, and brings it up to date
     before each run; the lit configurations hand it to `tools/ycxx-cxx --libdir=DIR`, so each
     test's transcript names the library it linked. `YCXX_LIBDIR` names another build.
   - Everything in both archives is instrumented, the ABI runtime included (the personality
     routine runs instrumented under the unwinder's calls; Clang's ThreadSanitizer pass gives
     every instrumented function an exception path that leaves its shadow frame). Not
     instrumented: the unwinder (the toolchain's `libgcc_s`) and the C library, which the
     sanitizers intercept where they need to.
   - The sanitizer runtimes are self-contained. Clang's static runtimes (linked whole, ahead of
     the program's archives) need nothing of a C++ runtime but `_Unwind_Backtrace` and
     `_Unwind_GetIP` (`nm -u`; ASan's C++ part also `__cxa_begin_catch`, for its
     `__clang_call_terminate`, which binds to libycxx's within the program's own link); GCC's
     `libtsan.so` needs `libc`, `libm` and `libgcc_s` only (`readelf -d`). All refer to
     `__cxa_demangle` weakly, which libycxx does not define: it stays null and the runtimes
     demangle with their own. No system C++ runtime enters the process.
   - What a runtime defines itself wins over libycxx's archive members, which are then never
     pulled in. ASan's global allocation functions are weak, so a program's replacement still
     wins; the tests of libycxx's own (new_handler loops, forwarding) are `UNSUPPORTED-SANITIZER:
     asan`. ThreadSanitizer's (Clang's `libclang_rt.tsan_cxx`) are strong: a replacement would be
     a second definition. They are its only C++ part, and race detection does not need them
     (libycxx's call malloc, which is intercepted), so `tools/ycxx-cxx` and `ycxx::ycxx` link
     Clang's TSan programs with `-fno-sanitize-link-c++-runtime`; libycxx's allocation functions
     and allocation table then serve the program, and the own suite's TSan runs set
     `allocator_may_return_null=1` so that malloc reports failure as C specifies.
     ThreadSanitizer's runtime also defines `__cxa_guard_acquire`/`release`/`abort`; libycxx's
     are an archive member of their own (`src/abi/guard.cpp`), so the sanitizer's, which it
     understands, are used. ASan's weak `__cxa_throw` and `__cxa_rethrow_primary_exception`
     interceptors give way to libycxx's (strong, in the program); its `_Unwind_RaiseException`
     interceptor sits in front of `libgcc_s`'s and unpoisons the stack on every throw.
   - GCC's ThreadSanitizer runtime is a shared library, `libtsan.so`, with the global
     allocation functions in it, and GCC's driver links it ahead of every input
     (`libtsan_preinit.o -ltsan` before the objects, `g++ -###`; no option moves it, and
     `-static-libtsan` links the runtime whole, its allocation functions included). A definition
     in a shared library satisfies an undefined reference as well as an archive member would, so
     the members holding libycxx's allocation functions were never linked and the runtime's served
     the program: 7 own tests of libycxx's own behaviour failed (`new/aligned_nothrow`,
     `aligned_edge_sizes`, `class_aligned_lookup`, `new_handler_loop`, `replacement_forwarding`,
     `memory/allocator_allocate_overflow`, `linkage/shared_library_replaced_new`). Naming the
     functions with `-u` does not help, for the same reason (checked with GNU ld and lld). But a
     definition in the program wins over a shared library's for every reference the program
     links, provided its archive member is linked, and a weak one as well (ELF: only a regular
     object's definition is preferred to a shared one; checked with both linkers). So each
     default allocation function's file also defines a hidden anchor,
     `__ycxx_allocation_anchor_<file name>`, which no other library defines, the defaults are weak
     definitions, and a ThreadSanitizer build's link options (`cmake/ycxx-link.cmake`, in
     `ycxx::ycxx` and `<build>/ycxx-link-options` for `tools/ycxx-cxx`) name all 20 anchors as
     undefined, as they name the allocation table's: libycxx's defaults are linked into every
     program and serve it, ahead of `libtsan.so`'s, and a program's own replacement, a strong
     definition, still wins over the weak default ([replacement.functions]). CMake checks at
     configure time that every allocation function file defines its anchor. Clang's TSan programs
     get the same options; there `-fno-sanitize-link-c++-runtime` is still needed, since
     `libclang_rt.tsan_cxx` is linked whole as regular objects, whose strong definitions would win
     over the weak defaults. Verified with GCC 16.2's own `libtsan.so` (built with libsanitizer by
     `tools/toolchain/provision --with-sanitizers`): the 7 tests pass, and the whole own suite
     under GCC's ThreadSanitizer (STATUS, own-suite configurations).
   - GCC's `-Wtsan` (on by default, an error under `-Werror`) is turned off for a ThreadSanitizer
     build: the library's seq_cst fences order a store before a later load (atomic
     wait/notify, hazard pointers, rcu); no data relies on them for happens-before.
   - Sanitized programs run slower and start slower; the own suite gives a test program 180 s
     instead of 60 under a sanitizer.
   - A ThreadSanitizer report is a race in the library (fixed, never suppressed) or in a test
     (the test fixed), or a false positive explained in `tests/ycxx/tsan.supp`, which the own
     suite passes in `TSAN_OPTIONS` on each program's command line. It holds one entry, for the
     test of fence-to-fence synchronization (`atomic/fences`), since ThreadSanitizer does not
     model stand-alone fences; nothing in the library is suppressed, and the library's
     reference counts use acq_rel decrements rather than fences for that reason (§15).

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
  static initialization; the classic locale is built on first use and never destroyed.
- **Named locales are the C library's** (glibc, Darwin's libc; `src/hosted/locale_named.cpp`).
  `"C"`, `"POSIX"` (named `"C"`) and `"C.UTF-8"`/`"C.utf8"` have the classic facets on every
  platform. Any other name is valid for a category when `newlocale` accepts it for that category
  (`locale(name)`: every category; `locale(other, name, cats)`: those of `cats`); `""` is the
  environment's name per category (`LC_ALL`, `LC_<category>`, `LANG`; `"C"` when that names no
  locale the C library has), and composite names have the form
  `LC_COLLATE=...;LC_CTYPE=...;LC_MONETARY=...;LC_NUMERIC=...;LC_TIME=...;LC_MESSAGES=...` (one
  name when all six agree). A named category holds the `_byname` facets of its name: ctype,
  `codecvt<wchar_t, char, mbstate_t>`, numpunct, moneypunct, time_get, time_put, collate and
  messages for char and wchar_t. `codecvt<char, char>`, the UTF `codecvt`s ([locale.codecvt.general]
  makes them locale-independent), num_get/num_put and money_get/money_put stay the classic
  objects: they read the locale through the other facets. The `_byname` facets of other
  character types check the name and have the classic semantics.
  - *One `locale_t` per (name, category)*, opened with that category and the name's LC_CTYPE (so
    strings convert in the name's encoding), reference-counted by the facets that use it and
    freed with the last; immutable once opened. Data the facets need often is read once: the
    ctype<char> table and case maps, widen/narrow tables (btowc; narrow is wctob by the reverse
    table); numpunct, moneypunct and time_get read theirs when constructed. Per-call work uses
    the `_l` functions (`is*_l`, `isw*_l`, `tow*_l`, `strcoll_l`, `strxfrm_l`, `wcscoll_l`,
    `wcsxfrm_l`, `strftime_l`, `wcsftime_l`, `nl_langinfo_l`), and the calling thread's
    `uselocale` (restored before returning) where the C library has none (`mbrtowc`/`wcrtomb`,
    `btowc`, `catopen`; glibc's `localeconv`, called under a lock because it fills one static
    object; Darwin's `localeconv_l` is found by a `requires` probe). The global C locale and other
    threads' locales are never changed, except by `locale::global` ([locale.statics]/2:
    `setlocale` per category for a named locale).
  - *Values the draft leaves to the implementation:* ctype<char> gives a byte that is not a
    character by itself (`btowc` is `WEOF`: a multibyte encoding's lead and continuation bytes)
    no class and no case mapping (glibc's `is*_l` agree; Darwin's read such a byte as the code
    point of its value); a numpunct/moneypunct separator that is
    not one char in the locale's encoding (fr_FR.UTF-8's U+202F) is `' '` for the narrow facet
    when it is a space character, else the classic value; the wide facet has the character; an
    empty `thousands_sep` gives `','` and no grouping. moneypunct's patterns follow POSIX's
    `cs_precedes`/`sep_by_space`/`sign_posn` (`int_` ones for `Intl`): a separating space is the
    pattern's `space` field (so money_get then requires white space there,
    [locale.money.get.virtuals]/2), sign position 0 gives the sign string `"()"`, and
    `curr_symbol()` is `currency_symbol`/`int_curr_symbol` unchanged (libstdc++'s choice;
    libc++ moves the space into the symbol). time_get reads the locale's day, month and AM/PM
    names and its `%c %x %X %r` formats (`D_T_FMT` & co.), its eras (`ERA`; `%EC %Ey %EY`) and
    era formats (`%Ec %Ex %EX`) and its alternative digits (every O form, `%OC` included: the
    locales' own formats use it); `get_date` reads the `%x` format;
    `date_order()` is the order of `%x`'s fields. `codecvt::encoding()` is 1 for single-byte
    encodings, else 0 (a state-dependent encoding is not detected: the only probe, `mbtowc(0, 0,
    0)`, resets a state shared by all threads). messages opens catalogs with `catopen`
    (`NL_CAT_LOCALE`, the name's LC_MESSAGES) and `catgets`; the classic messages has none.
    `locale::encoding()` is the C library's `CODESET` for the name's LC_CTYPE.
  - Rejected: precomputing every facet's data in the locale_t entry (the time and money data
    are only needed by those facets), and caching locale_t objects forever (a program that
    walks many names would keep them all).
- **Classic-locale choices** where the draft leaves them implementation-defined:
  `codecvt<wchar_t, char, mbstate_t>` converts UTF-32 to and from UTF-8 (so `encoding()` is 0
  and `max_length()` 4), `ctype<charT>` for character types other than `char` classifies ASCII
  only, `widen`/`narrow` map 0-255 one to one, `time_get`/`time_put` use the "C" conventions,
  `messages` has no catalogs. The Annex D `codecvt<char16_t/char32_t, char/char8_t, mbstate_t>`
  (and their `codecvt_byname`) are `[[deprecated]]`; the classic locale still holds them, so
  `src/hosted/locale.cpp` alone is built with `-Wno-deprecated-declarations`.
- **num_get / num_put** convert with `<charconv>` (stage 3 of num_get, stage 1 of num_put), not
  with the C library: the results are correctly rounded and independent of the C locale.
- **The standard stream objects** are raw storage in the runtime, constructed by a runtime
  `ios_base::Init` object with `init_priority(100)` (before any program static object, after the
  runtime's own), never destroyed; that object's destructor flushes them. Their buffers work on
  the C streams through C stdio (unbuffered while synchronized), so `sync_with_stdio(true)` needs
  no extra coordination. `sync_with_stdio(false)` gives cout and wcout a buffer of their own and
  changes nothing else.
- **The wide objects do wide I/O on the C streams.** [iostream.objects.overview]/6 makes mixing
  wide and narrow operations behave as on FILEs, and [ios.members.static]/3 makes the
  synchronized objects' characters the C stream's: so wcin, wcout, wcerr and wclog use `fgetwc`,
  `ungetwc` and `fputws`/`fputwc` (a null-terminated copy of each piece of a write), the C stream
  becomes wide-oriented (`fwide(stdout, 0) > 0` after `wcout << L"x"`), and `wprintf`, `fputws`
  and `fgetwc` interleave with them; mixing `cout` and `wcout` on one C stream is what mixing
  byte and wide functions is in C. The C library then converts, with its `LC_CTYPE`: until the
  program calls `setlocale`, that is the "C" locale, where (glibc) a character outside the basic
  character set fails and sets badbit, as `fputwc` does. A buffer whose locale gets another
  codecvt<wchar_t, char, mbstate_t> facet than the one it started with (a program's own facet,
  through `imbue`) behaves as basic_filebuf<wchar_t> ([iostream.objects.overview]/2): it converts
  through that facet and does byte I/O (`fwrite`, `getc`/`ungetc`), as libc++'s
  wcout-imbue/wcin-imbue tests expect. The classic locale and the named locales share the
  classic facet, so imbuing them keeps the wide C I/O. Rejected: converting with the classic
  codecvt (UTF-8) and writing bytes, as libycxx did before: the C stream was left byte-oriented,
  so `fputws` after `wcout` failed and `fgetwc` after `wcin.unget()` too (libstdc++
  27_io/objects/wchar_t/{9662,12048-2,12048-4}.cc); and the C library's conversion for every
  locale, imbued facets ignored, which breaks [iostream.objects.overview]/2.
- **Concurrent use of the synchronized standard objects** ([iostream.objects.overview]/7: no
  data race from concurrent formatted and unformatted input and output) without slowing down
  other streams or single-threaded programs:
  - *The stream state and `gcount`* of every stream are read and written with relaxed atomic
    operations, which compile to the ordinary loads and stores on x86-64 and AArch64 (the
    compiler can no longer merge or drop them; nothing else changes). `setstate` adds bits with an
    atomic OR only when one of them is new, so the read-modify-write instruction runs only on a
    transition to failure or end of file, once per stream in practice; `clear` is a store.
    `width(n)` stores only a changed value (every inserter ends with `width(0)`).
  - *The standard objects' buffers* hold a futex mutex of their own (`ycxx::detail::futex_mutex`)
    on their input side (underflow, uflow, pbackfail) and, for the wide buffers, around each
    write. It guards what a buffer keeps between calls (the last extracted character for
    sungetc; the wide buffers' locale and conversion states) and makes a peek (read, then give
    the character back with ungetwc, or ungetc) atomic with respect to the object's other
    readers, so they never see its characters out of order. Neither buffer holds characters
    itself: the wide one also returns a peeked or put-back character to the C stream (through an
    imbued codecvt, its bytes: more than ISO C's one byte of push-back for a multibyte
    character; glibc and Darwin's libc allow it), so C stdio
    and every reader see each character once, in order. Uncontended it costs one atomic exchange each
    way, on top of the C stream's own lock; in a single-threaded process (`single_threaded()`)
    plain loads and stores. Output through the narrow buffers takes no extra lock (putc and
    fwrite lock the C stream). Ordinary stream buffers are untouched: their get and put areas
    keep the inline fast paths of `basic_streambuf`, and the draft gives them no thread-safety
    guarantee.
  - Rejected: the C stream's own lock (`flockfile` with `getc_unlocked`) instead of the futex
    mutex. It would avoid the second lock, but ThreadSanitizer models neither it nor the C
    library's internal locks, and reports glibc's push-back storage (allocated by ungetc in one
    thread, freed by getc in another) as a race unless a lock it understands orders the calls.
    Also rejected: per-object locks in `basic_istream` (every stream would pay) and a one-
    character get area in the narrow buffers (C stdio would no longer see a peeked character,
    breaking [ios.members.static]/3).
- **Without initialization priorities (Mach-O)** the stream objects are constructed by an
  `ios_base::Init` object that `<iostream>` defines in every translation unit including it,
  exactly the model of [iostream.objects.overview]/5. Mach-O has a single list of initializers
  (`__mod_init_func`) in link order, which puts libycxx's archive members after the program's
  objects: GCC rejects `init_priority` there, and Clang honours it only within one object file.
  The per-TU object is initialized before every static object defined after the `#include` in its
  TU and destroyed after them, so any static constructor or destructor that can name `std::cout`
  finds it constructed, and the last `Init` destroyed flushes. `cfg::init_priority`
  (`YCXX_HAS_INIT_PRIORITY`, from `__ELF__`) selects the mechanism: on ELF the header's object is
  an empty one with no initializer, so ELF programs pay nothing. The runtime's own `Init` object
  and the classic locale's eager construction stay, as ordinary initializers, on Mach-O (built
  before main; the classic locale is also built on first use, so a program static initializer
  that runs earlier still finds it). Rejected: a constructor-attribute function in an object linked
  first (every link line, CMake's included, would have to name it), and constructing the objects
  lazily behind an accessor (`std::cout` must be an object, not a call). Verified on Linux by
  building with `-U__ELF__`: the static-initialization tests (`linkage/*`, `iostreams/*`, `ios/*`)
  pass, and fail without the per-TU object.
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

## 8. File systems (hosted, POSIX)

- **Layering.** `<filesystem>` is hosted: the classes are in `ycxx/hosted/filesystem.hpp`, every
  operation that touches the file system (the `error_code&` forms, directory iteration,
  `directory_entry::refresh` and its observers, `filesystem_error`'s constructors and key
  function) is in `src/hosted/filesystem.cpp`. That file calls POSIX directly, not through the
  PAL: the operations are specified "as if by" POSIX functions (stat, lstat, mkdir, link,
  symlink, rename, truncate, statvfs, fchmodat, utimensat, readdir), so PAL hooks would restate
  POSIX one function at a time; a non-POSIX port replaces the file, as `fstream.cpp` is replaced
  for a target without stdio. Linux and Darwin differences are absorbed without the
  preprocessor (`st_mtim`/`st_mtimespec` through a `requires` probe on `struct stat`).
- **Throwing forms are inline.** Each calls the `error_code` form and throws through
  `raise_with(ycxx_error_filesystem_error, ...)`, so `-fno-exceptions` programs reach
  `ycxx_error_handler`, and the runtime never throws on behalf of a TU built without exceptions.
  Errors are `error_code(errno, generic_category())`. Members that take no path argument
  (`directory_iterator::operator++`, `recursive_directory_iterator::pop`) throw without paths
  ([fs.err.report]/2.1) and name the directory in the message.
- **path.** `value_type` is `char`; the native ordinary encoding is taken to be UTF-8 (no C
  locale is consulted) and `wchar_t` is UTF-32. Conversions of `char8_t`/`char16_t`/`char32_t`/
  `wchar_t` sources and results are direct Unicode transcoding (U+FFFD for ill-formed input,
  appended in place, so assigning to a path with enough capacity does not allocate); the
  constructors taking a locale go through its `codecvt<wchar_t, char, mbstate_t>`. There are no
  root-names (a leading `//` is a root-directory). The native format is the generic one; the
  generic observers write each directory-separator as one slash. Lexical operations work on
  element offsets in the pathname (`path::iterator` stores one and the element it designates).
  `hash_value` hashes the elements, so equal paths with different separator runs hash equal.
  `string()`/`generic_string()` and `u8path` are provided as the draft's Annex D still has them
  (D.23), declared `[[deprecated]]` (§6, Annex D).
- **file_time_type** is `chrono::time_point<chrono::file_clock>`, nanoseconds in a `long long`
  since the Unix epoch (range 1677-2262); a time stamp outside it is `errc::value_too_large`.
  `ycxx/hosted/file_clock.hpp` (what `<filesystem>` includes) is `ycxx/hosted/chrono_clocks.hpp`;
  file_clock has `to_sys`/`from_sys` ([time.clock.file.members]), so `clock_cast` reaches every
  clock through system_clock.
- **directory_entry caching.** `refresh()` caches the results of `lstat` (and `stat` for a
  symbolic link) including their errors, so the observers return what the operations would.
  Directory iteration caches only the file type from `d_type` (no `refresh`, [fs.class.directory.
  iterator]/9); other attributes are queried on demand. `refresh(ec)` reports a missing file in
  `ec` (as both other implementations do) but the throwing `refresh()` does not throw for it, and
  the constructor keeps the path for a missing file; for any other error the path is cleared as
  [fs.dir.entry.cons]/2 says (libc++ and libstdc++ keep it).
- **Directory walks are descriptor-relative.** `remove_all` opens each directory with
  `O_NOFOLLOW` and removes entries with `unlinkat`, so a directory swapped for a symbolic link
  during the walk is unlinked, never followed; passes repeat until a pass finds nothing (some
  file systems skip entries of a directory modified while it is read). `recursive_directory_
  iterator` opens subdirectories with `openat` on the parent's descriptor and `O_NOFOLLOW`; a
  symbolic link it follows by request is opened by its whole path, since [fs.rec.dir.itr.members]
  /21.2 recurses into `(*this)->path()`: the system's limit on symbolic links per resolution then
  ends a loop (`d/self -> .`) with ELOOP, reported as an error as `status()` would report it
  ([fs.op.status]/6.1.3: file_type::none). Copies of a directory iterator share the open directory
  (input iterators); `recursion_pending()` belongs to each copy.
- `<filesystem>` also includes `<cstdlib>`, as `<fstream>` includes `<cstdio>`.
## 9. Diagnostics and other C++26 utilities

- **Replaceable runtime hooks in their own archive members.** `std::is_debugger_present`
  (`src/runtime/debugging`) and GCC's contract-violation handler `::handle_contract_violation`
  (`src/runtime/contracts`) are each alone in an archive member of both `libycxx.a` and the
  freestanding archive, like the allocation functions (§3), so a program's definition replaces
  the default. The defaults ask the PAL (`ycxx_pal_debugger_present`: TracerPid on Linux) or
  report through it (stderr); the freestanding defaults answer false / report nothing.
- **`contract_violation` has GCC's layout.** GCC 16 builds the violation object itself and passes
  it as `const std::contracts::contract_violation&`; its layout (four 16-bit fields with the
  enumerator values, comment, source-location data pointer, extension pointer) was read from
  GCC's generated code. Clang 23 has no contracts: the header is declarative there and
  `__cpp_lib_contracts` is defined only where `__cpp_contracts` is.
- **Stack traces are symbolized by the runtime itself**, without libbacktrace or addr2line: the
  toolchain unwinder (`_Unwind_Backtrace`) captures return addresses; new PAL hooks name the
  loaded object containing an address (`dl_iterate_phdr`), map its file and ask `dladdr`; the
  runtime reads ELF symbol tables and DWARF 2-5 (`.debug_line`, and `.debug_info` for inlined
  functions) and demangles with its own Itanium demangler (`src/hosted/demangle.cpp`).
  `basic_stacktrace::current` is `noinline` and passes its return address, so the trace starts
  at its caller however the library's frames were inlined.
- **`<text_encoding>`'s registry is generated** (`tools/gen_text_encoding.py`) from a copy of the
  IANA registry kept in `tools/data`; names are matched through their comp-name canonical form by
  binary search, so the class is usable in constant expressions. `locale::encoding()` of "C" is
  US-ASCII (the POSIX portable character set), although the classic `codecvt<wchar_t, char>`
  converts UTF-8 (§7).
- **`pointer_tag_pair` ([ptrtag]) keeps the tag in the pointer's low bits; in constant
  evaluation only the tag 0 can be stored.** Core (`ycxx/core/ptrtag.hpp`, freestanding, from
  `<memory>`). The one member is the tagged pointer itself, of `tagged_pointer_type` (cv `void*`),
  so the class is trivially copyable with the size and alignment of `Ptr` ([ptrtag.pair.general]/3)
  and `tagged_pointer()`/`from_tagged()` are plain copies. At run time the tag is or-ed into the
  low `bits_requested` bits of the pointer's address (`uintptr_t` round trip: GCC and Clang keep
  the value of an integer-pointer round trip, which is what [ptrtag.bits]/2's remark needs), and
  `pointer()`/`tag()` mask them apart, so any `DP` whose `bits_requested` covers the tag and the
  alignment decodes the same `tp` ([ptrtag.pair.tagops]/2). Implementation-defined:
  `max_pointer_bits_available` is the pointer width minus 1 (63 on LP64): only alignment bits are
  used, and a `size_t` alignment has at most that many trailing zeros, so the limit adds nothing
  to `pointer_bits_available(a)` = `min(countr_zero(a), max)` (the draft's note; P3125 suggests a
  page-size limit for segmented architectures, which libycxx's targets are not).
  **Constant evaluation.** Neither GCC 16.2 nor Clang 23.1 can put bits into a pointer during
  constant evaluation (verified: `reinterpret_cast` to and from integers, `bit_cast` of a pointer,
  arithmetic outside the object or on a null pointer, a `void*` cast to `char*` of a non-char
  object and reading the other member of a pointer/integer union are all rejected; Clang's
  `__builtin_align_down` only aligns). P3125 relies on new builtins; its fallback, a hidden object
  holding pointer and tag, would need a constant-evaluation allocation, which a trivially
  destructible type can never free. Keeping pointer and tag apart under `if consteval` is not
  possible either: the layout is one `sizeof(Ptr)` object in both worlds (an object built in
  constant evaluation is used at run time). So in constant evaluation the member holds the
  untagged pointer (`static_cast` to cv `void*` and back, which C++26 allows for the object's own
  type) and every constexpr member works as long as the tag is 0: the default constructor, the
  constructors and `from_overaligned` with tag 0 (or `TagT()`), `pointer()`, `tag()`, `swap`, the
  comparisons, `get`. A non-zero tag during constant evaluation is diagnosed ("needs compiler
  support") although the preconditions hold, which [ptrtag.pair.cons]/2 and
  [ptrtag.pair.overalign]/1 ("Constant When: Preconditions are met") do not allow: that part is
  compiler-blocked, the tests XFAIL it, and `__cpp_lib_pointer_tag_pair` stays undefined (as
  `__cpp_lib_constexpr_exceptions` on Clang and `__cpp_lib_start_lifetime` on GCC: the macro
  announces P3125, "constexpr pointer tagging", whose constexpr support is the point).
  **Preconditions.** With `YCXX_HARDENED` (and always in constant evaluation) the constructors
  check `tag-bit-width(t) <= bits_requested` and that the low bits are free (a misaligned `p`, or
  for `from_overaligned` a `p` not aligned to `PromisedAlignment`, [ptrtag.pair.overalign]/2.2);
  "`p` is not past the end of an object" cannot be checked. In constant evaluation the
  alignment of `from_overaligned`'s pointer is checked on Clang (`__builtin_is_aligned`); GCC has
  no such builtin, so there an unverifiable promise is accepted (it cannot matter: only the tag 0
  is stored then). **Comparisons** follow [ptrtag.pair.comp]/1, /3 (`pointer()` first, then
  `tag()`, through synth-three-way); at run time, when the tag's `<=>`/`==` is the built-in one
  (an integer tag, or an enumeration without a user-declared operator, found by a call of
  `operator<=>(t, t)` / `operator==(t, t)` that only user-declared functions can satisfy), the two
  tagged words are compared directly (/2, /4: the address bits are above the tag bits, so the
  order is the same). **Draft defects**, each resolved by the evident intent (STATUS, "Draft issues
  noticed"): [ptrtag.bits]/2's `tagged_pointer_pair` and `tp.tagged()` are `pointer_tag_pair` and
  `tagged_pointer()`; [ptrtag.pair.tagops]/2-3's `ptr`/`tag` are `pointer()`/`tag()` of `*this`
  and `pointer_tag_type` is `pointer_tag_pair`; the deduction guide `pointer_tag_pair(Ptr*, TagT)`
  names `bits-available<element-of<Ptr>>`, and `element-of<int>` (`pointer_traits<int>`) does not
  exist, so the guide could never be used: libycxx uses `bits-available<Ptr>` (the pointee's
  alignment, as the class's default argument does for `Ptr*`); the guide `pointer_tag_pair(Ptr*)`
  has no one-argument constructor to go with it: it is declared as written and deduces, and the
  initialization then fails (no constructor is invented).
- **`generator` nests without a stack of handles**: the promises of recursively yielded
  generators link to their parent and the root, and transfers between them are symmetric, so
  recursion depth costs no stack and no allocation besides the frames.
## 10. `<random>`

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

## 11. Formatting (`<format>`, `<print>`)

- **Layering.** The engine and every formatter are core headers (`ycxx/core/format_base.hpp`,
  `format_ranges.hpp`, `format_syserr.hpp`, `format_unicode*.hpp`): they need only `<string>`,
  `<charconv>` and the ranges core, and compile freestanding. `<format>` and `<print>` are hosted.
  The locale-dependent parts (`basic_format_context::locale()`, the numpunct values of the L
  option, the overloads taking a locale) are templates in `ycxx/hosted/format_locale.hpp`,
  explicitly instantiated in the hosted runtime (`src/hosted/format.cpp`) for `format_context`
  and `wformat_context`, so headers that need only the core (`<ostream>` for its print
  overloads, `<thread>` and `<filesystem>` for their formatters, `<system_error>` for
  `formatter<error_code>`) do not include `<locale>`. The print functions are out of line
  (`src/hosted/print.cpp`).
- **One context type per character type.** Every context the library creates writes through
  `ycxx::adl_free::fmt_iter<charT>`, an output iterator over a type-erased buffer (`fmt_buf`: an
  array, a size, and a function pointer that flushes it to the destination or grows it). The
  destinations are a growing local-then-heap buffer (`format`, nested width computations), an
  output iterator (`format_to`; a `charT*` is written in place), and a counter with a limit
  (`formatted_size`, `format_to_n`). Formatting a number allocates nothing; `format` allocates
  only the result string. A nested `format_to(ctx.out(), ...)` appends to the same buffer.
- **Compile-time checking.** `basic_format_string`'s consteval constructor runs the scanner that
  `vformat` uses and calls `formatter<remove_cvref_t<Argᵢ>, charT>::parse` for each field; the
  parse context then carries the argument count and kinds, so `next_arg_id`, `check_arg_id` and
  `check_dynamic_spec` reject a bad index or kind there (by calling a non-constexpr function
  named after the problem). Format errors throw `format_error` through `raise_with` (catchable
  in constant evaluation on GCC); on Clang, which cannot throw there, a non-constexpr call that
  takes the message is made first so the diagnostic shows it.
- **Unicode.** Width and escaping follow Unicode 18.0 (UAX #29 extended grapheme clusters with
  the 18.0 form of GB9c); `tools/gen_unicode_tables.py` turns the UCD files into two run tables
  (about 3,500 entries). char is UTF-8 when the ordinary literal encoding is (checked in a
  constant expression), wchar_t UTF-32.
- **Widths and precisions** have no upper bound ([format.string.std]/10): a number in the format
  string or a dynamic value beyond `size_t` saturates. A string's precision only limits the
  prefix; a floating-point conversion is computed with at most 32768 digits of precision (more
  than any exact value needs) and the remaining zeros are counted, not stored. Padding to a counting
  destination past its limit (`formatted_size`, the tail of `format_to_n`) is counted without being
  written, so a huge width there is O(1); `format` itself throws `bad_alloc` when the result cannot
  be held.
- **Floating point** uses `<charconv>` (`to_chars` of the value's own type, extended types
  included); `#` with `g`/`G` reproduces `%#g`; the precision of type none is a to_chars general
  conversion whose zeros are removed even with `#`.
- **Formatters per header.** Every header that declares a formatter specialization (`<vector>`,
  `<stack>`, `<queue>`, `<thread>`, `<stacktrace>`, `<chrono>`, `<filesystem>`, `<system_error>`,
  `<format>`) provides the character, string, arithmetic and pointer formatters
  ([format.formatter.spec]/2). Nothing can call `parse` or `format` without the contexts of
  `<format>`, so those formatters' class layouts live in a light header,
  `ycxx/core/format_decl.hpp` (with the primary template, the /4 disabled specializations,
  `enable_nonlocking_formatter_optimization` and `formattable`): their state is a `fmt_spec`, and
  their members call `ycxx::detail` functions declared there and defined in `format_base.hpp`,
  instantiated where a formatting function is used, so the include order does not matter.
  `<vector>` adds `formatter<vector<bool>::reference>` (`format_vector_bool.hpp`), `<stack>` and
  `<queue>` the adaptor formatters (`format_adaptors.hpp`, against declarations of the adaptors
  and `ranges::ref_view`); `<vector>` grows by 3% (12 KB preprocessed), not by the 59% of
  `format_base.hpp`. `basic_format_context` is complete only with `<format>`, so before it
  `formattable` is false, and the adaptor formatters (constrained on `formattable<Container,
  charT>`, holding the range formatter of `<format>`) are enabled only once `<format>` is in.
  Enabling them with `<stack>` alone would need the context classes (+43 KB) and the range
  formatter's layout with `ref_view`'s definition (+88 KB) in `<stack>` and `<queue>`.

## 12. Data-parallel types (`<simd>`)

- **ABI tags name the width, not the element type.** `deduce-abi-t<T, N>` is
  `ycxx::adl_free::simd_abi<N, R>` for every vectorizable `T` and `N` in [1, 64]; `native-abi<T>`
  is `simd_abi<R / sizeof(T), R>` (at least 1), where R is `cfg::simd_register_bytes` (16; 32 with
  `__AVX__`; 64 with `__AVX512F__`, detected in `config.hpp`). So masks of equal element size and
  width are one type and `rebind_t`/`resize_t` swap the width only. The element type picks the
  representation:
  power-of-two widths of arithmetic types are GCC/Clang vector-extension chunks (`vector_size`;
  one vector up to R bytes, else an array of R-byte vectors, so no by-value vector wider than the
  enabled registers crosses a call and no -Wpsabi ABI change arises); other widths and
  `complex<T>` are element arrays. Every layout has the object representation of `T[N]`, so
  constant evaluation reaches the elements through `bit_cast` (GCC 16 cannot assign to a vector
  element in a constant expression). A mask stores integer-from<Bytes> elements (64-bit for
  16-byte complex elements) that are all ones or zero, in the layout of a vec of that integer, so
  comparisons, `select` and the mask reductions are chunk operations. R is part of the tag
  because the layout depends on it: translation units built with different register-width flags
  get distinct types (and mangled names) rather than one type with two layouts; only the tags of
  the translation unit's own R are enabled.

## 13. Reflection (`<meta>`)

- **The metafunctions are the compiler's; the header declares them.** How GCC 16 expects them
  was found by experiment only (declarations of our own, its diagnostics, and the names of
  builtins and diagnostic strings in the compiler binary; no libstdc++ or GCC source was read).
  With `-freflection`, a call to a `consteval` function or function template declared in
  `std::meta` *without a definition* is evaluated by the compiler when it knows the name
  ("unknown metafunction 'X'" otherwise); it checks the return type ("incorrect 'int' return
  type, expected 'bool'"), not the parameters. The compiler builds the results with ordinary
  C++: `std::vector<info>` from a braced list, `string_view`/`u8string_view` from a
  `const charT*`, `source_location` through `__impl`, `member_offset` and `strong_ordering`, and
  `access_context::current()` from the class's two non-static data members in declaration
  order (scope, then designating class). It reads arguments with ordinary expressions too:
  `ranges::begin`/`end` on a `reflection_range`, `access_context::scope()`/
  `designating_class()`, `static_cast<bool>(o)` and `*o` on the `optional`s of
  `data_member_options`, and the members of `data_member_options::name-type` by the names
  `_M_is_u8`, `_M_u8s` and `_M_s` (a fixed contract, like `source_location::__impl`'s member
  names; other layouts are rejected as "unexpected 'data_member_options' argument"). The
  `operators` enumerators are found by name; their values are ours (1-44 in table order). It
  reports errors by throwing `meta::exception` built with the
  `(string_view, info, source_location)` constructor (from(): the metafunction; where(): the
  call). So libycxx's classes and containers work unchanged, and every metafunction in the draft
  except three is the compiler's.
- **Defined by the library:** `access_context::unprivileged`/`unchecked`/`via` (only `current` is
  a metafunction); `define_static_string`/`_array`/`_object` (the draft's equivalent code; the
  span extent asks `ranges::size` of a never-defined `extern T&` variable template, which P2280
  lets a constant expression use when the size does not depend on the object);
  `is_string_literal` (`__builtin_is_string_literal`); and `is_applicable_type`,
  `is_nothrow_applicable_type`, `apply_result`, which GCC 16 does not know: they evaluate the
  `<tuple>` traits through `substitute` and `extract`. Their `meta::exception`s go through
  `raise_with` and carry the library's own source location, not the caller's.
- **`meta::exception::what()`** is inherited from `ycxx::adl_free::meta_exception_what`. GCC 16
  still treats a class holding an `info` as a consteval-only type and requires every member
  function of it to be `consteval` ("function of consteval-only type must be declared
  'consteval'"), so the constexpr virtual `what()` cannot be declared in `meta::exception`; the
  base holds the ordinary-encoding message and the override. The transcoding between the
  ordinary literal encoding and UTF-8 is the identity when that encoding is UTF-8; otherwise only
  ASCII is treated as representable.
- **Preprocessor.** `^^` cannot be parsed without reflection, so `meta_reflection.hpp` is under
  `#if YCXX_HAS_REFLECTION` (from `__cpp_impl_reflection` in `config.hpp`, the one feature-macro
  test, §1 rule 4); on Clang 23 and without `-freflection`, `<meta>` declares nothing and
  `__cpp_lib_reflection`/`__cpp_lib_define_static` are undefined. `config.hpp` also defines
  `ycxx::detail::reflection` (`decltype(^^::)`, else an incomplete type), so `is_reflection`,
  `is_fundamental` and `is_scalar` need no `#if`. `is_structural` uses the type-taking
  `__builtin_is_structural` (GCC 16, also without `-freflection`), behind
  `YCXX_HAS_IS_STRUCTURAL`.
## 14. `<chrono>`

- **Layering.** The time arithmetic (`ycxx/core/chrono_base.hpp`) and the civil calendar,
  `hh_mm_ss` and the 12/24-hour functions (`ycxx/core/chrono_cal.hpp`) are core and constexpr.
  The clocks, the leap-second clocks, `clock_cast` and the time zones (`ycxx/hosted/chrono_tz.hpp`),
  the formatters and stream inserters (`chrono_io.hpp`) and the parsers (`chrono_parse.hpp`) are
  hosted. Days and dates convert with the era-based algorithm (March-based years, 400-year eras).
  The months overloads of the calendar arithmetic are `template <class = void>` functions, so an
  argument convertible to both months and years picks the years overload ([time.cal.ym.members]).
  Every duration alias counts in `long long`, the calendar ones (`days` to `years`) included, so
  `sys_days + seconds` is exact for every representable date; calendar arithmetic with counts at
  the ends of the range wraps instead of overflowing.
- **Time zone database: the system's compiled zoneinfo**, read by the hosted runtime
  (`src/hosted/tzdb.cpp`, POSIX file calls like `filesystem.cpp`) from `$TZDIR`, else
  `/usr/share/zoneinfo`. Names come from `tzdata.zi` (`Z` and `L` lines; without it, every TZif file
  of the tree is a zone and every symbolic link to one a link); the version from `+VERSION`, else
  `tzdata.zi`'s `# version` line. A `time_zone` holds its name and an opaque pointer; its TZif file
  (versions 1-4, the 64-bit block when present) is read on the first query under a `once_flag`, and
  times after the last transition come from the file's POSIX TZ footer (`Mm.w.d`, `Jn`, `n`, times
  beyond 24 h and negative; a rule whose DST lasts until the next year's begins, such as
  Africa/Casablanca's, is one DST period without end). Consecutive transitions to the same offset, save and abbreviation are
  merged, so a `sys_info` spans the whole period its values hold; before the first transition
  `begin` is `sys_seconds::min()`, without a later one `end` is `sys_seconds::max()`. TZif records
  only an is-DST flag, so `save` is the offset minus the nearest standard-time offset (60 min when
  that is zero). `local_info` examines the periods within 30 hours of the local time.
  `current_zone()` is `$TZ` (a zone or link name, optionally `:`-prefixed or a path into the
  directory), else the target of the `/etc/localtime` symbolic link below `zoneinfo/`, else
  `/etc/timezone`, else UTC. The "remote" database is the directory as it is now:
  `remote_version()` rereads the version, `reload_tzdb()` pushes a newly loaded database when it
  differs (under a lock; `front()` and iteration use acquire loads). The list and its databases
  are never destroyed.
- **Leap seconds**: the `leapseconds` file, else `leap-seconds.list`, else the 27 IERS insertions of
  1972-2016 built in. `leap_second::date()` is the first second after the insertion. `utc_clock`
  reads them from `get_tzdb()`; during a leap second `to_sys` returns the last tick before it (for a
  floating-point duration, the insertion's date).
- **Errors are thrown in the header** (`raise_with`): unknown zone names, unreadable zone data,
  `nonexistent_local_time`/`ambiguous_local_time`. The runtime's entry points return null/false.
- **Formatting.** Each value becomes one set of fields (date, weekday, day of the year, time of
  day with its fractional digits, zone abbreviation and offset); the chrono-specs run over them into
  a local buffer that is padded as a whole (default alignment left). A specifier for information
  the type lacks is rejected by `parse` (a compile-time error for a checked format string), a
  value-dependent one (`%a` of an invalid weekday, `%b` of an invalid month, `%Z` of a
  `local_time_format` without abbreviation) by `format`. Without `L` the "C" locale's names are
  built in; with it the locale-dependent conversions (`%a %A %b %B %c %p %r %x %X` and the E/O
  forms) go through the formatting locale's `time_put` (runtime: `src/hosted/chrono.cpp`), `%S`
  takes its decimal point and a duration's count without chrono-specs goes through its `num_put`
  (as `os << d` would; next item). When that `time_put` is the classic locale's facet (that of
  "C", "POSIX" and "C.UTF-8"; a named locale has its `time_put_byname`), its conventions are the "C" locale's and the built-in forms are
  used, so `{:L...}` with the "C" locale equals `{:...}` ([time.format]/2) even where a C `tm`
  cannot carry the value (a duration's hours beyond 23; years outside 1-9999, which `%Y` pads to
  four digits and `strftime` does not). A `time_put` of the locale's own gets a `tm` with the
  hour count of a duration where an int holds it (reduced modulo 24 for `%I %p %r`). The fields are
  computed from the count's magnitude with 128-bit products of the period's num and den, not with
  `hh_mm_ss`/`duration_cast` (whose ratio arithmetic overflows for periods such as atto, and whose
  negation overflows for the most negative count), for integer reps up to 64 bits and floating-point
  reps (in `long double`); other reps go through `hh_mm_ss`. `%c`, `%r` and `%X` show whole seconds, `%S` and `%T` the fraction.
  Without chrono-specs a value is written as its stream inserter would; a floating-point duration
  then uses `%g` with precision 6 (or the format precision). The micro suffix is "µs" (U+00B5) when
  the literal encoding is Unicode. The stream inserters write the same text (no `<sstream>`
  dependency: the duration inserter formats the count through a private stream on a string
  buffer). `time_point`'s default constructor is `noexcept` (a strengthening).
- **The L option and the facets** ([time.format]/2-3, /7). The wording names no facet for the
  locale-dependent specifiers ("the locale's abbreviated weekday name", "the locale's alternative
  representation"); the locale's `time_put<charT>` is the facet that defines them
  ([locale.time.put]: the locale's strftime conversions), so every one of them (`%a %A %b %B %h
  %c %p %r %x %X` and each E/O form) is written by `use_facet<time_put<charT>>(loc).put(..., spec,
  mod)`: a program's own time_put, derived from `time_put` or `time_put_byname`, sees every such
  call. The numbers of the other specifiers are "decimal numbers" with no locale in the wording;
  only `%S`'s decimal point is "localized according to the locale" (`numpunct::decimal_point`).
  Without chrono-specs, /7 formats "as if by streaming ... to basic_ostringstream<charT> os with the
  formatting locale imbued", and a duration's inserter ([time.duration.io]/1) is `s << d.count()`:
  the count goes through the locale's `num_put<charT>` with the stream's default flags (precision
  6, or the format's precision for a floating-point rep), as the `operator<<` overload of the rep
  would call it (`short`/`int` as `long`, `float` as `double`, ...). When that `num_put` is the
  classic object (every named locale shares it, §7) its stage 2 is computed in the header from
  `numpunct` (the same text, no stream); a program's own `num_put` is called through the hosted
  runtime (`src/hosted/chrono.cpp`, char and wchar_t). Character reps keep the number form.
- **Parsing** reads the stream buffer directly after an unformatted-input sentry. White space is
  the stream's `ctype`; `%S`'s decimal point is `.` or the stream locale's. Table 134's
  locale-dependent flags ("the locale's full or abbreviated case-insensitive weekday name",
  "the locale's date and time representation", "the locale's alternative representation", ...)
  read the stream's locale: its `time_get<charT, istreambuf_iterator<charT, traits>>` facet `tg`,
  the facet whose virtuals are the locale's strptime conversions ([locale.time.get.virtuals]/11).
  [time.parse] itself names no facet. Three cases, decided once per `from_stream`:
  - *No such facet, or the classic locale's object*: the "C" locale's names and `%c %x %X %r`
    are built in (as before; no virtual call), and E/O forms read as the plain ones.
  - *A `time_get_byname` of a named locale* (its data from §7: names, AM/PM, `D_T_FMT` & co., and
    now `ERA`, `ERA_D_T_FMT`, `ERA_D_FMT`, `ERA_T_FMT` and the alternative digits): `%c %x %X %r`
    and `%Ec %Ex %EX` expand to the locale's format (the E one when the locale has it), parsed by
    the scanner itself flag by flag, so their fields, `%S` fractions and a `%Z` inside them are
    recorded as if written in the format; `%EC` matches an era name and `%Ey` a year within it
    (year = the era's start year +/- (`%Ey` - its offset), POSIX `ERA` segments; without `%EC`,
    `%Ey` is `%y`); every O form reads the locale's alternative digits or ASCII digits (also
    `%OC`, which glibc's my_MM uses in its `%x`). The names, `%p` and `%EY` are one call of
    `tg.get(..., spec, mod)` each on a `tm`, so a program's facet derived from
    `time_get_byname` is still called for those.
  - *Any other facet* (a program's, derived from `time_get`): every locale-dependent flag,
    `%c %x %X %r` included, is one call of `tg.get(..., spec, mod)`; the fields it set are found
    by filling the `tm` with a sentinel first (-1200000: negative and a multiple of 12, so `%I`
    and `%p` combine in either order). `%EC` reads as `%C` and `%OU %OW %OV %Ou` as the plain
    forms (no `tm` member holds them).
  `time_get_byname` gains what this needs, as strptime does: `%Ec %Ex %EX` read the era formats
  (else the plain ones), `%EC` an era name, `%Ey` a year of that era within one `get(fmt)` call,
  `%EY` a full era year (the era formats, matched in parallel without backtracking), and every O
  form the locale's alternative digits (longest match) or ASCII digits. Alternative digits come
  from the locale itself: `strftime_l("%Oy")` of the years 1900-1999, kept only when they differ
  from the decimal forms (no knowledge of how a C library lays out `ALT_DIGITS`). The era
  segments are `nl_langinfo_l(ERA)`: POSIX separates them with `;`; glibc returns them separated
  by NULs with their count in `_NL_TIME_ERA_NUM_ENTRIES`, which CMake detects
  (`_YCXX_C_HAS_ERA_NUM_ENTRIES`, cmake/ycxx-c-library.cmake); a segment that does not have the
  POSIX form ends the list. Both are kept only where the C library uses them: an era when
  `strftime_l("%EC")` at its start date writes its name, an era format when `strftime_l` writes
  `%Ec`/`%Ex`/`%EX` with it on two probe dates (a C library may hold the items yet ignore the E
  modifier, as POSIX allows; Darwin's may); otherwise the E forms read as the unmodified ones
  (POSIX strftime: where the alternative form does not exist, the unmodified conversion is
  used). Names compare through `ctype<charT>::tolower` (so a multibyte
  UTF-8 name in a char stream compares its non-ASCII bytes exactly; a wchar_t stream folds them).
  Rejected: parsing every locale-dependent flag through `tg.get` (the `tm` loses `%S` fractions,
  `%Z`, week numbers and eras), and reading the C library's tables in the header (named locales
  would be read twice; a program's facet would be ignored). A width counts digits only (a sign does not count). The fields
  must agree (a weekday with a date, `%H` with `%I`/`%p`); a date comes from y/m/d, y + `%j`, an ISO
  week date or y + `%U`/`%W` + weekday. A duration parsed with a finer field than it can hold is
  truncated (`duration_cast`). For `utc_time`, a seconds field of 60 names the leap second.

## 15. Performance

- **Benchmarks** (`bench/`, `bench/run`). Each program uses only the standard library and is
  built per compiler at `-O2` (`--opt O3`), against libycxx's Release archives
  (`build/<cc>-release`), against libstdc++ (`tools/ref-cxx`) and, with Clang, against libc++
  when one is installed (`-stdlib=libc++`); the programs run in turn, `--runs` times interleaved,
  and the table shows the ratios libycxx / libstdc++ and libycxx / libc++ (`--json` writes every
  run). Results and the machine are recorded in `bench/RESULTS.md`. Wall-clock numbers on a
  shared machine are noisy: changes are judged with `valgrind --tool=callgrind` instruction
  counts as well (and `perf record -e cpu-clock` where perf works).
- **Regression check in CI** (`bench/check`, nightly in `full.yml`, one job per compiler). What
  is stored (`bench/baseline.json`) is each benchmark's ratio libycxx / libstdc++ in
  the same run; absolute times are never compared. Ratios still depend on CPU architecture,
  compiler and scheduling, so CI uses `--reference-baseline`: it measures the pinned
  `reference_commit` in a temporary worktree on the same runner with the same toolchain and
  options. The stored ratios remain useful on the machine that recorded them. The initial
  reference is `230111a4`, the final pass-2 code behind the stored results (their `906c1f8`
  identifier predates the history rewrite).
  Confirmation runs remeasure both revisions for the suspects; an unusually low initial
  baseline ratio must repeat too before it can fail the job.
  A benchmark fails when its ratio exceeds the baseline by more than 30% and 0.10 (a benchmark
  may have its own tolerance in the baseline), in the run and again in 2 confirmation runs of the
  suspects; a suspect that does not repeat is reported as noise. An intended slowdown, a new
  benchmark or a new CI machine updates the baseline: `bench/check --update` (or `--from` the
  JSON artifact of a CI run), reviewed and committed with the reason.
- **Single-threaded fast paths.** The PAL exports `ycxx_pal_single_threaded`, a pointer to a flag
  that is nonzero only while the process certainly has one thread (POSIX/glibc:
  `__libc_single_threaded`; freestanding default and other C libraries: a constant zero, i.e.
  "unknown"). While it is set, reference counts of process-private objects (`shared_ptr`/`weak_ptr`
  control blocks, `locale` implementations and facets) and uncontended locks (`futex_mutex`, the
  runtime's `pal_lock`) use plain loads and stores instead of atomic read-modify-write
  instructions (`ycxx/core/single_threaded.hpp`: `single_threaded()`, `ref_add`, `ref_release`).
  This is sound because the flag is cleared before a second thread starts and thread creation
  synchronizes with the new thread; a thread created behind the C library's back (a raw `clone`)
  would break it, as it breaks the C library itself. Otherwise counts are incremented relaxed
  and decremented with acq_rel (not release plus an acquire fence on reaching zero: the same
  instruction on x86, and ThreadSanitizer, which ignores fences, would report every last
  release as a race); a `shared_ptr`'s last owner drops the weak count without an RMW when it reads 1 (nobody can make
  a new reference then).
- **C++26 erroneous values and stack buffers.** In C++26 mode GCC 16 zero-fills every automatic
  variable without an initializer. Buffers the library always writes before reading are marked
  `[[indeterminate]]` in headers (format buffers, number formatting, num_get/num_put fields;
  Clang 23 ignores the attribute, without a warning in system headers), and the compiled runtime's
  charconv and ABI sources are built by GCC with `-ftrivial-auto-var-init=uninitialized`
  (multi-kilobyte bignum buffers; the visited-base tables of the hierarchy walks), since `src/`
  is not a system include and Clang would warn about the attribute there.
- **ABI runtime.** `__cxa_throw` has no helper frame of its own between the throw and
  `_Unwind_RaiseException` (every frame is unwound twice). `__dynamic_cast` decides a
  single-inheritance chain without a walk and otherwise walks the hierarchy once, gathering the
  downcast and cross-cast answers together and stopping early when the class has no repeated
  base (`__vmi_class_type_info::__flags` clear). Hierarchy walks (handler matching,
  `dynamic_cast`) remember the virtual bases already walked (with the path's publicness, or the
  walk's whole state), so stacked diamonds cost one walk per virtual base instead of one per
  path. Type comparisons inline the first eight characters of the name comparison.
- **Containers and algorithms** keep bulk element moves of trivially copyable types to
  `memmove` (vector single-element insert and erase too), `find` on narrow character ranges is
  `memchr`, `find_if` tests four elements per loop check on random-access ranges, and `pop_heap`
  uses Floyd's sift (hole to a leaf, then up) with a branch-free child choice. `deque`'s iterators
  compare element addresses only (the only null `cur_` is the past-the-end position after a full
  last block).
- **Streams in the classic locale.** num_get/num_put recognise the classic `ctype<char>` and
  `numpunct<char>` facets by address and then skip the virtual calls (atoms are the characters
  themselves, '.' and no grouping); fields are accumulated in place. The stream's locale is used
  in place (`ios_access::locale_of`) rather than through a `getloc()` copy.

## 16. The standard library modules (`std`, `std.compat`)

- **Interface units that only re-export.** `modules/std.cppm` and `modules/std.compat.cppm` are
  module interface units whose global module fragment includes the headers (std: every importable
  C++ library header and C++ header for C library facilities; std.compat: the `<name.h>` headers,
  `<stdbit.h>`, `<stdckdint.h>`) and whose purview is only `export namespace std { using std::x;
  ... }` (std.compat: `export import std;` and `export { using ::printf; ... }`). Every entity
  therefore stays attached to the global module, as when it is #included, so `import std;` and
  `#include <vector>` name the same entities in one program ([std.modules]/4-5), across
  translation units and, on Clang, within one. Nothing of `ycxx::` and none of the C library's
  global names are exported by std (they stay reachable, as instantiations need them, but not
  visible); no macro can be exported ([module.import]/7). The global `operator new`/`delete`
  are exported by std ([std.modules]/2). Rejected: defining the library in the module purview
  (one source of truth with the headers would need a header per declaration's attachment, and
  mixing `#include` and `import` would give entities two attachments).
- **The export lists are generated, never edited.** `tools/gen_std_module.py` reads them from
  the headers through the compilers: Clang's AST dump of one translation unit including every
  header gives every declaration in namespace std and its standard nested namespaces (classes,
  enumerations and unscoped enumerators, functions and operators, variables, aliases, concepts,
  templates, the C wrappers' using-declarations; not specializations, deduction guides or
  reserved names); a GCC probe that walks namespace std with reflection (`members_of`,
  `source_location_of`) adds what Clang cannot see. Declarations inside an `#if YCXX_HAS_<X>`
  region of a header (`<meta>` with `-freflection`, `std::is_structural`, the `<stdfloat>`
  aliases) are exported under the same `#if` (rule 4 of §1: a using-declaration of a name that is
  not declared is an error, and no language construct tests for a name). A nested namespace of
  std that is neither one the draft names (STD_NAMESPACES) nor inline stops the generator.
  std.compat's global names are those that the C headers declare in std and that the `<name.h>`
  headers declare globally, minus [support.c.headers.other]/1's exclusions, plus `<stdbit.h>`'s
  and `<stdckdint.h>`'s. The output is sorted; `tools/gen_std_module.py --check` (policy stage of
  `tools/test`, so `tools/check-all`) fails while it differs from the committed files. It also
  writes `include/bits/stdc++.h` (below). Generated on Linux: std.compat depends on the C
  library's headers, so the check is skipped elsewhere.
- **Inline namespaces are redeclared, the implementation's too.** The customization point
  objects live in `std::ranges::cpo` (and `std::cpo`), inline namespaces, so that the hidden
  friends `iter_move`/`iter_swap` of the views' iterators can be declared in `std::ranges`. A
  using-declaration of `std::ranges::iter_move` placed in `std::ranges` itself would conflict
  with those friends, and GCC 16 rejects an instantiation in the importer ("redeclared as
  different kind of entity"); the module redeclares `inline namespace cpo` and exports the
  objects there. The cost: the name `std::ranges::cpo` is visible to an importer (Clang 23 shows
  it to importers in any case).
- **Hidden visibility (§2).** The compilers emit each module's initializer (`_ZGIW3std`,
  `_ZGIW3stdW6compat`) with default visibility whatever `-fvisibility` says; the module hides it
  with an assembler directive (`asm((ycxx::detail::hide_symbol(...)))`,
  `ycxx/core/hidden_symbol.hpp`). Everything else the importer instantiates is declared hidden by
  the headers. Tested by `tests/ycxx/modules/no_exported_library_symbols` and `tests/cmake/run.sh`.
- **Delivery.** A BMI is valid only with the options it was built with, so libycxx ships
  sources and object code, never BMIs: the CMake package's `ycxx::modules` is a `FILE_SET
  CXX_MODULES` (CMake >= 3.28, Ninja; the importing project builds the BMIs) plus
  `libycxx-modules.a` (the initializers); `tools/ycxx-modules gcc|clang [-o DIR] [flags]` builds
  BMIs and the archive for given flags, and `tools/ycxx-cxx --std-modules[=DIR]` uses them. The
  own suite's `// MODULES:` directive and libc++'s `MODULE_DEPENDENCIES:` build them per compiler
  and flags (`tests/ycxxlit/stdmodules.py`, cached by the state of the sources). CMake's
  `CMAKE_CXX_MODULE_STD` (`import std` without naming a target) needs CMake >= 3.30 and is not
  supported: it would build the toolchain's own library's module.
- **GCC needs `<bits/stdc++.h>`.** With `-fmodules`, GCC 16 looks that header up on the include
  path for every `#include` of a standard header, to translate the `#include` into an import of
  its header unit when one was built; without it the `#include` is a fatal error. libycxx's
  (generated) includes every header, so a header unit built from it replaces any of them.

## 17. Senders and receivers (`<execution>`, [exec])

- **Layering.** Everything is core (`ycxx/core/exec_*.hpp`, included by `<execution>`), so the
  senders are freestanding-capable: `<stop_token>` moved to core for them (§3). The one hosted
  part is parallel_scheduler's default backend, a thread pool (`src/hosted/parallel_scheduler.cpp`),
  reached through the replaceable `query_parallel_scheduler_backend`, alone in its archive member
  (`src/hosted/parallel_scheduler_query.cpp`) like the replaceable allocation functions; a
  program's definition replaces it (own test `execution/exec_parallel_replace`).
  `__cpp_lib_parallel_scheduler` is defined only when hosted.
- **The exposition-only machinery is real code.** basic-sender is an aggregate of indexed leaves
  with a member `get<I>` and `tuple_size`/`tuple_element`, so `auto&& [tag, data, ...children] =
  sndr` works, as [exec.snd.expos]/45 requires. A leaf has `[[no_unique_address]]` only for
  an empty movable type: initializing a potentially-overlapping subobject from a prvalue is not a
  guaranteed elision, and operation states cannot be moved. `impls-for<Tag>` holds static
  member functions instead of the draft's lambdas, plus `csigs<Sndr, Env...>` (below).
  `tag_of_t` recognises tuple-like senders (every library sender); an aggregate of another
  shape is not recognised (detecting a structured binding of an arbitrary aggregate needs
  reflection).
- **Completion signatures are computed as types.** Every library sender has a member alias
  template `ycxx_csigs<Self, Env...>` naming its `completion_signatures`, or one of two error
  types: `dependent_sigs` (no environment and the signatures need one) or
  `invalid_sigs<problem, info...>` (what the draft reports by throwing from
  `get_completion_signatures`; the problem type's name, e.g.
  `function_not_invocable_with_these_arguments`, shows in diagnostics). The adaptors combine their
  children's results without constant evaluation. The public consteval
  `get_completion_signatures<Sndr, Env...>()` returns the specialization or throws
  `dependent_sender_error` or an exception derived from `std::exception`
  ([exec.getcomplsigs]/3); Clang 23 cannot throw during constant evaluation, so there the throw
  makes the call non-constant, which is all `sender_in` observes. A sender that is not the
  library's is asked through its static member function: when that is not a constant
  expression, GCC (`cfg::constexpr_exceptions`, new in `config.hpp`) catches to tell
  `dependent_sender_error` from other errors; on Clang it is dependent when no environment was
  given, else invalid. `is-constant<get_completion_signatures<...>()>` is written
  `typename constant<...>`: a concept-id whose argument is not a constant is still satisfied
  (the parameter mapping of `true` is empty).
- **make-sender's Mandates are a static_assert**: `then(just(string()), [](int){...})` is a
  compile-time error naming the problem, not a sender without completions.
- **No allocation in the common paths.** The operation states are built in place (connect results
  in leaves, let's second operation and continues_on's state through guaranteed elision; let's
  operation variant is a small union of operation states with an index, as `variant::emplace`
  cannot construct from emplace-from without a move); run_loop and the thread pool queue
  operation states intrusively (the pool's items live in the proxies' preallocated backend
  storage, 128 bytes). Allocating: a task's coroutine frame (with its allocator), connect of an
  awaitable that is not a sender (a coroutine frame), spawn/spawn_future's state (with the
  allocator of [exec.spawn]/9), and task_scheduler, which holds its backend through
  allocate_shared on every construction (a task constructs one at each start; the recommended
  practice of [exec.task.scheduler]/4, avoiding it for small schedulers, is not followed yet).
- **Concurrency.** when_all's count and disposition, the counting scopes' state and spawn_future's
  hand-over between complete/consume/try-set-stopped/abandon are small `__atomic` words or spin
  locks around a few stores; completions run outside the locks. run_loop blocks in the PAL's
  address wait.
- **Draft questions, and what libycxx does** (the draft is the working draft of 2026-10):
  - `sync_wait`/`sync_wait_with_variant` dispatch on `COMPL-DOMAIN(set_value_t, sndr, env)`
    rather than `get_completion_domain<set_value_t>(get_env(sndr), env)`, which is ill-formed
    for a sender without completion-domain attributes (every user sender).
  - `sync_wait_with_variant` returns `optional<value_types_of_t<Sndr, sync-wait-env>>` of the
    given sender; [exec.sync.wait.var]/1 names the into_variant sender's, whose value is a variant
    of tuples of that.
  - basic-state's state is computed from the stored receiver; the draft's mem-initializer names
    the constructor parameter, already moved from.
  - let-state's receiver cannot be a member of let-state: let-state's operation variant holds
    the operation connected to it. The receiver is keyed by let-state's parameters instead.
  - `let-cpo.transform_sender(s, es...)` and `affine.transform_sender(sndr, ev)` take no tag;
    libycxx lowers them on `set_value_t`, as the other adaptors.
  - The parallel scheduler's and task_scheduler's domains do not require `sender_in<Sndr, Env>`
    of the bulk sender: computing it transforms the sender with the same domain (endless
    recursion).
  - bulk_chunked's check-types tests f with bulk_unchunked's arity; libycxx tests
    `f(b, e, args...)`.
  - spawn_future's try-set-stopped ([exec.spawn.future]/12.1) would destroy the state while its
    operation runs; the state is destroyed when that operation completes instead. Its
    try-cancel requests a stop that can complete that operation, whose complete-then-destroy
    would free the state under try-cancel: try-cancel pins the state, and the last of the two
    destroys it.
  - `inline_scheduler::schedule()` is const and the scheduler answers
    `get_forward_progress_guarantee` (weakly_parallel): without them it does not model scheduler
    (`scheduler<const inline_scheduler&>`, and the query the concept requires).
  - run_loop's operation does not evaluate set_stopped when the receiver's token is
    unstoppable: its completion signatures are then `set_value_t()` alone ([exec.run.loop.types]/6).
  - stopped_as_optional, starts_on and affine compute their signatures before their
    transformation too (the draft gives default-impls', i.e. the child's).
  - The member type form `using completion_signatures = ...` of [exec.cmplsig]'s example is
    accepted; [exec.getcomplsigs] lists only the member function.
  - parallel_scheduler's schedule sender can complete with an error (the backend's proxy has
    set_error), so it is not an infallible-scheduler, and `task_scheduler(parallel_scheduler)`
    is ill-formed ([exec.task.scheduler]/2).
  - continues_on is also pipeable (`sndr | continues_on(sch)`, as in P2300);
    [exec.continues.on] calls it a customization point object.
  - `split` and `ensure_started` are not in the draft (P3682 removed them); not provided.
- **Attributes (the draft's undefined get-attrs, D2).** [exec.snd.expos]/43 has
  `basic-sender::get_env()` return `impls-for<Tag>::get-attrs(data, child...)`, but nothing
  defines `get-attrs`: P3826R5 (the paper that introduced `get_completion_domain`,
  `indeterminate_domain` and completion domains per tag) struck `default-impls::get-attrs` and
  every specialization's (schedule_from's, when_all's), and moved what they said into
  [exec.adapt.general]/3.2-3.3 and [exec.snd.general]/3-4. The call in /43 is a leftover.
  libycxx reads /43 as "the attributes those paragraphs give":
  - an adaptor with one child has the child's forwarding queries (FWD-ENV, /3.2), one with
    several children none (env<>, /3.3);
  - for each completion tag T, `get_completion_domain<T>` and `get_completion_scheduler<T>` follow
    [exec.snd.general]/3-4 from the agents the adaptor's semantics put T completions on. libycxx
    lists those agents as *sources*: a child's completions of some tag, the completions of the
    schedule sender of a scheduler the adaptor transfers to, or a domain known only as a type
    (the sender a let function returns, which exists only once the child completes);
  - the domain is the COMMON-DOMAIN of the sources' domains. Given an environment, a source that
    reports none counts as `indeterminate_domain<>` (COMPL-DOMAIN, [exec.snd.expos]/9; the
    common type of `indeterminate_domain<>` and D is D), as P3826 §5.6 computes when_all's;
    without one, every source must report a domain;
  - the scheduler is reported only for a single source that reports one ("can determine", /4);
    nothing tells two equal-typed schedulers apart at compile time;
  - an adaptor without completions of tag T, or whose signatures are invalid in the environment,
    reports neither for T (/3: ill-formed; [exec.get.compl.domain]/3 and [exec.get.compl.sched]/6
    make the program asking ill-formed). Which child completions occur is read from the children's
    signatures in the environment; without an environment a dependent child leaves the adaptor
    silent ("cannot determine").

  Per adaptor (the default implementations; each a source list per tag):
  - then, upon_error, upon_stopped, bulk, bulk_chunked, bulk_unchunked, into_variant,
    stopped_as_optional, stopped_as_error: each child completion maps to the tags the adaptor turns
    it into, an exception included (then(sndr, f): error from sndr's errors, and from its values
    when f can throw: [exec.snd.general] Examples 1-2). write_env and unstoppable: identity, the
    child asked in its receiver's environment (the written env joined to the forwarded one).
    schedule_from: the child's attributes;
  - when_all, when_all_with_variant ([exec.when.all]/15-17): value from every child's value
    completion. The operation completes on the agent of the last child to complete, so error from
    every child's errors and the values whose decay-copy can throw, and from every completion of a
    child when another child can fail; stopped likewise. The children are asked in
    `when-all-env`. when_all_with_variant is when_all of into_variant of each child;
  - let_value, let_error, let_stopped ([exec.let]/10, /16): the child's other completions pass
    through; error also from the child's set-cpo completions when decay-copying the datums,
    calling f or connecting can throw; and for each set-cpo signature, the completion domain of the
    sender f returns, in the environment the let-state gives it (JOIN-ENV(let-env(sndr, env),
    FWD-ENV(env)), [exec.let]/9): only given an environment;
  - continues_on ([exec.continues.on]/9-12): every completion arrives through the schedule
    sender's value completion (the child's result, or the exception of its decay-copy), plus
    the schedule sender's own error and stopped completions. The schedule sender, not the
    scheduler, is asked: it is what runs, and [exec.run.loop.types]/5 makes run_loop's answer
    without an environment where the scheduler cannot ([exec.get.compl.sched]/5.2). With an
    environment the two agree ([exec.sched]/6). When `schedule(sch)` can throw, the scheduler is
    asked instead (a query is noexcept);
  - starts_on ([exec.starts.on]/4): its let_value form. The child is the sender the let function
    returns, so, as for let, only its domains count, asked in the environment that form gives it
    (the start scheduler of continues_on(just(), sch), [exec.let]/2), plus the schedule sender's
    error and stopped completions. A scheduler for the child's completions would come from
    inline-attrs' `get_scheduler(env)`, which that environment does not set (it sets
    `get_start_scheduler`; STATUS "Draft issues noticed"), so it would name the receiver's;
  - on, affine: given an environment, the continues_on sender their transformation
    produces ([exec.on]/6, [exec.affine]/5), whose scheduler comes from the environment
    (get_start_scheduler) or the child (on(sndr, sch, closure)); none without one. affine of a
    sender with an `affine()` member reports the child's;
  - associate ([exec.associate]/11): domains only, the wrapped sender's, and for stopped also
    the starting agent's (`get_domain(env)`: a failed association completes inline). No
    scheduler and no forwarding: the wrapped sender is destroyed when the association fails;
  - read_env ([exec.read.env]/3): inline-attrs for set_value, and for set_error when the query
    can throw (TRY-SET-VALUE);
  - spawn_future: none. The state erases the spawned sender's type; a parent's COMPL-DOMAIN
    makes that `indeterminate_domain<>`, which is what is known.

  The draft's three-way disagreement about schedulers ([exec.sched]/6,
  [exec.get.compl.sched]/5.2, [exec.get.compl.domain]/2.3; STATUS "Draft issues noticed") is
  settled the same way throughout: a schedule sender's attributes say where its completions run,
  per tag, and the adaptors use them. A scheduler's own queries are those of [exec.get.compl.sched]
  /5 as written. Tests: tests/ycxx/execution/completion_attributes_adaptors,
  completion_attributes_when_all_let, domain_dispatch_through_adaptors,
  sync_wait_customization.
- **noexcept.** Where the draft gives a noexcept-specifier it is used as written; the sender
  factories and adaptors are noexcept when their decay-copies are (a strengthening
  [res.on.exception.handling] allows; make-sender has none in the draft).

## 18. Hosted layers: a hosted library without every OS primitive

"Hosted" does not have to mean "every OS primitive". The hosted library is split into **layers
of support**. Each layer is a small set of C-linkage primitives of `ycxx/pal.h` (the platform
layer, §3) plus the library features those primitives enable. The integrator chooses the layers
and supplies their primitives through **providers**: their own code, in their own targets. A
program then gets exactly the hosted features it has primitives for: containers, strings,
exceptions and `std::print` over a heap and a serial port, with no OS, no C library and no file
system (`examples/hosted-layers`). Using a feature whose layer is absent fails when the program is
built, never silently at run time.

**The layers.** The primitives are those of `ycxx/pal.h`, which groups them by layer.

| Layer | Primitives a provider defines | Enables | Absent |
|---|---|---|---|
| `abort` (always) | `ycxx_pal_abort(msg)`; `ycxx_pal_write` to `ycxx_pal_stderr` | `terminate` and its report, uncaught exceptions, `ycxx_error_handler`'s default (`-fno-exceptions`, hardened checks), contract violations, pure virtual calls | not allowed: the ABI runtime needs it |
| `memory` | `ycxx_pal_allocate`, `ycxx_pal_deallocate` | the default `operator new`/`delete` (all forms), so the containers, `string`, `function`, `any`, `shared_ptr`, `new_delete_resource`; exception objects | link error naming `ycxx_pal_allocate` (a program that replaces the allocation functions and throws nothing needs no provider) |
| `console` | `ycxx_pal_write` to `ycxx_pal_stdout`, `ycxx_pal_read` from `ycxx_pal_stdin`, `ycxx_pal_is_terminal` | `std::print`/`println`/`vprint_*` to standard output without a C library (with `clib` they write to C's `stdout`) | `print(fmt, ...)` fails to compile (a static_assert naming the layer); `println()` fails to link |
| `clock` | `ycxx_pal_clock_now` | `system_clock`, `steady_clock`, `high_resolution_clock` | link error naming `ycxx_pal_clock_now` |
| `threads` | `ycxx_pal_wait`/`_wake_one`/`_wake_all`/`_wait_until`, `ycxx_pal_thread_*`, `ycxx_pal_sleep_until`, `ycxx_pal_hardware_concurrency`, `ycxx_pal_single_threaded`, `ycxx_pal_thread_atexit`, `ycxx_pal_at_thread_end` | `thread`, `jthread`, `this_thread::sleep_*`, timed waits, `<future>`, `<rcu>`, `<hazard_pointer>`, `parallel_scheduler`; blocking `atomic::wait` and lock contention | one thread of execution: libycxx's single-thread fallbacks (address waits return at once, thread identity 1, `thread_local` destructors registered with `__cxa_atexit`); `thread`/`jthread` fail to compile (static_assert), the rest fails to link naming its primitive |
| `random` | `ycxx_pal_random_open`/`_read`/`_close`, `ycxx_pal_random` | `random_device` | link error naming `ycxx_pal_random_open` |
| `files` | `ycxx_pal_file_open`/`_close`/`_read`/`_write`/`_seek`/`_flush` | `basic_filebuf` and the file streams, over the provider's storage | with `clib`: files are C stdio's `FILE` (`src/hosted/fstream.cpp`, as with `posix`); without: no file streams (iostreams need `clib`) |
| `environment` | `ycxx_pal_error_message`, `ycxx_pal_environment_encoding` | the messages of `generic_category()`/`system_category()`, `text_encoding::environment()` | fallbacks: the message "error N"; an unknown environment encoding |
| `debug` | `ycxx_pal_debugger_present`, `ycxx_pal_object_of`, `ycxx_pal_dynamic_symbol`, `ycxx_pal_map_file`/`_unmap_file` | `is_debugger_present`, `<stacktrace>` | `is_debugger_present()` is false (fallback); `<stacktrace>`'s runtime is not built |
| `clib` | the C library itself, its headers and functions (the toolchain's: glibc, musl, newlib, ...) | the C library headers (`<cstdio>`, ...), iostreams and locales, `print(FILE*, ...)`, the C-library parts of `<string>` (`sto*`, floating-point `to_string`), `<cmath>`'s out-of-line functions, `<regex>`, `<syncstream>`, chrono I/O; `threads` and `debug` need it too (their headers use `<ctime>`) | the program and libycxx are compiled freestanding (below); the headers that need the C library stop with an `#error` naming the layer |
| `filesystem`, `tzdb` | none yet: `src/hosted/filesystem.cpp` and `tzdb.cpp` call POSIX directly (§8) | `<filesystem>`, the time zone database | built with `YCXX_PAL=posix` only |

`abort` and `console` share `ycxx_pal_write`: a provider of `abort` alone must accept
`ycxx_pal_stderr` (and may discard what it gets, if the target has nowhere to show it); the
console adds the standard output and input. `ycxx_pal_exit` is not used by the library.

**What "absent" means, and why.** A layer's library code is built whenever the configuration can
compile it (whether `clib` is present decides that). A layer's primitives are defined by its
provider when the layer is selected and by nobody otherwise, so:

- a feature whose only primitives are the layer's fails to **link**, and the linker names the
  missing primitive (`undefined reference to 'ycxx_pal_clock_now'`), which the table above maps
  to its layer;
- a template entry point checks first and fails to **compile** with a message naming the layer:
  `std::thread`'s and `std::jthread`'s constructors, `std::print`/`println` to standard output.
  The checks read `ycxx::detail::cfg::layer::<name>`, which `config.hpp` derives from the
  generated `<ycxx/generated/hosted_layers.hpp>` (`YCXX_LAYER_*` switches, §1 rule 8; the file
  exists only in `YCXX_PAL=none` builds, and without it every layer is present when hosted and
  none freestanding, as before);
- a header that cannot be compiled without the C library stops with `#error "... needs the C
  library (the 'clib' hosted layer) ..."` when compiled freestanding, instead of failing inside
  the C library's missing header (`<thread>`, `<mutex>`, `<fstream>`, `<iostream>`, ...). This is
  §1 rule 4's case of code that cannot be parsed, tested with the `YCXX_HOSTED` switch;
- three layers have **fallbacks**, because the library itself needs their primitives in programs
  that never use the layer's feature, and because the fallback states the truth for such a
  program rather than hiding a failure: without `threads` there is one thread of execution (the
  freestanding runtime's defaults, `src/freestanding/pal`, and
  `src/pal/fallback/thread_atexit.cpp`); without `environment` a system error's message is its
  number; without `debug` no debugger is attached. CMake builds a fallback only when its layer is
  absent, so it never competes with a provider at link time (no weak symbols, no archive-order
  games).

Rejected: weak default definitions of every primitive (a missing provider would show up at run
time, as a failed allocation or lost output); one archive per layer (static linking already takes
only the archive members a program uses, and the order of the archives would become the
integrator's problem); throwing "not supported" at run time.

**The C library is a layer too.** libycxx's headers already have two modes (§3): hosted, reading
the C library's headers (`#include_next`), and freestanding (`YCXX_HOSTED` 0, `-ffreestanding`),
where the C library headers with freestanding parts fall back to core. Without `clib`, libycxx and
every program using it are compiled freestanding (`ycxx::headers` adds `-ffreestanding -nostdinc`
and the compiler's own header directory, for C++ only: a provider written in C may use whatever C
library it has), while the selected layers keep their hosted features. What makes that work:
`<print>` declares its `FILE*` overloads only with the C library and writes standard output
through `ycxx::detail::vprint_stdout` (C's `stdout` with `clib`, `src/hosted/print.cpp`, as before;
the console otherwise, `src/hosted/print_console.cpp`); `<chrono>`'s clocks and calendar are
freestanding-capable (time zones and I/O need the C library); the `L` option of `<format>` uses
the classic locale's punctuation (`src/hosted/format_classic.cpp`: without `clib` there is only
the classic locale); `ycxx_error_handler`'s default reports through `ycxx_pal_abort` whenever the
`abort` layer exists, not only when hosted. The environment still provides what the compilers
call (`memcpy`, `memmove`, `memset`, `memcmp`, as for every freestanding program), the program's
start (static constructors) and `__cxa_atexit`, and what its unwinder needs when exceptions are
used (`examples/hosted-layers/README.md` lists what GCC's `libgcc_eh` needs on bare metal).

**CMake.** `YCXX_PAL` selects the platform layer: `posix` (the default: every layer, the
primitives from `src/pal/posix`, the library built exactly as before) or `none` (no platform
layer: the integrator provides the selected layers):

```cmake
set(YCXX_PAL none)
set(YCXX_HOSTED_LAYERS memory console clock)   # 'abort' is implied
add_subdirectory(libycxx)
ycxx_add_hosted_layer(memory  PROVIDER my_heap)  # a target that defines the layer's primitives
ycxx_add_hosted_layer(console SOURCES uart.c)    # or sources, made into a target
target_link_libraries(app PRIVATE ycxx::ycxx)    # brings the providers
```

`YCXX_PAL_<LAYER>_PROVIDER=<target>` cache variables do the same as `ycxx_add_hosted_layer` from
the command line. The layer set is fixed when libycxx is configured (it selects sources and writes
`hosted_layers.hpp`); providers can be attached afterwards, and `ycxx_add_hosted_layer` refuses a
layer that was not selected, saying how to select it. At the end of the configuration libycxx
lists each selected layer with its provider, or says that the program must define the layer's
primitives itself (allowed: they may live in the program's own targets).
`YCXX_HOSTED_LAYERS` is validated: unknown names, `threads` or `debug` without `clib`, and
`YCXX_FREESTANDING_RUNTIME` (an option of `posix` builds) are configuration errors.

## 19. Transitive includes (`YCXX_NO_TRANSITIVE_INCLUDES`)

[res.on.headers]/1: "A C++ header may include other C++ headers." Whether `<string>` provides
`std::min`, or `<mutex>` `errno`, is unspecified, yet much code relies on what libstdc++ and
libc++ happen to provide (the real-world projects' `transitive-include` patches, the external
suites' F-category tests). libycxx decides it by data, in one place, and lets a program opt out:

- **By default** each public C++ header also includes the standard headers that programs commonly
  rely on getting with it, so such code builds without changes.
- **With `YCXX_NO_TRANSITIVE_INCLUDES` defined** (`-DYCXX_NO_TRANSITIVE_INCLUDES`; any value, or
  none: only `defined` is tested, like `NDEBUG`) each header includes only what the draft's
  synopsis and the implementation need. That is the strict mode: shorter compiles, and a check
  that a program includes what it uses (portability to other libraries). It is a user-facing
  name (allowed.txt `[user]`; never renamed). Define it on the command line, or before the first
  `#include`: it is read when `ycxx/config.hpp` is first included. Translation units of one
  program may differ: it changes which headers are included, never what a declaration means.

**The pattern (§1).** An `#include` cannot be made conditional without the preprocessor (rule 4).
`config.hpp`, the one detection point, turns the user's macro into the switch
`_YCXX_TRANSITIVE_INCLUDES` (1 by default, 0 when `YCXX_NO_TRANSITIVE_INCLUDES` is defined), and
the switch is tested in exactly one way: a single block at the very end of a public C++ header,

```cpp
// Transitive includes (DECISIONS §19), generated by tools/gen_transitive_includes.py from
// tools/data/transitive-includes.txt; -DYCXX_NO_TRANSITIVE_INCLUDES leaves them out.
#if _YCXX_TRANSITIVE_INCLUDES
#  include <algorithm>
#  include <cstdlib>
#  if _YCXX_HOSTED
#    include <cstdio>
#  endif
#endif
```

holding nothing but `#include`s of public headers and at most one nested `#if _YCXX_HOSTED`.
`tools/check_preprocessor.py` allows exactly that: no other file and no other directive names the
switch (or the user's macro, outside `config.hpp`), nothing follows the block, and the block names
no internal header. Only whole public headers are included, never parts of them (an internal
header that declares `std::min` alone would be lighter, but then a program gets an unpredictable
part of `<algorithm>`; "`<string>` provides `<algorithm>`" is what a program can know and what the
probe below measures). No block in `<cassert>` (re-included, no `config.hpp`) or in the `.h` forms
of the C headers.

**Layering (§3).** A header of the hosted layer (`tools/headers.py` HOSTED) is only ever a
transitive include inside the nested `#if _YCXX_HOSTED`, whichever header includes it: a
freestanding build, and a hosted-layers build without the C library (§18, `_YCXX_HOSTED` 0), reach
only core headers through the blocks. `tools/check_includes.py` walks the blocks' core part from
every core header (it already skips `_YCXX_HOSTED` branches), and `tools/check_freestanding.sh`
compiles the core headers with their blocks, so a core header that transitively included a hosted
one fails the policy stage.

**Cycles and modules.** `tools/gen_transitive_includes.py --check` fails on any include cycle
through a transitive include (an edge of a block back to its header, through blocks or ordinary
includes); `#pragma once` would hide one, but it would make a header's content depend on which
header came first. The modules are unaffected: `modules/std.cppm` includes every header in its
global module fragment either way and exports the same declarations (`tools/gen_std_module.py
--check` passes unchanged in both modes); `import std;` provides everything regardless.

**Where the list lives.** `tools/data/transitive-includes.txt` is the one list (header: headers it
provides, each line a baseline or a reason); `tools/gen_transitive_includes.py` writes the blocks
from it, and `--check` (policy stage) fails when a block differs from it, when it disagrees with
the probe results, or on a cycle. No block is edited by hand.

**What is included: decided by a black-box probe** (`tools/probe_transitive.py`, results checked in
under `tools/data/transitive-probe/`). For every public C++ header H and every probe item (a
representative name of a standard header G used as a program uses it: `std::min(1, 2)` for
`<algorithm>`, `o << 1` on a `std::ostream&` for `<ostream>`, `errno` for `<cerrno>`;
`tools/data/transitive-probe/items.txt`, the first item of each G its primary item, the others
names the projects and suites are known to rely on), it compiles `#include <H>` plus the item with
`-fsyntax-only` against libstdc++ (GCC 16.2) and libc++ (23.1, `clang++ -stdlib=libc++`), and
records only whether it compiled. It never reads either library's headers, preprocessed output,
include trees or diagnostics (libycxx is implemented without reading other implementations' sources). Controls: an item that does not compile after its
own header is "n/a" for that library, and so is a header that does not compile alone.
- **Baseline: what both libraries provide** (the primary item of G after H, on both): what
  portable code can depend on. Every baseline pair is listed in the data file, or excluded there
  with a reason; `--check` enforces it against the checked-in probe results.
- **Added: what the real-world projects' patches and the external suites' F-category tests rely
  on, where either library provides it** (`<H>: <G> | <item> <who relies on it>`; `--check`
  verifies that libstdc++ or libc++ provides that item after H).
- **Not added: the heavy headers** (excluded for every H unless a reliance names them), and POSIX
  names (`PATH_MAX`, `pthread_*`, `cpu_set_t`: libycxx's headers include no POSIX header, the
  platform layer is the only OS boundary, §18; the probe records them for information).
- libycxx itself is probed the same way in both modes (`--lib ycxx`, `--lib ycxx-strict`): in the
  default mode every baseline and added item compiles.

**The data (2026-10-07).** 115 C++ headers H, 181 probe items (115 primary). Pairs that compile
after `#include <H>` alone: libstdc++ 4652 of 19364 item pairs, libc++ 3164 of 16770 (libc++ lacks
`<generator>`, `<inplace_vector>`, `<stacktrace>`, ...; neither has `<hive>`, `<rcu>`,
`<hazard_pointer>`, `<linalg>`). By primary item, H provides G: libstdc++ 2041 pairs, libc++ 1338,
**both 1066** (the baseline); libycxx before this section provided 734 of those 1066. Most
common in the baseline: `<type_traits>` (70 headers), `<concepts>` (65), `<compare>` (61),
`<limits>` (57), `<initializer_list>` (52), `<cstddef>` (49), `<utility>` (46), `<algorithm>`
(44), `<tuple>` (40), `<stdexcept>` (37), `<cctype>`, `<cwchar>`, `<cstdint>` (36 each), `<iosfwd>`
(32), `<cstdio>` (28), `<cerrno>` (25). Excluded as heavy for every H: `<chrono>`, `<execution>`,
`<filesystem>`, `<format>`, `<locale>`, `<meta>`, `<regex>` (0.6-1.9 s each to compile alone with
libycxx; e.g. both libraries provide `<chrono>`'s durations with `<mutex>`, but libycxx's
`<chrono>` is one header with the calendar, time zones and formatting). Added for the projects
and suites (either library provides them): `<cstdlib>` and `<cerrno>` with `<string>`,
`<cstdlib>` with `<memory>`, `<memory>` with `<deque>` and `<map>`, `<sstream>` with
`<syncstream>`, `<span>` with `<format>`, `<initializer_list>` with `<memory_resource>`, `<string>`
with `<ranges>`, `<streambuf>` with `<iterator>`, and `<cstdio>` where the character traits
provided `EOF` before (`ycxx/core/char_traits.hpp` no longer includes `<cstdio>`, so the strict
mode has no `EOF` with `<string>`). The result: 976 includes in 76 headers' blocks.

**Compile-time cost** (2026-10-08; a translation unit that only includes <H>, for each of the 115
C++ headers, best of 3, summed):

| | GCC 16.2 `-E` | GCC `-c` | Clang 23.1 `-E` | Clang `-c` | preprocessed lines (GCC) |
|---|---:|---:|---:|---:|---:|
| before §19 | 2.53 s | 15.4 s | 3.34 s | 17.8 s | 1.21 M |
| default (transitive includes) | 3.55 s | 24.4 s | 4.31 s | 28.8 s | 2.00 M |
| `-DYCXX_NO_TRANSITIVE_INCLUDES` | 2.42 s | 15.0 s | 3.32 s | 17.9 s | 1.19 M |

The default costs about 60% more over the headers taken one by one; the strict mode is as cheap
as before (a little cheaper: `<cstdio>` left the character traits). The cost falls on the headers
whose libstdc++/libc++ counterparts pull in iostreams or strings: with GCC `<complex>` 0.15 ->
0.61 s (`<ostream>`, `<sstream>`, `<string>` in the baseline), `<stack>` and `<queue>` 0.14 ->
0.55 s (`<deque>`, `<string>`), `<iterator>` 0.09 -> 0.48 s (`<string>`, `<iosfwd>`,
`<streambuf>`); `<string>` 0.18 -> 0.29 s, `<vector>` 0.12 -> 0.30 s, `<memory>` 0.16 -> 0.28 s;
`<algorithm>` and `<mutex>` unchanged. A real translation unit, which includes several headers,
pays most of it once.

`tools/probe_transitive.py && tools/gen_transitive_includes.py --propose` refreshes the probe
(for new library releases or items) and prints the baseline pairs the data file lacks.
