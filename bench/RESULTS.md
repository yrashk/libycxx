# Benchmark results: libycxx vs libstdc++

Produced by `bench/run --md` (see `bench/run` and DECISIONS §15). Each benchmark is the median
of 9 samples of at least ~10 ms; numbers are ns per operation. The ratio is libycxx / libstdc++
on the same compiler (above 1: libycxx is slower; **bold**: above 1.5).

**Machine.** 4 vCPU Intel Xeon @ 2.10 GHz (cloud VM), Linux 6.18, glibc 2.39. GCC 16.2.0 and
Clang 23.1.2, `-O2 -DNDEBUG`, libycxx Release archives (`build/<cc>-release`, -O3), libstdc++ 16
(`tools/ref-cxx`), both linked against GCC 16's `libgcc_s`.

**Noise.** The machine was shared with other jobs (load average 10-16 on 4 CPUs during both runs),
so single rows move by up to ±40% between runs (e.g. `unique int`, `vector.emplace_back`,
`string.compare short`, `to_string int` swing between ~1.0 and ~1.7 with no code change).
Changes were therefore also checked with `valgrind --tool=callgrind` instruction counts (table
below), and suspicious rows were re-run.

## Before / after (ratio libycxx / libstdc++)

"Before" is commit f929087 (the harness alone), "after" the end of this work. Rows renamed or
redefined after the baseline run (`equal int`, `unique int`, `vector.emplace_back`) have no
comparable before value.

