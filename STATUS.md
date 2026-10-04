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
| containers/sequences/{deque,list,forwardlist} + container.adaptors/{stack,queue,priority.queue} | 197/371 | 197/371 | yes | was 0; adaptor tests need `<vector>` (338/371 with a local stand-in `<vector>`); rest: `<map>`/`<set>`/`<random>` |
| containers/unord + container.node | 303/422 | 303/422 | yes | was 0; 417/422 on both with stand-in `<cmath>`/`<map>`/`<set>`; rest below |
| containers/views/mdspan | 65/85 | 66/85 | yes | was 0; rest: unqualified `int64_t` (16), `array::operator[]` not noexcept (1), rank-0 `layout_stride` -> `layout_left`/`layout_right` with a narrowing index type expected implicit (2; the draft makes it explicit), Clang: `_BitInt` index types (1) |
| numerics/{c.math,numbers,complex.number,numarray} + utilities/ratio | 325/348 | 327/348 | yes (run-time `<cmath>` calls need libm) | was 6; rest below (numerics) |
| input.output + localization (iostreams, `<locale>`) | 583/855 | 590/855 | no (hosted) | was 39; 213 of the failures need `<filesystem>`, `<codecvt>` (removed), `<format>`/`<print>`, `<mutex>`/`<chrono>` or `EOF` from `constexpr_char_traits.h`; rest under Known limitations |
| atomics + thread (incl. futures, stop tokens, latch/barrier/semaphore) | 449/453 | 449/453 | `<atomic>` yes (runtime archive); the rest hosted | was 16; rest: `<format>` for thread::id (4) |
| input.output + localization (iostreams, `<locale>`, `<filesystem>`) | 668/855 | 668/855 | no (hosted) | was 583 (Clang) / 590 (GCC) before `<filesystem>`; most failures need `<chrono>`, `<codecvt>` (removed), `<format>`/`<print>`, `<mutex>`/`<thread>` or `EOF` from `constexpr_char_traits.h`; rest under Known limitations |
| re (`<regex>`) | 163/164 | 163/164 | no (hosted) | was 12; 5 skipped (draft divergences, see tests/libcxx/skip.txt); rest: `EOF` from `constexpr_char_traits.h` |

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

<system_error> and constexpr <stdexcept>: error_category, error_code, error_condition,
system_error, generic/system categories (strerror_r through the PAL), hash, comparisons.
libc++ diagnostics/{syserr,std.exceptions} 11 -> 68/69 on both compilers (rest: `<ostream>`);
libstdc++ 19_diagnostics/{error_*,logic_error,runtime_error,system_error,headers,stdexcept.cc}
17 -> 38/45 on GCC, 36/45 on Clang (rest: `<locale>`/`<future>` (5), removed STREAMS errc
values, `__cpp_lib_constexpr_exceptions`, and on Clang the two constexpr tests that throw during
constant evaluation).

<memory_resource> and <scoped_allocator>: memory_resource, polymorphic_allocator (core; `<string>`
includes it, so `pmr::string` works with `<string>` alone), new_delete/null resources, the atomic
default resource, pool_options, synchronized/unsynchronized_pool_resource and
monotonic_buffer_resource (hosted runtime, DECISIONS §3); scoped_allocator_adaptor (core).
libc++ utilities/utility/mem.res 0 -> 57/78, allocator.adaptor 0 -> 32/32, allocator.uses
2 -> 3/4; libstdc++ 20_util memory_resource, monotonic_buffer_resource, polymorphic_allocator,
scoped_allocator 0 -> 20/20, *_pool_resource 0 -> 4/9 (both compilers). Remaining failures
need missing containers, `<initializer_list>` from `<memory_resource>`, libstdc++'s
`bits/move.h`, or are noted below.

Iostreams and localization (Phase 4, hosted; DECISIONS §7): `<iosfwd>`, `<ios>`, `<streambuf>`,
`<istream>`, `<ostream>`, `<iostream>`, `<sstream>`,
`<spanstream>`, `<fstream>`, `<syncstream>`, `<iomanip>`, `<locale>` (all standard facets for char
and wchar_t, the char8_t and deprecated char UTF-16/UTF-32 codecvts, the `_byname` facets for
"C"/"POSIX"/"C.UTF-8"/""), the stream iterators, and the stream operators of `<string>`,
`<string_view>`, `<bitset>`, `<memory>`, `<system_error>` and `<complex>`. Own suite ios, iostreams,
sstream, fstream, spanstream, syncstream, iomanip, locale, complex, system_error, bitset, string,
string_view, memory, iterator: 301/305 (GCC, plus 1 XFAIL), 302/305 (Clang); the three failures need
`<filesystem>`, `<thread>`, `<format>`. Clean under ASan (Clang). libc++ input.output + localization
39 -> 590/855 (GCC), 39 -> 583/855 (Clang); libstdc++ 27_io + 22_locale 8 -> 806/925 (GCC),
8 -> 805/925 (Clang); most remaining failures need missing headers or libstdc++ extensions
(`char_traits<unsigned char>`, deprecated manipulator overloads, transitive C headers).

