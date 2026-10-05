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

## Policy round of 2026-10-05: every former baseline failure classified

Since 49a167c a test reported FAIL fails CI and the baselines are gone. Every test of the old
`baseline/{linux,darwin}-{gcc,clang}.txt` outside `localization/` and `input.output/` (except
filesystems, string streams and syncstream, handled here) is now in exactly one state. Run:
`tools/run-conformance libcxx gcc|clang` over every top-level directory but `localization/`
and `input.output/` (plus `input.output/{filesystems,string.streams,syncstream}`), `-j4`, as root,
with the named locales of `tools/ci/gen-locales` installed; failures rerun with the final tree.
"Before" is the same run with this round's skip, xfail and unsupported entries and fixes taken out.

| 7841 tests | GCC before | GCC after | Clang before | Clang after |
|---|---|---|---|---|
| Passed | 7007 | 7009 | 7007 | 7006 |
| Failed | 259 | 2 | 265 | 8 |
| Expectedly failed | 3 | 9 | 1 | 6 |
| Unsupported | 572 | 821 | 568 | 821 |

(Before includes 52 tests that fail only for want of named locales: they are UNSUPPORTED now,
by the named-locale gating of the merged named-locale work.)

- **(a) fixed**: variant's converting constructor (variant.ctor/T.pass: Clang instantiated a
  constexpr constructor through FUN's array test; a Tj constructible from anything recursed,
  llvm.org/PR151328); `thread::id` `operator<<` inserted an integer, so `oct`/`showpos` and the
  locale's grouping changed it ([thread.thread.id]/9); chrono `%j %U %W %V %G %g` on a date that
  is not `ok()` ([time.format]/3; the same fix came with the merged named-locale branch).
- **(a) still failing** (large features): `__cpp_lib_senders` (execution.version,
  version.version: senders/receivers, [exec]); `import std;`/`import std.compat;` (modules/std,
  std.compat, Clang; [std.modules]); the C++ `<wchar.h>` and `<stddef.h>` wrappers (Clang:
  depr.c.headers/wchar_h, stddef_h, strings/c.strings/cwchar_include_order1/2;
  [support.c.headers.other]/1).
- **(b) skipped** (skip.txt): 29 version.compile tests on older-draft values (divergence); EOF/WEOF
  (86) and the global `intN_t` names (17) (divergence, pending decision, STATUS); transitive
  includes (equality_comparable_with, stream iterator types, mask_array, cmath.pass:
  implementation-specific); numbers/value and span<Incomplete> copy (divergence);
  make_from_tuple (extension); vector emplace.pass (an rvalue argument aliasing an element,
  [res.on.arguments]/1.3; libstdc++ fails it too); directory_entry path.pass and
  last_write_time.pass (divergence).
- **(b) unsupported in one configuration** (unsupported.txt): the 26 permission-error filesystem
  tests under the `root` feature; the 4 `_BitInt` tests under `clang` (extension).
- **(c) expected failures** (xfail.txt): begin/end.sizezero, op_subscript.runtime, PR31384 (GCC);
  convert_const_move, empty_in_place_t_does_not_clobber, type_traits.version (Clang);
  span.pass and types.pass (any: draft defect, span<volatile T> for a class T).
- **(E)** harness: libc++ test programs run without LANG/LANGUAGE/LC_* (as libc++'s lit runs
  them; CPython's locale coercion leaked LC_CTYPE=C.UTF-8: text_encoding environment.pass).
  variant.visit/visit_return_type.pass compiles in 186 s on GCC at load 20 and can reach the 300 s
  compile timeout on a loaded machine; it passes when the load is lower.
- **macOS only** (old darwin baselines, not run here): depr.c.headers/string_h.pass and
  uchar_h.compile (both), wchar_h.compile and strings/c.strings/cwchar_include_order1/2 (GCC),
  fs.op.copy_file/copy_file_procfs, numeric.limits.members/tinyness_before, c.math/hermite,
  c.strings/cstring.pass, locale.nm.put put_long_double (both); the func.search tests listed
  there pass since the searchers were added.

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
  C skip). Still failing: vector.bool.fmt/types.compile (formatter not declared by `<vector>`;
  fixed since), and template.bitset/includes.pass, then for a new (A): `<bitset>` did not include
  `<iosfwd>` (table above; fixed since). The four version.compile tests that had (A) macros fail only on C
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

## Re-triage of 2026-10-05: strings, string_view, char_traits, string streams, syncstream, format.arguments, tuple, span, `<version>`

