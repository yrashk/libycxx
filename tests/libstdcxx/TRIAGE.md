# libstdc++ testsuite: full-suite triage

Run of 2026-10-04 against commit `12ff176` (after `ninja -C build/gcc && ninja -C build/clang`):
`tools/run-conformance libstdcxx gcc|clang -- -j8 -sv` over the whole GCC 16.2 testsuite
(`/opt/src/libstdcxx-testsuite`, the directories `lit.cfg.py` does not exclude). Every failure was
read (compiler diagnostics and test source) and put in one category:

| Cat. | Meaning |
|---|---|
| **A** | libycxx bug: the test is valid for the current draft and libycxx is wrong |
| A-QoI | valid test that fails because the draft's constraints, implemented literally, recurse; libstdc++ fixed the same reports (PR number given). Worth fixing, lower priority |
| A-doc | valid test hitting a deviation DECISIONS.md already documents (plain template-parameter names vs. a user macro `C`) |
| **B** | libycxx missing feature |
| **C** | test relies on libstdc++ internals/extensions, implementation choices the draft leaves open, pre-C++26 rules, deprecated or removed features |
| **D** | compiler problem or compiler difference |
| **E** | harness / environment issue |
| **F** | test uses a name without including the header that declares it |

## Totals

| | Tests | Pass | Fail | Unsupported |
|---|---:|---:|---:|---:|
| GCC 16.2 | 8555 | 4754 | 484 | 3317 |
| Clang 23.1 | 8555 | 4715 | 523 | 3317 |

The skip entries added by this triage (end of `skip.txt`, see the last section) turn 302 of
those failures into UNSUPPORTED on each compiler (all category C; no passing test is affected;
checked by re-running the 302 tests). Expected totals on the next run:
GCC 4754 pass / **182** fail / 3619 unsupported, Clang 4715 / **221** / 3619.

## Re-run of 2026-10-05 (after the (A) fixes and the harness fixes)

Whole testsuite again on both compilers (`tools/run-conformance libstdcxx gcc|clang -- -j8 -sv`,
library at `7e6a93f` (no library change after it), harness as fixed below). Every failure was compared with the per-test
categories of the first run: **no new failure** on either compiler (no test that passed or was
unsupported before fails now, including after `_GLIBCXX_USE_CXX11_ABI=1`).

| | Tests | Pass | Fail | Unsupported |
|---|---:|---:|---:|---:|
| GCC 16.2, first run | 8555 | 4754 | 484 | 3317 |
| GCC 16.2, re-run | 8555 | 4823 | 141 | 3591 |
| GCC 16.2, with this round's skip entry | 8555 | 4823 | **139** | 3593 |
| Clang 23.1, first run | 8555 | 4715 | 523 | 3317 |
| Clang 23.1, re-run | 8555 | 4786 | 175 | 3594 |
| Clang 23.1, with this round's skip entry | 8555 | 4786 | **173** | 3596 |

(The expected totals of the first run, 182 / 221 failures, are reached and passed: the 302 C skips,
the 21 + 3 (A) fixes, 11 (GCC) / 18 (Clang) harness fixes, Clang's optional/constexpr/124910.cc
workaround, and on GCC the C++ `<stdckdint.h>` and an F test (20_util/stdbit/1.cc) that now passes.)

- **(A)**: all 21 (A) and 3 A-QoI tests pass on both compilers. The only (A)-table test still
  failing is 20_util/optional/relops/constrained.cc on Clang, a Clang 23 problem (D, see the
  outcome column). The six A-doc tests (macro `C`) still fail, as documented.
- **(E) fixed in the harness** (each fix commented in the code), all rerun:
  - `tests/ycxxlit/libstdcxx_format.py`: the `testsuite/data` files a test names are copied into
    its run directory (20_util/hash/chi2_q_document_words.cc and the basic_fstream/basic_ifstream
    native_handle tests pass); `-g` for tests that include `<stacktrace>` (DejaGnu's default flags
    are `-g -O2`; stacktrace/{entry,output}.cc pass, and current.cc on GCC); GCC-only optimiser
    options in `dg-options` are dropped for Clang (`-fno-assume-sane-operators-new-delete`,
    `-fvtable-verify=`, `-fdump-tree-`: 17_intro/freestanding.cc, 18_support/50594.cc,
    vector/bool/capacity/{110498,114758}.cc pass), and a test that needs `-fcontracts` /
    `-fcontract-evaluation-semantic=` is UNSUPPORTED on Clang (Clang 23 has no contracts; the 3
    18_support/contracts tests pass on GCC); `dg-xfail-run-if` is honoured with its
    include/exclude option lists (bit_ceil_neg.cc passes; the basic_string_view element_access
    tests with the same directive still pass); a `dg-error` marked `{ xfail SEL }` no longer
    makes the test a compile-fail test (27_io/fpos/mbstate_t/4_neg.cc passes).
  - `tests/libstdcxx/lit.cfg.py`: `-D_GLIBCXX_USE_CXX11_ABI=1` (27_io/ios_base/failure/error_code.cc passes).
  - `tests/libstdcxx/shim/bits/stdexcept_throw.h`: aborts instead of throwing under
    `-fno-exceptions` (18_support/exception_ptr/64241.cc passes).
  - Still E: 27_io/objects/wchar_t/13582-1_xin.cc (the `en_US.ISO8859-1` locale is not installed).