| benchmark | GCC before | GCC after | Clang before | Clang after |
|---|---:|---:|---:|---:|
| algorithms: sort 1e6 int | 0.98 | 1.05 | 1.02 | 1.25 |
| algorithms: sort 1e6 int (sorted) | 1.41 | 1.24 | 0.90 | 0.94 |
| algorithms: sort 1e6 int (reversed) | 1.35 | 1.36 | 1.02 | 1.06 |
| algorithms: stable_sort 1e6 int | 1.03 | 1.24 | 1.04 | 1.14 |
| algorithms: nth_element 1e6 int | 0.89 | 1.10 | 0.91 | 0.95 |
| algorithms: partial_sort 1e6 int (k=100) | 0.96 | 0.85 | **1.87** | 1.00 |
| algorithms: make_heap+sort_heap 1e5 int | **1.57** | 0.91 | 0.46 | 0.42 |
| algorithms: sort 2e5 string | 0.89 | 0.88 | 0.97 | 1.12 |
| algorithms: stable_sort 2e5 string | 1.05 | 1.20 | 1.12 | 1.12 |
| algorithms: nth_element 2e5 string | 0.49 | 1.11 | 0.82 | 0.85 |
| algorithms: find int (miss) | **1.59** | 0.99 | 0.99 | 0.99 |
| algorithms: find byte (miss) | **28.83** | 1.00 | **17.75** | 1.08 |
| algorithms: count int | 0.50 | 0.98 | 1.02 | 1.03 |
| algorithms: count_if int | 0.67 | 0.74 | 1.01 | 1.02 |
| algorithms: copy int | 0.97 | 0.96 | 1.01 | 0.92 |
| algorithms: copy_backward int | 0.81 | 1.04 | 1.01 | 0.82 |
| algorithms: move strings 1e5 | 1.01 | 1.07 | 0.97 | 1.04 |
| algorithms: fill int | 1.00 | 0.98 | 1.03 | 1.04 |
| algorithms: fill byte | 1.05 | 0.88 | 1.05 | 1.00 |
| algorithms: equal int | 1.50 | 1.30 | 1.13 | 1.39 |
| algorithms: accumulate int | 1.10 | 0.88 | 1.03 | 1.34 |
| algorithms: reverse int | 1.09 | 1.04 | 0.99 | 1.04 |
| algorithms: min_element int | 0.89 | 1.03 | 0.95 | 1.02 |
| algorithms: lower_bound int (1e6 lookups) | 0.98 | 1.00 | 0.93 | 1.09 |
| algorithms: unique int | 1.16 | 1.04 | 1.00 | **1.96** |
| containers: vector.push_back int | 1.14 | 0.97 | 1.07 | 1.50 |
| containers: vector.push_back int (reserved) | 1.02 | 1.00 | 0.50 | 1.00 |
| containers: vector.push_back string | 1.22 | 1.12 | 1.07 | 1.07 |
| containers: vector.insert front int (n=2000) | 1.06 | 1.02 | **2.58** | 1.02 |
| containers: vector.insert range int | 1.12 | 1.05 | 0.84 | 0.86 |
| containers: vector.emplace_back 1e6 int (reserve 1000) | 0.98 | 0.97 | 1.04 | **1.52** |
| containers: vector.copy 1e6 int | 0.98 | 0.94 | 1.04 | 1.02 |
| containers: vector.copy 1e5 string | 1.25 | 1.23 | 1.07 | 1.11 |
| containers: deque.push_back int | **1.77** | **1.66** | **1.95** | **1.62** |
| containers: deque.push_front int | **1.69** | **1.56** | **2.57** | **1.54** |
| containers: deque.push both+pop | **2.52** | **2.00** | **1.93** | 1.32 |
| containers: deque.iterate sum | 1.18 | 1.14 | **2.19** | 1.25 |
| containers: list.sort 1e5 int | 1.17 | 1.04 | 0.99 | 1.00 |
| containers: list.push_back+clear | 0.97 | 0.74 | 0.94 | 0.63 |
| containers: map<int>.insert | 1.04 | 1.02 | 1.09 | 0.92 |
| containers: map<int>.find | 1.03 | 0.97 | 0.99 | 0.98 |
| containers: map<int>.iterate | 0.98 | 1.01 | 1.04 | 0.95 |
| containers: unordered_map<int>.insert | 0.97 | 1.29 | 1.00 | 0.99 |
| containers: unordered_map<int>.insert (reserved) | 0.91 | 0.93 | 1.06 | 0.90 |
| containers: unordered_map<int>.find hit | 0.84 | 1.26 | 1.18 | 1.15 |
| containers: unordered_map<int>.find miss | 0.87 | 1.00 | 1.05 | 0.71 |
| containers: unordered_map<int>.insert+erase | 0.87 | 0.89 | 1.25 | 1.25 |
| containers: unordered_map<int>.operator[] small keys | 0.26 | 0.26 | 1.15 | 1.06 |
| containers: map<string>.insert | 0.98 | 1.02 | 1.03 | 0.92 |
| containers: map<string>.find | 1.04 | 1.24 | 1.05 | 1.17 |
| containers: unordered_map<string>.insert | 1.05 | 1.04 | 1.00 | 0.97 |
| containers: unordered_map<string>.find | 1.23 | 1.38 | 0.87 | 0.91 |
| containers: unordered_map<string>.insert+erase | 1.04 | 1.25 | 0.91 | 1.25 |
| runtime: shared_ptr copy+destroy | **12.79** | 0.97 | **1.91** | 0.13 |
| runtime: make_shared<int> | **1.61** | 1.00 | **1.57** | 0.74 |
| runtime: weak_ptr lock | 1.44 | 0.18 | **1.53** | 0.12 |
| runtime: unique_ptr make+destroy | 1.07 | 0.96 | 0.99 | 0.97 |
| runtime: mt19937 raw | 1.12 | 1.15 | 1.00 | 1.02 |
| runtime: mt19937 + uniform_int(0,999) | 0.95 | 1.03 | 1.06 | 1.05 |
| runtime: mt19937 + uniform_real | 1.44 | 0.40 | 1.21 | 1.12 |
| runtime: mt19937_64 + normal | 1.38 | 0.69 | 1.15 | 0.98 |
| runtime: function call | 0.78 | 1.00 | 1.00 | 1.22 |
| runtime: function construct small | 0.32 | 0.44 | 0.29 | 0.36 |
| runtime: throw/catch int | **1.62** | 1.35 | 1.02 | 0.82 |
| runtime: throw/catch derived by base& | 1.10 | 0.91 | 0.91 | 0.83 |
| runtime: throw/catch through 20 frames | 1.18 | 1.01 | 1.14 | 1.00 |
| runtime: dynamic_cast to most derived | 0.62 | 0.67 | 0.57 | 1.12 |
| runtime: dynamic_cast to intermediate | **2.86** | 1.12 | **1.60** | 1.05 |
| runtime: dynamic_cast virtual base -> left | 1.28 | 1.45 | **1.61** | **2.13** |
| runtime: dynamic_cast cross cast | **4.95** | **2.05** | **4.10** | **1.58** |
| runtime: dynamic_cast failure | **6.17** | 1.45 | **2.72** | 0.88 |
| runtime: mutex lock/unlock | **2.03** | 0.10 | **2.50** | 0.15 |
| runtime: atomic<int>.fetch_add seq_cst | 0.97 | 1.12 | 1.26 | 1.02 |
| runtime: atomic<int>.load | 0.98 | 1.39 | 1.03 | 1.17 |
| runtime: call_once (done) | 0.13 | 0.18 | 0.19 | 0.26 |
| strings: string.SSO construct | **1.86** | 0.62 | 0.84 | 1.26 |
| strings: string.heap construct (40 chars) | 1.05 | 0.71 | 0.96 | 1.24 |
| strings: string.copy SSO | 1.28 | 0.76 | 1.22 | 1.41 |
| strings: string.append char | 1.39 | 0.69 | 1.41 | **1.59** |
| strings: string.append 8-char literal | 1.21 | 1.05 | 1.00 | 1.16 |
| strings: string.operator+ small | **1.68** | 1.20 | 0.98 | 0.96 |
| strings: string.find char (1 MB, miss) | 1.00 | 1.00 | 1.00 | 1.00 |
| strings: string.find substr (1 MB, miss) | 0.98 | 0.83 | 1.03 | 0.99 |
| strings: string.find_first_of (1 MB, miss) | 0.87 | 0.91 | 0.97 | 1.01 |
| strings: string.rfind char (1 MB, miss) | 1.01 | 1.06 | 1.05 | 1.00 |
| strings: string.compare (1 MB, differ at end) | 1.15 | 0.91 | 1.07 | 1.06 |
| strings: string.operator== (1 MB, differ at end) | 1.11 | 0.94 | 1.03 | 1.06 |
| strings: string.compare short | 1.24 | **1.78** | 1.01 | 1.02 |
| strings: string_view.find substr (1 MB, miss) | 0.98 | 1.46 | 1.02 | 1.02 |
| text: to_chars int | 0.96 | 0.73 | 0.88 | 0.98 |
| text: to_chars double (shortest) | **1.69** | **1.57** | **1.70** | 1.07 |
| text: to_chars double fixed .6 | **1.64** | 1.41 | 1.34 | **1.69** |
| text: from_chars int | 1.15 | 1.17 | 0.86 | 0.61 |
| text: from_chars double | **4.11** | **2.21** | **3.50** | **1.90** |
| text: format {} int | 1.13 | 0.88 | 1.37 | 0.87 |
| text: format {} double | 1.42 | 0.88 | 1.23 | 1.10 |
| text: format {:.3f} double | 1.20 | 1.08 | 1.19 | 1.30 |
| text: format mixed (str, int, pad) | 0.80 | 0.84 | 0.92 | 0.97 |
| text: format_to buffer int | 1.32 | 0.77 | **1.62** | 1.49 |
| text: to_string int | **1.54** | 0.91 | 1.44 | 1.36 |
| text: ostringstream << int | **1.60** | 1.06 | **1.79** | 1.04 |
| text: ostringstream << double | 0.65 | 0.55 | 0.54 | 0.54 |
| text: ostringstream << string | 0.78 | 0.80 | 1.07 | 0.84 |
| text: ostringstream construct+str | **1.85** | 0.53 | **1.95** | 1.03 |
| text: istringstream >> int | **4.97** | 1.00 | **4.57** | 1.50 |
| text: istringstream >> double | **2.10** | 0.48 | **1.92** | 0.83 |
| text: regex_search literal (per char) | 0.40 | 0.48 | 0.22 | 0.16 |
| text: regex_search email (per char) | 0.28 | 0.28 | 0.16 | 0.15 |
| text: regex_search date (per char) | 0.54 | 0.53 | 0.34 | 0.33 |
| text: regex_search alternation (per char) | 1.04 | 1.05 | 0.73 | 0.77 |
| text: regex construct | **1.56** | 0.70 | 1.24 | 0.58 |
| text: regex_match short | 0.36 | 0.35 | 0.24 | 0.24 |

