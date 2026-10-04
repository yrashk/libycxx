# libycxx status

## Toolchain
| Compiler | Version | Notes |
|---|---|---|
| GCC | 16.2.0 (built from source, `/opt/gcc-16`) | latest release |
| Clang | 23.1.2 (apt.llvm.org) | latest release |

Conformance oracles (run only, never edited): libc++ tests from `llvmorg-23.1.2`
(`libcxx/test/std`), the libstdc++ testsuite from GCC 16.2.0, and our own spec-only suite
`tests/ycxx`.

## Per-header conformance (libc++ tests; pass / run, excluding documented skips)
| Area (libc++ test dir) | Clang | GCC | Freestanding | Notes |
|---|---|---|---|---|
| utilities/meta | 137/147 | 136/147 | yes | rest need `<vector>`/`<functional>`; GCC lacks `is_within_lifetime` |
| numerics/bit | 16/17 | 16/17 | yes | |
| language.support/cmp | all but missing-header tests | same | yes | |
| utilities/utility (pair, intcmp, ...) | see tuple row | | yes | |
| utilities/tuple + utility + meta + allocator.uses | 262/422 | 261/421 | yes | many need `<string>`/`<vector>`/`unique_ptr` |
| containers/sequences/array | 39/49 | 39/49 | yes | rest: `<string>`, `<regex>`, `<ranges>`, `unique_ptr` |
| utilities/optional | 74/87 | 75/87 | yes | rest: `<string>`/`<vector>`/`<ranges>`, `unique_ptr` |
| utilities/variant | 18/50 | 18/50 | yes | all 32 failures are missing `<string>` (libc++ `type_id.h`/tests) |
| utilities/expected | 75/87 | 75/87 | yes | rest: `unique_ptr`, `<algorithm>` |
| containers/views/views.span | 10/41 | 10/41 | yes | rest: `<string>` (23), `<ranges>`, `<algorithm>`, `<any>` |
| utilities/any | 9/25 | 9/25 | no (hosted) | all 16 failures: `<string>` |
| strings/string.view + char.traits | 36/170 | 36/170 | yes | 127 need `<string>` |
| utilities/template.bitset | 16/46 | 16/46 | yes | rest: `<vector>`, `<algorithm>`, `<sstream>`, `<string>` |
| iterators + range.access + concepts + function.objects | 185/515 | 185/515 | yes | most failures need `<ranges>`, `bind`, `function`, containers |

Whole-suite baseline (clang, before iterators/tuple/array/optional): 976 pass / ~8,000 run.

<typeindex>: libc++ utilities/type.index 8/9 (rest: `<string>`), libstdc++ 20_util/typeindex 5/5,
both compilers.
libstdc++ testsuite: 20_util/{function,move_only_function,copyable_function,function_ref,
constant_wrapper}: all pass on GCC except tests needing `<string>`/`<iostream>`; Clang also fails
constant_wrapper/generic.cc (throws during constant evaluation). libc++ utilities/const.wrap.class:
15/15 on both; func.wrap: all failures need `<algorithm>`/`<string>`.
libstdc++ testsuite: 20_util/{tuple,pair,uses_allocator}: 107 pass on both compilers.
20_util/variant: 27/31 on both (rest: missing `<string>`, `<vector>`, `<any>`).
20_util/any: 22/30 on both (rest: `<vector>`, `<string>`, `<set>`, `unique_ptr`).
21_strings/basic_string_view + char_traits: 98/130 on both (rest: `<string>`, `<sstream>`, `<iosfwd>`).
20_util/bitset + 23_containers/bitset: 22/38 on both (rest: `<string>`, `<sstream>`).
23_containers/span: 30/35 on both (rest: `<vector>`, `<deque>`).
20_util/expected: clang 18/20, gcc 18/20 (rest: `<string_view>`, `<vector>`). The libstdc++ harness
compiles with `-O2`, as DejaGnu's default flags do. Some tests rely on dead-code elimination:
`expected/cons.cc` declares `E(const int&)` without defining it, and links only when the
unreachable error branch is removed. `dg-options -fno-inline` is passed through.

## Freestanding
`tools/check_freestanding.sh`: every core header compiles with `-ffreestanding -nostdlib -nostdinc
-fno-exceptions -fno-rtti`; the smoke test links on x86_64-unknown-none-elf and
riscv64-unknown-elf (Clang) and x86_64 (GCC). Core headers: see `tools/headers.py`.

## Known compiler gaps and bugs
- GCC 16.2: no `__builtin_is_within_lifetime`, so `std::is_within_lifetime` is unavailable on GCC
  (constraint, probed in-language). Consequence: `std::start_lifetime` cannot detect an
  already-live object in constant evaluation on GCC, so it re-begins its lifetime and loses
  the values (own test `memory/start_lifetime`, XFAIL on GCC).
- Clang 23.1: no `__builtin_is_corresponding_member` or
  `__builtin_is_pointer_interconvertible_with_class`.
- Clang 23.1: `std::optional<Inner>` declared as a member of the class enclosing `Inner`, where
  `Inner` has default member initializers, leaves `Inner` non-default-constructible for
  `optional::emplace()` even after the enclosing class completes (libc++ test
  `empty_in_place_t_does_not_clobber`). Investigation time-boxed; trigger not yet isolated.
