# libycxx status

## Toolchain
| Compiler | Version | Notes |
|---|---|---|
| GCC | 16.2.0 (built from source, `/opt/gcc-16`) | latest release |
| Clang | 23.1.2 (apt.llvm.org) | latest release |

Conformance oracles (run only, never edited): libc++ tests from `llvmorg-23.1.2`
(`libcxx/test/std`), the libstdc++ testsuite from GCC 16.2.0, and our own spec-only suite
`tests/ycxx`.

## Conformance summary (full runs of 2026-10-05)
| Suite | GCC 16.2 | Clang 23.1 |
|---|---|---|
| Own suite `tests/ycxx` (2146 tests, `65e7235`) | 2131 pass / 10 fail / 5 xfail | 2124 pass / 6 fail / 16 xfail |
| libc++ `libcxx/test/std` (8543 tests, `7e6a93f`) | 7494 pass / 211 fail (209 + 2 unresolved: compile timeouts under load) / 836 unsupported (was 7439 / 532 raw) | 7495 pass / 215 fail / 832 unsupported (was 7440 / 536 raw) |
| libstdc++ testsuite (8555 tests; 2026-10-06, testsuite helpers and tests without `dg-do` running) | 6206 pass / 13 fail / 1 xfail / 2335 unsupported (was 4823 / 141 on 2026-10-05) | 6161 pass / 13 fail / 37 xfail / 2344 unsupported (projected from the full run with the final lists; was 4786 / 175) |
| Own suite against libstdc++ (reference, `tests/ycxx/REFERENCE.md`) | 1754 pass / 387 fail / 5 xfail | 1718 pass / 412 fail / 16 xfail |

Every libc++ and libstdc++ failure is categorised in `tests/libcxx/TRIAGE.md` and
`tests/libstdcxx/TRIAGE.md` (sections "Re-run of 2026-10-05"). With the skip entries added in that
round: libc++ 210 / 214 failures, libstdc++ 139 / 173. Open libycxx bugs (A) from them: `<bitset>`
does not include `<iosfwd>` ([bitset.syn]); plus the documented template-parameter name
`C` (DECISIONS §2, six libstdc++ tests). (`<vector>` now declares
`formatter<vector<bool>::reference>`, [vector.syn]; DECISIONS §11 "Formatters per header".) Most other failures are tests that rely on transitive
includes (F: 121 libc++, 79 libstdc++), libc++/libstdc++ specifics and pre-C++26 values (C), and
running as root (27 filesystem tests).

Counterparts of skipped tests: an external test skipped as implementation-specific, extension,
divergence or removed, or UNSUPPORTED for a library mode (libc++ hardening, warning-only verify,
experimental/; libstdc++ debug mode), ends its result with `covered by libycxx: tests/ycxx/...`
(own tests carry `// COUNTERPART:`, tests/ycxxlit/counterparts.py) or `no libycxx counterpart[:
reason]` (reasons: section "Skipped tests without a counterpart" of each TRIAGE.md); the suite
reports count both per category. All such libc++ tests are linked or triaged; of libstdc++'s
2633 extension skips, 2409 (testsuite-helper skips in std directories) are not triaged yet.

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
and wchar_t, the deprecated (Annex D) UTF-16/UTF-32 codecvts; named locales on the C library's
locales, glibc and Darwin's libc: every name `newlocale` accepts, with the `_byname` facets of char
and wchar_t built on its `locale_t`, `src/hosted/locale_named.cpp`, DECISIONS §7), the stream iterators, and the stream operators of `<string>`,
`<string_view>`, `<bitset>`, `<memory>`, `<system_error>` and `<complex>`. Own suite ios, iostreams,
sstream, fstream, spanstream, syncstream, iomanip, locale, complex, system_error, bitset, string,
string_view, memory, iterator: 301/305 (GCC, plus 1 XFAIL), 302/305 (Clang); the three failures need
`<filesystem>`, `<thread>`, `<format>`. Clean under ASan (Clang); iostreams/ and ios/ clean under TSan, including concurrent input on the synchronized standard objects (DECISIONS §7). libc++ input.output + localization
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
`<cstdio>` transitively or `-fno-char8_t`; some are skipped as implementation-specific (the
`visit_format_arg` tests run since Annex D is provided).
Header cost (GCC, `-fsyntax-only`): `<format>` 0.27 s, `<ostream>` 0.18 -> 0.26 s (its print
overloads need the core of `<format>`).
Formatters per header (DECISIONS §11): `<vector>`, `<stack>` and `<queue>` declare their formatters
and those of [format.formatter.spec]/2 (light `format_decl.hpp`); `<stacktrace>` sets
`enable_nonlocking_formatter_optimization` for its two formatters. Cost (preprocessed bytes,
median `-fsyntax-only`, GCC / Clang): `<vector>` 398 -> 410 KB, 108 -> 110 / 129 -> 130 ms;
`<stack>` 534 -> 548 KB, 134 -> 139 / 168 -> 171 ms; `<queue>` 742 -> 757 KB, 187 -> 193 /
234 -> 239 ms; `<format>` unchanged. Own tests format/formatter_header_* (9) and
formatter_include_* (3) pass on both compilers; libc++ utilities/format + containers 1824 -> 1825
(vector.bool.fmt/types.compile).
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
run on GCC (4 more need `testsuite_tr1.h`; the failure uses the removed `is_literal_type_v`, now skipped);
libc++ has no reflection tests.

Time (DECISIONS §14): `<chrono>` in full: the calendar (all types, `/` operators, arithmetic,
constants, literals), `hh_mm_ss`, the 12/24-hour functions (core, constexpr), utc/tai/gps clocks
with leap seconds, `clock_cast` (all five routes), the time zone database on the system's
zoneinfo (TZif v1-v4 with POSIX footer, lazily per zone; tzdata.zi names and links; leap seconds
from leapseconds / leap-seconds.list / built-in IERS table), `time_zone`, `zoned_time`,
`tzdb_list`/`reload_tzdb`, the exceptions with the draft's messages, every formatter (full
chrono-format-spec, E/O, L through `time_put`; `%j %U %W %V %G %g` of a calendar value that is
not a valid date throw `format_error`), `local_time_format`, every stream inserter, and
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

## Own-suite configurations (runs of 2026-10-05, 2438 tests)
`tools/test --hardened` / `--cxxflags=... --config-name=...` (README, Own tests); the nightly
`full.yml` runs them, and any failure fails the job. The hardened and noexcept rows predate the
senders' tests; execution/, stop_token/ and version/ pass in both (and under ASan+UBSan, and
TSan with a TSan-built runtime) on both compilers.