- **Reclassified E -> C (skipped)**: 27_io/basic_filebuf/native_handle/{char,wchar_t}/1.cc. With the
  data file present the test closes the native handle behind the filebuf and expects `sgetc()` to
  throw `ios_base::failure`; [filebuf.virtuals] `underflow` reports failure by returning `eof()`,
  which libycxx does (libstdc++ throws on a read error).
- `dg-xfail-if` (compile-time xfail) is still not interpreted: 20_util/specialized_algorithms/destroy/121024.cc
  (GCC PR c++/102284, D) still fails on GCC.

Failures of the re-run per category and top-level directory (with the new skip entry):

**GCC**

| Directory | A | A-QoI | A-doc | B | C | D | E | F | Total |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 18_support |  |  |  | 1 | 1 |  |  | 1 | 3 |
| 19_diagnostics |  |  |  | 1 |  |  |  |  | 1 |
| 20_util |  |  | 2 | 2 | 11 | 1 |  | 7 | 23 |
| 21_strings |  |  |  |  | 4 |  |  | 3 | 7 |
| 22_locale |  |  |  |  | 3 |  |  | 11 | 14 |
| 23_containers |  |  |  |  | 12 |  |  | 12 | 24 |
| 24_iterators |  |  |  |  | 2 |  |  |  | 2 |
| 25_algorithms |  |  |  |  | 1 |  |  | 2 | 3 |
| 26_numerics |  |  |  | 4 | 1 |  |  | 27 | 32 |
| 27_io |  |  | 4 |  | 6 |  | 1 | 10 | 21 |
| 30_threads |  |  |  |  | 2 |  |  |  | 2 |
| std/format |  |  |  |  | 1 |  |  | 2 | 3 |
| std/ranges |  |  |  |  |  |  |  | 1 | 1 |
| std/time |  |  |  |  |  |  |  | 3 | 3 |
| **Total** |  |  | **6** | **8** | **44** | **1** | **1** | **79** | **139** |

**Clang**

| Directory | A | A-QoI | A-doc | B | C | D | E | F | Total |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 18_support |  |  |  | 1 | 1 | 2 |  | 1 | 5 |
| 19_diagnostics |  |  |  | 1 |  | 3 |  |  | 4 |
| 20_util |  |  | 2 | 2 | 11 | 11 |  | 8 | 34 |
| 21_strings |  |  |  |  | 4 |  |  | 3 | 7 |
| 22_locale |  |  |  |  | 3 |  |  | 11 | 14 |
| 23_containers |  |  |  |  | 19 | 8 |  | 12 | 39 |
| 24_iterators |  |  |  |  | 3 |  |  |  | 3 |
| 25_algorithms |  |  |  |  | 1 |  |  | 2 | 3 |
| 26_numerics |  |  |  | 4 | 1 |  |  | 27 | 32 |
| 27_io |  |  | 4 |  | 7 |  | 1 | 9 | 21 |
| 29_atomics |  |  |  |  |  | 1 |  |  | 1 |
| 30_threads |  |  |  |  | 2 |  |  |  | 2 |
| std/format |  |  |  |  | 1 | 1 |  | 2 | 4 |
| std/ranges |  |  |  |  |  |  |  | 1 | 1 |
| std/time |  |  |  |  |  |  |  | 3 | 3 |
| **Total** |  |  | **6** | **8** | **53** | **26** | **1** | **79** | **173** |

The C, D, F entries are the first run's, unchanged; the sections below describe the first run,
with outcomes added.

## (A) libycxx bugs

15 distinct bugs, 21 failing tests (identical on both compilers). Grouped by header.
Outcome column: after the fixes on branch `worktree-agent-ae7534c886046a82a` (2026-10-04); all
21 + 3 tests pass on GCC, all but optional/relops/constrained.cc on Clang (a Clang problem, see (D)).