Run with `tools/run-conformance libcxx gcc|clang 'strings|input.output/string.streams|
input.output/syncstream|utilities/format/format.arguments|utilities/tuple/tuple.tuple|
containers/views/views.span|language.support/support.limits' -- -j4` (library at `6f13096`):
GCC 699 pass / 121 fail, Clang 696 / 124 (22 unsupported each). The GCC failures are exactly
this area's lines of `baseline/linux-gcc.txt`; Clang fails the same tests plus four known ones
(type_traits.version: D; cwchar_include_order1/2: B, no `<wchar.h>` wrapper;
tuple.cnstr/convert_const_move: D) and passes PR31384 (a GCC-only D). No test of the area is a
libycxx bug, so no library code and no baseline line changed.

To make sure the transitive-include failures hide no library bug, the area was run again with
the compiler forced to include `<cstdio>`, `<cwchar>` and `<cstdint>` first (a wrapper given as
`YCXX_GXX`/`YCXX_CLANGXX`; not a harness change): then **every** string.view, basic_string,
char_traits, string-stream, syncstream and format.arg test passes on both compilers, except
`deallocate_size` and `make_from_tuple` (global `::uint32_t`, see below). With `-include stdint.h`
`deallocate_size` passes on both; `make_from_tuple` then fails only on the C entry below.

- **(F) `EOF`/`WEOF` without `<cstdio>`/`<cwchar>`** (84 tests: string.view 43 and basic_string 9, each with its enabled_hashes test,
  string streams 16, syncstream 11, format.arg visit/visit.return_type/visit_format_arg 3,
  char.traits eof 2): `test/support/constexpr_char_traits.h` and `nasty_string.h`
  use `EOF` after including only `<string>`, `<cassert>`, `<cstddef>`; the eof tests compare with
  `EOF`/`WEOF` after `<string>` and `<cassert>`. [char.traits.specializations.char]: "eof()
  Returns: EOF." names the macro of `<cstdio>`, but [string.syn] includes only `<compare>` and
  `<initializer_list>`, and [res.on.headers]/1 only permits more: "A C++ header may include
  other C++ headers." libycxx's `<string>` is core and includes no C header (DECISIONS §3), so
  these stay failing by design.
- **(F) global `::uint32_t`/`::uint64_t`** (strings/basic.string/string.capacity/deallocate_size,
  tuple.apply/make_from_tuple): the tests include no `<cstdint>`/`<stdint.h>` and use the global
  names. Even `<cstdint>` would not guarantee them: [headers]/5 "It is unspecified whether these
  names ... are first declared within the global namespace scope and are then injected into
  namespace std".