File systems (hosted, POSIX; DECISIONS §8): `<filesystem>` in full (`formatter<path>` in
`ycxx/hosted/filesystem_format.hpp`). Own suite filesystem/ 17/17 on both compilers, fstream/ 5/5; clean under ASan and UBSan (Clang). libc++ input.output/filesystems
5 -> 67/149 (both compilers); with the concurrent `<chrono>` overlaid locally 117/149, the other
32: 29 permission tests that cannot fail as root (all but 2 pass when run as `nobody`), toctou
(`<thread>`), and two below. libstdc++ 27_io/filesystem 0 -> 28/35 run (both; 90 more are skipped
by the harness because they use `__gnu_test` helpers, whose `testsuite_fs.h` needs `<random>`);
5 of the 7 failures need `<random>`.
C++26 utilities and diagnostics: `indirect`, `polymorphic` (`<memory>`, constexpr, allocator-aware),
`<generator>` (`generator`, `pmr::generator`), `<debugging>` (core; runtime archives), `<contracts>`
(core; GCC's layout, see DECISIONS §9), `<text_encoding>` (the full IANA registry of 2026-10-02,
`environment()`, `locale::encoding()`), `<stacktrace>` (capture, ELF/DWARF symbolization, own
demangler). Own suite indirect, polymorphic, debugging, text_encoding, stacktrace, optional:
38 -> 57/57 on both compilers (with the formatters added after `<format>`), clean under ASan
(Clang). libstdc++ std/memory/{indirect,polymorphic} 2 -> 8, 24_iterators/range_generators 0 -> 11,
19_diagnostics/debugging 0 -> 4, 18_support/contracts 0 -> 3 (GCC), 19_diagnostics/stacktrace 0 -> 1
(GCC; the rest need formatter or -g), std/text_encoding 0 -> 1
(runnable tests; the others use libstdc++'s allocator helpers or named locales); libc++
text/text_encoding 0 -> 19/20.
<random> (core; `random_device` in the hosted runtime; DECISIONS §10): seed_seq, all engines and
adaptors (incl. C++26 philox_engine with set_counter), the predefined engines (all 10000th-value
requirements verified), random_device (getrandom, /dev/urandom, /dev/random through the PAL),
generate_canonical with the C++26 (P0952) algorithm, ranges::generate_random (P1068) with bulk
member detection, and all 20 distributions with exact stream round-trips. Own suite random +
algorithm/shuffle_sample: 0 -> 70/70 on both compilers, clean under ASan (Clang). libc++
numerics/rand 5 -> 452/455 on both (rest: generate_canonical expects the pre-C++26 algorithm;
`__int128` as UIntType/IntType). libstdc++ 26_numerics/random 0 -> 204/243 on both (24 use
`uint_fast32_t` etc. unqualified without `<stdint.h>` and pass with the names injected; 10
`cons/parms.cc` expect finite min()/max() where libc++ expects +-infinity, which libycxx returns;
the rest below). libc++ algorithms + numerics/numeric.ops 319 -> 341 (Clang); libstdc++ 25_algorithms +
26_numerics 430 -> 645 (Clang). Every distribution passes chi-square (discrete) or
Kolmogorov-Smirnov plus moment checks at 400k-4M samples (scratch harness, not in the repo).

Formatting (DECISIONS §11): `<format>` and `<print>` in full (C++26 draft: constexpr formatting,
compile-time checked format strings, `dynamic_format`/`runtime_format`, range/tuple/adaptor/
`vector<bool>::reference` formatters, escaped strings with Unicode 18.0 tables, locale overloads),
the `<ostream>` print overloads, and the formatters of `error_code`, `filesystem::path` and
`thread::id` (plus `thread::id`'s `operator<<`). Own suite format/ + print/ + system_error/format +
filesystem/path_format + thread/thread_id 0 -> 94/98 (both compilers); the four failures are
`print_every_kind` and `nonlocking_formatter_optimization` (need the chrono formatters) and two
test defects (see Known limitations). libc++ utilities/format 0 -> 68 pass, 11 fail (GCC) /
74 pass, 2 fail (Clang) of 113 with 34 skipped (`test_format_context.h` needs a libc++-internal
hook; divergences listed in tests/libcxx/skip.txt); print.fun + ostream.formatted.print 0 -> 6/6
run (both; 4 skipped: they call `std::fwide` without `<cwchar>`). libc++ input.output +
localization 668 -> 726/850 (GCC). libstdc++ std/format + 27_io/print 0 -> 24/29 run (GCC), 23/29
(Clang: `-fexec-charset=ISO8859-1` unsupported); the rest need libstdc++ internals, `<span>`/
`<cstdio>` transitively or `-fno-char8_t`; 4 are skipped as implementation-specific or deprecated.
Header cost (GCC, `-fsyntax-only`): `<format>` 0.27 s, `<ostream>` 0.18 -> 0.26 s (its print
overloads need the core of `<format>`).
<regex> (hosted): regex_traits<char>/<wchar_t> (on the locale's ctype and collate facets),
basic_regex with all six grammars, sub_match, match_results (allocator-aware, pmr aliases),
regex_match/regex_search/regex_replace with every match_flag_type, regex_iterator and
regex_token_iterator. ECMAScript backtracks on an explicit stack (ECMA-262 capture and empty-
iteration rules; failure memo for programs without back-references, so `(a|b)*c` and `(a*)*b`
are linear); the POSIX grammars find the leftmost-longest match with an NFA simulation and assign
subexpressions by the POSIX left-to-right longest rule. Own suite regex/: 8/8 on both compilers,
clean under ASan (Clang). libc++ std/re 12 -> 163/164 (+5 skipped; the failure needs `EOF` from
`constexpr_char_traits.h`); libstdc++ 28_regex 0 -> 103/104 (+6 skipped; the failure needs
`bits/move.h`; 61 others need `__gnu_test` helpers); both compilers. Checked against V8 on 63,000
random ECMAScript patterns (with and without icase): identical results.
<meta> (reflection; GCC 16 with `-freflection` only, DECISIONS §13): every [meta.syn] entity.
The metafunctions are GCC's own (declared without definitions); the library defines `info`,
`meta::exception`, `operators`, `access_context` (unprivileged/unchecked/via), `member_offset`,
`data_member_options`, `reflection_range`, `define_static_string`/`_array`/`_object`,
`is_string_literal`, and `is_applicable_type`/`is_nothrow_applicable_type`/`apply_result` (not
GCC 16 metafunctions; through `substitute`). Also `is_reflection` (and `info` as a fundamental,
scalar type) and `is_structural` (GCC) in `<type_traits>`. `__cpp_lib_reflection`,
`__cpp_lib_define_static` and `__cpp_lib_is_structural` are defined only where they work; on Clang
23 `<meta>` is empty. Own suite meta/: 2/2 (GCC), 2 XFAIL (Clang). libstdc++ 20_util reflection
traits (is_reflection, is_structural, is_scalar/reflection, variable_templates_for_traits) 2 -> 6/7
run on GCC (4 more need `testsuite_tr1.h`; the failure uses the deprecated `is_trivial_v`);
libc++ has no reflection tests.

Time (DECISIONS §14): `<chrono>` in full: the calendar (all types, `/` operators, arithmetic,
constants, literals), `hh_mm_ss`, the 12/24-hour functions (core, constexpr), utc/tai/gps clocks
with leap seconds, `clock_cast` (all five routes), the time zone database on the system's
zoneinfo (TZif v1-v4 with POSIX footer, lazily per zone; tzdata.zi names and links; leap seconds
from leapseconds / leap-seconds.list / built-in IERS table), `time_zone`, `zoned_time`,
`tzdb_list`/`reload_tzdb`, the exceptions with the draft's messages, every formatter (full
chrono-format-spec, E/O, L through `time_put`), `local_time_format`, every stream inserter, and
`parse`/`from_stream` for every parsable type with every flag. `__cpp_lib_chrono` 202306L,
`__cpp_lib_chrono_udls` 201304L. Own suite chrono/ 26 -> 66/66, plus print/print_every_kind and
format/nonlocking_formatter_optimization (both compilers; clean under ASan and UBSan, Clang).
libc++ std/time 128 -> 377/386 (GCC), 128 -> 378/386 (Clang); the rest construct `leap_second`
or `time_zone_link` through libc++'s private test helpers. libstdc++ std/time +
20_util/{duration,duration_cast,time_point,time_point_cast} 43 -> 95/114 (GCC and Clang); the
other 19: `ext/typelist.h` or `std::__format` internals (7), names expected from `<chrono>`
without their headers (`<sstream>`, `printf`, `int64_t`: 4), and libstdc++ choices the draft
leaves open (8: `%OS` without fraction, LWG 4118 character reps, file_clock's epoch, rounding
when parsing, `fractional_width` of ratio<1, 2^62>, `hh_mm_ss` layout, an error message).

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
- Clang 23.1: no reflection (P2996) and no `__builtin_is_structural`, so `<meta>` is empty and
  `std::is_structural` is not declared (own tests `meta/*` XFAIL on Clang).
- GCC 16.2: `is_applicable_type`, `is_nothrow_applicable_type` and `apply_result` are not
  metafunctions ("unknown metafunction"); the library implements them through `substitute`.
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

- Clang 23.1: copy-list-initialization `f({T()})` with candidates `f(X)` (X(T)) and
  `f(atomic_ref<T>)` (explicit, deleted `atomic_ref(T&&)`) picks `f(X)`; GCC (and the libstdc++
  test 29_atomics/atomic_ref/ctor) treat the explicit constructor as a candidate and find the call
  ambiguous ([over.match.list]).

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
- `<regex>` ECMAScript follows ECMA-262 where libc++'s tests expect otherwise: `\a` is an
  identity escape ([re.grammar]/3: "SourceCharacter but not c"), a back-reference may precede its
  group, and `(a*)*` leaves group 1 unmatched after a rejected empty iteration. Under
  `match_prev_avail`, `^` matches at `first` only in multiline mode after a line terminator (the
  previous character exists, so `first` is not the beginning of the input), which keeps
  regex_iterator from matching `^a` at every position. Details in `tests/libcxx/skip.txt`.
- x87 `long double` `%a`: normal values print with a leading 1 (`1.8p+0`), subnormal ones as the
  C library does (`0x0.000000000000001p-16385` is the smallest), so that both forms agree there.

## Known limitations and draft defects
- `<optional>`: on Clang 23, `x != y` (and the reversed `y == x`) still compile through
  `operator==` when `*x != *y` is unusable: Clang forms rewritten candidates from a template
  `operator==` although a corresponding template `operator!=` exists ([over.match.oper]/4;
  GCC does not). libstdc++ relops/constrained.cc fails there.
- `std::start_lifetime` on GCC 16 (no `__builtin_is_within_lifetime`): in constant evaluation it
  re-creates an object that is already within its lifetime, so `__cpp_lib_start_lifetime` is
  defined only on Clang.
- `std::div`/`ldiv`/`lldiv` are constexpr templates (like `abs`): an unqualified call under
  `using namespace std;` picks the C library's `::div`, which is not constexpr.
- `<format>`: own tests `format/float_shortest_plain_style` (1e-4f) and `format/extended_float`
  (16777217.0f, float16 65504, bfloat16 256) expect fixed notation for values outside
  [charconv.to.chars]/7's [l, u): float(1e-4) is below 10^-4, and u is 1e7 (float), 1000
  (float16), 100 (bfloat16); libycxx follows the draft (as its own charconv tests do). The int
  `c` presentation accepts sign, # and 0 ([format.string.std]/5, /7, /8 make them valid for
  arithmetic types other than charT; libc++ rejects them) and ignores them. `fmt-iter-for<charT>`
  is format_context's iterator, so a formatter accepting only `format_context&` is formattable.
  Dynamic widths and precisions above INT_MAX are format errors (like written ones). The
  deprecated `visit_format_arg` is not provided. Non-UTF-8 ordinary literal encodings are
  detected but untested. print writes the whole formatted output with one `fwrite` after
  formatting it (no partial output on a format error); no terminal needs a native Unicode API
  on POSIX. The stack/queue/priority_queue and vector<bool>::reference formatters are defined
  in `<format>` (against declarations of the adaptors), so naming them needs `<format>`.
- `<chrono>`: names, `%c %x %X %r` and `%p` in parsing are the "C" locale's (the stream's
  `time_get` is not consulted); `%OS` without L keeps the fraction like `%S` (libc++'s reading;
  libstdc++'s tests expect whole seconds); `hh_mm_ss` of a period whose denominator needs more
  than 18 decimal digits has `fractional_width` 6 per [time.hms.members]/1 (libstdc++ gives 18 for
  ratio<1, 2^62>); `duration` inserters print character reps as the stream does (LWG 4118 is not
  in the draft); parsing into a coarser time point floors (libstdc++ rounds decaseconds); time
  points format with `enable_nonlocking_formatter_optimization` true for every Rep. `sys_info`'s
  `save` is derived (TZif has only an is-DST flag), `begin`/`end` of the first/last period are
  `sys_seconds::min()`/`max()`. Zone data comes from the zoneinfo directory only (no IANA source
  parsing, no download for `remote_version`/`reload_tzdb`).
- `<filesystem>`: `ycxx/hosted/file_clock.hpp` includes `<chrono>`'s clocks. The native encoding
  is assumed UTF-8 whatever the C locale; ill-formed UTF-8 converts to U+FFFD rather than
  throwing (libstdc++ u8path test02 expects an exception; unspecified by the draft). No
  root-names (`//host` is not special). `file_time_type` spans 1677-2262; setting
  `file_time_type::min()` succeeds where the file system clamps the time silently (libc++
  last_write_time test_write_min_time expects `value_too_large`). `directory_entry(p, ec)`
  clears the path on errors other than "not found" as [fs.dir.entry.cons]/2 says (libc++ test
  path_ctor_cannot_resolve expects it kept). `permissions(..., nofollow)` on a symbolic link
  fails with `ENOTSUP` on Linux. Permission-error tests need a non-root user.
- `<stacktrace>`: symbolization reads ELF objects only and
  needs the object file on disk: compressed debug sections, separate debug files
  (`.gnu_debuglink`, build-id directories) and split DWARF are not read (the queries return ""
  and 0; the function name then comes from the symbol tables). The demangler shows no
  requires-clauses. GCC's own codegen reports an exception escaping a contract predicate with
  `detection_mode::predicate_false` (it passes the unmodified violation object).
- `<text_encoding>`: comp-name assumes an ASCII-compatible ordinary literal encoding.
- `<meta>`: needs GCC 16 with `-freflection` (Clang 23 has no reflection). Exceptions the library
  itself raises (`access_context::via`, the apply traits) carry the library's source location in
  `where()`, not the caller's. `meta::exception::what()` is inherited from a base class (GCC 16
  requires member functions of a class holding an `info` to be consteval). With a non-UTF-8
  ordinary literal encoding, only ASCII messages transcode. Under hosted `-fno-rtti`,
  `std::exception`'s destructor is out of line (DECISIONS §4), so a `meta::exception` cannot be
  caught (destroyed) during constant evaluation there. `data_member_options::name-type` is
  spelled `name_type` and stores the name in the members GCC reads (`_M_is_u8`, `_M_u8s`, `_M_s`)
  rather than a `variant`.
