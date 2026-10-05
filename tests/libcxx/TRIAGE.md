# libc++ conformance suite: full-run triage

Run of 2026-10-04 against HEAD `12ff176` (after the `<chrono>` merge). The suite is libc++'s
`libcxx/test/std` from `llvmorg-23.1.2`, run with
`tools/run-conformance libcxx gcc|clang -- -j8 -sv` (GCC 16.2, Clang 23.1). Every failure was
read and classified, using the compiler diagnostics, the libc++ test source and the current
draft (eel.is/c++draft). libc++'s library sources were not consulted.

Categories:

- **A**: libycxx bug. The test is valid per the current draft and libycxx is wrong.
- **B**: a feature libycxx does not have yet.
- **C**: the test relies on libc++ internals, libc++ extensions or strengthenings, behaviour the
  draft leaves unspecified, or pre-C++26 rules (removed or deprecated features, old values).
- **D**: a compiler problem, such as a compiler bug, a missing builtin or a constexpr step limit.
- **E**: a harness or environment problem, such as running as root or a timeout under load.
- **F**: the test uses a name without including the header that declares it, relying on
  transitive includes.

## Totals

| | GCC 16.2 | Clang 23.1 |
|---|---|---|
| Discovered | 8543 | 8543 |
| Passed | 7439 | 7440 |
| Unsupported | 570 | 566 |
| Expectedly failed | 2 | 1 |
| **Failed + unresolved + XPASS (raw run)** | **529 + 2 + 1 = 532** | **534 + 1 + 1 = 536** |
| A / B / C / D / E / F | 28 / 14 / 233 / 12 / 125 / 120 | 29 / 19 / 237 / 7 / 124 / 120 |
| Still failing with this round's harness and skip changes (re-run of the failing set) | **238** (A 28, B 14, C 35, D 12, E 29, F 120) | **242** (A 29, B 19, C 39, D 7, E 28, F 120) |

The raw-run numbers are before this round's harness and skip changes. Those changes leave no
category-C/E test unexplained, and no test that passed before is skipped now. That was checked
against both raw runs.

Environment note: the machine was shared with other jobs (load average 15 to 25 on 4 cores).
`variant.visit/visit.pass` and `visit_return_type.pass`, and on GCC also `stable_sort.pass`,
reached the harness's 300 s compile timeout and were reported UNRESOLVED. Compiled by hand,
both variant tests pass: GCC takes 85 s and 194 s to compile them under that load (E).

## Re-run of 2026-10-05 (after the (A) fixes)

Whole suite again (`tools/run-conformance libcxx gcc|clang -- -j8 -sv`, library at `7e6a93f`,
harness and skip list of the first round). Every failure was compared with the per-test category
of the first run; the few that were not in it were read and classified.

| | GCC 16.2 | Clang 23.1 |
|---|---|---|
| Discovered | 8543 | 8543 |
| Passed | 7494 (was 7439) | 7495 (was 7440) |
| Unsupported | 836 | 832 |
| Expectedly failed | 2 | 1 |
| **Failed + unresolved** | **209 + 2 = 211** (was 532 raw, 238 after the first round's skips) | **215 + 0 = 215** (was 536 / 242) |
| A / B / C / D / E / F | 2 / 14 / 38 / 5 / 31 / 121 | 2 / 20 / 42 / 3 / 27 / 121 |
| With this round's skip entry (optional.iterator, C) | **210** (C 37) | **214** (C 41) |

- **(A)**: 22 of the first run's (A) tests pass on both compilers (the CPO/union, range-access
  `auto(x)`, empty/size of unbounded arrays, ranges::advance/prev, common_iterator, stream
  iterator `==`, shift_left, parallel reduce, append_range, priority_queue, coroutine_handle,
  any_cast, and the utility/map/unordered_map/type_traits macros; robust_against_proxy... is the
  C skip). Still failing: vector.bool.fmt/types.compile (formatter not declared by `<vector>`,
  open), and template.bitset/includes.pass, now for a **new (A)**: `<bitset>` does not include
  `<iosfwd>` (table above). The four version.compile tests that had (A) macros fail only on C
  values (and the B `boyer_moore_searcher`, and on Clang the D builtins), and
  optional.iterator/iterator.pass is C (skipped).
