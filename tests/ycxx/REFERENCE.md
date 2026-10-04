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

Run of 2026-10-04, 1413 tests: GCC 1238 pass / 174 fail / 1 xfail; Clang 1219 pass / 190 fail / 4 xfail.
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
| `unordered_map/node_handle`, `unordered_set/node_handle` | G | C | a failed `insert(q, nh)` empties `nh` | [unord.req.general]/128: "nh is empty if insertion succeeds, unchanged if insertion fails" |
| `unordered_map/node_compat` | G | C | `unordered_map<K,T>::node_type` differs from `unordered_multimap<K,T,H2,E2>::node_type` | [container.node.overview] Table 75: compatible nodes have the same node handle type |
| `unordered_map/transparent` | G | C | `operator[]`, `try_emplace` and `insert_or_assign` with an existing heterogeneous key construct a key | [unord.map.elem], [unord.map.modifiers]: no effect when the key exists |
| `flat_set/transparent` | G | C | heterogeneous `insert(x)` of an existing key constructs a `value_type` | [flat.set.modifiers]/3: "If the set already contains an element equivalent to x, *this and x are unchanged" |
| `flat_set/deduction` | G | C | `flat_set(first, last)` does not deduce | [flat.set.defn]: `flat_set(InputIterator, InputIterator, Compare = Compare())` guide |
| `priority_queue/deduction` | G | C | no `priority_queue(InputIterator, InputIterator, Allocator)` guide | [priority.queue] synopsis |
| `inplace_vector/noexcept` | G | C | `shrink_to_fit` is not `noexcept` | [inplace.vector.overview]: `static constexpr void shrink_to_fit() noexcept;` |
| `algorithm/stable_partition` | G | C | in constant evaluation `ranges::stable_partition` returns `{i, last - 1}` (correct at run time) | [alg.partitions]/12.2: "{i, last} for the overloads in namespace ranges" |
| `algorithm/clamp` | G | C | 3 comparisons (and 5 projections for `ranges::clamp`) with libstdc++'s default -O0 assertions, which re-check the precondition | [alg.clamp]/5: "At most two comparisons and three applications of the projection" |
| `charconv/to_chars_float_plain_style`, `format/float_shortest_plain_style` | G | C | `to_chars(1e5)` and `format("{}", 1e5)` give "1e+05": f/e chosen by the shorter result (the C++17 wording) | [charconv.to.chars]/7: f if \|value\| is in [l, u) (for double [1e-4, 1e16)), otherwise e |
| `charconv/to_chars_float_general_shortest` | G | C | `to_chars(1234567.0, general)` gives "1.234567e+06", not the shorter "1234567" (interpretive: /2's smallest number of characters with the g specifier) | [charconv.to.chars]/2-3 |
| `random/distribution_param_set` | G | C | after `d.param(p)`, `student_t`, `fisher_f` and `negative_binomial` keep producing values for the old parameters (`fisher_f`'s `d(g, p)` too) | [rand.req.dist] Table 128, d(g): distributed according to p(z \| {p}) with p = d.param() |
| `random/negbin_p_one` | G | C | `negative_binomial_distribution(4, 1.0)` aborts in an internal `poisson_distribution(0)` assertion | [rand.dist.bern.negbin]/2: 0 < p <= 1 |
| `random/swc_full_width` | G | C | `subtract_with_carry_engine` with w equal to the width of UIntType computes Y = X(i-s) - X(i-r) - c wrongly when it wraps (diverges at the 24th value after zero seeding) | [rand.eng.sub]/3 |
| `random/mersenne_corner` |  | C | wrong values with r == w, and s, t, l == w == 64 (optimization-dependent: likely undefined shifts) | [rand.eng.mers]/4 allows r, s, t, l <= w |
| `random/lcong_seed_seq` | G | C | for m = 2^32 + 15, seeding from a seed sequence requests 4 values instead of k + 3 = 5 | [rand.eng.lcong]/6: k = ceil(log2(m) / 32), q.generate(a+0, a+k+3) |
| `random/philox_equality` | G | C | engines with equal key and counter compare unequal when the output buffer is exhausted (e.g. after `set_counter`) | [rand.req.eng] Table 127: x == y iff S_x = S_y |
| `random/piecewise_linear_nw` | G | C | the `(nw, xmin, xmax, fw)` constructor evaluates fw at b_k + delta | [rand.dist.samp.plinear]/12: w_k = fw(b_k) |
| `random/piecewise_constant_nw_zero` | G | C | with nw == 0 the intervals are {0, 1} instead of {xmin, xmax} | [rand.dist.samp.pconst]/11-12 |
| `random/piecewise_float_vectors` | G | C | `intervals()`/`densities()` return `vector<double>` for float and long double | [rand.dist.samp.pconst], [rand.dist.samp.plinear]: `vector<result_type>` |
| `chrono/tai_gps_noexcept` | G | C | `tai_clock`/`gps_clock` `to_utc`/`from_utc` are not noexcept | [time.clock.tai.overview], [time.clock.gps.overview] |
| `random/uniform_int_char`, `random/uniform_int_const`, `random/lcong_uchar`, `random/ibits_uchar`, `random/seed_seq_generate_narrow`, `random/seed_seq_generate_signed`, `random/seed_seq_iterator_float`, `chrono/duration_rep_const` (compile.fail) | G | C | ill-formed template arguments / Mandates violations accepted (char or const IntType, unsigned char UIntType, narrow or signed seed_seq output, non-integer seed_seq input, `duration<const int>`) | [rand.req.genl]/1, [rand.util.seedseq]/4,7, [time.duration.general]/2 |
| `ranges/view_interface_size_type` | G | C | `view_interface::size()` returns a signed type | [view.interface.general]: `to-unsigned-like(ranges::end(derived()) - ranges::begin(derived()))` |
| `ranges/concat_view_iterator_category` | G | C | `concat_view`'s iterator has no `iterator_category` for forward ranges | [range.concat.iterator]/2: declared iff all-forward<Const, Views...> |
| `ranges/as_input_view_borrowed` | G | C | `as_input_view` is not a borrowed range for a borrowed V | [ranges.syn]: `enable_borrowed_range<as_input_view<V>> = enable_borrowed_range<V>` |
| `ranges/ranges_to_emplace_hint` | G | C | `ranges::to` calls `insert` where only `emplace_hint` exists | [range.utility.conv.general]/4-5: `c.emplace_hint(c.end(), std::forward<Ref>(ref))` |
| `format/format_to_n_negative` | G | C | `format_to_n` with n < 0 writes every character | [format.functions]/19: M = clamp(n, 0, N) |
| `sstream/stringbuf_view_no_mode` | G | C | `stringbuf("abc", openmode()).view()` returns "abc" | [stringbuf.members]/12.3: neither in nor out set: "Otherwise, sv() is returned" |
| `syncstream/null_wrapped` | G | C | `osyncstream(nullptr).emit()` does not set badbit although `syncbuf::emit()` returns false | [syncstream.osyncstream.members]/1 |
| `iostreams/num_get_hexfloat` | G | C | extracting a double from "0x1a.bp+07p" stops after "0" | [facet.num.get.virtuals] Example 1: with %g, "0x1a.bp+07" is accumulated |
| `inplace_vector/from_range_mandates` (compile.fail) | G | C | a constant-size range larger than N is accepted | [inplace.vector.cons]/9: Mandates: ranges::size(rg) <= N when it is a constant expression |

| `cmath/lerp` | G | C | `lerp(0, 1, inf)` and `lerp(a, 0, inf)` return NaN | [c.math.lerp]/2.5: "If isfinite(t) \|\| !isnan(t) && b - a != 0, then !isnan(r)" |
| `cmath/nexttoward_extended` (compile.fail) | G |  | `nexttoward(float32_t, long double)` is accepted (Clang defines no extended types, so the test checks nothing there) | [cmath.syn]/4: nexttoward with an extended floating-point argument is ill-formed |
| `complex/additional_overloads` | G | C | `pow(complex<float>, int)` yields `complex<float>`; `arg(1.0)` is not constexpr | [cmplx.over]/3: both arguments cast to `complex<common_type_t<T1, T3>>`, T3 = double for integers; [cmplx.over]/1: constexpr |
| `indirect/allocator` | G | C | the allocator-extended move constructor with unequal allocators leaves the source non-valueless (line 45) | [indirect.ctor]: "Postconditions: other is valueless" |
| `mdspan/layout_padded_default` | G | C | `layout_left_padded<>`/`layout_right_padded<>` rejected | [mdspan.syn]: `template<size_t PaddingValue = dynamic_extent>` |
| `scoped_allocator/types` | G | C | no deduction guide | [allocator.adaptor.syn]: `scoped_allocator_adaptor(OuterAlloc, InnerAllocs...) -> ...` |
| `scoped_allocator/members` |  | C | `==` between adaptors with different outer types is ambiguous (an internal `operator==` and its reversed form) | [scoped.adaptor.operators]/1 |
| `regex/errors` | G | C | `"*a"` throws `error_paren`, `"a**"` is accepted, `"[[:nonsense:]]"` throws `error_collate` (line 32) | [re.err]: `error_badrepeat` ("not preceded by a valid regular expression"), `error_ctype` ("invalid character class name"); ECMA-262 rejects `a**` |
| `regex/ecmascript` | G | C | `\cJ` does not match `"\n"` (line 47) | [re.grammar]/1: ECMA-262 ControlEscape `\cX` |
| `debugging/replace_is_debugger_present` | G | C | a user definition collides with libstdc++exp's ("multiple definition") | [debugging.utility]/5: "This function is replaceable" |
| `algorithm/no_extra_memory` | G | C | with no memory available, `ranges::stable_partition` returns `{i, last - k}` instead of `{i, last}` (the partition itself is right) | [alg.partitions]/12.2: "{i, last} for the overloads in namespace ranges" |
| `execution/numeric_algorithms` | G |  | in-place `exclusive_scan(unseq/par_unseq, c, c+N, c, 0)` yields all zeros | [exclusive.scan]/8: "result may be equal to first" |
| `memory/make_shared_array_throw`, `memory/make_shared_for_overwrite_order` | G | C | array elements are destroyed in construction order (after a throw, and at end of lifetime for `_for_overwrite`) | [util.smartptr.shared.create]/7.10: "destroyed in the reverse order of their original construction" |
| `memory/allocate_shared_cv` | G | C | `allocate_shared<const T>` rebinds the allocator to `const T` and does not compile | [util.smartptr.shared.create]/7.5, /7.12: `remove_cv_t<U>*`; [allocator.requirements.general]: cv-unqualified value_type |
| `string/fancy_pointer_allocator` | G | C | `basic_string` with a class-type allocator pointer does not compile | [string.require]/3, [allocator.requirements.general] |
| `iterator/istreambuf_iterator` | G | C | an iterator built from `it++`'s proxy dereferences to the cached old character, not `sgetc()` (the proxy is exposition-only, so this is interpretive) | [istreambuf.iterator.cons]/5, [istreambuf.iterator.ops]/1 |
| `list/fancy_pointer_allocator` | G | C | with a class-type allocator pointer, `list::swap` corrupts both lists (endless iteration, then a double free) | [container.reqmts]/64 Note 2, /65; [allocator.requirements.general] |
| `deque/allocator_construct` | G | C | `deque::insert(p, n, t)` in the middle creates a copy of `t` without `allocator_traits::construct` | [container.alloc.reqmts]/2 and Note 2; [sequence.reqmts] (insert needs only Cpp17CopyInsertable) |
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
| `deque/*`, `list/*`, `forward_list/*`, `map/*`, `set/*`, `unordered_*/*`, `stack/*`, `queue/*`, `priority_queue/*` (most) | G | C | C++26 constexpr containers and adaptors (the runtime parts of these tests pass) |
| `inplace_vector/*` (some) | G | C | constexpr `inplace_vector` of non-trivial types |
| `hive/*` | G | C | `<hive>` |
| `atomic/store_key`, `atomic/float_fetch_minmax`, `atomic/constexpr` | G | C | atomic `store_add` ... `store_min`; `fetch_fmaximum` family; constexpr atomics |
| `stop_token/concepts`, `stop_token/inplace_stop` | G | C | `stoppable_token`/`unstoppable_token`/`never_stop_token`, `stop_callback_for_t`; `inplace_stop_source`/`_token`/`_callback` |
| `thread/thread_attributes` | G | C | `thread::name_hint`, `thread::stack_size_hint` |
| `future/packaged_task_allocator` | G | C | `packaged_task(allocator_arg_t, const Allocator&, F&&)` |
| `ranges/view_interface_at` | G | C | `view_interface::at` |
| `format/runtime_format`, `format/format_constexpr` | G | C | `std::runtime_format`; constexpr `std::format` |
| `random/generate_canonical`, `random/uniform_real_upper_bound` | G | C | the C++26 `generate_canonical` ([rand.util.canonical]/2-3: attempts until S < x r^d, returns floor(S/x)/r^d); libstdc++ rounds S/R^k and retries on 1, looping forever for a generator that always returns its maximum |
| `random/generate_random`, `random/version_macros` | G | C | `ranges::generate_random`, `__cpp_lib_ranges_generate_random` |
| `map/lookup`, `unordered_map/lookup`, `flat_map/lookup` | G | C | the C++26 `lookup` members |
| `version/*` | G | C | macros missing or with older values: `__cpp_lib_bitops` (202607L), `__cpp_lib_constexpr_bitset` (202207L), `__cpp_lib_expected` (202606L), `__cpp_lib_freestanding_optional` (202506L), `__cpp_lib_apply` (202603L), `__cpp_lib_initializer_list`, `__cpp_lib_freestanding_{iterator,tuple,utility,cwchar}`, `__cpp_lib_freestanding_operator_new` ([version.syn]/4), `__cpp_lib_atomic_min_max`, `__cpp_lib_barrier`, ... |
| `cmath/constexpr_exact`, `cmath/constexpr_special_values`, `cmath/hypot3` | G | C | constexpr `<cmath>` (P0533, P1383): e.g. `remquo`, `abs(long)`; GCC does not fold `exp(-inf)`, `fmax(1, NaN)` |
| `cmath/nextup_nextdown`, `cmath/fmaximum_fminimum` | G | C | C23 `nextup`/`nextdown`, `fmaximum` family |
| `complex/values`, `complex/constexpr_transcendentals` | G | C | constexpr `abs`, `arg`, `proj`, `polar` and transcendentals |
| `valarray/range_access` | G | C | `valarray::iterator`, member `begin`/`end` ([valarray.range]) |
| `mdspan/copy_fill` | G | C | `copy`/`fill` for mdspan ([mdspan.copy]) |
| `linalg/*` | G | C | `<linalg>` |
| `debugging/debugging`, `stacktrace/*`, `text_encoding/text_encoding` | G | C | link only with `-lstdc++exp` (the tests add no flags; with it they pass) |
| `execution/ranges_algorithms`, `execution/ranges_constraints` | G | C | the parallel range algorithms (P3179) |
| `ranges/reserve_hint` | G | C | `ranges::reserve_hint`, `approximately_sized_range`, the views' `reserve_hint` members (P2846) |

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
| containers using `std::from_range`, `insert_range`, `append_range` | `from_range` and the range members are unavailable with Clang |
| `format/dynamic_width_precision` (and `format/fmt_dynamic_width_double`, a compile.fail test that Clang rejects for this reason, not the intended one) | every dynamic width/precision (`{:{}}`, `{:.{}}`) is rejected: libstdc++'s compile-time check calls a function Clang reports as undefined in constant evaluation |
| `cwchar/freestanding_functions` | `std::wcschr` and friends on `const wchar_t*` return `wchar_t*` (glibc's declarations; libycxx documents the same limitation for unqualified calls) |
| `complex/arithmetic`, `complex/literals` | libstdc++'s compound operators use `__real__`/`__imag__`, which Clang cannot constant-evaluate ([complex.member.ops]: constexpr) |
| `cmath/constexpr_raising_call`, `cmath/constexpr_invalid_call` (compile.fail) | fail on the control line too: `log(1.0)`, `sqrt(4.0)` are not constexpr with Clang |
| `algorithm/adl_incomplete_holder` | GCC performs argument-dependent lookup for an unqualified `__builtin_memmove` call inside libstdc++, which instantiates `Holder<Incomplete>` (reproduced without any library; [contents]/3 forbids such lookups) |

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