- `<contracts>`: Clang 23 has no contracts (`-fcontracts` is unknown); the header only declares.
- `<regex>`: the backtracking matcher stops with `regex_error(error_stack)` beyond 4M stack frames
  (about 2M iterations of a quantified group, e.g. `(?:a|b)*` over 2M characters) and with
  `error_complexity` beyond a step budget that grows with the input (reached by exponential
  patterns with back-references, or with counted loops `(..){m,n}` or nullable-body loops with
  min 1 that the failure memo does not cover). POSIX patterns with back-references, or whose
  bounded repetitions expand beyond 256 copies or 65536 nodes, backtrack exhaustively (longest
  match; subexpressions in first-found order rather than by the POSIX rule). A combination of
  several grammar flags throws `regex_error(error_complexity)` (error_type has no code for it).
  Multi-character collating elements (`[[.ch.]]`) are not supported (no locale defines them).
  regex_traits::transform_primary returns the full sort key (the provided collate facets have no
  secondary weights) for collate and collate_byname facets alike; [re.traits]/7 would return an
  empty key for the classic locale's collate facet, making every `[[=x=]]` invalid.
- Iostreams/locale: named locales other than "C", "POSIX", "C.UTF-8" and "" throw
  `runtime_error` (the environment's conventions are not supported); `codecvt<wchar_t, char>`
  is UTF-8 in the classic locale, so `encoding()` is 0 and wide file streams cannot seek by an
  offset other than 0 (libc++ filebuf move/swap/seekoff wide cases and wchar_t encoding/max_length
  tests expect a single-byte C locale); long double hexfloat output is normalized (`0x1.…p+N`,
  not glibc's `0x9.…p+N`); `time_get` stops a number at the digit that leaves its range ("24" for
  %H reads "2"), as libstdc++ does and libc++ does not; `stringbuf` with `app` but not `ate`
  starts writing at the beginning ([stringbuf.members] init-buf-ptrs); bitmask types are
  enumerations, so `basic_stringbuf(s, 0)` does not compile; `basic_iostream`/`basic_istream`
  have no default constructor (libstdc++ extension); no `wstring_convert`/`wbuffer_convert`
  (removed in C++26), no `<codecvt>`; `fstream`'s path overloads are constrained templates.
  Standard stream objects synchronized with stdio write character by
  character through `putc` (bulk writes through `fwrite`).
- `<memory>`: no `atomic<shared_ptr<T>>` / `atomic<weak_ptr<T>>`, no execution-policy overloads of the specialized
  algorithms, no `pointer_tag_pair`. shared_ptr reference counts use
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
- `<system_error>`: `errc` has no `no_message_available`,
  `no_stream_resources`, `not_a_stream`, `stream_timeout` (removed from the draft; libstdc++'s
  `errc_std_c++0x.cc` still expects them). Messages are the C library's `strerror_r` text for
  both categories.
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
- `<random>`: the implementation-defined subsets of [rand.req.genl] are empty except that
  `generate_canonical` accepts the extended floating-point types and `__float128`; no `__int128`
  IntType/UIntType (libc++ int128 tests). `default_random_engine` is `mt19937` (libstdc++'s
  default_random_engine.cc expects minstd_rand0). `generate_canonical` follows the C++26 wording
  exactly, so libstdc++'s gencanon.cc / 64351.cc (which reject a rounded 1.0 and count extra
  calls) and libc++'s pre-P0952 generate_canonical test fail. seed_seq::generate rejects signed
  value types per its Mandates (libstdc++ seed_seq/97311.cc accepts them).
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