- **New failures** (not failing in the first run), all read:
  - utilities/format/format.arguments/format.arg/visit_format_arg.pass.cpp (both): was skipped as
    deprecated, now run (Annex D work); needs `EOF` from `constexpr_char_traits.h` (F, as STATUS
    says).
  - depr/depr.c.headers/stddef_h.compile.pass.cpp (Clang): was skipped as deprecated, now run;
    `::nullptr_t` after `<stddef.h>` needs the C++ `<stddef.h>` wrapper libycxx does not have (B,
    same as the own suite's `cstddef/stddef_global`; GCC's own `<stddef.h>` declares it).
  - atomics/atomics.ref/compare_exchange_{strong,weak}.pass.cpp (GCC): TIMEOUT under load (load
    average 11-15 on 4 cores; other agents' builds were running). Both pass when rerun (E).
  - variant.visit/{visit,visit_return_type}.pass.cpp (GCC): UNRESOLVED (compile timeout), the
    known E; both pass when rerun.
- **(D)**: GCC 12 -> 5 and Clang 7 -> 3 (the step-limit tests pass, see (D) below). Clang's
  type_traits.version (is_pointer_interconvertible, is_within_lifetime) moved from A to D.
- **(E)**: the 27 root-only filesystem tests remain (run as root), plus the timeouts above.
- B, C and F otherwise unchanged.

## (A) libycxx bugs

The draft text was checked for every entry. In the libycxx location column, `I/` is
`include/ycxx/`.

### `<concepts>`, `<iterator>`, `<ranges>`: customization point objects and iterator operations

| Test | Symptom | Draft | libycxx location | Suggested fix |
|---|---|---|---|---|
| concepts/concept.swappable/swappable_with.compile, iterators/iterator.cust.move/iter_move.pass | `ranges::swap(u, u)` / `ranges::iter_move(u)` on a **union** with an ADL overload: "no match" | [concept.swappable]/2.1, [iterator.cust.move]/1.1: "has class or enumeration type". A union is a class type | `I/core/concepts.hpp:73-75` (`swap_cpo::adl_swappable` uses `__is_class`), `I/core/iterator_core.hpp:292` (`adl_iter_move`), `:638-639` (iter_swap), `I/core/range_access.hpp:32` (`class_or_enum`, used by every range-access CPO) | Accept `__is_class(T) \|\| __is_union(T) \|\| __is_enum(T)` everywhere "class or enumeration type" is tested **Fixed**: unions count as class types in the swap, iter_move, iter_swap and range-access CPOs; both tests pass. |
| ranges/range.access/end.pass (lines 324, 332; the test's first failure is a C) | `noexcept(ranges::end(ntme))` is false for a noexcept `end()` returning a prvalue iterator whose move may throw | [range.access.end]/2.5-2.6 and [range.access.begin]: `auto(t.end())`, which copies nothing for a prvalue | `I/core/range_access.hpp:20` (`decay_copy` noexcept is `is_nothrow_convertible<T, decay_t<T>>`, which also counts a move of a prvalue); the same helper is used by begin/rbegin/rend (`iterator_adaptors.hpp:1252-1267`) | Use `auto(expr)` (both compilers support it) in the constraints and in `nothrow()`, or treat a prvalue of the decayed type as noexcept **Fixed**: every range-access CPO (begin, end, rbegin, rend, size, data, reserve_hint) writes `auto(x)`. The test now fails only in its C part (cend), so it is skipped. |
| ranges/range.access/empty.pass, size.pass | `is_invocable_v<decltype(ranges::empty), Incomplete[]>` is a hard error (static_assert in `ranges::begin`) instead of false | [range.prim.empty]/2.1, [range.prim.size]/2.1: "If T is an array of unknown bound, ranges::empty(E)/size(E) is ill-formed" (plain ill-formed, so it SFINAEs) | `I/core/range_access.hpp:169-211` (size_ns), `:238-269` (empty_ns) reach `:69` (`static_assert(complete_array_elem<T>)` in begin) | Reject `is_unbounded_array_v` in size's and empty's constraints before they try `ranges::begin` **Fixed** as suggested; both pass. |
| iterators/range.iter.ops.advance/iterator_count_sentinel, range.iter.ops.prev/iterator_count_sentinel | `ranges::advance(i, 0, bound)` (and `ranges::prev(i, 0, bound)`) with `bound` before `i` jumps to `bound` | [range.iter.op.advance]/6.1.1: go to bound only if \|n\| ≥ \|bound − i\| | `I/core/iterator_ops.hpp:100` (`n >= 0 ? n >= d : n <= d` with `n == 0` and `d < 0`) | Compare absolute values: `(n < 0 ? -n : n) >= (d < 0 ? -d : d)` **Fixed** (absolute values compared without overflow); both pass. |
| iterators/iterators.common/plus_plus.pass, iterator_traits.compile | `common_iterator<cpp17_output_iterator<int*>, S>` `it++`: "no type named value_type" | [common.iter.nav]/5: `requires {*i++} -> can-reference` **or** `!(indirectly_readable<I> && constructible_from<iter_value_t<I>, …>)`. The draft's condition must short-circuit | `I/core/iterator_adaptors.hpp:1157-1159`: `if constexpr (A \|\| !(B && C<iter_value_t<I>>))` still instantiates `iter_value_t<I>` | Put the second condition in a concept (a conjunction of concepts is checked lazily) **Fixed** with a concept; both pass. |
| iterators/istream.iterator.ops/equal.pass | `std::operator==(i1, i2)` for istream_iterator: no match | [iterator.synopsis]: `template<…> bool operator==(const istream_iterator<…>&, const istream_iterator<…>&)` and the istreambuf_iterator one are namespace-scope templates | `I/core/stream_iterators.hpp:62` (and `:148` for istreambuf_iterator) are hidden friends | Declare them as namespace-scope function templates (keep the `default_sentinel_t` overloads as friends) **Fixed**: [istream.iterator.ops]/8 and [istreambuf.iterator.ops]/5 still declare these two as namespace-scope templates (only the `default_sentinel_t` comparisons are hidden friends); passes. |

### `<algorithm>`, `<numeric>`

| Test | Symptom | Draft | libycxx location | Suggested fix |
|---|---|---|---|---|
| algorithms/alg.shift/ranges.shift_left.pass | `ranges::shift_left(forward_iterator, sized sentinel, n)`: "no viable `+=`" | [alg.shift]: `ranges::shift_left` takes `permutable I` + `sentinel_for<I> S` (forward iterators) | `I/core/algo_mutate.hpp:154` (`mid += n` under `sized_sentinel_for<S, I>` only) | `ranges::next(mid, n)` / `iter_advance`, or also require `random_access_iterator<I>` for that branch **Fixed** (`iter_advance`); passes. |
| numerics/numeric.ops/reduce/pstl.reduce.pass, transform.reduce/pstl.transform_reduce.binary.pass, .unary.pass | ExecutionPolicy reduce / transform_reduce with move-only `T`: "use of deleted copy constructor" | [reduce]/6.1, [transform.reduce]: T needs only Cpp17MoveConstructible | `I/core/numeric_parallel.hpp:20, 26, 33, 41, 48` (and the scan overloads from `:55`) forward `init` as an lvalue | `std::move(init)`. The unary test also checks that no value is moved twice ("Banane"), so re-check after the fix **Fixed**: every ExecutionPolicy overload of `<numeric>` moves `init`; the three tests pass. |
| algorithms/robust_against_proxy_iterators_lifetime_bugs.pass (low priority) | `stable_sort` / heap ops on a proxy iterator: `less<>(int&, Reference)` is ambiguous | [alg.sorting.general]: comp is required on dereferenced iterators (`comp(*i, *j)`) | `I/core/algo_sort.hpp:513` (merge_with_buffer compares a buffered `value_type` with `*middle`), `:188` (sift-down compares `value` with `*c`) | Compare through references to elements, or move the buffered value back before comparing. Debatable: libstdc++ also compares buffered values **Not a bug (now C, skipped)**: these std:: algorithms need a Cpp17RandomAccessIterator for a mutable iterator ([algorithms.requirements]/4.7), whose reference is `value_type&`. Running the test without the three sorting lines found a real bug: `shuffle` (std:: and ranges::) swapped an element with itself; fixed. |

### `<vector>`, `<queue>`, `<bitset>`, `<utility>`

| Test | Symptom | Draft | libycxx location | Suggested fix |
|---|---|---|---|---|
| containers/vector.modifiers/append_range.pass | `append_range` of a sized range with a type that is not move-assignable: "use of deleted operator=" | [sequence.reqmts]/110 (append_range): only Cpp17EmplaceConstructible, plus Cpp17MoveInsertable for vector | `I/core/vector.hpp:646` (`insert_counted(size(), …)`) reaches the rotate in `I/core/sequence_support.hpp:114` | When the position is `end()`, construct at the end without rotating **Fixed** (`append_counted`); passes. |
| containers/priqueue.members/push_range.pass | priority_queue's move constructor/assignment call `c.clear()`. The test's Container has no `clear` | [priqueue.overview]/1: the container needs random-access iterators, `front`, `push_back`, `pop_back` | `I/core/queue.hpp:208-221` (extension: a moved-from queue is left empty) | `if constexpr (requires { q.c.clear(); })`, or drop the extension **Fixed**: the moved-from queue is cleared only when the container has `clear()`; passes. |
| containers/vector.bool.fmt/types.compile | `std::formatter` is not declared after `#include <vector>` | [vector.syn] declares `template<class T, class charT> requires is-vector-bool-reference<T> struct formatter<T, charT>;` | the partial specialization is defined with `<format>` (documented in STATUS as a choice) | Declare `formatter` (primary) and the partial specialization from `<vector>`; the definition can stay with the format core. **Still failing** (re-run of 2026-10-05) |
| utilities/template.bitset/includes.pass | `std::string s;` after only `#include <bitset>`: incomplete type | [bitset.syn] begins `#include <string>` | `include/bitset`, `I/core/bitset.hpp:8` (deliberately does not include `<string>`) | Include `<string>` from `<bitset>` (both are core) **Fixed** (`<string>`), but the test still fails on the second half of [bitset.syn]: see the next row |
| utilities/template.bitset/includes.pass (re-run of 2026-10-05, **new, open**) | `std::ios`, `std::istream`, `std::ostream`, `std::iostream` are not declared after only `#include <bitset>` | [bitset.syn] begins `#include <string>` and `#include <iosfwd> // for istream, ostream, see [iosfwd.syn]` (checked with draft.sh bitset.syn) | `include/bitset:7` includes `<string>` only; `<iosfwd>` (`include/iosfwd`, hosted) is never reached | In hosted builds also `#include <iosfwd>` from `<bitset>` (`#if YCXX_HOSTED`, as `<cstdlib>`/`<cstring>` do; `<iosfwd>` is hosted, `<bitset>` core) |
| utilities/pairs.pair/pair.incomplete.compile (**Clang only**, low priority) | `struct Test { vector<pair<int, Test>> v; }; pair<int, Test> p;`: instantiating `pair<int, Test>` checks `implicitly_default_constructible<Test>`, which instantiates `~vector<pair<int,Test>>` while pair is incomplete | [pairs.pair] `explicit(see below) pair()`; [vector.overview]/4 allows the incomplete element type; libc++ and libstdc++ accept the code | `I/core/pair.hpp:53-54` (explicit-specifier evaluated at class instantiation; the concept at `:12`) | Make the default constructor a constrained template (`template<class U1 = T1, class U2 = T2>`) so the trait is checked only when it is used **Fixed** as suggested; passes on Clang. |

### `<coroutine>`, `<optional>`, `<any>`

| Test | Symptom | Draft | libycxx location | Suggested fix |
|---|---|---|---|---|
| language.support/coroutine.handle.prom/promise.pass | `coroutine_handle<const P>::from_promise(p)`: "invalid conversion from const void* to void*" | [coroutine.handle.con]/2: `from_promise(Promise&)` for any Promise, const included | `I/core/coroutine.hpp:54` | `__builtin_coro_promise(const_cast<void*>(static_cast<const volatile void*>(addressof(p))), …)` **Fixed** as suggested; passes. |
| utilities/optional.iterator/iterator.pass | `std::format_kind<optional<T>>` is not declared by `<optional>`, and even with `<format>` it is not `range_format::disabled` | [optional.syn]: `template<class T> constexpr auto format_kind<optional<T>> = range_format::disabled;` | missing (`I/core/optional.hpp`) | Declare `format_kind`/`range_format` in a core header that `<optional>` includes, and add the specialization. Without it, optional (a range since C++26) is range-formattable **Fixed** (libstdc++ triage round). The test still fails only because it reads `decltype(it)::value_type`, i.e. expects a class-type iterator, while `optional::iterator` is implementation-defined ([optional.iterators]/1) and libycxx's is `T*`: C, skipped (the rest of the test, compiled without those four lines, passes on both compilers) |
| utilities/any.cast/any_cast_pointer.pass | `any_cast<NoCopy>(&a)` instantiates NoCopy's copy constructor | [any.nonmembers]/9: Mandates only `!is_void_v<T>` | `I/hosted/any.hpp:132-133` (`holds<T>()` compares with `table_for<T>`, which instantiates `ops<T>::copy`, `:66`, `:76`, `:93`) | Identify the type with a per-type tag that does not instantiate the copy operation **Fixed** (libstdc++ triage round); passes |

### `<version>` (feature-test macros for features that are implemented)

| Test | Symptom | Draft | libycxx location | Suggested fix |
|---|---|---|---|---|
| support.limits.general/{utility,tuple,map,unordered_map,version}.version.compile | `__cpp_lib_tuple_like` undefined, although P2165 is implemented (pair from array, tuple/pair comparison, map from a range of tuples: checked) | [version.syn] `__cpp_lib_tuple_like 202311L` (utility, tuple, map, unordered_map) | `I/core/version.hpp` | Define it **Fixed**: utility/map/unordered_map.version pass; tuple/version.version fail only on C values (below) |
| …/{tuple,version}.version.compile | `__cpp_lib_apply` undefined (`apply` and the `is_applicable` / `apply_result` traits exist) | `__cpp_lib_apply 202603L` (tuple, type_traits, meta) | `I/core/version.hpp` | Define it after checking the 202603 additions **Fixed** (202603L; libc++ 23 expects the old 201603L, a C failure) |
| …/{functional,type_traits,version}.version.compile | `__cpp_lib_result_of_sfinae` and `__cpp_lib_constexpr_functional` undefined | `201210L`, `201907L` | `I/core/version.hpp` | Define them (invoke_result is SFINAE-friendly; `std::invoke` is constexpr) **Fixed**: type_traits.version passes on GCC (on Clang it fails only on the D macros `is_pointer_interconvertible`/`is_within_lifetime`); functional.version fails only on C values and the B `boyer_moore_searcher` |

The `<version>` audit in the appendix also lists macros that no libc++ test checks.

## Counts per category and top-level directory (raw run)

GCC 16.2:

| Directory | A | B | C | D | E | F | Total |
|---|---|---|---|---|---|---|---|
| algorithms | 2 |  | 16 | 3 |  |  | 21 |
| concepts | 1 |  |  |  |  | 1 | 2 |
| containers | 3 |  | 18 | 2 | 3 | 16 | 42 |
| depr |  | 3 | 41 |  |  |  | 44 |
| experimental |  |  |  |  | 88 |  | 88 |
| input.output |  |  | 12 |  | 29 | 28 | 69 |
| iterators | 6 |  | 6 |  |  | 4 | 16 |
| language.support | 8 | 1 | 31 |  |  |  | 40 |
| localization |  |  | 55 |  |  |  | 55 |
| numerics | 3 |  | 8 |  |  | 11 | 22 |
| ranges | 2 |  | 8 | 2 | 1 |  | 13 |
| re |  |  |  |  |  | 1 | 1 |
| strings |  |  | 4 | 1 |  | 55 | 60 |
| time |  |  | 8 |  |  |  | 8 |
| utilities | 3 | 10 | 26 | 4 | 4 | 4 | 51 |
| **Total** | **28** | **14** | **233** | **12** | **125** | **120** | **532** |

Clang 23.1:

| Directory | A | B | C | D | E | F | Total |
|---|---|---|---|---|---|---|---|
| algorithms | 2 |  | 16 | 2 |  |  | 20 |
| concepts | 1 |  |  |  |  | 1 | 2 |
| containers | 3 |  | 19 | 1 | 3 | 16 | 42 |
| depr |  | 4 | 41 |  |  |  | 45 |
| experimental |  |  |  |  | 88 |  | 88 |
| input.output |  |  | 12 |  | 29 | 28 | 69 |
| iterators | 6 |  | 6 |  |  | 4 | 16 |
| language.support | 8 | 1 | 31 |  |  |  | 40 |
| localization |  |  | 55 |  |  |  | 55 |
| modules |  | 2 |  |  |  |  | 2 |
| numerics | 3 |  | 10 | 2 |  | 11 | 26 |
| ranges | 2 |  | 8 |  | 1 |  | 11 |
| re |  |  |  |  |  | 1 | 1 |
| strings |  | 2 | 4 |  |  | 55 | 61 |
| time |  |  | 8 |  |  |  | 8 |
| utilities | 4 | 10 | 27 | 2 | 3 | 4 | 50 |
| **Total** | **29** | **19** | **237** | **7** | **124** | **120** | **536** |

Each test is counted once, under the cause of its first diagnostic. Two tests also contain a
second cause: `ranges/range.access/end.pass` (C first, then the A `decay_copy` noexcept bug) and
`utilities/variant/variant.ctor/T.pass` (see C).

## (B) Missing features

| Feature | Draft | Tests |
|---|---|---|
| `boyer_moore_searcher`, `boyer_moore_horspool_searcher` (and `__cpp_lib_boyer_moore_searcher`) | [func.search.bm], [func.search.bmh] | func.search/func.search.bm/* (5), func.search.bmh/* (5) |
| C++ wrappers `<stdlib.h>` (abs overloads; no `abs(unsigned)`), `<complex.h>`, `<tgmath.h>`, `<wchar.h>` (const-correct `wcschr`/`wcsstr`… as global names) | [support.c.headers.other]/1, [complex.h.syn], [tgmath.h.syn] | depr/depr.c.headers/{stdlib_h, complex_h, tgmath_h}; Clang also wchar_h.compile, strings/c.strings/cwchar_include_order{1,2}.compile.verify |
| senders/receivers in `<execution>` (`__cpp_lib_senders`, `counting_scope`, `parallel_scheduler`, `task`) | [exec] | support.limits.general/execution.version.compile |
| standard library modules `import std;` / `import std.compat;` (`__cpp_lib_modules`) | [std.modules] | modules/std.pass, modules/std.compat.pass (Clang) |
| Not exercised by the suite, found by the `<version>` audit: `<stdbit.h>`, `<stdckdint.h>` (`__cpp_lib_stdbit_h`, `__cpp_lib_stdckdint_h` 202603), `pointer_tag_pair`, `__cpp_lib_view_interface` 202606, `__cpp_lib_start_lifetime` 202603, the `__cpp_lib_hardened_*` macros (YCXX_HARDENED exists, the macros are not defined), `__cpp_lib_freestanding_{cstdlib,functional,memory,execution}`, `__cpp_lib_constexpr_exceptions` (known: Clang 23) | [version.syn] | none |

## (C) Not applicable to the current draft or to libycxx

Grouped by pattern. Entries marked *skip* were added to `tests/libcxx/skip.txt` in this round,
each with its reason.

- **Removed or deprecated features** (*skip*): `<strstream>` (40 tests, P2867); `<codecvt>`
  (47 tests: the localization/locale.stdcvt tests, codecvt_utf8*, and tests that include it,
  P2871); `<cstdalign>`, `<cstdbool>`, `<ciso646>`, `<ctgmath>`, `<ccomplex>`;
  negators/not1/not2; `uncaught_exception`; `std::iterator`; `rel_ops` (2); the
  `move_iterator::operator->` member; static `vector<bool>::swap(reference, reference)`;
  `basic_string::reserve()` without an argument; free `begin`/`end` for `initializer_list`
  (P3016); `tuple_element` and `variant_alternative` of `volatile` types.
- **Feature-test macros at older values** (27 version.compile tests, not skipped because some
  of these tests also contain A or B macros). libc++ 23 expects older values for bind_back,
  function_ref, expected, span (and the merged span_at), constexpr_memory, freestanding_*,
  bitops, atomic_min_max, barrier, chrono, constexpr_cmath/complex/string, debugging, format,
  format_path, hazard_pointer, inplace_vector, linalg, parallel_algorithm, print,
  ranges_as_const, submdspan, to_chars, and names the draft does not have
  (`__cpp_lib_default_template_type_for_algorithm_values`, `__cpp_lib_generate_random`). In
  each case libycxx has the [version.syn] value.
- **noexcept strengthenings that libc++ adds** (*skip*): tuple copy, converting and pair
  assignment, and the default constructor; `array::operator[]` (mdspan
  extents/ctor_from_integral); `vector::operator[]` (time.zone name/target, 3 tests);
  `polymorphic_allocator(memory_resource*)` (equality). Also `cbegin`/`cend` of an iterator
  that is not a const iterator (the cend part of end.pass).
- **SFINAE where the draft has Mandates or no constraint** (*skip*): `make_optional`
  (3 tests); `bind_front` with non-movable arguments.
- **Pre-P2278 range access** (*skip*: begin, rbegin, rend, data; end.pass is not skipped
  because of the A entry above): `cbegin`/`crbegin`/`cend` on types whose const view is not a
  range, and on `int(&)[]`.
- **is_permutation Mandates** (*skip*, 12 sort/heap tests): `MoveOnly*` compared with `int*`
  ([alg.is.permutation]/2).
- **Divergences already documented in STATUS** (*skip*): initializer_list deduction guides
  deduce `const Key` (map/multimap deduct_const, unordered deduct and deduct_const); container
  `iterator::pointer` (2); `construct_piecewise_pair_evil`; `to_string(float)` under P2587;
  charconv.msvc; from_chars results that are out of range; mdspan layout_left/right from
  layout_stride; the pre-P0952 `generate_canonical`; `__int128` engines and distributions (2);
  `polar(-0.0, θ)`; long double `%La` digits; `time_get` field extent (2); the UTF-8 classic
  `codecvt<wchar_t, char>` (wchar_t_encoding/max_length/unshift and 5 wide filebuf
  seek/swap/move tests); `codecvt_byname("en_US")` (4); `stringbuf` with app and without ate
  (overflow); `get_bool` eofbit after a complete match; move_iterator `>`, `>=`, `<=` (3).
- **Exposition-only constructors and libc++ internals** (*skip*): the istream_view iterator
  constructor (3 tests); test_chrono_leap_second.h (`#error` for other vendors, 5 tests);
  `basic_filebuf::__open`; openmode passed as `0`; `_LIBCPP_HAS_C8RTOMB_MBRTOC8`; the XFAIL in
  invoke_r.temporary.verify (libycxx diagnoses as the test intends, so lit reported XPASS).
- **Unspecified counts and QoI diagnostics** (*skip*): move counts in iter_swap; libc++'s
  set_intersection operation counts (the draft-bound part passes); copies of the fold_right_last
  function object; libc++'s static_assert diagnostics for requirement violations
  (min/max_element with input iterators; vector of a type that is not move-insertable).
- **Invalid test types** (*skip*): optional.hash (a user `hash<B>` whose `operator()` is not
  const); unique_ptr dereference.single (a pointer type that is not Cpp17NullablePointer);
  ranges_replace_copy_if (line 66 passes a predicate type as the iterator); unique_ptr
  move_convert.pass (deleters that cannot be assigned, [unique.ptr.single.asgn]/6.3);
  is_swappable `AmbiguousSwap`: with a requires-clause `std::swap`, partial ordering prefers it.
- **Not skipped, kept visible**:
  - views.span/types.pass, span.cons/span.pass: `span<volatile std::string>::const_iterator`
    needs `input_iterator<volatile string*>`. By [meta.trans.other] COMMON-REF, `volatile
    string*` is not indirectly_readable, which looks like a draft defect.
  - span.cons/copy.pass: `span<Incomplete>`, but [span.overview]/4 requires a complete type.
  - variant.ctor/T.pass: llvm PR151328 lazy constraints, where the Matcher/ConvertibleFromAny
    constraint recursion depends on itself. This is QoI: `I/core/variant.hpp:317-319` computes
    FUN in a default template argument before the other constraints. On Clang the test also
    instantiates `BoomOnAnything`'s constexpr constructor body in the narrowing check at
    `variant.hpp:163`. A workaround is to skip the array-init check for class types.
  - last_write_time.pass (min time; also needs a non-root run).
  - numbers/value.pass and c.math/cmath.pass (STATUS).
  - Clang's `_BitInt` as an integer type (5 Clang tests: intcmp.bitint, saturating.bitint,
    mdspan extents/bitint, abs.pass): not an extended integer type in libycxx.

## (D) Compiler problems

- Constexpr step/operation limits. These are reached even with libc++'s own raised limits, so
  libycxx's constant-evaluation paths cost more than libc++'s:
  - GCC `-fconstexpr-ops-limit`: alg.count/count.pass, ranges.count.pass,
    stable.sort/stable_sort.pass, string_replace/replace_with_range.pass,
    flat.multimap/flat.multiset capacity/size.pass.
  - Clang: count.pass, ranges.count.pass, complex_times_complex.pass,
    complex_divide_complex.pass, vector.bool/find.pass.

  **Resolved:** all of them pass on both compilers now, and so does vector.modifiers/emplace,
  which STATUS listed. On GCC the harness lacked the feature `has-fconstexpr-ops-limit`, so
  libc++'s raised `-fconstexpr-ops-limit` was never passed. It is declared in lit.cfg.py now, and
  that alone fixes 4 of the 6 GCC tests. The rest were libycxx costs. fill, find and count on
  vector<bool> worked bit by bit; they now work a word at a time (`I/core/bit_iter_algos.hpp`).
  The constexpr complex multiplication and division of special values were chains of small calls,
  and Clang counts each call and statement as a step. Division also scaled with the bitwise
  logb/scalbln. These helpers are flattened now, with exact power-of-two fast paths. Steps needed
  under Clang's 2M limit: divide went from 7.0M to 1.9M, multiply from 2.5M to 1.86M.
  With more steps, complex_divide_complex also exposed a real bug (A, fixed): a NaN denominator
  part took div's fast path, whose arithmetic then produced a NaN, which is not a constant
  expression.
- GCC 16: no `__builtin_is_within_lifetime` (is_within_lifetime.compile).
- GCC 16: `__is_array(A[10])` is false when `sizeof(A) == 0` (zero-length-array extension),
  which affects range.access/begin.sizezero and end.sizezero. A partial-specialization
  `is_array` would avoid it (`I/core/meta_base.hpp:198-200`).
- GCC 16 bugs documented in STATUS: tuple.cnstr/PR31384.pass and
  unique.ptr.observers/op_subscript.runtime.pass.
- GCC 16: optional.object.assign/assign_value.pass. Checking
  `is_assignable<optional<F>&, optional<F>&>` (F constructible from `optional<F>&`) recurses
  into `is_constructible_v<F, F>` ("not usable in a constant expression"); Clang accepts.
  libycxx's `optional.hpp:175-178`/`:267` constraint order follows the draft.
- Clang 23 bugs documented in STATUS: tuple.cnstr/convert_const_move.pass and
  optional.ctor/empty_in_place_t_does_not_clobber.pass.

## (E) Harness and environment

- **experimental/** (88 tests): `LibcxxFormat` ignored `config.unsupported`, which
  `experimental/lit.local.cfg` sets without `c++experimental`. *Fixed* in
  `tests/ycxxlit/libcxx_format.py`.
- **libc++ hardening assertion tests** (7: array assert.front/back/indexing,
  unique_ptr assert.subscript, streambuf setg/setp.assert, iota_view assert.ctor.value.bound): they
  are `UNSUPPORTED: libcpp-hardening-mode=none` and check libc++'s messages. *Fixed*: lit.cfg.py
  declares `libcpp-hardening-mode=none` (libycxx is not hardened by default).
- **Running as root** (27 filesystem tests): permission-denied paths cannot fail (directory_entry
  cons/mods/obs, directory_iterator and recursive_directory_iterator ctor/increment, the status
  predicates (`is_*`, `exists`, `status`, `symlink_status`), remove, remove_all, bad_perms_parent,
  temp_directory_path). Not skipped: they are valid as a non-root user.
- **Compile timeouts under load** (UNRESOLVED): variant.visit/visit.pass, visit_return_type.pass
  (and stable_sort.pass on GCC, plus one more Clang test, in the re-run). They pass when compiled alone. They are slow to
  compile (85 s and 194 s on GCC under load), which is a QoI note for `<variant>` visit.

## (F) Transitive-include reliance (120 on both compilers)

- `EOF` from `test/support/constexpr_char_traits.h` (it includes only `<string>`, `<cassert>`,
  `<cstddef>`), plus char.traits `eof.pass` for `EOF` and `WEOF`: 85 tests (string.view 43,
  basic_string 9, string streams 16, syncstream 11, format.arg visit 2, re.submatch,
  mem.res header_string_synop, basic.string.hash).
- Unqualified `int64_t`/`uint32_t` without `<cstdint>`: 18 (mdspan layout_left/right/stride 16,
  deallocate_size, make_from_tuple).
- `std::count`/`std::min` without `<algorithm>`: 12 (valarray mask_array 11, ifstream
  offset_range).
- `std::istream`/`ostream`/`streambuf` from `<iterator>` alone: 4 (stream iterator types.pass).
- `std::unique_ptr` without `<memory>`: 1 (equality_comparable_with.compile).

## Changes made in this round (no library code changed)

- `tests/ycxxlit/libcxx_format.py`: honour `test.config.unsupported` (from `lit.local.cfg`).
- `tests/libcxx/lit.cfg.py`: feature `libcpp-hardening-mode=none`.
- `tests/libcxx/skip.txt`: 62 entries covering 198 tests (category C) and 1 (E, the XPASS), each
  with its reason. None of them matches a test that passed in the raw runs.

## Appendix: `<version>` audit

Every [version.syn] macro was compared with what libycxx defines in `<version>` and in each
header named for it. The audit used `-dM -E` on both compilers. Undefined or different, apart
from the B items above:

- Both compilers: `__cpp_lib_apply`, `__cpp_lib_tuple_like`, `__cpp_lib_result_of_sfinae`,
  `__cpp_lib_constexpr_functional` (A above).
- GCC: `__cpp_lib_is_within_lifetime` (D). Without `-freflection`: `__cpp_lib_reflection` and
  `__cpp_lib_define_static`.
- Clang: `__cpp_lib_contracts`, `__cpp_lib_is_pointer_interconvertible` and
  `__cpp_lib_is_structural` (compiler gaps, documented in STATUS).