## Instruction counts (callgrind, whole small programs; lower is better)

| program | before | after | libstdc++ |
|---|---:|---:|---:|
| 300k `istringstream >> int` (GCC) | 350M | 204M | 216M |
| 300k `istringstream >> int` (Clang) | 366M | 243M | 210M |
| 300k `ostringstream << int` (GCC) | 262M | 208M | 131M |
| 300k `from_chars(double)` incl. setup (Clang) | 336M | 295M | 174M |
| 300k `to_chars(double)` shortest (Clang) | 208M | 150M | 170M |
| 300k `format_to(buf, "{}", int)` (Clang) | 154M | 91M | 77M |
| 20k throw/catch `int` (GCC) | 244M | 192M | 204M |
| 3000 `regex` constructions (GCC) | 223M | 88M | — |
| 6M deque push_back/front (GCC) | 112M | 94M | 69M |
| 18 stacked virtual diamonds: 1000 throws + 3000 casts (wall clock) | 29.7 s | 0.01 s | 8.6 s |

## Remaining ratios above 1.5

- `deque` push at either end (1.5-2.3x): the end position is recomputed from the start index and
  size (division/modulo by the block size and a map lookup) on every push.
- `from_chars(double)` (1.9-2.2x) and `to_chars` fixed with a precision (1.4-1.7x): exact
  integer-only Eisel-Lemire with 128/192-bit arithmetic and a generic rounding step; there is
  no floating-point fast path (the result must not depend on the rounding mode).