- `<vector>`: `pmr::vector` names the
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
- The shared container feature-test macros (`__cpp_lib_containers_ranges`, `__cpp_lib_erase_if`,
  `__cpp_lib_nonmember_container_access`, `__cpp_lib_incomplete_container_elements`,
  `__cpp_lib_node_extract`, `__cpp_lib_associative_heterogeneous_erasure`/`_insertion`,
  `__cpp_lib_map_lookup`, `__cpp_lib_map_try_emplace`) are defined now that every container
  provides the feature.
- `<deque>`, `<list>`, `<forward_list>`, `<stack>`, `<queue>` (core, constexpr): everything in the
  draft (the adaptors' formatters are defined with `<format>`, DECISIONS §11). Own suite: deque 14/17,
  list 16/19, forward_list 10/12, stack/queue/priority_queue 4/4 each, on both compilers (adaptors
  measured with a local stand-in `<vector>`); clean under ASan. Remaining: `range_kinds` need
  `views::iota`/`counted`, `pmr_alias` needs `<memory_resource>`, and `adl_robustness` cannot
  compile with any library: its `evil::Iter<Holder<Incomplete>*>` makes every operator call on the
  iterator instantiate `Holder<Incomplete>` through ADL (reduced case, both compilers).
  libstdc++ 23_containers/{deque,list,forward_list,stack,queue,priority_queue}: 0 -> 154/199 (GCC),
  152/199 (Clang); 175/173 with a stand-in `<vector>`; the rest need `<vector>`,
  `<memory_resource>`, `<scoped_allocator>`, iostreams, `__cpp_lib_erase_if` (defined once every
  container has erase_if), or test libstdc++ extensions (mismatched allocator value_type,
  assigning non-assignable elements, trivially copyable iterators, a non-constexpr `swap` overload).
- deque: blocks of about 1 KiB (a power of two, at least 16 elements) and a map of block pointers;
  emptied blocks are freed eagerly, an empty deque keeps one block until `shrink_to_fit`. A middle
  `insert`/`emplace` of one element or of n copies builds the value in allocator storage first
  (it may alias an element). list: at run time the sentinel is a member; during constant
  evaluation it is allocated with `std::allocator` (GCC 16 mis-evaluates pointers from heap nodes
  into an object returned with NRVO). Extensions: the adaptors' default constructors are
  constrained, a moved-from priority_queue is empty, `X(X&&, const A&)` is noexcept for
  always-equal allocators. `<queue>` includes `<vector>`, so it (and the include-graph and
  freestanding checks for it) needs `<vector>` to exist.
- `<map>`, `<set>` (core, constexpr): everything in [associative] including the C++26
  heterogeneous members (P2363), `lookup`, node handles (shared `ycxx/core/node_handle.hpp`),
  merge across comparators and unique/equivalent containers. One red-black tree
  (`ycxx/core/rb_tree.hpp`) with a header node (the run-time header is a member; during
  constant evaluation it is allocated, as for list) and cached leftmost/rightmost nodes; a correct
  hint costs O(1) comparisons, so construction from sorted input is linear. Unique-key insertion
  reads the key from the arguments when it is a key_type (or an arithmetic value for an
  arithmetic key) and allocates nothing for a present key; of several elements equivalent to a
  heterogeneous key, the first is found. Extensions: noexcept default construction, swap
  (nothrow-swappable comparator) and allocator-extended move (always-equal allocator); the move
  constructor copies the comparator. Element-wise moves of maps move the (const) key out of the
  source node. Own suite map + set: 39/39 on both compilers, clean under ASan. libc++
  containers/associative + container.node: 0 -> 344/347 (both); the rest: iterator_types expects
  `iterator::pointer` to be the allocator's pointer, deduct_const encodes the pre-C++23
  iter-mapped-type (no remove_cvref) and remove_const on initializer_list keys. libstdc++
  23_containers/{map,multimap,set,multiset}: 0 -> 153/162 (GCC), 149/162 (Clang); the rest:
  explicit_instantiation/3 and alloc_ptr_ignored (allocator of another value_type,
  testsuite_allocator.h), `<sstream>`, hetero/insert.cc counting libstdc++'s exact comparisons
  for hinted insertion, and (Clang) swap/1.cc declaring a non-constexpr `std::swap`
  specialization.
- `<flat_map>`, `<flat_set>` (core, constexpr): everything in [flat.map]/[flat.set]. Bulk
  insertion appends, then stably sorts the new rows through an index permutation and merges them
  in (`ycxx/core/flat_support.hpp`), linear for sorted input; flat_map moves keys and values
  through the same permutation. If any member exits via an exception the underlying containers
  are cleared (invariants restored, [flat.map.overview]/6); a moved-from adaptor is empty.
  flat_map iterators have proxy references (`pair<const Key&, T&>`), `iterator_category`
  random_access (as libc++ expects) and an arrow proxy as `pointer`. Extensions: operator[]
  is constrained on the try_emplace it is equivalent to; the hidden-friend operator<=> is a
  template so a key without `<` does not make the class ill-formed. Own suite flat_map + flat_set:
  27/27 on both, clean under ASan. libc++ container.adaptors/flat*: 0 -> 314/314 (Clang),
  312/314 (GCC: two size tests inserting 1000 elements into a deque exceed GCC's constexpr
  operation limit). The libstdc++ flat_* tests are skipped by the harness (they use
  `__gnu_test`).
- `<unordered_map>`, `<unordered_set>` (core, constexpr): everything in [unord] including the
  C++26 heterogeneous members (P2363), `lookup`, node handles (`ycxx/core/node_handle.hpp`, shared
  with the node-based associative containers) and merge across compatible containers. One hash
  table (`ycxx/core/hash_table.hpp`): separate chaining over a singly linked node list with
  cached hashes, power-of-two bucket counts with Fibonacci hashing, the before-begin node in the
  bucket array (no heap pointer into the container object). New equivalent elements go to the
  end of their group, or right after an equivalent hint; other hints are ignored. Own suite
  unordered_map + unordered_set: 20/31 on both compilers, 31/31 with stand-ins for the missing
  views, `std::erase` (the sequence harness) and `sorted_unique` (flat_map); clean under
  ASan/UBSan. On Clang `unordered_set/unord_reqs` exceeds Clang's default constexpr step limit (passes
  with `-fconstexpr-steps=2000000`; the test's own O(n^3) adjacency checks dominate).
  libstdc++ 23_containers/unordered_*: 0 -> 187/199 (GCC and Clang); the rest: 4
  explicit_instantiation/3 tests use an allocator of another value_type (Mandates violation),
  `__cpp_lib_erase_if` (left to the containers' integration), `<set>`, libstdc++'s
  `__is_fast_hash`, and 3 operations/1.cc cases expecting a heterogeneous key that matches
  several elements of a unique-key container ([unord.req.general]/10.20 excludes it) or
  libstdc++'s order of equivalent elements. libc++: 4 deduct tests expect `remove_const` on the
  Key deduced by the initializer_list guides (the draft deduces `const Key`), and
  iterator_difference_type expects `iterator::pointer` to be the allocator's pointer (it is
  `T*`). Extensions: the default constructor is constrained and noexcept when the hash,
  predicate and allocator are nothrow default constructible (it allocates nothing); the move
  constructors copy the hash and predicate (the moved-from container stays usable) and are
  noexcept when those copies are, with an allocator also when it always compares equal; the
  (first, last, alloc), (from_range, rg, alloc) and (il, alloc) constructors (LWG 2713) that the
  deduction guides already name. A non-copy-constructible Hash or Pred is diagnosed.
  Not defined yet (shared with `<map>`/`<set>`): `__cpp_lib_node_extract`,
  `__cpp_lib_associative_heterogeneous_erasure`/`_insertion`, `__cpp_lib_map_lookup`.
- `<mdspan>` (core, constexpr throughout; [views.multidim] of the current draft, including the
  C++26 names `extent_slice`, `range_slice`, `canonical_slices`, `subextents`, `dims`, the padded
  layouts, `aligned_accessor`, `at` and the mdspan `copy`/`fill`). Own suite mdspan + linalg:
  18/18 on both compilers, clean under ASan (Clang). libstdc++ 23_containers/mdspan: 0 -> 33/41 (GCC),
  25/41 (Clang), 5 unsupported; the rest: unqualified `uint8_t`/`uint16_t` (2), submdspan_mapping.cc
  slices an extent of 11 with `extent_slice{2, cw<7>, cw<2>}` (a precondition violation, diagnosed in
  constant evaluation), and on Clang test code GCC accepts (`Layout::mapping<E>` without `template`,
  a constexpr variable template without initializer) and constexpr step limits. Choices: pair-like
  slices are tuple-protocol types and aggregates of exactly two fields; the positivity checks of
  layout_stride strides and of a padding value are skipped for an empty index space (submdspan of
  an empty mdspan produces zero strides and paddings, [mdspan.sub.map.common]/6, a draft defect);
  the static padding stride is not stored; the standard mappings' `operator()` checks the index once
  for `mdspan::operator[]`. Strengthenings: `layout_left::mapping(const layout_stride::mapping&)` is
  noexcept (as layout_right's), `mdspan()` and the `is_[always_]*` members are noexcept when the
  mapping's are. No `_BitInt` index types.
- `<linalg>` (core; the norms and Givens rotations call libm at run time): everything in [linalg].
  Plain loops (no blocking, no vendor BLAS); not constexpr (the draft does not make it so); the
  ExecutionPolicy overloads run sequentially, are noexcept, and accept any cv/ref execution policy
  type. The 2-norms use a scaled sum of squares (no spurious overflow/underflow; NaN and infinity
  propagate). Draft questions: complex `setup_givens_rotation` returns r with the phase of a (LAPACK
  xLARTG); "r is the Euclidean norm" cannot hold with a real c unless a is real and nonnegative. The
  overloads taking a BinaryDivideOp exclude mdspan arguments, otherwise `triangular_matrix_vector_solve(A,
  t, d, b, x)` (and the matrix solves) would be ambiguous with the in-place overload. Neither
  external suite has linalg tests.
- `<simd>` (core; the `<cmath>` overloads call libm at run time): everything in [simd] of the
  current draft (P1928 with the C++26 follow-ups: `vec`/`mask`/`basic_vec`/`basic_mask`,
  `unchecked_`/`partial_` loads, stores, gathers and scatters, flags, static/dynamic/mask
  permutes, `chunk`/`cat`, `iota`, `vec<complex<T>>`, the `<bit>` and `<cmath>` overloads and their
  using-declarations in `std`), constexpr where the draft says; `__cpp_lib_simd`, `_bitops`,
  `_complex`, `_permutations`. Representation in DECISIONS §12 (vector-extension chunks sized by
  the enabled registers; codegen: `vec<float, 4>` `a * b` is one `mulps`). Own suite simd: 22/24
  on both compilers, also under ASan (Clang); the two failures are test defects: `iota` multiplies
  `vec<short, 9>` by the int literal 2, whose broadcast is not value-preserving, so no `operator*`
  is viable ([simd.ctor]/2.2); `compress_expand` expects `compress(mk, M(0b10101010u), false)[0]`
  false, but the first selected index is 1 and `mk[1]` is true ([simd.permute.mask]/3-4).
  libstdc++ std/simd (24 tests) is skipped: its harness uses libstdc++-internal names; libc++ has
  only the Parallelism TS's `experimental/simd`. Limitations: no implicit conversions to the
  compiler's vector types (recommended practice only); widths that are not powers of two are
  element arrays (no padded vectors); the mathematical, bit and complex functions other than
  `fabs`/`abs` apply the scalar function element by element; `uninit_element` gives `T()`.
- `<hive>` (core; only the constructors without elements and the limit queries are constexpr,
  as specified): element blocks with a 16-bit jump-counting skipfield and per-block free lists of
  erased runs, so insertion, erasure and iteration are O(1); blocks are numbered for O(1)
  iterator ordering. Hard limits [2, 65535]; default limits [8, min(8192, max(64, 1 MiB / slot))];
  limits outside the hard limits (erroneous) are diagnosed when hardened and clamped. A slot is
  at least 4 bytes (it holds a free run's links when empty). Emptied blocks stay as reserved
  blocks until `trim_capacity()`/`shrink_to_fit()`; `shrink_to_fit` only frees reserved blocks.
  `reshape` reallocates every element (in order) when an active block is outside the new limits.
  `sort` allocates two N-element arrays (pointers and a permutation). Own suite hive: 8/9 on both
  compilers (move_only needs views; 9/9 with a stand-in); clean under ASan/UBSan, as is a
  randomized model check. Neither external suite has hive tests.
- `<algorithm>`/`<numeric>`/`<execution>` (core): every std:: and ranges:: algorithm of the
  draft, constexpr where specified. The std:: ExecutionPolicy overloads run sequentially and
  are noexcept (an escaping exception calls terminate); so are the ranges:: ExecutionPolicy
  overloads (P3179, `algo_ranges_parallel.hpp`: each algorithm object's type adds them to the
  sequential niebloid), including the ranges:: uninitialized_*/destroy ones of `<memory>`;
  `__cpp_lib_parallel_algorithm` is 202506L. Not provided: the
  senders/receivers part of `<execution>`, `boyer_moore(_horspool)_searcher` (need hashing
  containers).
- stable_sort / stable_partition / inplace_merge take their buffer from `operator new(nothrow)`
  (std::allocator during constant evaluation) and fall back to O(N log^2 N) / O(N log N)
  rotation algorithms when it fails; sort and nth_element are introsort/introselect.
- `<ranges>` (core): every factory, adaptor and range utility of the draft, constexpr, in
  `ranges_adaptor.hpp` (closures, movable-box, caches), `ranges_factories.hpp`,
  `ranges_adaptors.hpp`, `ranges_zip.hpp`, `ranges_join.hpp`, `ranges_chunk.hpp` and
  `ranges_to.hpp`. libc++ std/ranges: 534/561 (Clang), 532/561 (GCC), was 21/46; the rest
  need missing headers (`<map>`, `<sstream>`, `<regex>`, `<istream>`) or are range-access CPO
  tests that predate possibly-const-range ([range.access.cbegin]); tests of exposition-only
  constructors and draft divergences are skipped. libstdc++ std/ranges: 39/49 on both, was 14
  (the rest missing headers). Notes:
  - `basic_istream_view` needs only the stream's interface: `basic_istream` is declared in
    core without default arguments and must be complete where the view is used.
  - Choices where the draft leaves room: IOTA-DIFF-T of the 64-bit types (and of `__int128`)
    is `__int128`; `cartesian_product_view` uses `__int128` as its difference type when it
    has two or more ranges; filter, drop, drop_while and reverse cache a random-access
    position as an offset. `iota_view::size()` is unsigned also for types narrower than `int`
    (the specified expression would promote to `int`), and never negates a minimum value.
  - The ranges:: parallel set_difference returns the loop's position in the second range when
    the output is complete; [set.difference]/4.3.1 counts "skipped" elements, which differs
    when elements of the second range lie between later elements of the first.
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

- Numerics (`<cmath>`, `<math.h>`, `<numbers>`, `<ratio>`, `<complex>`, `<valarray>`):
  own suite 70/71 on both compilers (complex/io needs `<sstream>`); libstdc++
  26_numerics + 20_util/ratio + special_functions 164/208 on both (was 13). Every `<cmath>` function
  is a template per floating-point type (extended types included), so the C library's `::sin` wins
  unqualified calls under `using namespace std` instead of being ambiguous. Constant evaluation
  follows Annex F: exact functions use soft code; transcendentals use the compiler's folding
  where it folds (GCC/MPFR) and otherwise correctly rounded multiprecision soft code; operations
  that raise invalid, divide-by-zero or overflow are not constant expressions. At run time the
  calls go to libm (linked by CMake and `tools/ycxx-cxx`). Hosted, the macros come from the C
  library's `<math.h>` (freestanding: `cmath_c_macros.hpp`, checked against glibc at build time);
  libycxx's `<math.h>` adds the global names except the special functions and lerp
  ([support.c.headers.other]/1). Special functions report domain errors as EDOM/FE_INVALID and
  return NaN. `<complex>` is constexpr throughout, with Annex G special values (the libc++
  `complex_times_complex`/`complex_divide_complex` constexpr stress tests exceed Clang's step
  limit); I/O is not provided yet (no streams). valarray evaluates eagerly (no expression templates).
  Remaining external failures: `<ctgmath>`/`<ccomplex>`/`<complex.h>` (not provided), libc++
  `cmath.pass` (expects overloads in the global namespace without `<math.h>`), `abs` of
  `_BitInt` (Clang), `numbers/value.pass` (expects the double value for long double),
  `polar(-0.0, θ)` (libc++ expects NaN; -0 is not negative, so libycxx computes it), the
  mask_array tests (call `std::count` without `<algorithm>`), libstdc++ `special_functions/*/compile_2`
  (global names `<math.h>` must not declare), `fabs(complex)` (extension), `complex/synopsis`
  (explicit specialisation declarations), `abs(__float128)` returning `__float128`, `::abs(long)`
  from `<stdlib.h>` (no `<stdlib.h>` wrapper), the valarray `mask-*_neg` tests (abort only with
  `_GLIBCXX_ASSERTIONS`; libycxx checks only under YCXX_HARDENED), and tests needing
  `<sstream>`/`<iostream>`/`<chrono>`/`<map>`/`<limits>`.

- Concurrency support (`<atomic>`, `<stdatomic.h>`, `<thread>`, `<stop_token>`, `<mutex>`,
  `<shared_mutex>`, `<condition_variable>`, `<semaphore>`, `<latch>`, `<barrier>`, `<future>`,
  `<rcu>`, `<hazard_pointer>`; DECISIONS §3): own suite atomic, thread, mutex,
  condition_variable, future, latch, barrier, semaphore, stop_token, ratio and
  memory_resource/synchronized_pool_threads 128/128 on both compilers,
  stable over repeated runs, clean under ASan and under TSan (Clang, with a runtime built with
  `-fsanitize=thread`: `YCXX_LIBDIR=build/clang-tsan SANITIZER=tsan`; `atomic/fences` is reported
  because TSan does not model fences). `<chrono>` is complete (see Time below). libc++
  atomics 5 -> 115/115 and thread 11 -> 334/338 (both compilers); libstdc++ 29_atomics 0 -> 82/82
  (GCC), 81/82 (Clang, compiler gap above) and 30_threads 0 -> 309/315 (both; the rest need
  `<iostream>`/`<sstream>`/`<format>` or utc_clock). Limitations: `notify_one` can wake more than
  one waiter when another waiter has registered but not yet blocked (a permitted spurious wakeup;
  libc++ `condvar/notify_one.pass` assumes none and can fail rarely); atomic wait slots are shared
  between addresses, so notify wakes every waiter of the slot; no `native_handle` for mutexes and
  condition variables; `notify_all_at_thread_exit` and the `*_at_thread_exit` results never run
  for the thread that ends the process; RCU has one domain, and `rcu_barrier` called from inside
  a scheduled evaluation returns without waiting (waiting would deadlock); retired hazard-pointer
  and RCU objects still pending at exit are not reclaimed. `atomic<T>` for a non-default-
  constructible T has a constrained (not mandated) default constructor. The deprecated atomics
  features are kept (DECISIONS §3).

## Open issues / next
- Every header of the C++26 library is provided (Phases 1-4 complete; `<meta>` needs GCC's
  `-freflection`, `<contracts>` GCC's `-fcontracts`). Own suite (1822 tests): GCC 1811 pass /
  10 fail / 1 xfail, Clang 1805 / 6 / 11. Every remaining failure is a documented limitation:
  `char_traits<char16_t>::eof`, the Itanium ABI handler limits (`except/handler_*`), GCC's
  contract detection mode, non-null constexpr `exception_ptr`, `std::mbstate_t` being core's own
  type, and no `<stddef.h>` wrapper.
- Next (Phase 5): full libc++/libstdc++ sweeps with triage (tests/libcxx/TRIAGE.md,
  tests/libstdcxx/TRIAGE.md), fixing the libycxx bugs they find; then performance and a
  whole-library review.
- libstdc++ triage (A) fixed (outcomes in tests/libstdcxx/TRIAGE.md): `<compare>` CPO noexcept
  and `compare_three_way`'s constraint (LWG 3530); `less<>` & co. no longer take a rewritten
  `operator<=>` for a built-in pointer comparison; `std::ignore` from `<utility>`; `<bitset>`
  includes `<string>`; `tuple t(func)`; `<optional>` (const T assignment, `optional<T&>::value_or`,
  corresponding `==`/`!=`, `format_kind<optional<T>>`, PR 117858/104606 recursion, Clang reset
  workaround); `any_cast<T>(any*)` for non-copyable T; `basic_const_iterator` PR 112490. New
  macros: `apply`, `tuple_like`, `constexpr_functional`, `result_of_sfinae`,
  `freestanding_{cstdlib,execution,functional,memory}`, `start_lifetime` (Clang only); `div`,
  `ldiv`, `lldiv` are constexpr and `memalignment` exists.
- Constexpr exceptions (P3068): done for `exception`, `bad_alloc`, `bad_array_new_length`,
  `bad_exception`, `bad_cast`, `bad_typeid`, `bad_optional_access`, `bad_variant_access`, and
  `bad_expected_access`. Those the library throws are thrown from headers through `raise_with`,
  so GCC can throw them during constant evaluation; Clang 23 cannot throw during constant
  evaluation at all. Under -fno-rtti the six classes libsupc++ defines keep an out-of-line
  destructor, so they are not constexpr-destructible there (DECISIONS §4).
  The nine `<stdexcept>` classes are constexpr too (DECISIONS §4; under hosted -fno-rtti they
  also keep out-of-line destructors), and the library's
  `throw_out_of_range`/`throw_length_error`/... throw them during constant evaluation, so on GCC
  `std::string("ab").at(5)` can be caught in a constant expression. `__cpp_lib_constexpr_exceptions`
  is still undefined: Clang 23 cannot throw during constant evaluation, a non-null
  `exception_ptr` is not available there on GCC. (`format_error` is constexpr.)