| Configuration | GCC 16.2 | Clang 23.1 |
|---|---|---|
| default (the `precondition/` death tests UNSUPPORTED; 2457 tests, 2026-10-06, with `<execution>`'s senders) | 2391 pass / 0 fail / 12 xfail / 54 unsupported | 2384 pass / 0 fail / 19 xfail / 54 unsupported |
| hardened (`-DYCXX_HARDENED=1`) | 2425 pass / 0 fail / 13 xfail | 2418 pass / 0 fail / 20 xfail |
| noexcept (`-fno-exceptions`; tests `REQUIRES: exceptions` UNSUPPORTED) | 1941 pass / 0 fail / 7 xfail / 490 unsupported | 1941 pass / 0 fail / 7 xfail / 490 unsupported |

Every expected failure carries its reason in the test (`// XFAIL:` for causes outside the library
and the test, `// XFAIL-COMPILER:` for a missing compiler feature): the draft defect
`char_traits/eof`, the Itanium ABI and GCC handler-recording limits (`except/handler_*`), GCC's
contract detection mode (`contracts/observe`), and on
Clang the features it lacks (constant-evaluation throws, contracts, reflection, builtins).

## Freestanding
`tools/check_freestanding.sh`: every core header, every header with a freestanding subset and
every header of [compliance]'s Table 27 compiles with `-ffreestanding -nostdlib -nostdinc
-fno-exceptions -fno-rtti`; the smoke test (which also uses the freestanding parts of `<cstdlib>`,
`<cstring>`, `<cwchar>`, `<cerrno>`, `<cstdarg>`, `<stdbit.h>`, `<system_error>`) links on
x86_64-unknown-none-elf and riscv64-unknown-elf (Clang) and x86_64 (GCC). Header lists: see
`tools/headers.py` (CORE, FREESTANDING_SUBSET, FREESTANDING_REQUIRED). The C headers' freestanding
subsets (DECISIONS §3) need from the environment only memcpy/memmove/memset/memcmp (as the
compilers do) and, when called, abort/atexit/at_quick_exit/exit/_Exit/quick_exit.
CMake builds the freestanding runtime archive for the compiler's target with
`-DYCXX_FREESTANDING_RUNTIME=ON` and installs it as `ycxx::freestanding`; `tests/cmake/run.sh`
links the smoke program with the installed archive.

## Hosted layers (DECISIONS §18)
`-DYCXX_PAL=none` with `YCXX_HOSTED_LAYERS` builds the hosted library from the integrator's
providers. The layers and their primitives (`include/ycxx/pal.h`) are `abort` (always), `memory`,
`console`, `clock`, `threads`, `random`, `files`, `environment`, `debug` and `clib` (the C
library). `YCXX_PAL=posix`, the default, is unchanged.

Status of the examples in `examples/hosted-layers` (`tests/cmake/run.sh`, part 11):

| Example | Layers | GCC 16.2 | Clang 23.1 |
|---|---|---|---|
| A, `host/`: a host program with its own heap, console and clock | abort memory console clock | runs | runs |
| B, `limine/`: a bare-metal x86_64 kernel, booted by Limine in QEMU 8.2 (TCG), exceptions included | abort memory console clock | boots, passes | boots, passes |
| C, `files/`: the file streams over the program's own RAM disk | clib memory files | runs | runs |

The absent-layer diagnostics are checked (`absent_*` targets): `std::thread` is a static_assert,
`sleep_for` and `random_device` are link errors naming `ycxx_pal_sleep_until` and
`ycxx_pal_random_open`, and `<fstream>` without `clib` is an `#error`.

Not yet done:

- `<filesystem>` and the time zone database have no primitives (POSIX only).
- `threads`, `files` and `debug` require `clib`: `thread.cpp` uses `<cfenv>`.
- macOS is untested; the CMake test runs part 11 on Linux only.

## macOS (Darwin)
Target: Apple Silicon (arm64) first, x86_64 kept in mind, with Homebrew GCC 16.2 and Clang 23.1
against Apple's SDK and libSystem (`tools/toolchain/provision`, `activate.sh`, `tools/ycxx-cxx`,
`cmake/ycxx-toolchain.cmake`). CI: job `macos` (macos-15, arm64), `tools/test -j3 policy build
freestanding ycxx`. **Nothing below has run on macOS yet**: it was checked on Linux and, for the
Darwin code paths, with Clang `-target arm64-apple-macos14` / `x86_64-apple-macos13
-fsyntax-only` against stand-in SDK declarations (`src/abi`, the PAL, `cmath_check.cpp`,
`<system_error>`, `<cmath>`; every core header and the freestanding runtime compile for both
Darwin targets). The first CI run is the real test.

