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
| algorithms + numerics/numeric.ops | 236/370 | 237/370 | yes | was 2; rest: `<vector>`/`<deque>`/`<random>` (107), is_permutation Mandates (12), `<atomic>`/`<map>`/`<list>`..., views |
| strings/basic.string + string.conversions + hash/literals/erasure | 137/252 | 137/252 | yes (sto*/fp to_string: hosted runtime) | was 0; 109 need `<vector>`/`<deque>` (via asan_testing.h), `<algorithm>`, `<sstream>`, `<ranges>`, `<cmath>`; rest below |
| utilities/charconv | 7/12 | 7/12 | yes (fp: runtime archive) | rest need `<algorithm>`, `<cmath>`, `<string>` |
| containers/sequences/vector + vector.bool | 99/155 | 100/155 | yes | was 0; 134/155 (Clang), 136 (GCC) with `<deque>` declared (asan_testing.h); rest below |

Whole-suite baseline (clang, before iterators/tuple/array/optional): 976 pass / ~8,000 run.

<charconv>: own suite 15/20 on both compilers (the other 5 need `<string>`/`<cmath>`; all 20 pass
with stand-ins for those); libstdc++ 20_util/{to,from}_chars 15/31 (rest: `<string>`, `<cmath>`,
`<numbers>`, `<iostream>`, and the pre-P3505 expectations below). The MSVC-derived data of libc++'s
charconv.msvc (17,414 cases) all pass through a scratch harness except 18 shortest cases that
encode the C++17 plain-overload rule and MSVC's reading of general (see divergences) and 18 NaN
spellings that are MSVC's own (`-nan(ind)`). Verified against glibc: every float32 bit pattern (shortest form round-trips, is shortest and closest),
every float16/bfloat16 value, and randomised double, x87 long double and float128 values
(shortest, precision forms, decimal/hex parsing, halfway strings).
<typeindex>: libc++ utilities/type.index 8/9 (rest: `<string>`), libstdc++ 20_util/typeindex 5/5,
both compilers.
libstdc++ testsuite: 20_util/{function,move_only_function,copyable_function,function_ref,
constant_wrapper}: all pass on GCC except tests needing `<string>`/`<iostream>`; Clang also fails
constant_wrapper/generic.cc (throws during constant evaluation). libc++ utilities/const.wrap.class:
15/15 on both; func.wrap: all failures need `<algorithm>`/`<string>`.
libstdc++ testsuite: 25_algorithms + 26_numerics: 263/714 run (clang), 261 (gcc), was 38/36; nearly
all the rest need `<random>`, `<vector>`, `<valarray>`, `<cmath>`, `<complex>`, `<sstream>`.
libstdc++ testsuite: 20_util/{tuple,pair,uses_allocator}: 107 pass on both compilers.
20_util/variant: 27/31 on both (rest: missing `<string>`, `<vector>`, `<any>`).
20_util/any: 22/30 on both (rest: `<vector>`, `<string>`, `<set>`, `unique_ptr`).
21_strings/basic_string_view + char_traits: 98/130 on both (rest: `<string>`, `<sstream>`, `<iosfwd>`).
21_strings/basic_string (+ basic_string_view): 97 -> 266/307 on both compilers (33 need missing
headers, mostly `<sstream>`; the rest are listed under Known limitations). Own suite string/:
55/60 on both (rest: `<algorithm>`, `<ranges>`, `<list>`, and `pmr::string` needs
`polymorphic_allocator`); also clean under ASan on Clang (GCC 16 here has no ASan runtime).
20_util/bitset + 23_containers/bitset: 22/38 on both (rest: `<string>`, `<sstream>`).
23_containers/span: 30/35 on both (rest: `<vector>`, `<deque>`).
20_util/expected: clang 18/20, gcc 18/20 (rest: `<string_view>`, `<vector>`). The libstdc++ harness
compiles with `-O2`, as DejaGnu's default flags do. Some tests rely on dead-code elimination:
`expected/cons.cc` declares `E(const int&)` without defining it, and links only when the
unreachable error branch is removed. `dg-options -fno-inline` is passed through.
<vector>, <inplace_vector> (Phase 3, core): vector, vector<bool>, pmr::vector, inplace_vector, all
constexpr. Own suite vector/ + inplace_vector/ + containers/: 67/79 on both compilers (rest: `<list>`,
views, `<memory_resource>`, and the two adl_robustness test defects below); with stand-ins for the
missing views and `<list>` every remaining vector test passes. libstdc++ 23_containers/vector +
inplace_vector: 0 -> 107/136 (GCC), 104/136 (Clang); the rest need missing headers
(`<memory_resource>`, `<iostream>`, `<regex>`, testsuite_iterators.h's libstdc++ internals),
`<algorithm>` through `<vector>`, or libstdc++ extensions (below). Clean under ASan on Clang.
<memory> (Phase 3): specialized algorithms (std and ranges, constexpr), unique_ptr, shared_ptr /
weak_ptr / enable_shared_from_this / make_shared family (constexpr), owner_less / owner_hash /
owner_equal, out_ptr / inout_ptr. Own suite memory: Clang 81/83, GCC 80/83 plus 1 XFAIL;
make_shared.pass and make_unique.pass need `<string>`. libc++ utilities/memory
67 -> 141 and utilities/smartptr 15 -> 51 (GCC) / 52 (Clang); libstdc++ 20_util smart pointer
and specialized-algorithm directories 37 -> 189 (GCC), 36 -> 190 (Clang). The remaining failures
need `<string>`, `<vector>`, `<algorithm>`, `<ranges>`, `<sstream>`, `<atomic>` or are noted below.

<memory_resource> and <scoped_allocator>: memory_resource, polymorphic_allocator (core; `<string>`
includes it, so `pmr::string` works with `<string>` alone), new_delete/null resources, the atomic
default resource, pool_options, synchronized/unsynchronized_pool_resource and
monotonic_buffer_resource (hosted runtime, DECISIONS §3); scoped_allocator_adaptor (core).
libc++ utilities/utility/mem.res 0 -> 57/78, allocator.adaptor 0 -> 32/32, allocator.uses
2 -> 3/4; libstdc++ 20_util memory_resource, monotonic_buffer_resource, polymorphic_allocator,
scoped_allocator 0 -> 20/20, *_pool_resource 0 -> 4/9 (both compilers). Remaining failures
need missing containers, `<initializer_list>` from `<memory_resource>`, libstdc++'s
`bits/move.h`, or are noted below.

## Freestanding
`tools/check_freestanding.sh`: every core header compiles with `-ffreestanding -nostdlib -nostdinc
-fno-exceptions -fno-rtti`; the smoke test links on x86_64-unknown-none-elf and
riscv64-unknown-elf (Clang) and x86_64 (GCC). Core headers: see `tools/headers.py`.

## Reference runs against libstdc++
`YCXX_STDLIB=libstdcxx tools/run-conformance ycxx gcc|clang` runs the own suite against GCC 16's
libstdc++. `tests/ycxx/REFERENCE.md` lists every failure: libstdc++ bugs (e.g. `variant::swap`
with a valueless operand, `numeric_limits<bool>::traps`, `function_ref` assignment), C++26 parts
libstdc++ 16 lacks, GCC/Clang differences, C-header gaps and ABI limits. No failure was traced to
a defect in a test.

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

- `to_chars(fixed)` without precision prints an integer value whose spacing is 2 or more exactly
  (e.g. `1e300` as its 301 exact digits). Where the rounding interval reaches below 10^(L-1)
  (L the value's digit count), L-1 nines would also round-trip and be one character shorter,
  which [charconv.to.chars]/2 read literally prefers; MSVC prints the exact value too, and
  libc++'s MSVC-derived tests expect it.
- `to_chars(general)` without precision is the shortest %g output over all precisions P (the
  draft's "smallest number of characters"; own test `to_chars_float_general_shortest`). MSVC,
  and libc++'s MSVC-derived tests, decide the style with P = 6 (`1234000` vs `1.234e+06`).
- The plain `to_chars` follows P3505 ([charconv.to.chars]/7: f for 10^-4 <= |v| < 10^U);
  libc++'s and libstdc++'s tests still encode the C++17 shortest-of-f-and-e rule, and
  libstdc++'s `to_chars/version.cc` expects `__cpp_lib_to_chars` 202306L, not 202606L.
- Floating-point `from_chars` leaves the value unmodified on `result_out_of_range` (overflow, or a
  nonzero value that rounds to zero), as the draft says; MSVC stores +-inf or +-0.
- x87 `long double` `%a`: normal values print with a leading 1 (`1.8p+0`), subnormal ones as the
  C library does (`0x0.000000000000001p-16385` is the smallest), so that both forms agree there.

## Known limitations and draft defects
- `<memory>`: no `atomic<shared_ptr<T>>` / `atomic<weak_ptr<T>>`, no `operator<<` for
  unique_ptr/shared_ptr (no `<ostream>`), no execution-policy overloads of the specialized
  algorithms, no `pointer_tag_pair`, `indirect`, `polymorphic`. shared_ptr reference counts use
  the `__atomic` builtins unconditionally (no single-threaded fast path). get_deleter identifies
  the deleter type by a per-type tag address (same shared-library caveat as `any`).
  make_shared of a multi-dimensional array of a non-trivial class type cannot be
  constant-evaluated on Clang (Clang will not let element construction begin the enclosing
  array's lifetime).
- GCC 16.2: `new T[3]` of a class with a non-trivial destructor and a 256-byte
  `std::array` member (default member initializer) reads back wrong values in a generic lambda
  (libc++ unique.ptr.observers/op_subscript.runtime; fails with libstdc++ too). libstdc++
  destroy/121024.cc fails on GCC (PR c++/102284, marked dg-xfail-if, which the harness ignores).
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
- `exception_ptr` is constexpr only for null values: neither compiler documents a way to reach
  the constant-evaluation exception state (own test `exception/exception_ptr_constexpr` fails on
  GCC; Clang 23 cannot throw during constant evaluation at all).
- The Itanium ABI records a handler's type without its reference-ness, so `catch (T*&)` also
  accepts pointer conversions that only `catch (T*)`/`catch (T* const&)` may ([except.handle]/3;
  own tests `except/handler_pointer_reference*`). libsupc++ behaves the same.
- GCC 16 omits `__noexcept_mask` in the type_info of pointers to noexcept member functions, and
  records a member function's cv- and ref-qualifiers only in the name (`M1BKFivE` has the same
  `__pointee` as `M1BFivE`); the runtime compares the mangled names instead.
- GCC 16 records `catch (int(&)())` as a handler for `int(*)()` and `catch (int(&)[3])` as one
  for `int*` ([except.handle] Note 1: neither may match); the runtime cannot tell. Own tests
  `except/handler_function_pointer` and `except/handler_array_decay` fail on GCC.
- Programs link the shared unwinder (`-shared-libgcc`): glibc's pthread_exit/pthread_cancel
  unwind through libgcc_s.so, and a second, static unwinder copy would abort.
- `<system_error>` has only `errc` (core; Linux errno values, checked against `<errno.h>` by the
  hosted header) and the `is_error_*_enum` traits; `error_category`, `error_code` and
  `system_error` need `<string>`.
- Floating-point `<charconv>` for `long double`/`float128_t` works on stack-allocated big integers
  (no heap, so it stays freestanding): parsing needs about 21 KB of stack (two 38,500-bit numbers
  and an 11,566-digit buffer, exact for any input length), `%g` with a large precision about 20 KB.
- Not yet provided: `<cxxabi.h>` (`abi::__cxa_demangle`, `__cxa_vec_*`,
  `abi::__forced_unwind`). Catch matching against deep virtual-diamond hierarchies enumerates
  every inheritance path (exponential), and `dynamic_cast` other than to the most derived type
  is about 2x slower than libsupc++'s.
- The default terminate handler prints the thrown type's mangled name (no demangler yet).
- `any` without RTTI identifies types by the address of a per-type table, so `any_cast` across a
  shared library built with hidden visibility or `-Bsymbolic` does not recognise the type.
- No `<stddef.h>` wrapper: `::max_align_t` comes from the compiler's header and is not
  `std::max_align_t` (see Deliberate divergences).

- `<string>`: the libc++ tests using `constexpr_char_traits.h`/`nasty_string.h` (`EOF`),
  `deallocate_size` (`::uint32_t`) and libstdc++'s `errno.cc` expect `<string>` to pull in C
  headers, which core `<string>` does not (DECISIONS §3). With those headers and `<vector>`/
  `<deque>`/`<algorithm>` stubbed, libc++ strings passes 225/252 on Clang; the remaining
  failures are missing headers, `reserve()` without argument (removed in C++26), and
  `to_string(double)` tests that expect the pre-C++26 `"%f"` output (libycxx implements
  `format("{}", v)`, P2587).
- `<string>` deviations by design: the string-view-like constraint also excludes classes derived
  from `basic_string` (so a derived rvalue is moved, not copied through a `string_view`);
  `resize_and_overwrite` passes `p` and `m` as prvalues and leaves the string unchanged if the
  (precondition-violating) operation throws; the libstdc++ tests checking
  `__cpp_lib_constexpr_string == 201907` see 202511 (constexpr integral `to_string`).
- Floating-point `to_string`/`to_wstring` are the plain `to_chars` output (`format("{}", v)`,
  [string.conversions]): shortest round trip, fixed notation only in [1e-4, 10^U)
  ([charconv.to.chars]/7). Defined out of line in the hosted runtime.

- `<vector>`: no `formatter<vector<bool>::reference>` (no `<format>`); `pmr::vector` names the
  forward-declared `polymorphic_allocator` until `<memory_resource>` exists. No AddressSanitizer
  container annotations (libc++'s asan tests check them: 16 libc++ tests fail under ASan for that
  reason only). Not provided: the pre-C++26 `static vector<bool>::swap(reference, reference)` and
  libstdc++'s `vector<bool>::insert(pos)` / mismatched-allocator extensions. vector<bool> shifts on
  insert/erase bit by bit. shrink_to_fit swallows an allocation failure (a non-binding request).
  Strengthened noexcept: `vector(vector&&, const Allocator&)` when the allocator is always equal;
  inplace_vector's copy operations when T's are. libc++ `vector.modifiers/emplace` and
  `vector.bool/find` exceed Clang's default constexpr step limit (pass with 2x).
- `<inplace_vector>`: Clang 23 cannot begin the lifetime of one element of a union array member
  in constant evaluation (P3074; GCC 16 can, probed in-language). On Clang a trivially destructible,
  default-constructible T is value-initialized as a whole array first, a non-trivially-destructible
  T lives in std::allocator storage during constant evaluation (so a constant-initialized
  inplace_vector of such a T cannot hold elements), and other T cannot be used in constant
  evaluation. A second union member (a pointer) is added for the second case, which can raise
  sizeof/alignof on Clang only.
- Own-suite test defects (reported, not changed): `vector/adl_robustness` and
  `inplace_vector/adl_robustness` `pointers()` part: every operator expression on
  `evil::Iter<Holder<Incomplete>*>` or `vector<Holder<Incomplete>*>` (including the test's own
  `a == d`, `++it`) performs ADL that instantiates `Holder<Incomplete>`, which no library can avoid
  (libstdc++ fails it too). `inplace_vector/adl_robustness` also calls `reserve(200)` on an
  `inplace_vector<T, 128>`, which must throw bad_alloc ([inplace.vector.capacity]/9). Both pass
  with those parts removed.
- The shared feature-test macros `__cpp_lib_containers_ranges`, `__cpp_lib_erase_if`,
  `__cpp_lib_nonmember_container_access`, `__cpp_lib_incomplete_container_elements` and
  `__cpp_lib_allocator_traits_is_always_equal` are left to the containers' integration (they
  cover headers that do not exist yet).
- `<algorithm>`/`<numeric>`/`<execution>` (core): every std:: and ranges:: algorithm of the
  draft, constexpr where specified. The std:: ExecutionPolicy overloads run sequentially and
  are noexcept (an escaping exception calls terminate). Not provided: the ranges::
  ExecutionPolicy overloads (P3179, so `__cpp_lib_parallel_algorithm` is undefined), the
  senders/receivers part of `<execution>`, `boyer_moore(_horspool)_searcher` (need hashing
  containers), `__cpp_lib_interpolate` (needs `std::lerp` in `<cmath>`).
- stable_sort / stable_partition / inplace_merge take their buffer from `operator new(nothrow)`
  (std::allocator during constant evaluation) and fall back to O(N log^2 N) / O(N log N)
  rotation algorithms when it fails; sort and nth_element are introsort/introselect.
- `<ranges>` is partial: concepts, range access, `view_interface`, `subrange`, `dangling`,
  `views::all` (callable, not yet pipeable). No other factories or adaptors.
- `std::is_permutation` enforces its Mandates (same value type); libc++'s sort/heap tests call
  it with `MoveOnly*` and `int*` and fail to compile for that reason (12 tests).
- shuffle/sample assume the generator's results fit in 64 bits.

- `<memory_resource>`: synchronized_pool_resource is the unsynchronized pool behind one lock
  (no thread-specific pools). Pool block sizes are powers of two from 8 bytes to 64 KiB (the
  `largest_required_pool_block` limit; `max_blocks_per_chunk` limit 65536, chunks also capped
  at about 1 MiB); every chunk is aligned to its block size. monotonic_buffer_resource starts
  at 1 KiB and doubles. new_delete_resource uses the aligned allocation functions only above
  `__STDCPP_DEFAULT_NEW_ALIGNMENT__`. libc++ `construct_piecewise_pair_evil` expects
  polymorphic_allocator::construct to pass a non-const allocator; the draft's uses-allocator
  construction passes `const Alloc&`, so libycxx rejects it.

## Open issues / next
- Phase 2 is complete. The ABI runtime (src/abi) replaced libsupc++: broad sweep 4483 -> 4535
  passes with no regressions.
- Then Phase 3 (containers, algorithms), Phase 4 (ranges, charconv, format, ...).
- Constexpr exceptions (P3068): done for `exception`, `bad_alloc`, `bad_array_new_length`,
  `bad_exception`, `bad_cast`, `bad_typeid`, `bad_optional_access`, `bad_variant_access`, and
  `bad_expected_access`. Those the library throws are thrown from headers through `raise_with`,
  so GCC can throw them during constant evaluation; Clang 23 cannot throw during constant
  evaluation at all. Under -fno-rtti the six classes libsupc++ defines keep an out-of-line
  destructor, so they are not constexpr-destructible there (DECISIONS §4).
  Still open: the `<stdexcept>` classes (their message storage lives in the hosted runtime), so
  `__cpp_lib_constexpr_exceptions` is not yet defined.