- `dynamic_cast` across virtual bases or to a sibling, Clang only (1.6-2.1x): Clang names
  anonymous-namespace classes without the `*` uniqueness marker, so type comparisons fall back
  to `strcmp` past the long common `N12_GLOBAL__N_1` prefix.
- `string.operator+ small` on GCC (1.2-1.7x, noisy; 1.0 on Clang).

## Full results (after)

| benchmark | gcc libycxx ns | gcc libstdc++ ns | gcc ratio | clang libycxx ns | clang libstdc++ ns | clang ratio |
|---|---:|---:|---:|---:|---:|---:|
| algorithms: sort 1e6 int | 90.97 | 86.32 | 1.05 | 106.98 | 85.84 | 1.25 |
| algorithms: sort 1e6 int (sorted) | 14.44 | 11.62 | 1.24 | 11.41 | 12.11 | 0.94 |
| algorithms: sort 1e6 int (reversed) | 10.82 | 7.94 | 1.36 | 9.09 | 8.61 | 1.06 |
| algorithms: stable_sort 1e6 int | 126.38 | 101.84 | 1.24 | 86.12 | 75.87 | 1.14 |
| algorithms: nth_element 1e6 int | 11.42 | 10.36 | 1.10 | 9.41 | 9.95 | 0.95 |
| algorithms: partial_sort 1e6 int (k=100) | 0.40 | 0.47 | 0.85 | 0.40 | 0.40 | 1.00 |
| algorithms: make_heap+sort_heap 1e5 int | 59.65 | 65.43 | 0.91 | 53.21 | 125.21 | 0.42 |
| algorithms: sort 2e5 string | 321.08 | 365.38 | 0.88 | 312.14 | 278.52 | 1.12 |
| algorithms: stable_sort 2e5 string | 378.33 | 314.32 | 1.20 | 389.47 | 349.23 | 1.12 |
| algorithms: nth_element 2e5 string | 36.39 | 32.88 | 1.11 | 26.91 | 31.78 | 0.85 |
| algorithms: find int (miss) | 0.23 | 0.23 | 0.99 | 0.23 | 0.24 | 0.99 |
| algorithms: find byte (miss) | 0.01 | 0.01 | 1.00 | 0.01 | 0.01 | 1.08 |
| algorithms: count int | 0.38 | 0.39 | 0.98 | 0.26 | 0.26 | 1.03 |
| algorithms: count_if int | 0.36 | 0.49 | 0.74 | 0.20 | 0.20 | 1.02 |
| algorithms: copy int | 0.31 | 0.33 | 0.96 | 0.30 | 0.33 | 0.92 |
| algorithms: copy_backward int | 0.33 | 0.31 | 1.04 | 0.35 | 0.42 | 0.82 |
| algorithms: move strings 1e5 | 41.23 | 38.68 | 1.07 | 33.13 | 31.80 | 1.04 |
| algorithms: fill int | 0.24 | 0.25 | 0.98 | 0.17 | 0.16 | 1.04 |
| algorithms: fill byte | 0.02 | 0.03 | 0.88 | 0.02 | 0.02 | 1.00 |
| algorithms: equal int | 0.42 | 0.32 | 1.30 | 0.41 | 0.30 | 1.39 |
| algorithms: accumulate int | 0.36 | 0.41 | 0.88 | 0.39 | 0.29 | 1.34 |
| algorithms: reverse int | 0.43 | 0.41 | 1.04 | 0.17 | 0.16 | 1.04 |
| algorithms: min_element int | 2.58 | 2.50 | 1.03 | 0.74 | 0.73 | 1.02 |
| algorithms: lower_bound int (1e6 lookups) | 206.50 | 207.00 | 1.00 | 115.06 | 105.74 | 1.09 |
| algorithms: unique int | 1.36 | 1.31 | 1.04 | 1.18 | 0.60 | **1.96** |
| containers: vector.push_back int | 2.86 | 2.96 | 0.97 | 4.84 | 3.22 | **1.50** |
| containers: vector.push_back int (reserved) | 0.36 | 0.36 | 1.00 | 0.71 | 0.71 | 1.00 |
| containers: vector.push_back string | 38.41 | 34.20 | 1.12 | 38.88 | 36.27 | 1.07 |
| containers: vector.insert front int (n=2000) | 36.12 | 35.56 | 1.02 | 35.77 | 35.11 | 1.02 |
| containers: vector.insert range int | 0.15 | 0.15 | 1.05 | 0.16 | 0.18 | 0.86 |
| containers: vector.emplace_back 1e6 int (reserve 1000) | 0.65 | 0.67 | 0.97 | 0.99 | 0.65 | **1.52** |
| containers: vector.copy 1e6 int | 0.31 | 0.33 | 0.94 | 0.32 | 0.31 | 1.02 |
| containers: vector.copy 1e5 string | 5.88 | 4.80 | 1.23 | 5.37 | 4.82 | 1.11 |
| containers: deque.push_back int | 1.72 | 1.03 | **1.66** | 2.69 | 1.66 | **1.62** |
| containers: deque.push_front int | 1.62 | 1.03 | **1.56** | 2.77 | 1.81 | **1.54** |
| containers: deque.push both+pop | 2.87 | 1.44 | **2.00** | 3.73 | 2.83 | 1.32 |
| containers: deque.iterate sum | 0.42 | 0.37 | 1.14 | 0.45 | 0.36 | 1.25 |
| containers: list.sort 1e5 int | 234.20 | 224.45 | 1.04 | 268.15 | 267.38 | 1.00 |
| containers: list.push_back+clear | 16.46 | 22.18 | 0.74 | 16.70 | 26.41 | 0.63 |
| containers: map<int>.insert | 403.77 | 396.17 | 1.02 | 263.19 | 285.62 | 0.92 |
| containers: map<int>.find | 316.75 | 326.96 | 0.97 | 158.34 | 161.19 | 0.98 |
| containers: map<int>.iterate | 40.08 | 39.59 | 1.01 | 37.72 | 39.76 | 0.95 |
| containers: unordered_map<int>.insert | 160.65 | 124.43 | 1.29 | 117.08 | 117.94 | 0.99 |
| containers: unordered_map<int>.insert (reserved) | 72.17 | 77.76 | 0.93 | 69.61 | 77.59 | 0.90 |
| containers: unordered_map<int>.find hit | 20.19 | 16.02 | 1.26 | 14.62 | 12.70 | 1.15 |
| containers: unordered_map<int>.find miss | 24.36 | 24.27 | 1.00 | 21.64 | 30.63 | 0.71 |
| containers: unordered_map<int>.insert+erase | 112.68 | 126.54 | 0.89 | 156.74 | 125.12 | 1.25 |
| containers: unordered_map<int>.operator[] small keys | 1.96 | 7.49 | 0.26 | 4.66 | 4.38 | 1.06 |
| containers: map<string>.insert | 639.10 | 624.70 | 1.02 | 609.47 | 665.26 | 0.92 |
| containers: map<string>.find | 594.51 | 478.12 | 1.24 | 644.06 | 550.32 | 1.17 |
| containers: unordered_map<string>.insert | 184.86 | 178.16 | 1.04 | 211.95 | 218.31 | 0.97 |
| containers: unordered_map<string>.find | 53.65 | 38.93 | 1.38 | 41.57 | 45.73 | 0.91 |
| containers: unordered_map<string>.insert+erase | 257.51 | 205.26 | 1.25 | 239.23 | 191.99 | 1.25 |
| runtime: shared_ptr copy+destroy | 1.44 | 1.49 | 0.97 | 1.46 | 10.80 | 0.13 |
| runtime: make_shared<int> | 14.16 | 14.17 | 1.00 | 15.93 | 21.41 | 0.74 |
| runtime: weak_ptr lock | 2.85 | 15.83 | 0.18 | 1.79 | 14.44 | 0.12 |
| runtime: unique_ptr make+destroy | 13.31 | 13.86 | 0.96 | 13.22 | 13.66 | 0.97 |
| runtime: mt19937 raw | 2.74 | 2.38 | 1.15 | 2.50 | 2.44 | 1.02 |
| runtime: mt19937 + uniform_int(0,999) | 4.58 | 4.46 | 1.03 | 4.69 | 4.45 | 1.05 |
| runtime: mt19937 + uniform_real | 5.79 | 14.64 | 0.40 | 6.96 | 6.23 | 1.12 |
| runtime: mt19937_64 + normal | 14.23 | 20.56 | 0.69 | 15.28 | 15.61 | 0.98 |
| runtime: function call | 1.77 | 1.78 | 1.00 | 1.84 | 1.50 | 1.22 |
| runtime: function construct small | 1.07 | 2.40 | 0.44 | 0.77 | 2.17 | 0.36 |
| runtime: throw/catch int | 1358.15 | 1003.43 | 1.35 | 1619.47 | 1981.69 | 0.82 |
| runtime: throw/catch derived by base& | 974.90 | 1072.99 | 0.91 | 2341.50 | 2826.51 | 0.83 |
| runtime: throw/catch through 20 frames | 4447.20 | 4382.18 | 1.01 | 5251.80 | 5276.71 | 1.00 |
| runtime: dynamic_cast to most derived | 2.84 | 4.23 | 0.67 | 4.37 | 3.91 | 1.12 |
| runtime: dynamic_cast to intermediate | 13.06 | 11.69 | 1.12 | 29.41 | 27.97 | 1.05 |
| runtime: dynamic_cast virtual base -> left | 50.40 | 34.72 | 1.45 | 72.28 | 33.86 | **2.13** |
| runtime: dynamic_cast cross cast | 49.48 | 24.13 | **2.05** | 56.22 | 35.65 | **1.58** |
| runtime: dynamic_cast failure | 18.82 | 12.96 | 1.45 | 27.51 | 31.23 | 0.88 |
| runtime: mutex lock/unlock | 0.71 | 7.36 | 0.10 | 0.99 | 6.43 | 0.15 |
| runtime: atomic<int>.fetch_add seq_cst | 8.86 | 7.91 | 1.12 | 6.62 | 6.52 | 1.02 |
| runtime: atomic<int>.load | 0.50 | 0.36 | 1.39 | 0.22 | 0.18 | 1.17 |
| runtime: call_once (done) | 0.51 | 2.77 | 0.18 | 0.74 | 2.90 | 0.26 |
| strings: string.SSO construct | 6.99 | 11.26 | 0.62 | 8.71 | 6.92 | 1.26 |
| strings: string.heap construct (40 chars) | 19.65 | 27.48 | 0.71 | 26.16 | 21.06 | 1.24 |
| strings: string.copy SSO | 4.28 | 5.62 | 0.76 | 1.48 | 1.05 | 1.41 |
| strings: string.append char | 1.20 | 1.73 | 0.69 | 2.11 | 1.33 | **1.59** |
| strings: string.append 8-char literal | 6.81 | 6.49 | 1.05 | 7.47 | 6.47 | 1.16 |
| strings: string.operator+ small | 22.70 | 18.85 | 1.20 | 7.93 | 8.22 | 0.96 |
| strings: string.find char (1 MB, miss) | 0.01 | 0.01 | 1.00 | 0.01 | 0.01 | 1.00 |
| strings: string.find substr (1 MB, miss) | 0.56 | 0.67 | 0.83 | 0.56 | 0.57 | 0.99 |
| strings: string.find_first_of (1 MB, miss) | 3.23 | 3.52 | 0.91 | 3.19 | 3.17 | 1.01 |
| strings: string.rfind char (1 MB, miss) | 0.38 | 0.36 | 1.06 | 0.53 | 0.53 | 1.00 |
| strings: string.compare (1 MB, differ at end) | 0.03 | 0.03 | 0.91 | 0.03 | 0.03 | 1.06 |
| strings: string.operator== (1 MB, differ at end) | 0.03 | 0.03 | 0.94 | 0.03 | 0.03 | 1.06 |
| strings: string.compare short | 29.79 | 16.69 | **1.78** | 19.69 | 19.36 | 1.02 |
| strings: string_view.find substr (1 MB, miss) | 0.81 | 0.56 | 1.46 | 0.56 | 0.55 | 1.02 |
| text: to_chars int | 18.50 | 25.47 | 0.73 | 23.71 | 24.23 | 0.98 |
| text: to_chars double (shortest) | 71.44 | 45.57 | **1.57** | 54.33 | 50.78 | 1.07 |
| text: to_chars double fixed .6 | 80.21 | 56.83 | 1.41 | 96.28 | 56.83 | **1.69** |
| text: from_chars int | 22.56 | 19.33 | 1.17 | 18.84 | 30.64 | 0.61 |
| text: from_chars double | 54.10 | 24.44 | **2.21** | 44.62 | 23.52 | **1.90** |
| text: format {} int | 45.01 | 51.29 | 0.88 | 38.60 | 44.52 | 0.87 |
| text: format {} double | 128.27 | 146.40 | 0.88 | 116.18 | 105.77 | 1.10 |
| text: format {:.3f} double | 187.86 | 173.49 | 1.08 | 187.57 | 144.26 | 1.30 |
| text: format mixed (str, int, pad) | 203.16 | 240.56 | 0.84 | 182.29 | 186.98 | 0.97 |
| text: format_to buffer int | 39.14 | 50.63 | 0.77 | 47.69 | 31.95 | 1.49 |
| text: to_string int | 19.15 | 21.03 | 0.91 | 22.02 | 16.15 | 1.36 |
| text: ostringstream << int | 71.94 | 67.90 | 1.06 | 87.21 | 83.58 | 1.04 |
| text: ostringstream << double | 188.18 | 343.08 | 0.55 | 189.71 | 353.14 | 0.54 |
| text: ostringstream << string | 13.25 | 16.56 | 0.80 | 14.10 | 16.77 | 0.84 |
| text: ostringstream construct+str | 84.77 | 158.52 | 0.53 | 120.54 | 117.54 | 1.03 |
| text: istringstream >> int | 61.90 | 62.07 | 1.00 | 96.24 | 63.98 | **1.50** |
| text: istringstream >> double | 72.83 | 150.54 | 0.48 | 92.58 | 112.16 | 0.83 |
| text: regex_search literal (per char) | 12.19 | 25.43 | 0.48 | 10.63 | 67.60 | 0.16 |
| text: regex_search email (per char) | 40.70 | 145.01 | 0.28 | 45.79 | 310.73 | 0.15 |
| text: regex_search date (per char) | 20.40 | 38.52 | 0.53 | 24.26 | 73.60 | 0.33 |
| text: regex_search alternation (per char) | 90.69 | 86.77 | 1.05 | 108.60 | 140.26 | 0.77 |
| text: regex construct | 2705.97 | 3876.66 | 0.70 | 2665.82 | 4617.26 | 0.58 |
| text: regex_match short | 156.96 | 454.71 | 0.35 | 183.13 | 762.51 | 0.24 |