Ported:
- C-library values core spells out, selected once in `config.hpp` (`YCXX_TARGET_DARWIN`,
  `cfg::darwin`): `errc` and the freestanding `<cerrno>` (BSD numbers, `ENODATA` 96), `FP_NAN` ..
  `FP_SUBNORMAL` (1-5), `FP_ILOGBNAN` (INT_MIN), `math_errhandling` (MATH_ERREXCEPT: that libm
  never sets errno; `cmath_check.cpp` compares the C library's only when it is a constant),
  `mbstate_t` (128 bytes, aligned to 8). The hosted checks against the C headers remain.
- C23 functions libSystem lacks (found by `cmake/ycxx-c-library.cmake`, not assumed), provided by
  the hosted runtime: `strfromd/f/l` (`src/hosted/strfrom.cpp`, on snprintf), `mbrtoc8`/`c8rtomb`,
  and the four char16_t/char32_t conversions (the SDK has no `<uchar.h>`; `src/hosted/uchar.cpp`,
  on mbrtowc/wcrtomb; libycxx's `<uchar.h>` places them in the global namespace),
  `timespec_getres` (`src/hosted/ctime.cpp`, TIME_UTC only, on clock_getres; global through
  libycxx's `<time.h>`). `_PRINTF_NAN_LEN_MAX` is the probe's measurement of libSystem's printf
  (3: it prints every NaN as `nan`). `atexit`/`at_quick_exit` are redeclared noexcept, and the
  const-correct `strchr` ... `wmemchr` pairs are libycxx's at global scope too.
- Static initialization: Mach-O has no init priorities, so `<iostream>` defines an
  `ios_base::Init` per translation unit there (DECISIONS §7); checked on Linux by building with
  `-U__ELF__`.
- ABI runtime: Apple's `<unwind.h>` (LSDA as uintptr_t; no text/data-relative bases, which its
  libunwind aborts in), Clang's Apple arm64 non-unique-RTTI bit (bit 63 of the name pointer),
  Mach-O assembler names in the freestanding `<cstdlib>`. Guards: Apple arm64 code tests bit 0 of
  the guard's first byte, which `__cxa_guard_release` sets. `thread_local` destructors: Clang
  calls `_tlv_atexit` itself; GCC (emulated TLS) calls `__cxa_thread_atexit`, which the PAL maps
  to `_tlv_atexit`.
- PAL: `nl_langinfo_l` from `<xlocale.h>`; address waits on `__ulock_wait`/`__ulock_wake` (they
  were a 50 µs poll); malloc's 16-byte alignment; `is_debugger_present` from sysctl's P_TRACED.
- `std::float128_t` `<cmath>` (GCC): Darwin's libm has no `*f128` functions
  (`cfg::c_math_float128`), so libycxx's soft implementations, which constant evaluation uses,
  compute it at run time too; `rint`/`nearbyint`/`lrint` read the rounding direction through
  `fegetround` (`src/hosted/cmath.cpp`). Checked on Linux by forcing that path (Annex F values,
  the four rounding directions and the inexact flag). libgcc has the binary128 arithmetic.
- `<cinttypes>`: C23's `PRIbN`/`SCNbN` (and LEAST, FAST, MAX, PTR), which Darwin's
  `<inttypes.h>` lacks, with that C library's length modifiers (checked against the types);
  `PRIBN` stay undefined ([cinttypes.syn]/2: only if fprintf supports `%B`).
- GCC -O3 arm64's maybe-uninitialized report in `fp_from_chars.cpp` removed at its source.
- Tests and tools: the whole-program tests re-execute themselves through `_NSGetExecutablePath`;
  `tools/ycxx-cxx` drops `-latomic` where the toolchain has no libatomic (libycxx needs none).

Apple's C++ runtime in the same process: libycxx's `__cxa_*`, `__gxx_personality_v0`, RTTI and
`operator new` come from its static archives and are bound inside the executable (two-level
namespace; the archives precede the implicit `-lSystem`), so system libraries that use Apple's
libc++abi keep theirs: two runtimes coexist, each with its own exception globals and handlers.
An exception thrown by Apple's runtime ("CLNGC++\0") is foreign to libycxx's (catch(...) only).
dyld coalesces exported weak definitions across images, a non-weak one winning: that rebound
libycxx's header-emitted definitions (`std::current_exception`, and with GCC the exception
classes' type_info and members) to libc++'s, and patched libycxx's `operator delete` into the
shared cache. Fixed by hidden visibility (DECISIONS §2): nothing of libycxx is exported, so no
weak-definition binds remain. GCC's fundamental type_info objects are hidden too (the list comes
from a configure-time probe of the compiler): on aarch64-apple-darwin GCC 16.2 emits 300 such
symbols, 150 of which (the SVE, `__bf16`, `__mfp8`, decimal and `_FloatN` forms) Apple's libc++abi
does not export, so tolerating them as "libc++abi's objects" was wrong. Expected in CI: `exception`, `except`, `rtti`, `future` pass on both
compilers apart from the documented `except/handler_pointer_reference{,_exact}` (both) and
`handler_array_decay`, `handler_function_pointer` (GCC) handler limitation;
`linkage/no_exported_library_symbols` passes;
`tests/cmake/run.sh` (not in CI) shows no exports and "mine 3 other 3" with Apple's libc++.

Unverified or known gaps on macOS:
- Darwin's C library predates C23 in places libycxx forwards to it: its printf has neither `%b`
  nor `%B` (macOS 26: `snprintf("%b", 5u)` gives "b"), so own test `cstdio/c23_conversions` fails
  there (a C library gap: libycxx's `<cstdio>` is the C library's printf) and `PRIBN` stay
  undefined ([cinttypes.syn]/2); whether its scanf has `%b` and its `strto*` the `0b` prefix is
  probed by `cinttypes/functions_macros` (a note when missing); its `strftime` `%z` gives the
  local offset for a `gmtime` result (own test `ctime/c23_functions` passes only with `TZ=UTC`,
  as in CI); its `iswctype` with `wctype("...")` may disagree with the `isw*` functions
  beyond ASCII under "C.UTF-8" (`cwctype/classification` notes the C library's disagreements).
- `<stacktrace>`: frames are captured (libSystem's `_Unwind_Backtrace`), but only `dladdr`
  names them (exported symbols only) and there are no file names or lines: the runtime reads ELF
  and DWARF, not Mach-O or dSYM bundles (`ycxx_pal_object_of` reports ENOSYS).
- `<cuchar>` fallback: assumes Darwin's conversion states use at most the first 16 of
  mbstate_t's 128 bytes; `mbsinit` does not see code units still to be delivered. macOS has no
  "C.UTF-8" locale, so the own test checks the UTF-8 forms only where the C library has one.
- Named locales (`src/hosted/locale_named.cpp`), written for Darwin but never run there; the macOS
  session must check: the `_l` functions come from `<xlocale.h>` (`is*_l`, `isw*_l`, `tow*_l`,
  `strcoll_l`, `strxfrm_l`, `wcscoll_l`, `wcsxfrm_l`, `strftime_l`, `wcsftime_l`,
  `nl_langinfo_l`) and `localeconv_l` is found by the `requires` probe (no lock then);
  `mbrtowc`/`wcrtomb`/`btowc`/`MB_CUR_MAX` follow the thread's `uselocale`; `catopen` with
  `NL_CAT_LOCALE` may read the global LC_MESSAGES rather than the thread's; the ctype<char> table
  of a UTF-8 locale gives bytes 0x80-0xFF no class (Darwin's `is*_l` would classify them as
  Latin-1); `strxfrm_l`/`wcsxfrm_l` sizes; `tm_zone`/`tm_gmtoff` reach `strftime_l` for `{:L%c}`
  ([time.format]); the own tests `locale/named_*` (UNSUPPORTED for a name the machine lacks:
  macOS has de_DE.UTF-8, fr_FR.ISO8859-15, en_US.UTF-8 but no C.UTF-8) and the chrono L
  conversions (`to_utf8` assumes wchar_t is Unicode only in ISO-8859-1 locales there).
- Possible test-environment differences: APFS is case-insensitive by default, `/tmp` and
  `$TMPDIR` lie behind symbolic links (`/private`), `statvfs` block counts are 32-bit; the zoneinfo
  tree has no `tzdata.zi`, so every TZif file counts as a zone and only symbolic links as links
  (should the tree use hard links or copies, `tzdb::links` is empty and `chrono/tzdb` fails).
- GCC on Darwin uses emulated TLS: `thread_local` destructors registered through `_tlv_atexit`
  rely on libSystem running them before emutls frees the thread's storage (key destructor order).
- Linking: CMake repeats the cyclic archive pair `libycxx.a`/`libycxx-abi.a`; Xcode 15's linker
  warns about duplicate libraries (harmless).

## Reference runs against libstdc++
`YCXX_STDLIB=libstdcxx tools/run-conformance ycxx gcc|clang` runs the own suite against GCC 16's
libstdc++. `tests/ycxx/REFERENCE.md` lists every failure: libstdc++ bugs (e.g. `variant::swap`
with a valueless operand, `numeric_limits<bool>::traps`, `function_ref` assignment), C++26 parts
libstdc++ 16 lacks, GCC/Clang differences, C-header gaps and ABI limits. No failure was traced to
a defect in a test.

## Known compiler gaps and bugs
- GCC 16.2, modules (`-fmodules`): one translation unit cannot both #include a standard header
  and `import std;`. Importing after an #include of some of the headers fails to read the module
  ("failed to read compiled module cluster N: Bad file data"; reduced: a module whose global
  module fragment has `<vector>` and `<string>`, imported after `#include <vector>`); an #include
  after the import redefines what the module made reachable ("redefinition of ...": textual
  merging after an import is not implemented, gcc.info "C++ Modules", reproduced without
  libycxx). Own tests `modules/include_then_import`, `modules/import_then_include` are XFAIL on
  GCC; separate translation units mix freely (`modules/mixed_translation_units`). Clang handles
  both orders.
- GCC 16.2, modules: a declaration of the C library's that a program redeclares differently
  before `import std;` (`extern "C" void abort();` without `noexcept`, as
  `tests/ycxx/support/check.hpp` does) is rejected ("conflicting 'noexcept' specifier for imported
  declaration"); module tests use `module_check.hpp`.
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
- GCC 16.2: `requires (void* p) { delete p; }` is satisfied (deleting `void*` is only a
  warning), so `shared_ptr<void>` is constructible from `void*` alone although
  [util.smartptr.shared.const]/3 requires `delete p` to be well-formed; own test
  `memory/shared_ptr_void_pointer` is XFAIL on GCC.
- GCC 16.2: `PR31384` (conversion function vs converting constructor in direct-init of `tuple`)
  resolves differently from Clang; the libc++ expectation matches Clang.

- GCC 16.2 and Clang 23.1: neither implements [expr.new]/20.2 (retrying the allocation
  function lookup with an added `align_val_t` argument for an over-aligned type); own test
  `new/class_aligned_lookup_added_alignment` is XFAIL on both.
- GCC 16.2: a `&&`/`const&&`-qualified member coroutine gets `S&` as the object parameter type
  for `coroutine_traits`; own test `coroutine/traits_object_parameter_rvalue` is XFAIL on GCC.
- Clang 23.1: copy-list-initialization `f({T()})` with candidates `f(X)` (X(T)) and
  `f(atomic_ref<T>)` (explicit, deleted `atomic_ref(T&&)`) picks `f(X)`; GCC (and the libstdc++
  test 29_atomics/atomic_ref/ctor) treat the explicit constructor as a candidate and find the call
  ambiguous ([over.match.list]).

- Clang 23.1 (also Apple clang 21) on Darwin: an `inline thread_local` variable with dynamic
  initialization and hidden visibility (from `-fvisibility=hidden` or from its type's visibility,
  so every such variable of a libycxx class type, DECISIONS §2) gets its TLS init function
  `_ZTH<name>` as a strong private external symbol: the IR has a `linkonce_odr hidden alias` to the
  TU's `__tls_init`, which the Mach-O backend emits without the weak bit (with default visibility
  it is a local symbol; ELF targets emit it weak). Two TUs defining the variable fail to link with
  "duplicate symbol 'thread-local initialization routine for ...'". GCC 16.2 (weak `_ZTH`) links.
  Repro without libycxx: `s.h`: `struct S { S(); int v; }; inline thread_local S t;`; `a.cpp`:
  `#include "s.h"` `S::S() : v(1) {} int* f() { return &t.v; }`; `b.cpp`: `#include "s.h"`
  `int* f(); int main() { return f() != &t.v; }`; `clang++ -fvisibility=hidden a.cpp b.cpp`.
  Own test `linkage/odr_inline_entities` is XFAIL on Clang on Darwin (`XFAIL: clang-darwin`).
- Clang 23.1: the type_info name string of a class with internal linkage is emitted without the
  leading `*` (GCC emits `*N12_GLOBAL__N_1...`) that tells the Itanium runtime to compare
  type_info objects by address, so same-named unnamed-namespace classes of different translation
  units compare equal and match each other's handlers (with libstdc++ too); own test
  `except/handler_internal_linkage_types` is XFAIL on Clang.

## Deliberate omissions
Removed features are not implemented (`auto_ptr`, `result_of`, `is_literal_type`,
`random_shuffle`, `<strstream>`, `<codecvt>`/`wstring_convert`, the `shared_ptr` atomic free
functions, ...; `removed` in `tests/SKIPPED.md`).

## Annex D (deprecated features)
Every deprecated library feature the current draft still specifies (Annex D, [depr]) is provided,
and every Annex D entity carries `[[deprecated("...")]]` with a short hint where the language
allows the attribute (DECISIONS §6); own tests `depr/` check each one (`-Werror=deprecated-
declarations`) and that the non-deprecated neighbours stay silent:
[depr.numeric.limits.has.denorm] `float_denorm_style` and its enumerators, `has_denorm`,
`has_denorm_loss`; [depr.cerrno] the `errc` enumerators `no_message_available`,
`no_stream_resources`, `not_a_stream`, `stream_timeout`; [depr.meta.types] `is_trivial(_v)`,
`is_pod(_v)`, `aligned_storage(_t)`, `aligned_union(_t)` (`type_traits_depr.hpp`); [depr.relops]
`std::rel_ops`; [depr.tuple] and [depr.variant] `tuple_size`/`tuple_element`/`variant_size`/
`variant_alternative` of `volatile` and `const volatile` T; [depr.vector.bool.swap] static
`vector<bool>::swap(reference, reference)`; [depr.iterator] `std::iterator`; [depr.move.iter.elem]
`move_iterator::operator->`; [depr.locale.category] `codecvt` and `codecvt_byname` for
char16_t/char32_t with char/char8_t; [depr.format.arg] `visit_format_arg`; [depr.ctime]
`asctime`/`ctime` (`<ctime>` redeclares the C library's functions with the attribute, so
`::asctime` after `<ctime>` warns too, as in C23); [depr.fs.path.factory] `u8path`,
[depr.fs.path.obs] `path::string()`/`generic_string()`; [depr.atomics] `memory_order::consume`,
`memory_order_consume`, `kill_dependency`, `atomic_init`, and the volatile members of `atomic<T>`
for a T that is not always lock-free (each is a separate, deprecated overload constrained on
`!is_always_lock_free`; the lock-free ones are not deprecated; the volatile `store_key` members are
constrained on `is_always_lock_free` as [atomics.types.int] requires). The volatile non-member
functions ([atomics.nonmembers]: they call the member) are split the same way, so
`atomic_load(volatile atomic<Big>*)` warns too; [depr.istream.extractors]/
[depr.ostream.inserters] the `signed char`/`unsigned char` stream operators.
Not marked, because the attribute cannot apply to a macro: `FLT_HAS_SUBNORM`, `DBL_HAS_SUBNORM`,
`LDBL_HAS_SUBNORM`, `DECIMAL_DIG` ([depr.c.macros], `<cfloat>`), `INFINITY`/`NAN` from `<cmath>`,
`__bool_true_false_are_defined` (the compiler's `<stdbool.h>`), `ENODATA`/`ENOSR`/`ENOSTR`/
`ETIME` ([depr.cerrno], `<cerrno>`) and `ATOMIC_VAR_INIT` ([depr.atomics.types.operations]).
Compiler gap: GCC 16.2 ignores `[[deprecated]]` on a class template partial specialization (Clang
23.1 honours it). The `volatile` forms of `tuple_size`/`tuple_element`/`variant_size`/
`variant_alternative` therefore also mark their `value`/`type` member: on GCC a direct
`tuple_size<volatile T>::value` warns, but `tuple_size_v<volatile T>`/`tuple_element_t<I,
volatile T>` (which name the member inside the library's system header) do not.
libc++'s `*.deprecated.verify.cpp` tests check only warnings and are reported unsupported by the
harness (`infrastructure`). Formerly skipped as `deprecated`, now run: libc++ 21 of 22 pass (both
compilers; `visit_format_arg.pass.cpp` needs `EOF` from `constexpr_char_traits.h`); libstdc++ 0 ->
28 pass (both); the rest are reclassified (`removed`, `pre-c++26`, `extension`,
`implementation-specific`) in the skip lists.

## Deliberate divergences
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
- `num_get` into an unsigned type converts a negative field as strtoull does
  ([facet.num.get.virtuals] Stage 3): "-1" is ULLONG_MAX, so `unsigned int`/`unsigned short`
  get their maximum and failbit, a 64-bit type ULLONG_MAX without failbit. libc++'s tests negate
  in val's type (UINT_MAX without failbit); libstdc++ reads "-4294967295" as 1.
- Floating-point `from_chars` leaves the value unmodified on `result_out_of_range` (overflow, or a
  nonzero value that rounds to zero), as the draft says; MSVC stores +-inf or +-0.
- `<regex>` ECMAScript follows ECMA-262 where libc++'s tests expect otherwise: `\a` is an
  identity escape ([re.grammar]/3: "SourceCharacter but not c"), a back-reference may precede its
  group, and `(a*)*` leaves group 1 unmatched after a rejected empty iteration. Under
  `match_prev_avail`, `^` matches at `first` only in multiline mode after a line terminator (the
  previous character exists, so `first` is not the beginning of the input), which keeps
  regex_iterator from matching `^a` at every position. Details in `tests/libcxx/skip.txt`.
- `num_put::do_put(bool)` with `boolalpha` pads the name to `width()` (and resets the width) as
  the other conversions do; [facet.num.put.virtuals]/6 read literally inserts the name unpadded.
  libc++ and libstdc++ pad, and their tests expect it. Likewise a character-sequence inserter
  whose output fails sets `badbit` (as `write()` and the arithmetic inserters do), where
  [ostream.formatted.reqmts]/1 says `failbit`; `fail()` is true either way.
- x87 `long double` `%a`: normal values print with a leading 1 (`1.8p+0`), subnormal ones as the
  C library does (`0x0.000000000000001p-16385` is the smallest), so that both forms agree there.

## Known limitations and draft defects
- Reserved names (DECISIONS §2): the headers spell every name of their own as a reserved
  identifier, so a program may `#define` any name the standard library does not declare
  (`tests/ycxx/conformance/nasty_macros*`, 5874 such macros; `tools/uglify.py --check` in the
  policy stage). The standard names come from the draft's index of library names (a snapshot,
  `tools/uglify.py --fetch-index`), the std modules' export lists and a hand-kept list of the
  names the index misses (`tools/data/uglify/allowed.txt`); a standard name missing from all
  three is renamed, harmlessly. Not covered: 17 names that glibc's own headers break on when
  they are macros (`f`, `l`, `y0`, `link`, ...; `tools/data/uglify/nasty-macros.txt`). The PAL
  (`__ycxx_pal_*`) and the allocation table (`__ycxx_allocation_functions`) changed their
  symbol names; a port's PAL implements the reserved names.
- Modules (`import std;`, `import std.compat;`; DECISIONS §16): built per project from
  `modules/*.cppm` (CMake `ycxx::modules`, `tools/ycxx-modules`), never shipped as BMIs. CMake's
  `CMAKE_CXX_MODULE_STD` is not supported (needs CMake >= 3.30, and would build the toolchain's
  library's module; CMake here is 3.28). The export lists are generated on Linux/glibc; on Darwin
  the modules are untested (std.compat's global C names may differ there). The implementation's
  inline namespace `std::ranges::__cpo` (and `std::__cpo`) is visible to importers (the CPOs must be
  exported from it, DECISIONS §16). GCC: see known compiler gaps (no #include and import of the
  library in one translation unit). `<bits/stdc++.h>` exists (every header) because GCC's
  `-fmodules` looks it up for every standard #include.
- `submdspan` of a `layout_stride` (or non-unit-stride) mapping: [mdspan.sub.map.common]/6 builds
  a `layout_stride::mapping` whose strides need not meet [mdspan.layout.stride.cons]/4.3, although
  the layout is unique: that condition is sufficient, not necessary, despite its Note (extents
  {2, 4} with strides {6, 9}, the slice `extent_slice{0, 2, 3}, full_extent` of {4, 4} with
  {2, 9}, is unique and fails it). libycxx constructs submdspan results without that check (the
  other preconditions are still checked in hardened builds); a draft defect to report.
- Hidden visibility (DECISIONS §2): a program or shared library exports none of libycxx's
  symbols; its images share their default allocation functions through the allocation table
  `ycxx_allocation_functions` (DECISIONS §2), kept in a program by link options the CMake
  package and `tools/ycxx-cxx` add (`-u`, and `--export-dynamic-symbol` on ELF); other build
  systems add them themselves (`<build>/ycxx-link-options` lists them). Without them, a program
  that references no default allocation function lacks the table, and its replacements do not
  reach the libycxx shared libraries it loads. **GCC warns** (`-Wattributes`: "'S' declared with greater visibility than the type of
  its field" / "than its base") for every program class outside libycxx's namespaces with a
  member or base of a library class type (`struct S { std::string s; };`, a class derived from
  `std::runtime_error`); GCC has no way to hide a class's members and type_info without hiding
  its type. The CMake package and `tools/ycxx-cxx` pass `-Wno-attributes` to GCC; other build
  systems add it themselves. Clang does not warn. Images that each link
  libycxx have separate runtimes: exceptions cross between them, but `uncaught_exceptions()` in
  one does not count the other's exception while it unwinds through its frames, and each has its
  own `generic_category()`/`system_category()` objects, so an `error_code` made in one compares
  unequal to an `errc` or category of the other (`value()` and `category().name()` agree).
  The same holds for the other library singletons (`locate_zone` results, the default memory
  resources), and a program's replacement `operator new`/`delete` replaces the program's own:
  allocations made inside a shared library that links libycxx use that library's copy (both reach
  `malloc`/`free`, so objects may still be deleted in the other image).
- C library wrappers: `std::free_sized`/`free_aligned_sized` call `free` (glibc 2.39 has neither);
  `memset_explicit` is memset plus a compiler barrier; `strfrom*`, `memccpy`, `strdup`, `strndup`
  are the C library's (on Darwin, which lacks them, `strfrom*` and `mbrtoc8`/`c8rtomb` are
  libycxx's own, and so is `timespec_getres`; see "macOS (Darwin)"). `<ctime>` has C23's `timegm`,
  `gmtime_r`, `localtime_r` and `timespec_getres` from the C library. `abs`, `labs`, `llabs`,
  `div`, `ldiv`, `lldiv`, `bsearch` (the const/non-const pair), `imaxabs` and `imaxdiv` are
  libycxx's own in both namespaces: `<cstdlib>`/`<cinttypes>` hide the C library's declarations
  (DECISIONS §3). `<stdlib.h>`, `<inttypes.h>`, `<string.h>`, `<complex.h>` and `<tgmath.h>` are
  libycxx's (the global names of [support.c.headers.other]; `<complex.h>`/`<tgmath.h>` are
  `<complex>`/`<cmath>` in C++). Freestanding (`-ffreestanding`), `<cstdlib>`/`<cstring>`/`<cwchar>`
  are libycxx's own code, and the termination functions forward to the environment's.
- `make_exception_ptr` under `-fno-exceptions -fno-rtti` returns a null exception_ptr (the
  object's type_info cannot be named; DECISIONS §4). With RTTI it works without exceptions.
- `recursive_directory_iterator` with `follow_directory_symlink` opens a followed symbolic link by
  its whole path, so in a loop it reports ELOOP once the path holds more links than the system
  resolves (40 on Linux), and a followed link deeper than PATH_MAX reports ENAMETOOLONG.
- Own test `bit/oracle_cxx26` (Clang only): its `static_assert(all8_shifts())` needs 4-8 million
  constant-evaluation steps, beyond Clang's default `-fconstexpr-steps` (1,048,576); the oracle
  alone needs more than 2 million (test defect; the run-time checks pass, GCC passes).
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
  Widths and precisions have no upper bound (written or dynamic; beyond size_t they saturate):
  `formatted_size`/`format_to_n` count huge padding and floating-point zeros without writing
  them, `format` throws `bad_alloc` when the result cannot be held. The
  deprecated `visit_format_arg` is provided ([[deprecated]]). Non-UTF-8 ordinary literal encodings are
  detected but untested. print writes the whole formatted output with one `fwrite` after
  formatting it (no partial output on a format error); no terminal needs a native Unicode API
  on POSIX. `<vector>`, `<stack>`, `<queue>` (and the other headers declaring a formatter) provide
  the formatters of [format.formatter.spec]/2 and their own as class layouts (DECISIONS §11);
  `formattable` is false until `<format>` defines the contexts, so with `<stack>`/`<queue>` alone
  the adaptor formatters are disabled even for a program-defined formattable container (they also
  need the range formatter of `<format>`), and a program that checks `formattable` before including
  `<format>` and again after it is ill-formed, no diagnostic required ([temp.constr.atomic]/3:
  GCC reports the changed satisfaction value, Clang keeps the first answer).
- `<chrono>`: names, `%c %x %X %r` and `%p` in parsing are the "C" locale's (the stream's
  `time_get` is not consulted); with L, a locale whose `time_put` is not the classic facet writes
  `%c %x %X` etc. from a C `tm` (so its `%Y` there is `strftime`'s, unpadded, and the hours of a
  duration are passed as is up to INT_MAX), while the classic facet's conventions are built in
  (`{:L%c}` equals `{:%c}` for the "C" locale); the duration count of `{:L}` is grouped from the
  locale's `numpunct` (a replaced `num_put` is not called); `%OS` without L keeps the fraction like `%S` (libc++'s reading;
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
  requires-clauses.
- `<contracts>` (GCC): an exception escaping a contract predicate is reported with
  `detection_mode::predicate_false`, where [basic.contract.eval]/7.2 and Table 46 call for
  `evaluation_exception` (own test `contracts/observe`, XFAIL). GCC emits one constant violation
  object per assertion (mode 1, `.data`) and passes the same object from the normal path and from
  the implicit handler of the exception path. The library cannot recover the mode:
  `current_exception()` is non-null in the handler in both cases when the assertion is evaluated
  inside an active handler, and nothing marks the start of the predicate's evaluation.
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
  Groups nested more than 1000 deep throw `regex_error(error_space)` (the translator and the
  matchers recurse over the tree).
  Multi-character collating elements (`[[.ch.]]`) are not supported (no locale defines them).
  regex_traits::transform_primary returns the full sort key (the provided collate facets have no
  secondary weights) for collate and collate_byname facets alike; [re.traits]/7 would return an
  empty key for the classic locale's collate facet, making every `[[=x=]]` invalid.
- Iostreams/locale: named locales are the C library's (DECISIONS §7): a name the C library
  lacks throws `runtime_error`, and tests that need one are UNSUPPORTED (`tests/ycxxlit/locales.py`;
  `tools/ci/gen-locales` generates the suites' names on glibc). Where the draft leaves a choice,
  libycxx's differs from libc++'s in places its tests check (skip.txt, "Named locales"): money
  patterns keep the C library's `curr_symbol` and put the separating space in the pattern
  (libstdc++'s choice); `money_put` without `showbase` then writes that space; a numpunct
  separator that is not one char (fr_FR.UTF-8's U+202F) is `' '` for char; `time_get` of a named
  locale reads its `%x`/`%c`/`%X`/`%r` formats strictly (no libc++-style separator leniency);
  the base `time_get`/`time_put` facets are the "C" locale's whatever the stream's locale (only
  the `_byname` facets read a named locale; libstdc++'s base facets consult the stream's);
  the classic `moneypunct::negative_sign()` is "-" (libstdc++'s tests expect the C locale's "");
  `messages` has no gettext extension (`catopen`/`catgets` catalogs only); `codecvt::encoding()`
  of a named locale does not detect state-dependent encodings (0); a composite name lists the six
  standard categories, not glibc's twelve. `codecvt<wchar_t, char>` is UTF-8 in the classic locale, so `encoding()` is 0 and wide file streams cannot seek by an
  offset other than 0 (libc++ filebuf move/swap/seekoff wide cases and wchar_t encoding/max_length
  tests expect a single-byte C locale); long double hexfloat output is normalized (`0x1.…p+N`,
  not glibc's `0x9.…p+N`); `time_get` stops a number at the digit that leaves its range ("24" for
  %H reads "2"), as libstdc++ does and libc++ does not; `stringbuf` with `app` but not `ate`
  starts writing at the beginning ([stringbuf.members] init-buf-ptrs); bitmask types are
  enumerations, so `basic_stringbuf(s, 0)` does not compile; `basic_iostream`/`basic_istream`
  have no default constructor (libstdc++ extension); no `wstring_convert`/`wbuffer_convert`
  (removed in C++26), no `<codecvt>`; `fstream`'s path overloads are constrained templates.
  Standard stream objects synchronized with stdio write character by
  character through `putc` (bulk writes through `fwrite`); the wide ones do wide I/O on the C
  streams (`fputws`, `fgetwc`, `ungetwc`; DECISIONS §7), so the C library converts with its
  `LC_CTYPE`: before `setlocale`, `wcout << L"\u00e9"` fails (badbit) as `fputwc` does on glibc,
  unless a codecvt facet of the program's own is imbued. The UTF-16 codecvts' `out` takes a
  high surrogate into the state (so out(from, from + 1) succeeds, as [locale.codecvt.virtuals]/4
  requires of a filebuf facet); `unshift` reports `error` while one is pending. libstdc++'s
  `codecvt_unicode.h` expects `partial` with from_next before it instead; `money_get` with
  frac_digits() > 0 accepts a value without a decimal point as the digits that appear ("1056"),
  but a decimal point must be followed by exactly frac_digits() digits.
- `<memory>`: no `pointer_tag_pair`. `atomic<shared_ptr<T>>` / `atomic<weak_ptr<T>>` are
  lock-based (the striped lock table of `<atomic>`); the execution-policy overloads of the
  specialized algorithms run sequentially. shared_ptr reference counts are plain while the
  process has one thread (DECISIONS §15), atomic otherwise. get_deleter identifies
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
- The searching functions (`strchr` ... `wmemchr`) are libycxx's const/non-const pairs in both
  namespaces on every C library: the C library's declarations are renamed while its header is
  read (DECISIONS §3), so a C header read before libycxx's (bypassing its include directory)
  leaves the C signature in place.
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
- Non-null `exception_ptr`s during constant evaluation exist on GCC only, through GCC 16's
  undocumented builtins `__builtin_current_exception`/`__builtin_eh_ptr_adjust_ref` (found with
  `__has_builtin`, DECISIONS §4); Clang 23 cannot throw during constant evaluation at all.
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
  `abi::__forced_unwind`). Hierarchy walks (handler matching, `dynamic_cast`) remember up to 64
  visited virtual bases; a hierarchy with more falls back to walking repeated paths again.
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
  inplace_vector's copy operations when T's are. fill, find and count (std:: and ranges::, a
  bool value, no projection) work a word at a time on vector<bool>'s iterators
  (`ycxx/core/bit_iter_algos.hpp`).
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
  constrained, a moved-from priority_queue is empty (when its container has `clear()`), `X(X&&, const A&)` is noexcept for
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
  25/41 (Clang), 5 unsupported; the rest: unqualified `uint8_t`/`uint16_t` (2; padded.cc also passes a
  padding of 0, canonical_slices.cc a constant stride of 0: tests/libstdcxx/TRIAGE.md), submdspan_mapping.cc
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
  `__cpp_lib_parallel_algorithm` is 202506L. The senders/receivers part: next entry.
- `<execution>` senders and receivers ([exec], core; DECISIONS §17): queries and environments
  (`forwarding_query`, `get_allocator`, `get_stop_token`, `get_env`, `get_domain`,
  `get_scheduler`, `get_start_scheduler`, `get_delegation_scheduler`,
  `get_forward_progress_guarantee`, `get_completion_scheduler`, `get_completion_domain`,
  `get_await_completion_adaptor`, `prop`, `env`), receivers, operation states, completion
  signatures, the sender and scheduler concepts, `default_domain`/`indeterminate_domain`,
  `transform_sender`, `apply_sender`, `get_completion_signatures`, `connect` (awaitables too);
  factories `just`, `just_error`, `just_stopped`, `read_env`, `schedule`; adaptors `write_env`,
  `unstoppable`, `then`, `upon_error`, `upon_stopped`, `let_value`, `let_error`, `let_stopped`,
  `bulk`, `bulk_chunked`, `bulk_unchunked`, `when_all`, `when_all_with_variant`, `into_variant`,
  `stopped_as_optional`, `stopped_as_error`, `schedule_from`, `continues_on`, `starts_on`, `on`,
  `affine`, `associate`, `spawn_future`, the pipe syntax (`sender_adaptor_closure`); consumers
  `this_thread::sync_wait`, `sync_wait_with_variant`, `spawn`; `run_loop`, `inline_scheduler`,
  `as_awaitable`, `with_awaitable_senders`, `simple_counting_scope`, `counting_scope`, `task`,
  `task_scheduler`, `with_error`, `parallel_scheduler` with the `parallel_scheduler_replacement`
  interface and a thread-pool backend in the hosted runtime. `__cpp_lib_senders`,
  `__cpp_lib_counting_scope`, `__cpp_lib_task` 202506L, `__cpp_lib_parallel_scheduler` 202506L
  (hosted). Own suite execution: every test passes on both compilers, also under ASan+UBSan and
  (the threaded ones) TSan. Known limitations: `split`/`ensure_started` are not in the draft
  (P3682) and not provided; `tag_of_t` recognises tuple-like senders only; task_scheduler
  allocates its backend at every construction; when_all and let report no completion
  scheduler/domain; the draft questions of DECISIONS §17.
- `boyer_moore_searcher`/`boyer_moore_horspool_searcher` (`ycxx/core/searcher.hpp`): bad-character
  table (a 256-entry array for byte-sized integers compared with `equal_to`, otherwise a hash table of
  the pattern's equivalence classes that calls pred only on equal hash values), plus the good-suffix
  table for Boyer-Moore; the tables are heap arrays, deep-copied with the searcher. Every alignment
  makes at most m predicate calls, lookups included, so [func.search.bm]/8 holds for any hash. A
  match ending at `last` is found ([func.search.bm]/7.1 says `[first, last - m)`, which excludes it:
  a draft defect).
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
  return NaN. `<complex>` is constexpr throughout, with Annex G special values (kept cheap
  in constant evaluation: the libc++ `complex_times_complex`/`complex_divide_complex` stress
  tests fit Clang's limit); I/O is not provided yet (no streams). valarray evaluates eagerly (no expression templates).
  Remaining external failures: `<ctgmath>`/`<ccomplex>` (removed from the draft), libc++
  `cmath.pass` (expects overloads in the global namespace without `<math.h>`), `abs` of
  `_BitInt` (Clang), `numbers/value.pass` (expects the double value for long double),
  `polar(-0.0, θ)` (libc++ expects NaN; -0 is not negative, so libycxx computes it), the
  mask_array tests (call `std::count` without `<algorithm>`), libstdc++ `special_functions/*/compile_2`
  (global names `<math.h>` must not declare), `fabs(complex)` (extension), `complex/synopsis`
  (explicit specialisation declarations), `abs(__float128)` returning `__float128`, the valarray `mask-*_neg` tests (abort only with
  `_GLIBCXX_ASSERTIONS`; libycxx checks only under YCXX_HARDENED), and tests needing
  `<sstream>`/`<iostream>`/`<chrono>`/`<map>`/`<limits>`.

- Concurrency support (`<atomic>`, `<stdatomic.h>`, `<thread>`, `<stop_token>` (core since the
  senders: DECISIONS §3), `<mutex>`,
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
  a scheduled evaluation returns without waiting (waiting would deadlock); `rcu_barrier` inside
  a region does not wait for objects retired after the region began (they cannot be reclaimed
  before it ends) and runs the deleters it evaluates inside that region (such a deleter must not
  call `rcu_synchronize`); retired hazard-pointer
  and RCU objects still pending at exit are not reclaimed. `atomic<T>` for a non-default-
  constructible T has a constrained (not mandated) default constructor. The deprecated atomics
  features are provided, declared [[deprecated]] (Annex D).

## Performance
`bench/` (manual, not in CI; DECISIONS §15) times the hot paths against libstdc++ on both
compilers; `bench/RESULTS.md` has the full tables. Before this pass 31 of 104 benchmark rows were
more than 1.5x slower than libstdc++ on at least one compiler; afterwards (on a noisy shared
machine) about 10, all listed in RESULTS.md. Largest changes (libycxx/libstdc++, GCC / Clang):

| benchmark | before | after |
|---|---|---|
| find byte (memchr) | 28.8 / 17.8 | 1.0 / 1.1 |
| shared_ptr copy+destroy | 12.8 / 1.9 | 1.0 / 0.1 |
| mutex lock/unlock | 2.0 / 2.5 | 0.1 / 0.2 |
| istringstream >> int / >> double | 5.0 / 4.6, 2.1 / 1.9 | 1.0-1.4 / 1.5, 0.5 / 0.8 |
| ostringstream << int, construct+str | 1.6 / 1.8, 1.9 / 2.0 | 1.1 / 1.0, 0.5 / 1.0 |
| dynamic_cast cross cast / failure / to intermediate | 5.0 / 4.1, 6.2 / 2.7, 2.9 / 1.6 | 1.5-2.0 / 1.6-1.9, 1.5 / 0.9, 1.1 / 1.0 |
| from_chars double | 4.1 / 3.5 | 2.2 / 1.9 |
| vector insert at front | 1.1 / 2.6 | 1.0 / 1.0 |
| make_heap + sort_heap | 1.6 / 0.5 | 0.9 / 0.4 |
| throw/catch int | 1.6 / 1.0 | 1.0-1.3 / 0.8 |
| regex construct | 1.6 / 1.2 | 0.7 / 0.6 |
| mt19937_64 + normal | 1.5 / 1.2 | 0.7 / 1.0 |

Stacked virtual diamonds no longer make handler matching and `dynamic_cast` exponential (18
levels: 29.7 s -> 0.01 s; libstdc++ 8.6 s). Remaining above 1.5x: deque push at the ends,
`from_chars(double)`, `to_chars` fixed with precision, Clang `dynamic_cast` across virtual bases
(anonymous-namespace type names have no `*` marker), GCC `string + "x" + string`.

## Open issues / next
- Every header of the C++26 library is provided (Phases 1-4 complete; `<meta>` needs GCC's
  `-freflection`, `<contracts>` GCC's `-fcontracts`). Own suite: no failures on either compiler (configurations above); the expected
  failures carry their reasons in the tests.
- **Decided (user, 2026-10-05): C names through `<string>` and `<cstdint>`.** Hosted `<string>`
  (the character traits) provides `EOF` (it includes `<cstdio>`; `WEOF` comes with `<wchar.h>`),
  and `<cstdint>` also declares the global `::int64_t`... names, as libstdc++, libc++ and MSVC do.
- libstdc++ suite, still failing (tests/libstdcxx/TRIAGE.md, "Whole suite with the DejaGnu
  default"; every other failure is fixed, skipped or an expected compiler failure): the
  template-parameter name `C` vs. a user macro (bitset/cons/string_view{,_wide}.cc, DECISIONS §2).
  Fixed since: the locale facets (named-locale branch: money_get's optional symbol, time_get
  fields with failbit on entry, facet refs wraparound, unbuffered wide filebuf reads), the wide
  standard streams (C wide I/O when synchronized, DECISIONS §7) and
  `__cpp_lib_constexpr_exceptions` on GCC (DECISIONS §4; Clang cannot throw in constant evaluation).
- libc++ suite: the former gaps are closed: `import std;`/`import std.compat;` (DECISIONS §16;
  modules/std and std.compat pass on Clang) and senders/receivers (DECISIONS §17). support.limits
  execution.version and version.version are skipped (divergence: they expect older drafts'
  values, `__cpp_lib_senders` 202406L among them). (The `<wchar.h>` and
  `<stddef.h>` wrappers exist since the own-suite fixes.)
- Next (Phase 5): full libc++/libstdc++ sweeps with triage (tests/libcxx/TRIAGE.md,
  tests/libstdcxx/TRIAGE.md), fixing the libycxx bugs they find; then a whole-library review
  (performance pass done, see Performance).
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
  is defined on GCC and undefined on Clang 23, which cannot throw during constant evaluation;
  `current_exception`, `uncaught_exceptions` and the `nested_exception` facilities are not
  constexpr in the draft (P3818; DECISIONS §4). On GCC `make_exception_ptr`,
  `rethrow_exception` and `exception_ptr_cast` work there too (DECISIONS §4). (`format_error` is constexpr.)
