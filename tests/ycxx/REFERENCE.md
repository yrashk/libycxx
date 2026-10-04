# libycxx's own test suite run against libstdc++

The spec-derived suite in `tests/ycxx` is also run against the toolchain's own standard library,
GCC 16.2's libstdc++, with both compilers:

```sh
YCXX_STDLIB=libstdcxx tools/run-conformance ycxx gcc     # g++-16, libstdc++
YCXX_STDLIB=libstdcxx tools/run-conformance ycxx clang   # clang++-23 -stdlib=libstdc++ (GCC 16's)
```

(`tools/ref-cxx` is the compiler wrapper; the binaries run against GCC 16's `libstdc++.so`.)

This checks the tests themselves (a test that fails everywhere deserves a second look) and
records where libstdc++ and the current draft disagree. The draft is the reference throughout:
a failure below is a libstdc++ gap, a libstdc++ bug, or a compiler issue, never a reason to
change a test. **After triage no failure was traced to a defect in a test.**

Run of 2026-10-04 (905 tests on GCC, 917 on Clang, the latter including tests added while the
runs were in progress): GCC 830 pass / 74 fail / 1 xfail; Clang 819 pass / 94 fail / 4 xfail.
The same suite against libycxx: see `STATUS.md`.

Legend: **G** fails with GCC + libstdc++, **C** with Clang + libstdc++.

## 1. libstdc++ bugs (the draft is clear, libstdc++ 16 does otherwise)

| Test | G | C | What happens | Draft |
|---|---|---|---|---|
| `variant/valueless` | G | C | `x.swap(z)` with `x` valueless and `z` holding a value leaves the wrong states (line 104) | [variant.swap]/3.3: "Otherwise, exchanges values of rhs and *this" |
| `limits/bool` | G | C | `numeric_limits<bool>::traps` is `true` | [numeric.special]: `numeric_limits<bool>` has `traps = false` |
| `iterator/move_iterator_types` | G | C | `move_iterator<I>` is default-constructible although `I` is not | [move.iterator]: `move_iterator() requires default_initializable<Iterator> = default` |
| `functional/function_ref_constraints`, `functional/function_ref_from_specialization` | G | C | `function_ref<R()> = function_ref<R() noexcept>` selects the deleted `operator=(T)` | [func.wrap.ref.ctor]/21: Constraints: is-convertible-from-specialization<T> is false |
| `functional/bind_sfinae` | G | C | `bind<R>(f, args)` is invocable when `INVOKE(f, ...)` is not implicitly convertible to `R` | [func.require]/2: INVOKE<R> "implicitly converted to R" |
| `variant/visit_constraints` | G | C | `visit<R>` with a non-variant argument is not constrained away | [variant.visit]/2 (both forms constrained on as-variant) |
| `tuple/compare_heterogeneous` | G | C | `tuple<int,int,int> == pair<int,int>` is viable | [tuple.rel]/2: Constraints: sizeof...(TTypes) equals tuple_size_v<UTuple> |
| `string/find_noexcept` | G | C | `compare(const basic_string&)` and the string-view-like `find` are not `noexcept` | [basic.string]: `compare(const basic_string&) const noexcept`; find(const T&) noexcept(is_nothrow_convertible_v<...>) |
| `algorithm/is_permutation_value_type` (compile.fail) | G | C | iterators with different value types are accepted | [alg.is.permutation]/1: Mandates: same value type |
| `functional/not_fn_mandates` (compile.fail) | G | C | the Mandates of `not_fn` are not diagnosed | [func.not.fn]/2 |
| `exception/exception_ptr_cast_volatile` (compile.fail) | G | C | `exception_ptr_cast<volatile E>` is accepted | [propagation]/13: Mandates: E is cv-unqualified |
| `algorithm/unique` | G | C | `unique_copy` with input iterators is not usable in constant evaluation | [alg.unique]: constexpr |
| `memory/ranges_construct_destroy_at` | G |  | `ranges::construct_at` on `int(*)[]` hard-errors instead of being constrained away | [specialized.construct] (constrained on the construct-at expression) |
| `memory/ranges_uninitialized_fill`, `memory/ranges_uninitialized_copy_move` |  | C | in constant evaluation the algorithms assign to objects whose lifetime has not begun (GCC accepts, Clang rejects) | [specialized.algorithms]: construct, not assign |
| `char_traits/move_copy_assign` |  | C | `char_traits<char>::move(p, p, n)` reads an object outside its lifetime in constant evaluation | [char.traits.require]: move works for overlapping ranges |
| `containers/allocator_aware` |  | C | `basic_string` move assignment with an unequal, non-propagating allocator is rejected in constant evaluation | [container.alloc.reqmts]/28, all members constexpr |

## 2. Missing in libstdc++ 16 (newer C++26 additions, constexpr, API revisions)

| Test(s) | G | C | Missing |
|---|---|---|---|
| `bit/permute`, `bit/constraints`, `bit/shl_shr`, `bit/signatures_all_types` | G | C | `bit_reverse`, `bit_repeat`, `bit_compress`, `bit_expand`, `shl`, `shr` |
| `bitset/reference` | G | C | `bitset::reference::operator=(bool) const` |
| `expected/observers`, `expected/void_ctor_assign`, `expected/void_observers_monadic` | G | C | `expected::has_error()` |
| `initializer_list/data_empty`, `initializer_list/list_init` | G | C | `initializer_list::data()`, `empty()` |
| `iterator/basic_const_iterator` | G | C | `basic_const_iterator::iterator_type` |
| `iterator/range_access_via_optional` | G | C | the [iterator.range] functions via `<optional>` ([iterator.range]/1.10) |
| `optional/nullopt_compare` | G | C | `nullopt_t` comparisons |
| `type_traits/is_applicable` | G | C | `is_applicable` |
| `memory/uses_allocator_construction` | G | C | the pair-like overload of `uses_allocator_construction_args` |
| `functional/function_deduction_forms` | G | C | `function` deduction from `volatile` call operators ([func.wrap.func.con]/16: "cv &opt") |
| `exception/exception_ptr_cast*`, `exception/make_exception_ptr*`, `exception/exception_signatures` | G | C | `exception_ptr_cast` returns `const E*` (an earlier revision); the draft returns `optional<const E&>` |
| `memory/shared_ptr_constexpr`, `memory/pointer_traits_pointer_to`, `string/to_string_constexpr` | G | C | constexpr `shared_ptr`/`make_shared`, `pointer_traits::pointer_to`, `to_string` |
| `memory/start_lifetime` | (xfail) | C | `start_lifetime` |
| `deque/*`, `list/*` (tests in progress) | G | C | constexpr `deque` and `list` |
| `version/*` | G | C | macros missing or with older values: `__cpp_lib_bitops` (202607L), `__cpp_lib_constexpr_bitset` (202207L), `__cpp_lib_expected` (202606L), `__cpp_lib_freestanding_optional` (202506L), `__cpp_lib_apply` (202603L), `__cpp_lib_initializer_list`, `__cpp_lib_freestanding_{iterator,tuple,utility,cwchar}`, `__cpp_lib_freestanding_operator_new` ([version.syn]/4), `__cpp_lib_atomic_min_max`, `__cpp_lib_barrier`, ... |

## 3. Differences between GCC and Clang with the same libstdc++

These pass with GCC and fail with Clang; libstdc++ makes the feature depend on the compiler, or
Clang rejects code GCC accepts.

| Test(s) | Cause |
|---|---|
| `string/literals`, `string/cons_pointer`, `string/string_view_conversion` | constexpr `basic_string` construction: Clang reports "undefined function `_M_construct`" (libstdc++'s explicit-instantiation declarations hide the definition from constant evaluation) |
| `expected/bad_expected_access_constexpr`, `variant/bad_access` | libstdc++'s constexpr exception classes are only constexpr with GCC (Clang 23 cannot throw in constant evaluation) |
| `compare/type_order`, `version/header_compare` | `std::type_order` (needs a builtin only GCC has) |
| `utility/observable_checkpoint_monostate`, `version/header_utility` | `std::observable_checkpoint` |
| `version/header_type_traits` | `is_layout_compatible` (needs builtins Clang lacks) |
| `utility/constant_wrapper_call` | `constant_wrapper::operator()` with a member pointer and constant_wrapper arguments is rejected |
| `cstddef/stddef_global`, `cstddef/stddef_global_reverse` | `::nullptr_t` missing (Clang's `<stddef.h>` in C++ mode); [support.c.headers.other]/1 |
| `cwchar/freestanding_functions` | `std::wcschr` and friends on `const wchar_t*` return `wchar_t*` (glibc's declarations; libycxx documents the same limitation for unqualified calls) |

## 4. C library headers

| Test(s) | G | C | Cause |
|---|---|---|---|
| `cfloat/macros`, `cstdint/macros`, `cwchar/macros` | G | C | `__STDC_VERSION_FLOAT_H__`/`_STDINT_H__`/`_WCHAR_H__`, `WCHAR_WIDTH`, and `INFINITY`/`NAN` in `<cfloat>` (C23 additions the C++26 headers include) are not provided by the C library/compiler headers libstdc++ wraps |

## 5. Compiler and ABI limits (same with libycxx's runtime)

| Test | G | C | Cause |
|---|---|---|---|
| `except/handler_pointer_reference`, `except/handler_pointer_reference_exact` | G | C | the Itanium ABI records `catch (T*&)` like `catch (T*)` (STATUS: known limitations) |
| `except/handler_array_decay`, `except/handler_function_pointer` | G |  | GCC records `catch (int(&)[3])` / `catch (int(&)())` as pointer handlers |
| `except/handler_member_pointer` | G |  | libsupc++ ignores the member function's cv/ref-qualifiers that GCC records only in the type name (libycxx's runtime handles this) |