| Header | Test(s) | Symptom | Draft | libycxx location | Suggested fix | Outcome |
|---|---|---|---|---|---|---|
| `<compare>` | 18_support/comparisons/algorithms/{strong_order,weak_order,partial_order}.cc | `noexcept(std::strong_order(1, 1))` is false (same for weak/partial, also on floating point) | [cmp.alg]/1-3: each CPO call is *expression-equivalent* to the selected expression, so it carries that expression's noexcept | `include/ycxx/core/compare_alg.hpp:97,115,137` (also the three `compare_*_order_fallback` at :161,173,189) | give each `operator()` a `noexcept(...)` matching the branch `if constexpr` selects (noexcept(true) for the floating-point branches) | Fixed (`compare_alg.hpp`: each CPO picks its branch in a consteval `choose()` and carries that expression's noexcept). Pass, both |
| `<compare>` | 18_support/comparisons/object/lwg3530.cc | `compare_three_way{}(long*, X)` is callable for an `X` that is not `three_way_comparable_with<long*>` (ambiguous conversions) | [comparisons.three.way]/1 (LWG 3530): Constraints: `T` and `U` satisfy `three_way_comparable_with` -- nothing else | `include/ycxx/core/compare.hpp:302` (`requires three_way_comparable_with<T, U> \|\| builtin_ptr_three_way<T, U>`) | drop the `\|\| builtin_ptr_three_way` disjunct; keep the pointer branch only inside the body | Fixed (constraint is `three_way_comparable_with` alone; a reversed user `operator<=>` now also rules out the pointer branch). Pass, both |
| `<functional>` | 20_util/function_objects/comparisons_pointer_spaceship.cc | `std::less<>{}(cs1, cs2)` compares as pointers although `cs1 < cs2` uses the class's `operator<=>` (rewritten candidate); same for greater/less_equal/greater_equal and the ranges:: objects | [comparisons.general]/2: the total pointer order applies only "if the call operator calls a built-in operator comparing pointers" | `include/ycxx/core/functional_base.hpp:22-32` (`builtin_ptr_less`/`builtin_ptr_eq` only look for `operator<`/`operator==`, not for `operator<=>` member/non-member candidates) | also exclude the case where a (member or ADL) `operator<=>` (and for `==`, a reversed `operator==`) is viable, e.g. test that the expression is *not* resolved to a built-in by checking `operator<=>(t, u)` / `t.operator<=>(u)` too | Fixed (`functional_base.hpp`: per-operator user-candidate checks plus `operator<=>` in either order; reversed `operator==` for `ranges::equal_to`; class operands still examined only after the non-class test, so libc++ robust_against_adl passes). Pass, both |
| `<functional>` | 20_util/function_objects/constexpr_searcher.cc, 20_util/function_objects/invoke/constexpr.cc | `__cpp_lib_constexpr_functional` undefined | [version.syn]: `__cpp_lib_constexpr_functional 201907L` (freestanding, also in `<functional>`) | `include/ycxx/core/version.hpp` | define it (P1032/P1065 constexpr invoke, function objects and searchers; the tests' constexpr uses compile once the macro is defined -- verify default_searcher) | Fixed (macro defined; constexpr invoke/bind/not_fn/mem_fn/searchers verified). Pass, both |
| `<tuple>` | 20_util/tuple/apply/1.cc | `__cpp_lib_apply` undefined | [version.syn]: `__cpp_lib_apply 202603L` (also in `<tuple>`) | `include/ycxx/core/version.hpp` (next to `__cpp_lib_tuples_by_type`, l.33) | define it (apply is `noexcept(is_nothrow_applicable_v)`, i.e. the 202603 version, `tuple.hpp:725`) | Fixed (macro defined). Pass, both |
| `<tuple>` | 20_util/tuple/tuple_like_ftm.cc | `__cpp_lib_tuple_like` undefined although P2165 is implemented (pair from array, tuple == pair, tuple_cat of tuple-likes, map::insert(tuple) all compile) | [version.syn]: `__cpp_lib_tuple_like 202311L` | `include/ycxx/core/version.hpp` | define it | Fixed (macro defined; complex tuple protocol checked too). Pass, both |
| `<tuple>` | 20_util/tuple/cons/121771.cc | `std::tuple t(func);` (function lvalue) is a hard error: instantiates `tuple_leaf<0, void()>` | [tuple.cnstr]/9 + [over.match.class.deduct]: the implicit guide from `tuple(const Types&...)` has Types = `void()`; its constraints (sizeof...(Types) >= 1, is_copy_constructible) are false, so the `tuple(UTypes...)` guide deduces `tuple<void(*)()>` | `include/ycxx/core/tuple.hpp:239` (`requires(N >= 1)` names the static member `N`, which instantiates `tuple<void()>`; the same `N` is used by other constructors) | write the constraints with `sizeof...(Types)` instead of the class member `N` | Fixed (`N` removed, `sizeof...(Types)` everywhere). Pass, both |
| `<utility>` | 20_util/headers/utility/ignore.cc | `std::ignore` not declared by `<utility>` | [tuple.general]/2 (P2968): "ignore ([tuple.syn]) is available when `<utility>` is included" | `include/utility` (ignore is defined in `include/ycxx/core/tuple.hpp:654-664`) | move `ignore_type`/`ignore` to a small header both `<utility>` and `<tuple>` include | Fixed (`ycxx/core/ignore.hpp`). Pass, both |
| `<bitset>` | 20_util/bitset/121054.cc, access/dr396.cc, access/to_string.cc, cons/dr396.cc, cons/dr1325-2.cc | `std::string`/`basic_string` incomplete or without default template arguments after `#include <bitset>` | [bitset.syn]: `#include <string>` | `include/bitset` (includes only `ycxx/core/bitset.hpp`) | `#include <string>` in `<bitset>` (`<string>` is core, so no layering issue) | Fixed. All five pass, both |
| `<optional>` | 20_util/optional/cons/lwg3886.cc | `optional<const Tracker> o; o = {0,0};` ambiguous `operator=`: the engaged branch assigns to the non-const `stored` object, so the deleted non-const copy assignment competes | [optional.assign] `operator=(U&&)`: "assigns `std::forward<U>(v)` to `*val`" where `*val` is a `const T` lvalue | `include/ycxx/core/optional.hpp:251` (`u_.val = ...` with `stored = remove_cv_t<T>`, l.106) | assign through `static_cast<T&>(u_.val)` (an lvalue of type `T`, i.e. const-qualified) | Fixed (also the converting/copy assignments and swap go through `T&`). Pass, both |
| `<optional>` | 20_util/optional/ref/access.cc | `requires { o.value_or(t); }` for `optional<NonMovable&>` is a hard error (the Mandates static_assert fires) instead of `true` | [optional.ref.observe]/8-11: `template<class U = remove_cv_t<T>> constexpr remove_cv_t<T> value_or(U&& u) const;` -- declared return type; Mandates are checked only when the body is used | `include/ycxx/core/optional.hpp:613` (`constexpr auto value_or`) | declare the return type `remove_cv_t<T>` (the constraint already restricts T to non-array object types) | Fixed: return type `remove_cv_t<T>`, named through a class template that depends on `U` (a plain `remove_cv_t<T>` is formed at class instantiation, a hard error for `optional<int()&>`). Pass, both |
| `<optional>` | 20_util/optional/relops/constrained.cc | `optional<D> != optional<D>` (and `*t != u`, `t != *u`) compile for `D` with a deleted `operator!=`; same for `K` (non-bool `==`) and `optional<D> == optional<C>` through the reversed `==` | [optional.relops], [optional.comp.with.t] (P2944): `x != y` is constrained on `*x != *y`; the draft declares `operator!=` with the same signature as `operator==`, so by [over.match.oper]/4 the `operator==` templates are not rewrite targets | `include/ycxx/core/optional.hpp:653-760`: the `==`/`!=` templates have different requires-clauses, so they do not *correspond* and `!(x == y)` / reversed `y == x` become candidates | make each `operator!=` correspond to its `operator==` (identical template-heads and parameter lists; express the differing Constraints elsewhere, e.g. return-type SFINAE), so `==` is no rewrite target. Medium confidence on the mechanism; GCC 16's libstdc++ rejects all these expressions (checked with a reduced test) | Fixed per the draft: each `!=` now corresponds to its `==` (same template-head, parameters, return type); the Constraints moved to default template arguments. Pass on GCC. **Clang 23 still fails** (D): it forms `!(x == y)` and the reversed `y == x` from a template `operator==` even when a corresponding `operator!=` template exists (reduced test: differing default template arguments or return-type SFINAE; GCC honours [over.match.oper]/4 for both) |
| `<optional>` | 20_util/optional/range.cc | `formattable<optional<int>, char>` is true and `format_kind<optional<int>>` is not `disabled` | [optional.syn]: `template<class T> constexpr auto format_kind<optional<T>> = range_format::disabled;` | missing; `include/ycxx/core/format_ranges.hpp:78-81` defines `format_kind` | add the partial specialization (in `format_ranges.hpp`, or in `optional.hpp` with a forward declaration of `format_kind`/`range_format`) | Fixed: `range_format`/`format_kind` moved to `ycxx/core/format_kind.hpp`, which `<optional>` includes (so `std::format_kind` is usable after `#include <optional>` alone); `optional.hpp` declares the draft's partial specialization `format_kind<optional<T>>`. Pass, both |
| `<any>` | 20_util/any/misc/any_cast.cc | `any_cast<noncopyable>(&a)` fails to compile: instantiates `any_impl::ops<noncopyable>::create` (copy) | [any.nonmembers]/9-10: the pointer forms only Mandate `!is_void_v<T>`; a non-copyable `T` simply yields `nullptr` | `include/ycxx/hosted/any.hpp:132` (`holds<T>()` compares with `&table_for<T>`, l.92-96, which odr-uses `ops<T>::copy` -> `create`, l.60-67) | in `any_cast`/`holds`, return `false`/`nullptr` without naming `table_for<T>` when `T` is not copy-constructible (such a T can never be stored) | Fixed (`holds<T>()` is false for a non-copy-constructible T without naming `table_for<T>`). Pass, both |

A-QoI (valid tests; the draft's constraints recurse when checked literally; fixed in libstdc++):

| Test | Symptom | libycxx location | Suggested fix | Outcome |
|---|---|---|---|---|
| 20_util/optional/cons/117858.cc (PR 117858/117889) | `f = f;` / `optional<Focus> f2 = f;` for `struct Focus { template<class T> Focus(T); }`: "satisfaction of `is_constructible_v<Focus, optional<Focus>&>` depends on itself" | `include/ycxx/core/optional.hpp:175-190` (converting constructors/assignments evaluate `converts_from_any_cvref<T, optional<U>>` for `U == T`) | short-circuit the converting overloads with `!is_same_v<remove_cv_t<U>, remove_cv_t<T>>` first (they are never better than the copy/move members for U == T) | Fixed with `!is_same_v<U, T>` (exact, not cv-stripped: `optional<const X>` -> `optional<X>` must stay) first in the four converting members; the `U&&` constructor tests the `in_place_t`/`optional` exclusions before `is_constructible_v`. Pass, both |
| 20_util/optional/relops/104606.cc (PR 104606) | `c <= o` with `optional<Value>`, `struct Value : variant<vector<Value>>`: `three_way_comparable_with<Value, Comparator>` recurses through `vector<Value>`'s synth-three-way | `include/ycxx/core/optional.hpp` (`operator<=>(const optional<T>&, const U&)`), `compare.hpp:206` | order the constraint so that a cheap, non-recursive check on `U` fails first, or defer `three_way_comparable_with<T, U>` until `U` is known to be comparable at all | Fixed: `three_way_comparable<U>` (a part of `three_way_comparable_with<T, U>`) is tested first. Pass, both |
| 24_iterators/const_iterator/112490.cc (PR 112490) | `totally_ordered<reverse_iterator<basic_const_iterator<vector<int>::iterator>>>` recurses | `include/ycxx/core/iterator_adaptors.hpp:493-505` (the `not_a_const_iterator I` comparison friends of `basic_const_iterator`) | break the cycle, e.g. exclude `I` that are iterator adaptors over a `basic_const_iterator` before checking `totally_ordered_with<Iterator, I>` | Fixed: the comparisons with another type `I` first test `it < i` (a part of `totally_ordered_with<Iter, I>`), which fails for `I` = `reverse_iterator<basic_const_iterator<...>>` without asking about `I < I`. Pass, both |

A-doc (documented deviation, DECISIONS §2: template parameters use plain names): the tests
`#define C char`/`wchar_t` before including headers, and `ycxx/core/invoke.hpp:21` (and other
headers) use `C` as a template-parameter name: 20_util/bitset/cons/string_view{,_wide}.cc,
27_io/{basic_istringstream,basic_ostringstream,basic_stringbuf,basic_stringstream}/cons/wchar_t/string_view.cc.
[macro.names] allows these macros, so these are real (known) non-conformances; renaming the
`C` template parameters would fix all six.

## Category counts per top-level directory

**GCC** (failures of the run; "now skipped" = reported UNSUPPORTED with the new skip entries)

| Directory | A | A-QoI | A-doc | B | C | D | E | F | Total |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 17_intro |  |  |  |  | 4 |  |  |  | 4 |
| 18_support | 4 |  |  | 1 | 13 |  | 1 | 1 | 20 |
| 19_diagnostics |  |  |  | 1 | 1 |  | 2 |  | 4 |
| 20_util | 17 | 2 | 2 | 2 | 88 | 1 | 1 | 8 | 121 |
| 21_strings |  |  |  |  | 13 |  |  | 3 | 16 |
| 22_locale |  |  |  |  | 42 |  |  | 11 | 53 |
| 23_containers |  |  |  |  | 60 |  |  | 12 | 72 |
| 24_iterators |  | 1 |  |  | 17 |  |  |  | 18 |
| 25_algorithms |  |  |  |  | 15 |  |  | 2 | 17 |
| 26_numerics |  |  |  | 6 | 30 |  | 1 | 27 | 64 |
| 27_io |  |  | 4 |  | 23 |  | 9 | 10 | 46 |
| 28_regex |  |  |  |  | 1 |  |  |  | 1 |
| 30_threads |  |  |  |  | 2 |  |  |  | 2 |
| special_functions |  |  |  |  | 21 |  |  |  | 21 |
| std/format |  |  |  |  | 3 |  |  | 2 | 5 |
| std/memory |  |  |  |  | 1 |  |  |  | 1 |
| std/ranges |  |  |  |  | 5 |  |  | 1 | 6 |
| std/text_encoding |  |  |  |  | 1 |  |  |  | 1 |
| std/time |  |  |  |  | 9 |  |  | 3 | 12 |
| **Total** | **21** | **3** | **6** | **10** | **349** | **1** | **14** | **80** | **484** |
| now skipped |  |  |  |  | 302 |  |  |  | 302 |
| still failing | 21 | 3 | 6 | 10 | 47 | 1 | 14 | 80 | 182 |

**Clang** (failures of the run; "now skipped" = reported UNSUPPORTED with the new skip entries)

| Directory | A | A-QoI | A-doc | B | C | D | E | F | Total |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 17_intro |  |  |  |  | 4 |  | 1 |  | 5 |
| 18_support | 4 |  |  | 1 | 13 | 2 | 5 | 1 | 26 |
| 19_diagnostics |  |  |  | 1 | 1 | 3 | 2 |  | 7 |
| 20_util | 17 | 2 | 2 | 2 | 88 | 11 | 1 | 8 | 131 |
| 21_strings |  |  |  |  | 13 |  |  | 3 | 16 |
| 22_locale |  |  |  |  | 42 |  |  | 11 | 53 |
| 23_containers |  |  |  |  | 68 | 8 | 2 | 12 | 90 |
| 24_iterators |  | 1 |  |  | 18 |  |  |  | 19 |
| 25_algorithms |  |  |  |  | 15 |  |  | 2 | 17 |
| 26_numerics |  |  |  | 4 | 30 |  | 1 | 27 | 62 |
| 27_io |  |  | 4 |  | 24 |  | 9 | 9 | 46 |
| 28_regex |  |  |  |  | 1 |  |  |  | 1 |
| 29_atomics |  |  |  |  |  | 1 |  |  | 1 |
| 30_threads |  |  |  |  | 2 |  |  |  | 2 |
| special_functions |  |  |  |  | 21 |  |  |  | 21 |
| std/format |  |  |  |  | 3 | 1 |  | 2 | 6 |
| std/memory |  |  |  |  | 1 |  |  |  | 1 |
| std/ranges |  |  |  |  | 5 |  |  | 1 | 6 |
| std/text_encoding |  |  |  |  | 1 |  |  |  | 1 |
| std/time |  |  |  |  | 9 |  |  | 3 | 12 |
| **Total** | **21** | **3** | **6** | **8** | **359** | **26** | **21** | **79** | **523** |
| now skipped |  |  |  |  | 302 |  |  |  | 302 |
| still failing | 21 | 3 | 6 | 8 | 57 | 26 | 21 | 79 | 221 |

## (B) missing features

- `__cpp_lib_constexpr_exceptions` (18_support/exception/version.cc, 19_diagnostics/headers/stdexcept/version.cc): undefined by design until Clang can throw in constant evaluation (STATUS, Open issues).
- `boyer_moore_searcher` / `boyer_moore_horspool_searcher`, `__cpp_lib_boyer_moore_searcher` (20_util/function_objects/83607.cc, searchers.cc): documented as not provided.
- C++ `<complex.h>` and `<tgmath.h>` ([complex.h.syn], [tgmath.h.syn]: they include `<complex>`/`<cmath>`; glibc's C headers are found instead): 26_numerics/headers/{ccomplex/complex.h.cc, complex.h/std_c++11.cc, complex.h/std_gnu++11.cc, ctgmath/complex.h.cc} (documented as not provided).
- C++ `<stdckdint.h>` (26_numerics/stdckdint/1.cc, extended.cc; GCC only -- glibc's C header uses `_Bool`/C macros under g++, the Clang path happens to work) and `__cpp_lib_stdckdint_h`. Likewise no C++ `<stdbit.h>` / `__cpp_lib_stdbit_h` (20_util/stdbit/1.cc fails first on an F issue).
- Feature-test macro audit ([version.syn] vs. `<version>`; not all have tests): besides the three (A) macros above, undefined are `__cpp_lib_freestanding_{cstdlib,execution,functional,memory}`, all 17 `__cpp_lib_hardened_*`, `__cpp_lib_start_lifetime` (std::start_lifetime exists), `__cpp_lib_stdbit_h`, `__cpp_lib_stdckdint_h`, `__cpp_lib_view_interface` (202606), `__cpp_lib_pointer_tag_pair`, `__cpp_lib_modules`, `__cpp_lib_reflection`/`__cpp_lib_define_static` (GCC), the senders family (`senders`, `task`, `counting_scope`, `parallel_scheduler`), `__cpp_lib_is_within_lifetime` (GCC: no builtin), and on Clang `__cpp_lib_is_pointer_interconvertible`/`__cpp_lib_is_structural`/`__cpp_lib_contracts` (no builtins). The freestanding_* and start_lifetime ones looked like plain omissions. **Outcome:** checked against
  the feature: `__cpp_lib_freestanding_{execution,functional,memory}` are now defined, and
  `__cpp_lib_freestanding_cstdlib` in hosted builds (like cstring/cwchar; `<cstdlib>` is hosted)
  after making `div`/`ldiv`/`lldiv` constexpr (they were the C library's, which
  `__cpp_lib_constexpr_cmath` also claims) and adding `memalignment`. `__cpp_lib_start_lifetime`
  is defined only with `__builtin_is_within_lifetime` (Clang): without it, `start_lifetime` on an
  object already within its lifetime re-creates it during constant evaluation (GCC; own test
  memory/start_lifetime is XFAIL there). `__cpp_lib_result_of_sfinae` (201210L, in the draft) is
  defined too. `__cpp_lib_freestanding_operator_new` stays: the current [version.syn] still lists
  it ("see below": 202306L when the default allocation functions are hosted ones).

## (C) libstdc++-specific tests

Already skipped now (302 per compiler; counts per skip entry; reasons in `skip.txt`):
testsuite helpers that include `bits/`/`ext/`/`tr1/` headers, directly (24) or through a sibling file (5 + 15 more reaching `bits/stdc++.h`, `ext/type_traits.h`, `experimental/*`, `cxxabi.h`, `std::__format` or `<ciso646>`);
`<cxxabi.h>` (3); removed headers `<codecvt>`, `<strstream>`, `<ciso646>`, `<cstdalign>`, `<cstdbool>`, `<ccomplex>`, `<ctgmath>` (26);
`wstring_convert`/`wbuffer_convert` (6); `_GLIBCXX*` macros (13); `#undef __cpp_lib_*` + re-include (8);
removed features: auto_ptr, raw_storage_iterator, get_temporary_buffer, random_shuffle, binders, uncaught_exception, is_literal_type, result_of, unary_negate (25), reference_wrapper's result_type/argument_type (3);
deprecated features: rel_ops, is_pod/is_trivial, aligned_storage/union, std::iterator base (18), volatile tuple-like traits (9);
special-function, `::abs`, `abs(__float128)`, `fabs(complex)` extensions (24); explicit instantiation with a foreign-value_type allocator (12); facets, streams or char_traits for non-character types (17 + 9);
istream >> setfill (4); deleted-swap expectations (4); noexcept strengthenings (7); hash ABI size (1); greedy_ops (2);
explicit instantiation hitting `swap() const` Mandates (3) or of `allocator<void>` (1); make_from_tuple SFINAE (1); `tuple<>` triviality (1);
variant emplace exception guarantee (2); `_GLIBCXX_DEBUG` span checks (2); queue/stack/priority_queue default ctor copying (3); istreambuf_iterator::pointer (1);
pre-P3505 to_chars data (2); outdated macro values (constexpr_string, parallel_algorithm, exception_ptr_cast: 4); random min()/max(), default engine (11), generate_canonical / seed_seq (3);
valarray mask asserts (5); std/time choices (7); libstdc++ internals (`std::__*`, `__num_put_type`, `_M_buf_size`, `__resize_and_overwrite`: 11);
pre-C++26 synopsis redeclarations (7); inplace_vector `#error` self-check (3).

Still failing (47 on GCC, 57 on Clang), left as failures because the test also covers standard
behaviour or the expectation is narrow:
- noexcept strengthening inside larger tests: 24_iterators/common_iterator/1.cc ("GCC extension"), 26_numerics/bit/bit.pow.two/bit_ceil.cc
- deleted-swap expectation mixed with deprecated `variant_alternative<volatile>`: 20_util/variant/compile.cc
- unspecified counts/orders: 23_containers/{map,set}/modifiers/hetero/insert.cc (comparison counts), 23_containers/unordered_{map,set,multimap}/operations/1.cc (equivalent keys in unique containers / order of equivalent elements), 23_containers/vector/modifiers/insert_vs_emplace.cc (special-member counts), 30_threads/async/async.cc (Clock::now() calls in wait_until), 25_algorithms/copy_n/istreambuf_iterator/1.cc (input increments of copy_n, LWG 2471; libycxx increments n-1 times by design)
- implementation-defined text/values: 20_util/bad_function_call/what.cc, 30_threads/thread/id/output.cc, 27_io/basic_stringbuf/str/char/123100.cc (stringbuf::setbuf), 27_io/filesystem/path/factory/u8path.cc (documented)
- libstdc++ QoI beyond the draft: 21_strings/basic_string/cons/113841.cc and, on Clang, 23_containers/vector/cons/113841.cc (default ctor constrained on a default-constructible allocator; the draft's `basic_string() : basic_string(Allocator())` is unconstrained), 23_containers/deque/types/92267.cc, 23_containers/forward_list/{cons/12,modifiers/122661}.cc (assigning non-assignable elements; [sequence.reqmts] precondition), 23_containers/list/61347.cc (`__builtin_constant_p` after optimisation), 20_util/allocator_traits/requirements/rebind_neg.cc and five `incomplete*_neg.cc` (diagnostics for UB), 24_iterators/operations/prev_neg.cc (libstdc++'s static_assert), 22_locale/ctype/2.cc (ctype of a class type), 22_locale/time_get/get/{char,wchar_t}/4.cc (4-digit %y extension), 27_io/basic_iostream/cons/16251.cc and 27_io/rvalue_streams-2.cc (default-constructible iostreams), 23_containers/vector/bool/modifiers/insert/104559.cc (`insert(pos)`)
- draft says otherwise / pre-C++26: 20_util/duration/io.cc (`duration<const char>` is ill-formed), 20_util/function_objects/comparisons_pointer.cc (array comparison, removed by P2865; GCC only), 18_support/initializer_list/range_access.cc (P3016), 23_containers/vector/bool/modifiers/swap/constexpr.cc (static `swap(reference, reference)`), Clang only: 23_containers/{deque,list,map,multimap,multiset,set,vector}/modifiers/swap/1.cc (non-constexpr member-swap specialization), 24_iterators/headers/iterator/range_access.cc and 20_util/headers/utility/synopsis.cc (redeclarations without the draft's noexcept)
- documented divergences/defects: char16_t eof (21_strings/char_traits/requirements/char16_t/eof.cc, 27_io/basic_streambuf/{sgetc,sputc}/char16_t/80624.cc), 21_strings/char_traits/requirements/constexpr_functions_c++{17,20}.cc (macro not in the draft), 20_util/to_chars/version.cc, 19_diagnostics/headers/system_error/errc_std_c++0x.cc, 23_containers/mdspan/submdspan/submdspan_mapping.cc (precondition violation), std/format/functions/114519.cc (`-fno-char8_t`)
- UB in the test: 21_strings/basic_string/capacity/char/resize_and_overwrite.cc (test04 throws from the operation)

## (D) compiler problems / differences

- GCC: 20_util/specialized_algorithms/destroy/121024.cc (PR c++/102284; the test is `dg-xfail-if`, which the harness ignores).
- Clang 23 (all documented in STATUS unless noted): cannot throw in constant evaluation (19_diagnostics/{logic,runtime}_error/constexpr.cc, 20_util/constant_wrapper/generic.cc); no `__builtin_is_corresponding_member` / `__builtin_is_pointer_interconvertible_with_class` / `__builtin_is_structural` / reflection (6 tests in 20_util/is_*); `atomic_ref` copy-list-init overload resolution (29_atomics/atomic_ref/ctor.cc); mdspan test code only GCC accepts or constexpr step limits (8 tests in 23_containers/mdspan); `-fexec-charset=ISO8859-1` (std/format/fill_nonunicode.cc).
- Clang, not yet documented: source_location `column()` values are GCC's (18_support/source_location/{1,consteval}.cc; implementation-defined); `[[gnu::optimize("O0")]]` ignored, so frame counts differ (19_diagnostics/stacktrace/current.cc); invalid default argument is a hard error inside `is_constructible` (20_util/is_constructible/68430.cc); no `std::float32_t` (20_util/to_chars/float16_c++23.cc); 20_util/optional/constexpr/124910.cc: a constexpr `optional<B>` (non-trivial destructor) after `reset()` is rejected ("subobject 'val' is not initialized": Clang still treats the destroyed union member as active). libycxx can work around it by re-activating `optional_storage::empty` (`std::construct_at(&u_.empty)`) after destroying `val` in `reset()` (`include/ycxx/core/optional.hpp:78-85`). **Done** (only in constant evaluation); the test passes on Clang.
- Clang, new: 20_util/optional/relops/constrained.cc (see the (A) outcome column): Clang 23 does not treat a template `operator==` as excluded from rewriting when a corresponding template `operator!=` is declared ([over.match.oper]/4); GCC does.

## (E) harness / environment

- Data files: tests that open files from `testsuite/data` (`filebuf_members-1.txt`, `thirty_years_among_the_dead_preproc.txt`) run in an empty temp dir: 27_io/{basic_filebuf,basic_fstream,basic_ifstream}/native_handle/{char,wchar_t}/1.cc (crash on the unopened handle), 20_util/hash/chi2_q_document_words.cc. Fix: copy (or symlink) `data/*` into the run directory in `libstdcxx_format.py`.
- Flags: DejaGnu's default flags include `-g`; the harness passes only `-O2`, so 19_diagnostics/stacktrace/{entry,output}.cc see no source lines. GCC-only `dg-options` reach Clang: `-fcontracts` (18_support/contracts/*, 3), `-fno-assume-sane-operators-new-delete` (18_support/50594.cc, 23_containers/vector/bool/capacity/{110498,114758}.cc), `-fvtable-verify=none` (17_intro/freestanding.cc).
- Directives: `dg-xfail-run-if` is not honoured (26_numerics/bit/bit.pow.two/bit_ceil_neg.cc aborts as expected); a `dg-error` with `{ xfail *-*-* }` is treated as an expected error (27_io/fpos/mbstate_t/4_neg.cc).
- Configuration macros: `_GLIBCXX_USE_CXX11_ABI` is undefined, so 27_io/ios_base/failure/error_code.cc takes its old-ABI branch (64 tests test this macro; defining it to 1 in `lit.cfg.py` matches libycxx's behaviour).
- Shim: `shim/bits/stdexcept_throw.h` uses `throw` even under `-fno-exceptions` (18_support/exception_ptr/64241.cc); it should call `__builtin_abort()` when `__cpp_exceptions` is undefined.
- Locales: 27_io/objects/wchar_t/13582-1_xin.cc needs `en_US.ISO8859-1`.

**Outcome (re-run of 2026-10-05):** all of the above except the locale are fixed in the harness
(see the re-run section); the basic_filebuf native_handle tests then turned out to be C (skipped).

## (F) missing includes in the test

- C library names used unqualified or without their header (48): `mbstate_t` (22_locale/codecvt*, 27_io/basic_filebuf/{seekoff,seekpos,underflow}/wchar_t), `wmemset`/`wcslen`/`wcschr`/`WEOF` (22_locale/codecvt/in/wchar_t, 22_locale/time_put/put/wchar_t/12439_*, 25_algorithms/copy/streambuf_iterators/wchar_t/2.cc, 27_io/basic_stringbuf/setbuf/wchar_t, 27_io/basic_ostream/inserters_other/wchar_t/4.cc), `uint_fast32_t`/`uint_fast64_t`/`uint16_t` with only `<random>` (26_numerics/random/{independent_bits,shuffle_order,subtract_with_carry}_engine, 25 tests), `int8_t` (26_numerics/bit/bit.byteswap/byteswap.cc), `std::time_t` with only `<chrono>` (20_util/system_clock/{1,99832}.cc), `::uintptr_t` (20_util/align/1.cc), `int32_t`/`int64_t`/`uint8_t` (23_containers/span/everything.cc, 20_util/duration/cons/dr3050.cc, 23_containers/mdspan/{layouts/padded,submdspan/canonical_slices}.cc), `errno` (21_strings/basic_string/numeric_conversions/char/errno.cc), `wcscmp`/`wint_t` (21_strings/basic_string_view/operations/compare/wchar_t/1.cc, 21_strings/char_traits/requirements/wchar_t/typedefs.cc), `std::printf`/`std::puts` (std/time/tzdb/1.cc, std/format/formatter/ext_float.cc).
- Library names without their header: `std::equal`/`std::fill` without `<algorithm>` (9 in 23_containers/vector, vector/bool, array/creation), `std::iota` (25_algorithms/shuffle/1.cc), `std::numeric_limits` (26_numerics/headers/cmath/hypot.cc), `std::same_as` (18_support/comparisons/common/1.cc, 20_util/tuple/comparison_operators/three_way.cc), `std::is_same_v` (20_util/stdbit/1.cc), `std::span` (std/format/pr121765.cc), `std::string` (std/ranges/adaptors/93978.cc), `std::stringbuf` with `<syncstream>` (27_io/basic_ostream/emit/1.cc), `std::[io]stringstream` with `<chrono>`/`<format>` (std/time/clock/{local,tai}/io.cc), braced range-for without `<initializer_list>` (20_util/{,un}synchronized_pool_resource/118681.cc).

## Skip entries added

All at the end of `tests/libstdcxx/skip.txt`, each with its reason; every one matched only tests
that failed on both compilers (302 per compiler, no passing test):
content rules for the helper headers with libstdc++-internal includes, `<cxxabi.h>`, removed
headers, `wstring_convert`/`wbuffer_convert`, `_GLIBCXX*` macros and `#undef __cpp_lib_*`; path
rules for volatile tuple-like traits, removed and deprecated features, special-function and
`<stdlib.h>`/complex extensions, foreign-allocator and non-character-type explicit
instantiations, istream manipulators, the noexcept/deleted-swap/greedy_ops/ABI-size extensions,
explicit instantiations hitting Mandates, documented divergences (to_chars, macro values,
exception_ptr_cast, random, valarray, std/time), libstdc++ internals, pre-C++26 synopses and the
inplace_vector self-check.