- **(C) tuple.apply/make_from_tuple.pass, LWG 3528 part** (lines 248-261): `decltype(std::
  make_from_tuple<int*>(std::tuple<A*>&))` is expected to be a substitution failure. The draft
  ([tuple.apply]/3) puts `requires is_constructible_v<T, decltype(get<I>(declval<Tuple>()))...>`
  only on the exposition-only `make-from-tuple-impl`, called from the body ("Effects: ...
  Equivalent to: return make-from-tuple-impl<T>(...)"); `make_from_tuple` itself has no
  Constraints element (only "Mandates: If tuple_size_v<remove_reference_t<Tuple>> is 1, then
  reference_constructs_from_temporary_v<...> is false"), and its return type `T` needs no body
  instantiation, so the expression is well-formed in an unevaluated operand. libycxx diagnoses the
  call when it is instantiated; the SFINAE-friendly signature is a libc++ extension.
- **(C) views.span**: span.cons/copy.pass (`span<Incomplete>`; [span.overview]/4 "ElementType
  is required to be a complete object type that is not an abstract class type"), span.cons/
  span.pass and types.pass (`span<volatile std::string>::const_iterator` needs
  `input_iterator<volatile string*>`; COMMON-REF(`volatile string&&`, `const string&`) does not
  exist, because `const volatile string&` cannot bind the rvalue, and the fallback
  `common_reference_t` is `string`, to which `volatile string&&` does not convert, so
  `indirectly_readable` is false): unchanged, see (C) below.
- **(C)/(B) support.limits.general** (31 GCC, 32 Clang): every failing macro was compared with
  `draft.sh version.syn`. Each macro libycxx defines has the draft's value (e.g.
  `__cpp_lib_span 202311L`, `__cpp_lib_freestanding_algorithm 202502L`,
  `__cpp_lib_freestanding_cstring 202311L`, `__cpp_lib_freestanding_optional 202506L`,
  `__cpp_lib_debugging 202403L`, `__cpp_lib_to_chars 202606L`, `__cpp_lib_format 202603L`);
  libc++ 23 expects older values, or names no longer in the draft
  (`__cpp_lib_span_at`, merged into `__cpp_lib_span`; `__cpp_lib_generate_random`;
  `__cpp_lib_default_template_type_for_algorithm_values`) (C). Undefined because the feature is
  not implemented (B, must stay undefined): `__cpp_lib_senders`, `__cpp_lib_modules`; on Clang
  also the D builtins of type_traits.version. (`__cpp_lib_boyer_moore_searcher` was in this list
  until the searchers were merged the same day; it is now defined, 201603L.)
- **(D)** tuple.cnstr/PR31384 (GCC) and convert_const_move (Clang): compiler bugs listed in STATUS.

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
| containers/vector.bool.fmt/types.compile | `std::formatter` is not declared after `#include <vector>` | [vector.syn] declares `template<class T, class charT> requires is-vector-bool-reference<T> struct formatter<T, charT>;` | the partial specialization is defined with `<format>` (documented in STATUS as a choice) | Declare `formatter` (primary) and the partial specialization from `<vector>`; the definition can stay with the format core. **Fixed** (`<vector>` includes `format_vector_bool.hpp`; DECISIONS §11) |
| utilities/template.bitset/includes.pass | `std::string s;` after only `#include <bitset>`: incomplete type | [bitset.syn] begins `#include <string>` | `include/bitset`, `I/core/bitset.hpp:8` (deliberately does not include `<string>`) | Include `<string>` from `<bitset>` (both are core) **Fixed** (`<string>`), but the test still fails on the second half of [bitset.syn]: see the next row |
| utilities/template.bitset/includes.pass (re-run of 2026-10-05; **fixed**: `<bitset>` includes `<iosfwd>` when hosted, passes on both) | `std::ios`, `std::istream`, `std::ostream`, `std::iostream` are not declared after only `#include <bitset>` | [bitset.syn] begins `#include <string>` and `#include <iosfwd> // for istream, ostream, see [iosfwd.syn]` (checked with draft.sh bitset.syn) | `include/bitset:7` includes `<string>` only; `<iosfwd>` (`include/iosfwd`, hosted) is never reached | In hosted builds also `#include <iosfwd>` from `<bitset>` (`#if YCXX_HOSTED`, as `<cstdlib>`/`<cstring>` do; `<iosfwd>` is hosted, `<bitset>` core) |
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
| **Done** (see the filesystem/mdspan/valarray/searchers re-triage below): `boyer_moore_searcher`, `boyer_moore_horspool_searcher` (and `__cpp_lib_boyer_moore_searcher`) | [func.search.bm], [func.search.bmh] | func.search/func.search.bm/* (5), func.search.bmh/* (5): pass |
| C++ wrapper `<wchar.h>` (const-correct `wcschr`/`wcsstr`… as global names). (`<stdlib.h>`, `<complex.h>`, `<tgmath.h>`: **Done**, depr/depr.c.headers/{stdlib_h, complex_h, tgmath_h} pass) | [support.c.headers.other]/1 | Clang: depr/depr.c.headers/wchar_h.compile, strings/c.strings/cwchar_include_order{1,2}.compile.verify |
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
- Unqualified `int64_t`/`uint32_t`: 18 (mdspan layout_left/right/stride 16, deallocate_size,
  make_from_tuple). The mdspan tests do include `<cstdint>`; they rely on it declaring the global
  names too, which is unspecified (see the re-triage below).
- `std::count`/`std::min` without `<algorithm>`: 12 (valarray mask_array 11, ifstream
  offset_range).
- `std::istream`/`ostream`/`streambuf` from `<iterator>` alone: 4 (stream iterator types.pass).
- `std::unique_ptr` without `<memory>`: 1 (equality_comparable_with.compile).

## Re-triage: filesystem, mdspan, valarray and searchers (2026-10-05)

Directories: input.output/filesystems, containers/views/mdspan, numerics/numarray,
utilities/function.objects/func.search (`tools/run-conformance libcxx gcc|clang <dirs> -- -j4`).
Each remaining failure was also checked past its first failing assertion: the test was rebuilt
with the one cause removed (run as an unprivileged user, a prelude declaring `::int64_t`, an
added `#include <algorithm>`, or the offending assertion deleted) and the rest of it passes.

| | GCC 16.2 before | GCC after | Clang 23.1 before | Clang after |
|---|---|---|---|---|
| func.search | 10 failed | 0 | 10 | 0 |
| filesystems (fs.op.funcs 17, class.directory_entry 8, directory iterators 3) | 28 | 28 | 28 | 28 |
| mdspan | 16 | 16 | 17 | 17 |
| numarray (template.mask.array) | 11 | 11 | 11 | 11 |

- **(a) fixed**: `boyer_moore_searcher` and `boyer_moore_horspool_searcher` were missing
  (`ycxx/core/searcher.hpp`, `__cpp_lib_boyer_moore_searcher` 201603L). func.search/* (10) pass on
  both compilers; so do the own suite's functional/searchers_boyer_moore and libstdc++
  20_util/function_objects/searchers.cc.
- **Environment, running as root** (26 of the 28 filesystem tests: directory_entry
  cons/mods/obs, directory_iterator and recursive_directory_iterator ctor/increment, `exists`,
  the `is_*` predicates, `status`, `symlink_status`, remove, remove_all, bad_perms_parent,
  temp_directory_path): `perms::none` does not deny root access. Built by hand and run as uid
  65534, all 26 pass on both compilers, except the two below.
- **(b) directory_entry.cons/path.pass** (`path_ctor_cannot_resolve`): with the parent directory
  unreadable, the test expects `directory_entry(file, ec)` to keep the path and
  `directory_entry(file)` not to throw. [fs.dir.entry.cons]/2: "Postconditions: path() == p if no
  error occurs, otherwise path() == filesystem::path()"; /1 calls `refresh()`, and
  [fs.dir.entry.mods]/5: "If an error occurs, an error is reported ([fs.err.report])". EACCES from
  stat is an error, so libycxx clears the path and the throwing form throws (STATUS, known
  limitations). The rest of the test passes as uid 65534.
- **(b) fs.op.last_write_time/last_write_time.pass** (`test_write_min_time`): it sets
  `file_time_type::min()` (1677 in libycxx, representable as a `timespec`) and expects
  `value_too_large` because the file system clamps the time silently. [fs.op.last.write.time]/3:
  "Sets the time of last data modification of the file resolved to by p to new_time, as if by
  POSIX futimens", and its Note 1: "A postcondition of last_write_time(p) == new_time is not
  specified". `futimens` succeeds, so no error is reported. The rest passes as uid 65534.
- **(b) mdspan layout_left/right/stride ctor.default, ctor.extents(_array, _span),
  index_operator, required_span_size, stride** (16): the tests include `<cstdint>` and name
  `int64_t` unqualified. [headers]/5: "the declarations ... are within namespace scope of the
  namespace std. It is unspecified whether these names ... are first declared within the global
  namespace scope and are then injected into namespace std"; libycxx's `<cstdint>` declares only
  `std::int64_t`. With `using std::int64_t;` all 16 pass on both compilers. Clang also has
  extents/bitint.pass (`_BitInt` index types, D, already listed).
- **(b) template.mask.array** (11: mask.array.assign/valarray, mask.array.comp.assign/*): they call
  `std::count` with only `<valarray>` included. [res.on.headers]/1: "A C++ header may include
  other C++ headers": whether `<valarray>` provides `count` is unspecified. With `<algorithm>`
  included all 11 pass on both compilers.
- language.support/support.limits/{filesystem,mdspan}.version stay as listed (C: libc++ expects
  `__cpp_lib_format_path` 202403 and `__cpp_lib_submdspan` 202306; [version.syn] has 202506 and
  202603).

## Changes made in this round (no library code changed)

- `tests/ycxxlit/libcxx_format.py`: honour `test.config.unsupported` (from `lit.local.cfg`).
- `tests/libcxx/lit.cfg.py`: feature `libcpp-hardening-mode=none`.
- `tests/libcxx/skip.txt`: 62 entries covering 198 tests (category C) and 1 (E, the XPASS), each
  with its reason. None of them matches a test that passed in the raw runs.

## Re-triage: tests that need a named locale or file I/O (2026-10-05)

`ce0ef1b` made the harness run the tests that `REQUIRE` a `locale.<name>` feature when the C
library has that locale (`tests/ycxxlit/locales.py`; the seven names libc++ uses are generated by
`tools/ci/gen-locales`). Directories `localization input.output strings utilities/format time`,
`tools/run-conformance libcxx gcc|clang <dirs> -- -j4`. Failures among the newly enabled tests
(the known-failure baseline of the time told them apart):

| | GCC 16.2 before | GCC after | Clang 23.1 before | Clang after |
|---|---|---|---|---|
| localization | 70 | 70 | 70 | 70 |
| input.output | 18 | 18 | 18 | 18 |
| utilities/format | 1 | 1 | 1 | 1 |
| time | 49 | 48 | 50 | 48 |
| strings | 0 | 0 | 0 | 0 |
| **total** | **138** | **137** | **139** | **137** |

**Outcome (harness, `aee577b`):** a `locale.<name>` feature is now provided only when the C library
has the locale **and** libycxx's `std::locale` accepts the name (`tests/ycxxlit/locales.py` asks a
probe program built from the library under test). libycxx accepts none of the seven names, so all
137 tests below are reported UNSUPPORTED ("Test requires the following unavailable features:
locale.<name>"), and run again as soon as libycxx accepts the name. No test of these directories
that requires a named locale fails now. The classification below is what applies once they run.

Of the 137, **133 fail only because libycxx rejects the locale name** (`runtime_error`
"unsupported locale name" from `locale(const char*)` or a `_byname` constructor). The four others
are (b) below. Clang's two extra "before" failures (time.cal.ymd.members/ctor and ctor.sys_days:
diagnostics at nonsensical places in `charconv.hpp`) did not reproduce and pass in the "after" run.

- **(E) fixed in the harness**: the `%{LOCALE_CONV_<LOCALE>_<FIELD>}` substitutions of
  `ADDITIONAL_COMPILE_FLAGS` (fr_FR and ru_RU thousands separators and decimal point; 7 tests:
  money.get/put `*_fr_FR`/`*_ru_RU`, moneypunct.byname and numpunct.byname thousands_sep,
  time.duration.nonmember/ostream) reached the compiler unexpanded. `tests/ycxxlit/locales.py`
  now expands them to a wide string literal holding the C library's `localeconv()` field of that
  locale (`L"\U0000202F"` for fr_FR.UTF-8's `thousands_sep`), so the expectation is the C
  library's, not a guess. The 7 tests now compile and fail on the locale name.
- **(a) fixed**: time.syn/formatter.year_month_day, _day_last, _weekday, _weekday_last
  (`test_invalid_values`): `%j`, `%U`, `%W`, `%V`, `%G`, `%g` (and `%OU`/`%OV`/`%OW` with `L`)
  of a `year_month_day` that is not `ok()` printed numbers computed from the invalid date.
  [time.format]/3: "If the formatted object does not contain the information the conversion
  specifier refers to, an exception of type format_error is thrown"; an invalid date has no day
  of the year or week, so they now throw (`ycxx/hosted/chrono_io.hpp`; own test
  chrono/format_invalid_date). formatter.year_month_weekday_last now passes on GCC too (it passed
  on Clang before); the other three now fail only on `ja_JP.UTF-8`.
- **Design gap: named locales.** DECISIONS §7: the valid names are "C", "POSIX", "C.UTF-8" and
  ""; "any other name, and the `_byname` facets with such a name, throw `runtime_error`", and
  "no C-library locale is consulted". The draft allows that: [locale.cons]/4 "Remarks: The set of
  valid string argument values is "C", "", and any implementation-defined values", /3 "Throws:
  runtime_error if the argument is not valid". So these 133 are not conformance bugs, but they
  test nothing until libycxx's named locales are built on the C library's. Names used:
  en_US.UTF-8 (68 tests), fr_FR.UTF-8 (36), ja_JP.UTF-8 (16), ru_RU.UTF-8 (9), zh_CN.UTF-8 (2),
  fr_CA.ISO-8859-1 (2). Tests:
  - localization: locale.cons/{assign,char_pointer,copy,locale_char_pointer_cat,locale_facetptr,
    locale_locale_cat,locale_string_cat,name_construction,string}, locale.members/{encoding,
    name}, locale.operators/eq, locale.statics/global, locale.collate.byname/{compare,hash,
    transform,types}, locale.codecvt.byname/{ctor_char,ctor_wchar_t},
    locale.ctype.byname/{is_1,is_many,narrow_1,narrow_many,scan_is,scan_not,tolower_1,
    tolower_many,toupper_1,toupper_many,types,widen_1,widen_many},
    locale.money.get.members/get_{long_double_{en_US,fr_FR,ru_RU,zh_CN},string_en_US},
    locale.money.put.members/put_{long_double_{en_US,fr_FR,ru_RU,zh_CN},string_en_US},
    locale.moneypunct.byname/{curr_symbol,decimal_point,frac_digits,grouping,neg_format,
    pos_format,positive_sign,thousands_sep}, locale.time.get.byname/{date_order,get_date,
    get_monthname,get_one,get_time,get_weekday,get_year}{,_wide}, locale.time.put.byname/put1,
    locale.numpunct.byname/{decimal_point,grouping,thousands_sep};
  - input.output: ext.manip/{get,put}_{money,time}, ostream.formatted.print/locale-specific_form,
    ios.base.callback/register_callback, ios.base.locales/imbue, basic.ios.members/{copyfmt,
    imbue,move,swap}, streambuf.cons/{copy,default}, streambuf.locales/locales,
    streambuf.assign/{assign,swap};
  - utilities/format: format.functions/locale-specific_form;
  - time: the `ostream` tests of every time.cal type (15), of file/gps/local/sys (2)/tai/utc
    time, duration, hh_mm_ss and zoned_time, and time.syn/formatter.{day,duration,file_time,
    gps_time,hh_mm_ss,local_time,month,month_day,month_day_last,month_weekday,sys_info,sys_time,
    tai_time,utc_time,weekday,weekday_index,weekday_last,year,year_month,year_month_day,
    year_month_day_last,year_month_weekday,zoned_time}.
  Whether each passes once the name is accepted can only be known then; the expectations of the
  moneypunct/numpunct/time tests are the C library's (see the proposal in the report of this
  round: named locales and `_byname` facets on `newlocale`/`nl_langinfo_l`/`localeconv`).
- **(b) locale.moneypunct.byname/negative_sign** (the "C" part, before any named locale): it
  expects `moneypunct_byname<charT, Intl>("C").negative_sign()` to be empty (the C library's
  `localeconv()->negative_sign` in "C") while moneypunct.members/negative_sign expects "-" from
  the classic facet. [locale.statics]/4: `classic()` "Returns: A locale that implements the
  classic "C" locale semantics, equivalent to the value locale("C")", so the "C" byname facet has
  the classic facet's values in libycxx ("-"), and [locale.moneypunct.virtuals]/5 leaves the
  value to the implementation: "do_negative_sign() returns the string to use to indicate a
  negative value".
- **(b) locale.cons/default** (line 70, before the named locale): `globalMemCounter.
  checkOutstandingNewEq(0)` after `std::locale loc;` counts the 33 allocations libycxx's runtime
  makes for the classic locale's facets before `main`. The draft does not say where the classic
  locale's facets live; the test checks libc++'s choice of static storage.
- **(b) filebuf.virtuals/overflow** (line 51) and **underflow** (line 68): they check the
  buffer layout: after `overflow('a')` on a buffered filebuf `pptr() == pbase()` and
  `epptr() - pbase() == 4095`; after `pubsetbuf(0, 0)` a get area of exactly 8 characters with
  4 of put-back ("This test is not entirely portable", says the test). [filebuf.virtuals]/12:
  "If setbuf(0, 0) is called on a stream before any I/O has occurred on that stream, the stream
  becomes unbuffered. Otherwise the results are implementation-defined. "Unbuffered" means that
  pbase() and pptr() always return null and output to the file should appear as soon as
  possible"; nothing is said about the get area or the size of the buffer. Both use
  en_US.UTF-8 later as well.

## Named locales (2026-10-05)

libycxx's named locales are now the C library's (DECISIONS §7), so `tests/ycxxlit/locales.py`
provides `locale.<name>` for every name the C library has, and the 137 tests of the previous
section run. Directories `localization input.output time strings utilities/format`, both
compilers; 25 (GCC) / 23 (Clang) failed at first. Outcome:

- **Fixed in libycxx**: locale/locale.cons/name_construction (`locale(other, one, none)` kept
  other's name although `one` has none; [locale.cons]: named iff both are); the chrono `L`
  conversions now give `time_put` the value's zone (`tm_zone`, `tm_gmtoff`), so `%c` of a locale
  that shows `%Z` no longer writes the process's zone (libstdc++ pr117214 likewise).
- **Harness**: locale.codecvt.byname/ctor_char16_t, ctor_char32_t (and `_char8_t`) construct a
  facet with "en_US" without declaring it: `tools/ci/gen-locales` generates en_US, and the suite
  provides `missing-locale.en_US` where it is absent (`unsupported.txt`); they pass here.
- **Skipped** (skip.txt, block "Named locales", each with its reason and, where it applies, a
  check that the test passes with that part removed): filebuf.virtuals/overflow, underflow
  (buffer layout); ext.manip/get_time (white space before `%a`; glibc's strptime agrees with
  libycxx); ostream.formatted.print and format.functions locale-specific_form (P3505 to_chars
  ranges, shortest long double); locale.ctype.byname/widen_1, widen_many (the "C" part's
  glibc btowc); money.get/put `*_fr_FR`, `*_ru_RU`, moneypunct.byname curr_symbol, neg_format,
  pos_format (libc++'s money patterns: space moved into the symbol); moneypunct.byname/
  negative_sign (the "C" part, see above); time.get.byname/get_date, get_date_wide (zh_CN without
  separators; libc++'s stop position); time.syn/formatter.duration (hours of a duration reduced
  modulo 24; shortest long double), formatter.year (LWG 4022 "-01"; glibc's `%EC` of year 0),
  formatter.weekday, weekday_index, weekday_last (glibc strftime's `%u` of weekday(8); libycxx
  throws, as libc++'s year_month_weekday invalid-value tests require).
- No XFAIL was added: none of these is a compiler bug.

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

<!-- counterparts:begin (generated) -->
## Skipped tests without a counterpart

Tests skipped (or UNSUPPORTED) as tied to the other library's internals, extensions or modes whose subject the draft does not specify, so libycxx's own suite has no test for it: the trace reports them as "no libycxx counterpart". Patterns are anchored regexes (like skip.txt); the linked ones carry `// COUNTERPART:` in tests/ycxx (tests/ycxxlit/counterparts.py).

| tests | why there is no standard counterpart |
|---|---|
| `experimental/iterator/.*` | ostream_joiner (Library Fundamentals TS v2) is not in the draft |
| `experimental/memory/.*` | observer_ptr (Library Fundamentals TS v2) is not in the draft |
| `experimental/utilities/propagate_const/.*` | propagate_const (Library Fundamentals TS v2) is not in the draft |
| `experimental/utilities/meta/.*` | the detection idiom (is_detected, nonesuch; Library Fundamentals TS v2) is not in the draft |
| `experimental/utilities/utility/.*` | erased_type (Library Fundamentals TS) is not in the draft |
| `experimental/simd/simd.reference/.*` | the Parallelism TS v2 simd reference proxy; the draft's basic_vec::operator[] returns a value ([simd.subscr]) |
| `experimental/simd/simd.traits/is_(abi_tag\|simd\|simd_flag_type\|simd_mask).pass.cpp` | the Parallelism TS v2 traits is_abi_tag, is_simd, is_simd_mask, is_simd_flag_type are not in the draft's [simd] |
| `algorithms/alg.sorting/alg.min.max/requires_forward_iterator.verify.cpp` | min_element etc. with an input iterator violates a template-parameter requirement ([algorithms.requirements]/4): undefined, no diagnostic required; the test checks libc++'s diagnostic text |
| `containers/sequences/vector/vector.modifiers/resize_not_move_insertable.verify.cpp` | a Cpp17MoveInsertable precondition violation is undefined behaviour ([res.on.required]); the test checks libc++'s static_assert text |
| `numerics/rand/rand.dist/rand.dist.uni/rand.dist.uni.int/int128.pass.cpp` | __int128 is not in the IntType / UIntType sets of [rand.req.genl]/1.5-1.6 (an implementation may add extended types) |
| `ranges/range.adaptors/range.lazy.split/range.lazy.split.outer.value/ctor.default.pass.cpp` | [range.lazy.split.outer.value] declares no default constructor (only the exposition-only one from outer-iterator) |
| `localization/locale.stdcvt/.*` | <codecvt> (codecvt_utf8, codecvt_utf16, codecvt_utf8_utf16, codecvt_mode) was removed in C++26 (P2871) |
| `localization/locales/locale.convenience/.*` | wstring_convert and wbuffer_convert were removed in C++26 (P2872) |
| `depr/depr.str.strstreams/.*` | <strstream> was removed in C++26 (P2867) |
| `utilities/memory/util.smartptr/(util.smartptr.shared.atomic/.*\|util.smartptr.shared/util.smartptr.shared.obs/unique.pass.cpp)` | the atomic_* free functions for shared_ptr were removed in C++26 (P2869), shared_ptr::unique() in C++20 (P0521) |
| `utilities/meta/meta.trans/meta.trans.other/result_of.*` | result_of was removed in C++20 (P0619) |
| `utilities/meta/meta.unary/meta.unary.prop/is_literal_type.*` | is_literal_type was removed in C++20 (P0619) |
| `utilities/memory/storage.iterator/.*` | raw_storage_iterator was removed in C++20 (P0619) |
| `utilities/memory/temporary.buffer/.*` | get_temporary_buffer / return_temporary_buffer were removed in C++20 (P0619) |
| `utilities/function.objects/negators/.*` | not1, not2, unary_negate and binary_negate were removed in C++20 (P0619) |
| `language.support/support.runtime/cstd(align\|bool).*` | <cstdalign> and <cstdbool> were removed in C++20 (P0619) |
| `depr/depr.c.headers/ciso646.compile.pass.cpp` | <ciso646> was removed in C++20 (P0619) |
| `numerics/c.math/ctgmath.pass.cpp` | <ctgmath> was removed in C++20 (P0619) |
| `numerics/complex.number/ccmplx/.*` | <ccomplex> was removed in C++20 (P0619) |
| `language.support/support.exception/uncaught/.*` | uncaught_exception() was removed in C++20 (P0619) |
| `language.support/support.initlist/support.initlist.range/.*` | the free begin / end for initializer_list were removed from [initializer.list.syn] (P3016; the <iterator> ones apply) |
| `strings/basic.string/string.capacity/reserve.pass.cpp` | basic_string::reserve() without an argument was removed in C++26 (P2870) |
| `thread/futures/futures.promise/uses_allocator.pass.cpp` | uses_allocator<promise<R>, Alloc> is no longer in [futures.promise] (P2875) |
| `containers/.*/empty(.nodiscard)?.verify.cpp` | [[nodiscard]] on empty(): the draft marks no library function [[nodiscard]] (P2422R1 removed them); a warning is only recommended practice ([dcl.attr.nodiscard]/4) |
| `(input.output/filesystems/class.path/path.member/path.decompose\|iterators/iterator.container\|re/re.results/re.results.size\|strings/basic.string/string.capacity\|strings/string.view/string.view.capacity)/empty.*.verify.cpp` | [[nodiscard]] on empty(): the draft marks no library function [[nodiscard]] (P2422R1); warnings are QoI |
| `.*nodiscard.*.verify.cpp` | [[nodiscard]]: the draft marks no library function [[nodiscard]] (P2422R1); warnings are QoI ([dcl.attr.nodiscard]/4) |
| `language.support/support.dynamic/new.delete/new.delete.placement/new(_array)?_ptr.verify.cpp` | [[nodiscard]] on placement operator new: not in [new.syn] (P2422R1); warnings are QoI |
| `utilities/allocator.adaptor/allocator.adaptor.members/allocate_size(_hint)?.verify.cpp` | [[nodiscard]] on scoped_allocator_adaptor::allocate: not in [allocator.adaptor.syn] (P2422R1); warnings are QoI |
| `depr/depr.cpp.headers/c(complex\|iso646\|stdalign\|stdbool\|tgmath).verify.cpp` | <ccomplex>, <ciso646>, <cstdalign>, <cstdbool> and <ctgmath> were removed in C++20 (P0619) |
| `depr/depr.lib.binders/.*` | bind1st, bind2nd, binder1st and binder2nd were removed in C++17 (N4190) |
| `utilities/memory/util.smartptr/util.smartptr.shared/util.smartptr.shared.obs/unique.deprecated_in_cxx17.verify.cpp` | shared_ptr::unique() was removed in C++20 (P0521) |
| `utilities/tuple/tuple.tuple/tuple.cnstr/default.lazy.verify.cpp` | whether a nested class's default member initializer makes it default-constructible inside the incomplete enclosing class is a core-language question (CWG 1397, 2335): GCC and Clang reject it with libstdc++ as well |
| `algorithms/alg.sorting/alg.clamp/assert.ranges_clamp.pass.cpp` | the ordering of lo and hi is a precondition of ranges::clamp, not a hardened one ([alg.clamp]/2): violating it is undefined |
| `algorithms/alg.sorting/alg.heap.operations/pop.heap/assert.(ranges_)?pop_heap.pass.cpp` | a non-empty heap is a precondition of pop_heap, not a hardened one ([pop.heap]/2) |
| `input.output/file.streams/fstreams/[a-z]+.members/native_handle.assert.pass.cpp` | is_open() is a precondition of native_handle(), not a hardened one ([filebuf.members]) |
| `input.output/stream.buffers/streambuf/streambuf.protected/streambuf.(get\|put).area/set[gp].assert.pass.cpp` | valid ranges are preconditions of setg / setp, not hardened ones ([streambuf.get.area], [streambuf.put.area]) |
| `numerics/numeric.ops/numeric.ops.sat/saturating_div.assert.pass.cpp` | y != 0 is a precondition of saturating_div, not a hardened one ([numeric.sat.div]) |
| `ranges/range.factories/range.iota.view/assert.ctor.value.bound.pass.cpp` | bound reachable from value is a precondition of iota_view's constructor, not a hardened one ([range.iota.view]) |
| `utilities/smartptr/unique.ptr/unique.ptr.class/unique.ptr.observers/assert.subscript.pass.cpp` | the index bound is a precondition of unique_ptr<T[]>::operator[], not a hardened one ([unique.ptr.runtime.observers]) |
| `utilities/utility/utility.unreachable/assert.unreachable.pass.cpp` | calling unreachable() is undefined ([utility.undefined]/1), not a hardened precondition |

<!-- counterparts:end -->