- Clang 23.1: `tuple<X>` where `X` is constructible from `const tuple<X>&&` gives "satisfaction of
  constraint depends on itself" (libc++ `convert_const_move`); GCC accepts.
- GCC 16.2: `Pack...[I]` inside a pack expansion over an empty `I` is diagnosed ("cannot index an
  empty pack") although nothing is instantiated; `bind` uses `tuple_element_t` instead.
- GCC 16.2: `static constexpr decltype(auto) v = (X);` with a class-type template parameter
  object `X` deduces `const T` instead of `const T&`; constant_wrapper spells the type out.
- GCC 16.2: a default template argument `decltype(X)` is computed from the substituted
  expression when `X` is given a dependent expression (`L::value ->* R::value` gives
  `constant_wrapper<9, const int>`); constant_wrapper's default is `remove_cvref_t<decltype(X)>`.
- Clang 23.1: the address of an explicit-object member function cannot be a template argument
  ("must explicitly qualify name of member function"); own test
  `functional/function_ref_cw_explicit_object` is XFAIL on Clang.
- GCC 16.2: `PR31384` (conversion function vs converting constructor in direct-init of `tuple`)
  resolves differently from Clang; the libc++ expectation matches Clang.

## Deliberate omissions
Deprecated and removed features are not implemented (`is_pod`, `is_trivial`, `aligned_storage`,
`has_denorm`, `tuple_size<volatile T>`, ...). See `tests/SKIPPED.md`.

## Deliberate divergences
- `std::max_align_t` and `::max_align_t` (from `<stddef.h>`) are distinct types with identical
  size and alignment. [support.c.headers.other]/1 would make them the same, but core cannot
  include a C header to name the C library's class. The same applies to `std::mbstate_t`
  (DECISIONS §3).
- `expected<T, E>`: `operator==(const expected&, const T2&)` deduces its left operand (it must be
  the expected or derived from it). With the draft's literal `const expected&` parameter, a
  constraint check such as `int == pair<int, expected<int, int>>` found through ADL re-enters
  itself. The cost is that a type which only *converts* to `const expected&` is no longer
  accepted on the left.

## Known limitations and draft defects
- `FLT_ROUNDS` is the constant 1 with GCC (no `__builtin_flt_rounds`), as in GCC's own
  `<float.h>`; it does not follow `fesetround`. Clang reports the current mode.
- `<cwchar>` with Clang on glibc: glibc declares `::wcschr` etc. only with the C signature, so an
  unqualified call on a const pointer under `using namespace std;` returns `wchar_t*`. Qualified
  `std::` calls are const-correct. (A `<wchar.h>` wrapper would be needed.)
- `std::any` allocates large values with a plain new-expression, honouring a class-specific
  `operator new`. A type that deletes it cannot be stored (libstdc++ any/83658 relies on this).
- Freestanding programs built with GCC link libgcc (helpers such as `__popcountdi2`).
- `char_traits<char16_t>::eof()`: [char.traits.require] wants a value distinct from
  `to_int_type(c)` for every `c`, but `int_type` is `uint_least16_t` (16 bits here), so no such
  value exists. libycxx returns 0xFFFF (own test `char_traits/eof` fails by design).
- `std::function`, `move_only_function` and `copyable_function` (40 bytes: a 24-byte buffer and
  two pointers) store targets that fit and are nothrow-move-constructible in place, others with a
  plain new-expression. Built from another owning wrapper with the same return type and
  argument passing, they adopt its target instead of wrapping it ([func.wrap.general]/2-3); an
  empty `std::function` so adopted becomes a stateless target that throws `bad_function_call`.
  Construction is noexcept when nothing can throw (a strengthening, as libstdc++ does).
  `function::target<T>()` without RTTI uses the same table-address identity as `any`.
- `any` without RTTI identifies types by the address of a per-type table, so `any_cast` across a
  shared library built with hidden visibility or `-Bsymbolic` does not recognise the type.
- No `<stddef.h>` wrapper: `::max_align_t` comes from the compiler's header and is not
  `std::max_align_t` (see Deliberate divergences).

## Open issues / next
- Phase 2 remaining: <exception> propagation (exception_ptr, nested_exception,
  exception_ptr_cast).
- Then Phase 3 (containers, algorithms), Phase 4 (ranges, charconv, format, ...).
- Constexpr exceptions (P3068): done for `exception`, `bad_alloc`, `bad_array_new_length`,
  `bad_exception`, `bad_cast`, `bad_typeid`, `bad_optional_access`, `bad_variant_access`, and
  `bad_expected_access`. Those the library throws are thrown from headers through `raise_with`,
  so GCC can throw them during constant evaluation; Clang 23 cannot throw during constant
  evaluation at all. Under -fno-rtti the six classes libsupc++ defines keep an out-of-line
  destructor, so they are not constexpr-destructible there (DECISIONS §4).
  Still open: the `<stdexcept>` classes (their message storage lives in the hosted runtime), so
  `__cpp_lib_constexpr_exceptions` is not yet defined.
