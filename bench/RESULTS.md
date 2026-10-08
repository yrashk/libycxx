# Benchmark results: libycxx vs libstdc++ and libc++

## Performance pass 2 (2026-10-07): against libstdc++ and libc++

`bench/run` now builds each program against libycxx, libstdc++ and (Clang) libc++ 23.1 (Debian's
`libc++-23-dev`, the build of the same LLVM release as the compiler), runs them R times
interleaved and reports medians; `bench/check` compares the ratios to libstdc++ with
`bench/baseline.json` (nightly, `full.yml`). Ratios below are libycxx / reference (below 1:
libycxx is faster). The machine was shared with other jobs (load 10 to 17 on 4 CPUs for most of
the day), so wall-clock rows move by ±30% or more between runs; every change was also judged with
callgrind instruction counts and, for the hot loops, by reading libycxx's generated assembly.

### Hypotheses tried, kept and not kept

| area | hypothesis | change | result |
|---|---|---|---|
| deque ends | the end address is recomputed from start/size per push, the block allocation path is inlined (GCC) or emplace_back not inlined at all (Clang), max_size() checked per push | head_/tail_ address caches, end_ index instead of size_, out-of-line grow paths that do not take the arguments, no max_size() check on the fast path | kept: push_back 1.45/1.66 -> 0.65/0.66 (GCC/Clang vs libstdc++), Clang vs libc++ 2.36 -> 0.88; both+pop 1.56/1.70 -> 0.78/0.88 |
| deque (first try) | an out-of-line slow path taking the arguments by reference | — | not kept as is: the loop variable had to live in memory on Clang |
| deque (second try) | caches with size_ kept | — | not kept: still three stores per pop; replaced size_ by end_ |
| sort | Hoare partitioning mispredicts half the comparisons on random scalars; no pattern detection | pdqsort with BlockQuicksort's branchless partition for contiguous scalars | kept: random 1e6 ints 1.05 -> 0.40..0.44, sorted 1.05 -> 0.09..0.15, vs libc++ 2.45 -> 0.95, 8.9 -> 1.26 |
| stable_sort ints | stability of equal integers is unobservable; radix needs no comparisons | LSD radix through a buffer for n >= 512 (std orders), pdqsort otherwise | kept: 1.11/0.97 -> 0.12/0.11; vs libc++ 4.14 -> 0.38 |
| string copies | a short string's copy calls memcpy with a variable size; the copy constructor pays its allocation path's register saves | whole 16-byte buffer copy, two overlapping fixed-size copies from a pointer, allocation paths out of line, exact-size operator+ | kept: SSO construct 0.41/0.66 (Ir 116M -> 44M on GCC), copy SSO 0.37/0.75 |
| string operator+ (GCC) | — | — | partly: 2.3x fewer instructions than libstdc++, but the 16-byte copy right after byte stores stalls on store forwarding (1.3x wall clock) |
| from_chars double | the general Eisel-Lemire (192-bit product, generic rounding) and scanner run for every number | 19-digit short scanner and a one-64x64-product Eisel-Lemire for binary32/64, exact paths otherwise | kept: 2.45/1.88 -> 1.20/1.01..1.14; vs libc++ 1.20 -> 0.6 |
| from_chars tie test | the tie test's branch is on the (unpredictable) rounding bit | one rarely taken branch | kept: GCC 31 -> 22 ns |
| dynamic_cast | kind_of built a 9-entry table per call; Clang's unnamed-namespace names cost a strcmp per comparison | inline kind tests, sole type_infos by address, address-only first pass on the chain | kept: to intermediate Ir 147M -> 43M (Clang), 58M -> 23M (GCC); failure 164M -> 46M |
| dynamic_cast (try) | matchers without the address pass | — | not kept: slower for successful casts (67M vs 43M Ir) |
| to_chars shortest | GCC copies the 128-bit significand record with a 16-byte load after two 8-byte stores (store-forwarding stall, 26% of samples) | binary32/64 plain form decoded in registers | kept: GCC 1.69 -> 0.97 |
| to_chars set_digits | trailing zeros 8/4/2/1 at a time and digits written in place | — | not kept: 1770M -> 1782M Ir |
| to_chars %.Pf | the exact decimal expansion with big integers for every value | m * 10^P in 128 bits, quotient and remainder (P <= 18, 64-bit quotient) | kept: 1.44/1.36 -> 0.35/0.52; vs libc++ 1.47 -> 0.50 |
| find/equal/mismatch | early-exit loops are not vectorized | 256-byte blocks without an early exit; memcmp for equal on integers | kept: find int 0.72/0.58 (libc++ 1.89 -> 1.08), mismatch bytes 0.19, equal int 0.97/0.89 |
| search bytes | restarts a comparison at every position | memchr + memcmp | kept: 0.05/0.04 |
| getline | one sgetc/snextc/push_back per character | append buffered runs found with traits::find | kept: istringstream 4.48 -> 0.65 (GCC), file 6.67 -> 0.85; vs libc++ 2.8 -> 0.73 |
| condition_variable | notify always did a locked increment | return when no waiter is registered | kept: 2.9/3.2 -> 0.16/0.21 |
| atomic notify | a seq_cst fence per notify | no fence while single-threaded | kept: 7.4 -> 0.33; a fetch_add(0) instead of fence + load was not faster (14 vs 10.6 ns), not kept |
| vector emplace_back | the inlined reallocation made Clang refuse to inline emplace_back in larger callers (one call per element in flat_map's range constructor) | first: reallocation out of line (kept the loop variable out of memory) | not kept: Clang then kept a local vector's end pointer in memory (reserved push_back 2.3x); instead emplace_back is [[gnu::always_inline]] with the reallocation inline: 1.00 vs both, flat_map construction Clang 2.85 -> 0.96 |
| flat_map range insert | geometric growth of both containers | reserve + emplace_back for std::vector containers | kept: GCC 2.26 -> 1.1..1.9, Clang 2.85 -> 1.16 |
| hash<string> | 8-byte words read byte by byte (GCC does not merge them) | fixed-size loads at run time | kept: GCC 1.93 -> 0.70 (12 chars), 2.44 -> 1.11 (200 chars) |
| accumulate int | one dependent add per element; neither compiler vectorizes the sign-extending sum at -O2 | four independent unsigned 64-bit sums for contiguous integers (modular arithmetic, any order) | kept: 1.06/1.66..2.23 -> 0.54/0.89; vs libc++ 0.72 |
| move strings 1e5 | — | new benchmark | dominated by page faults of fresh 3 MB vectors (libc++ moved between 6 and 21 ns between runs); "move-assign strings (no allocation)" measures the moves |
| shared_ptr copy+destroy, queue push/pop on GCC | — | none | instruction counts within 1.1x of libstdc++ (queue: Clang beats both); the wall-clock ratios moved between 0.97 and 2.4 between runs |


### Full results, end of pass 2 (commit 230111a, -O2, median of 3 interleaved runs)

One run on the shared machine (load 8 to 10 on 4 CPUs): single rows move by up to ±50% between runs (e.g. accumulate int on Clang read 0.89 and 1.49 within minutes); the per-change numbers above are medians of several runs and callgrind counts. This run is also `bench/baseline.json`.

| benchmark | gcc libycxx | gcc libstdc++ | gcc ratio | clang libycxx | clang libstdc++ | clang ratio | clang libc++ | ratio libc++ |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| algorithms: sort 1e6 int | 47.73 | 107.88 | 0.44 | 33.45 | 88.55 | 0.38 | 38.48 | 0.87 |
| algorithms: sort 1e6 int (sorted) | 1.11 | 15.56 | 0.07 | 1.15 | 10.94 | 0.10 | 0.87 | 1.32 |
| algorithms: sort 1e6 int (reversed) | 2.60 | 9.86 | 0.26 | 2.04 | 7.89 | 0.26 | 2.03 | 1.00 |
| algorithms: stable_sort 1e6 int | 16.01 | 133.12 | 0.12 | 12.93 | 77.67 | 0.17 | 23.47 | 0.55 |
| algorithms: nth_element 1e6 int | 13.48 | 14.18 | 0.95 | 13.00 | 14.48 | 0.90 | 11.13 | 1.17 |
| algorithms: partial_sort 1e6 int (k=100) | 0.59 | 0.41 | 1.42 | 0.54 | 0.54 | 0.98 | 0.41 | 1.31 |
| algorithms: make_heap+sort_heap 1e5 int | 78.11 | 76.61 | 1.02 | 53.46 | 109.84 | 0.49 | 60.23 | 0.89 |
| algorithms: sort 2e5 string | 363.78 | 390.43 | 0.93 | 336.61 | 282.51 | 1.19 | 237.94 | 1.41 |
| algorithms: stable_sort 2e5 string | 373.18 | 438.02 | 0.85 | 300.24 | 317.03 | 0.95 | 241.41 | 1.24 |
| algorithms: nth_element 2e5 string | 36.87 | 43.55 | 0.85 | 35.66 | 31.41 | 1.14 | 29.21 | 1.22 |
| algorithms: find int (miss) | 0.16 | 0.36 | 0.44 | 0.13 | 0.24 | 0.56 | 0.14 | 0.92 |
| algorithms: find byte (miss) | 0.01 | 0.01 | 1.00 | 0.01 | 0.01 | 1.00 | 0.01 | 1.00 |
| algorithms: count int | 0.56 | 0.54 | 1.03 | 0.35 | 0.26 | 1.35 | 0.26 | 1.35 |
| algorithms: count_if int | 0.52 | 0.51 | 1.02 | 0.28 | 0.20 | 1.38 | 0.20 | 1.37 |
| algorithms: copy int | 0.47 | 0.39 | 1.22 | 0.46 | 0.31 | 1.48 | 0.31 | 1.49 |
| algorithms: copy_backward int | 0.42 | 0.35 | 1.21 | 0.43 | 0.31 | 1.39 | 0.30 | 1.43 |
| algorithms: move strings 1e5 | 8.85 | 35.22 | 0.25 | 9.99 | 33.51 | 0.30 | 6.73 | 1.48 |
| algorithms: move-assign strings 1e5 (no allocation) | 3.54 | 5.58 | 0.63 | 3.59 | 6.59 | 0.54 | 1.94 | **1.85** |
| algorithms: fill int | 0.23 | 0.23 | 1.01 | 0.17 | 0.17 | 1.04 | 0.16 | 1.06 |
| algorithms: fill byte | 0.02 | 0.02 | 1.00 | 0.02 | 0.02 | 1.00 | 0.02 | 1.00 |
| algorithms: equal int | 0.29 | 0.29 | 1.01 | 0.46 | 0.30 | **1.54** | 0.30 | **1.55** |
| algorithms: accumulate int | 0.30 | 0.42 | 0.72 | 0.32 | 0.21 | 1.49 | 0.21 | **1.51** |
| algorithms: reverse int | 0.54 | 0.45 | 1.21 | 0.16 | 0.16 | 0.99 | 0.17 | 0.98 |
| algorithms: min_element int | 3.51 | 3.50 | 1.00 | 0.73 | 0.74 | 0.99 | 0.74 | 0.99 |
| algorithms: lower_bound int (1e6 lookups) | 236.54 | 252.28 | 0.94 | 124.20 | 103.59 | 1.20 | 100.90 | 1.23 |
| algorithms: search bytes (5-byte needle, miss) | 0.02 | 0.37 | 0.05 | 0.02 | 0.36 | 0.05 | 0.36 | 0.05 |
| algorithms: mismatch bytes | 0.06 | 0.51 | 0.11 | 0.05 | 0.26 | 0.20 | 0.04 | 1.24 |
| algorithms: equal bytes | 0.05 | 0.03 | 1.48 | 0.05 | 0.04 | 1.35 | 0.04 | 1.39 |
| algorithms: ranges filter|transform sum | 1.89 | 1.30 | 1.46 | 1.31 | 0.90 | 1.45 | 1.01 | 1.30 |
| algorithms: remove_if int | 6.96 | 6.98 | 1.00 | 6.80 | 4.78 | 1.42 | 4.78 | 1.42 |
| algorithms: partition int | 6.96 | 6.70 | 1.04 | 5.44 | 5.20 | 1.05 | 4.88 | 1.11 |
| algorithms: rotate int | 1.02 | 0.82 | 1.25 | 0.41 | 0.58 | 0.71 | 0.81 | 0.51 |
| algorithms: unique int | 1.50 | 1.50 | 1.00 | 0.63 | 0.63 | 1.00 | 0.65 | 0.97 |
| containers: vector.push_back int | 4.75 | 4.69 | 1.01 | 3.28 | 2.94 | 1.12 | 2.99 | 1.10 |
| containers: vector.push_back int (reserved) | 0.36 | 0.36 | 1.00 | 0.36 | 0.36 | 1.00 | 0.36 | 1.00 |
| containers: vector.push_back string | 51.69 | 51.24 | 1.01 | 36.82 | 36.88 | 1.00 | 22.21 | **1.66** |
| containers: vector.insert front int (n=2000) | 51.76 | 49.65 | 1.04 | 35.70 | 35.48 | 1.01 | 35.95 | 0.99 |
| containers: vector.insert range int | 0.16 | 0.33 | 0.48 | 0.16 | 0.20 | 0.82 | 0.16 | 1.02 |
| containers: vector.emplace_back 1e6 int (reserve 1000) | 0.66 | 0.67 | 0.99 | 0.66 | 0.68 | 0.97 | 3.40 | 0.19 |
| containers: vector.copy 1e6 int | 0.43 | 0.43 | 0.99 | 0.31 | 0.31 | 1.02 | 0.31 | 1.01 |
| containers: vector.copy 1e5 string | 3.59 | 5.01 | 0.72 | 4.33 | 5.46 | 0.79 | 3.26 | 1.33 |
| containers: deque.push_back int | 0.61 | 1.88 | 0.33 | 1.06 | 1.68 | 0.63 | 1.14 | 0.93 |
| containers: deque.push_front int | 0.90 | 1.46 | 0.62 | 1.08 | 1.69 | 0.64 | 1.54 | 0.70 |
| containers: deque.push both+pop | 1.91 | 2.02 | 0.94 | 2.88 | 3.01 | 0.96 | 2.16 | 1.33 |
| containers: deque.iterate sum | 0.43 | 0.37 | 1.16 | 0.60 | 0.71 | 0.85 | 0.48 | 1.25 |
| containers: queue<int> push+pop (BFS-like) | 3.08 | 1.34 | **2.29** | 3.20 | 2.83 | 1.13 | 2.49 | 1.29 |
| containers: priority_queue<int> push+pop | 38.53 | 65.27 | 0.59 | 40.33 | 46.87 | 0.86 | 44.81 | 0.90 |
| containers: list.sort 1e5 int | 357.58 | 312.85 | 1.14 | 258.07 | 231.47 | 1.11 | 228.71 | 1.13 |
| containers: list.push_back+clear | 23.89 | 25.71 | 0.93 | 23.82 | 28.35 | 0.84 | 16.26 | 1.46 |
| containers: map<int>.insert | 465.60 | 406.25 | 1.15 | 267.20 | 251.22 | 1.06 | 422.19 | 0.63 |
| containers: map<int>.find | 365.55 | 358.02 | 1.02 | 126.40 | 122.93 | 1.03 | 256.98 | 0.49 |
| containers: map<int>.iterate | 54.48 | 51.17 | 1.06 | 38.39 | 39.73 | 0.97 | 35.25 | 1.09 |
| containers: unordered_map<int>.insert | 156.96 | 161.86 | 0.97 | 117.13 | 119.57 | 0.98 | 109.79 | 1.07 |
| containers: unordered_map<int>.insert (reserved) | 91.23 | 117.94 | 0.77 | 72.99 | 76.87 | 0.95 | 79.58 | 0.92 |
| containers: unordered_map<int>.find hit | 19.55 | 21.70 | 0.90 | 14.47 | 12.99 | 1.11 | 20.02 | 0.72 |
| containers: unordered_map<int>.find miss | 30.06 | 23.36 | 1.29 | 21.86 | 21.90 | 1.00 | 34.54 | 0.63 |
| containers: unordered_map<int>.insert+erase | 155.26 | 167.59 | 0.93 | 122.72 | 147.61 | 0.83 | 125.68 | 0.98 |
| containers: unordered_map<int>.operator[] small keys | 3.49 | 9.18 | 0.38 | 4.20 | 3.37 | 1.25 | 4.51 | 0.93 |
| containers: unordered_map<int>.iterate | 12.23 | 19.71 | 0.62 | 11.73 | 18.70 | 0.63 | 21.33 | 0.55 |
| containers: unordered_set<int>.insert | 147.97 | 162.09 | 0.91 | 118.40 | 119.44 | 0.99 | 108.14 | 1.09 |
| containers: unordered_set<int>.contains hit | 17.49 | 21.91 | 0.80 | 14.34 | 13.87 | 1.03 | 21.21 | 0.68 |
| containers: set<int>.insert | 444.22 | 316.25 | 1.40 | 294.61 | 245.33 | 1.20 | 310.12 | 0.95 |
| containers: flat_map<int>.find | 157.76 | 161.39 | 0.98 | 56.88 | 49.57 | 1.15 | 49.49 | 1.15 |
| containers: flat_map<int>.insert sorted range | 6.11 | 3.23 | **1.89** | 4.11 | 4.29 | 0.96 | 6.47 | 0.63 |
| containers: flat_set<int>.insert (random, 1e4) | 210.10 | 206.90 | 1.02 | 125.10 | 119.98 | 1.04 | 117.71 | 1.06 |
| containers: map<string>.insert | 635.82 | 770.48 | 0.83 | 641.77 | 656.53 | 0.98 | 553.75 | 1.16 |
| containers: map<string>.find | 573.03 | 584.34 | 0.98 | 618.88 | 586.14 | 1.06 | 417.74 | 1.48 |
| containers: unordered_map<string>.insert | 207.23 | 225.51 | 0.92 | 170.00 | 175.01 | 0.97 | 154.41 | 1.10 |
| containers: unordered_map<string>.find | 36.85 | 46.20 | 0.80 | 35.60 | 45.58 | 0.78 | 46.75 | 0.76 |
| containers: unordered_map<string>.find miss | 148.55 | 113.73 | 1.31 | 107.11 | 124.23 | 0.86 | 137.80 | 0.78 |
| containers: unordered_map<string>.insert+erase | 259.24 | 274.94 | 0.94 | 188.78 | 200.55 | 0.94 | 181.42 | 1.04 |
| runtime: shared_ptr copy+destroy | 4.15 | 2.82 | 1.47 | 1.96 | 10.56 | 0.19 | 17.01 | 0.12 |
| runtime: make_shared<int> | 22.64 | 21.04 | 1.08 | 22.51 | 15.91 | 1.41 | 19.69 | 1.14 |
| runtime: weak_ptr lock | 1.18 | 21.78 | 0.05 | 1.82 | 14.26 | 0.13 | 20.73 | 0.09 |
| runtime: unique_ptr make+destroy | 14.46 | 18.61 | 0.78 | 16.34 | 13.74 | 1.19 | 13.43 | 1.22 |
| runtime: make_shared<string> | 22.48 | 22.41 | 1.00 | 21.10 | 23.21 | 0.91 | 23.92 | 0.88 |
| runtime: new/delete 16..4096 bytes | 30.52 | 29.32 | 1.04 | 20.75 | 21.64 | 0.96 | 22.09 | 0.94 |
| runtime: vector<int>(1000) construct+destroy | 72.35 | 69.20 | 1.05 | 50.13 | 49.85 | 1.01 | 51.55 | 0.97 |
| runtime: mt19937 raw | 3.63 | 3.54 | 1.03 | 2.50 | 2.50 | 1.00 | 8.95 | 0.28 |
| runtime: mt19937 + uniform_int(0,999) | 4.51 | 6.22 | 0.73 | 4.63 | 4.32 | 1.07 | 11.91 | 0.39 |
| runtime: mt19937 + uniform_real | 8.33 | 19.88 | 0.42 | 7.01 | 6.25 | 1.12 | 10.11 | 0.69 |
| runtime: mt19937_64 + normal | 22.00 | 28.96 | 0.76 | 15.42 | 14.47 | 1.07 | 17.91 | 0.86 |
| runtime: function call | 2.42 | 2.38 | 1.02 | 1.42 | 1.58 | 0.90 | 1.87 | 0.76 |
| runtime: function construct small | 1.48 | 4.14 | 0.36 | 0.79 | 3.00 | 0.26 | 1.76 | 0.45 |
| runtime: move_only_function call | 1.02 | 0.99 | 1.03 | 1.99 | 2.03 | 0.98 |  |  |
| runtime: function construct large (5 captures) | 17.58 | 22.34 | 0.79 | 18.84 | 16.04 | 1.17 | 15.25 | 1.24 |
| runtime: mt19937_64 + uniform_int<long long> | 3.90 | 4.95 | 0.79 | 5.58 | 4.00 | 1.39 | 45.44 | 0.12 |
| runtime: hash<int> + hash<double> | 0.92 | 5.12 | 0.18 | 0.60 | 3.84 | 0.16 | 0.60 | 1.00 |
| runtime: throw/catch int | 804.65 | 868.48 | 0.93 | 2387.43 | 2981.32 | 0.80 | 2198.30 | 1.09 |
| runtime: throw/catch derived by base& | 982.02 | 1116.32 | 0.88 | 5766.74 | 3978.32 | 1.45 | 2670.59 | **2.16** |
| runtime: throw/catch through 20 frames | 4011.03 | 4051.56 | 0.99 | 6284.21 | 6452.62 | 0.97 | 5791.57 | 1.09 |
| runtime: dynamic_cast to most derived | 2.26 | 5.57 | 0.41 | 3.13 | 4.95 | 0.63 | 3.60 | 0.87 |
| runtime: dynamic_cast to intermediate | 13.26 | 16.49 | 0.80 | 12.04 | 27.86 | 0.43 | 12.05 | 1.00 |
| runtime: dynamic_cast virtual base -> left | 49.73 | 40.91 | 1.22 | 57.92 | 33.96 | **1.71** | 26.46 | **2.19** |
| runtime: dynamic_cast cross cast | 38.33 | 33.00 | 1.16 | 53.67 | 30.88 | **1.74** | 21.34 | **2.51** |
| runtime: dynamic_cast failure | 16.65 | 17.75 | 0.94 | 13.99 | 31.39 | 0.45 | 13.72 | 1.02 |
| runtime: mutex lock/unlock | 0.99 | 8.97 | 0.11 | 0.90 | 9.13 | 0.10 | 12.34 | 0.07 |
| runtime: atomic<int>.fetch_add seq_cst | 8.93 | 8.99 | 0.99 | 6.44 | 6.41 | 1.00 | 8.92 | 0.72 |
| runtime: atomic<int>.load | 0.50 | 0.37 | 1.37 | 0.19 | 0.24 | 0.78 | 0.19 | 0.99 |
| runtime: call_once (done) | 0.50 | 3.30 | 0.15 | 0.54 | 2.92 | 0.19 | 0.36 | **1.50** |
| runtime: shared_mutex lock_shared/unlock | 2.86 | 29.77 | 0.10 | 7.05 | 22.56 | 0.31 | 23.55 | 0.30 |
| runtime: condition_variable notify_one (no waiter) | 1.18 | 6.33 | 0.19 | 0.36 | 4.29 | 0.08 | 4.29 | 0.08 |
| runtime: atomic notify_one (no waiter) | 2.25 | 5.13 | 0.44 | 1.51 | 3.47 | 0.43 | 7.10 | 0.21 |
| runtime: mutex ping-pong 2 threads (per handoff) | 6396.99 | 4435.76 | 1.44 | 2223.94 | 2270.30 | 0.98 | 6455.02 | 0.34 |
| runtime: atomic wait/notify ping-pong (per handoff) | 2836.68 | 2945.54 | 0.96 | 1096.57 | 850.28 | 1.29 | 1054.60 | 1.04 |
| runtime: atomic<int> fetch_add 4 threads (contended) | 21.61 | 21.01 | 1.03 | 16.96 | 16.66 | 1.02 | 18.59 | 0.91 |
| runtime: throw/catch runtime_error with what() | 681.37 | 756.66 | 0.90 | 761.66 | 822.49 | 0.93 | 977.28 | 0.78 |
| strings: string.SSO construct | 5.11 | 11.09 | 0.46 | 4.93 | 8.14 | 0.61 | 6.41 | 0.77 |
| strings: string.heap construct (40 chars) | 30.77 | 27.76 | 1.11 | 28.95 | 21.02 | 1.38 | 19.96 | 1.45 |
| strings: string.copy SSO | 2.07 | 6.05 | 0.34 | 0.96 | 0.72 | 1.35 | 0.61 | **1.57** |
| strings: string.append char | 1.39 | 1.39 | 1.00 | 1.36 | 1.37 | 1.00 | 4.60 | 0.30 |
| strings: string.append 8-char literal | 10.72 | 9.82 | 1.09 | 7.56 | 6.84 | 1.11 | 16.41 | 0.46 |
| strings: string.operator+ small | 23.73 | 24.85 | 0.95 | 1.52 | 8.22 | 0.18 | 17.63 | 0.09 |
| strings: string.find char (1 MB, miss) | 0.01 | 0.01 | 1.00 | 0.01 | 0.01 | 1.00 | 0.01 | 0.92 |
| strings: string.find substr (1 MB, miss) | 0.78 | 0.79 | 0.99 | 0.56 | 0.58 | 0.97 | 0.57 | 0.98 |
| strings: string.find_first_of (1 MB, miss) | 4.52 | 4.49 | 1.01 | 3.24 | 3.21 | 1.01 | 1.99 | **1.63** |
| strings: string.rfind char (1 MB, miss) | 0.55 | 0.50 | 1.09 | 0.54 | 0.54 | 1.00 | 0.54 | 1.00 |
| strings: string.compare (1 MB, differ at end) | 0.04 | 0.03 | 1.12 | 0.03 | 0.03 | 0.97 | 0.04 | 0.92 |
| strings: string.operator== (1 MB, differ at end) | 0.05 | 0.05 | 1.00 | 0.03 | 0.03 | 1.00 | 0.04 | 0.94 |
| strings: string.compare short | 19.43 | 24.28 | 0.80 | 13.32 | 20.42 | 0.65 | 13.09 | 1.02 |
| strings: string_view.find substr (1 MB, miss) | 0.80 | 0.74 | 1.09 | 0.56 | 0.55 | 1.02 | 0.57 | 0.99 |
| strings: hash<string> (12 chars) | 4.25 | 5.82 | 0.73 | 4.25 | 5.86 | 0.72 | 5.37 | 0.79 |
| strings: hash<string> (200 chars) | 55.27 | 50.95 | 1.08 | 58.16 | 50.40 | 1.15 | 52.33 | 1.11 |
| strings: string.operator== short (equal) | 3.60 | 4.08 | 0.88 | 3.46 | 3.54 | 0.98 | 4.01 | 0.86 |
| strings: string.append string (to 1e5) | 0.96 | 1.15 | 0.84 | 0.93 | 1.04 | 0.89 | 1.27 | 0.73 |
| strings: string.find char (64 B) | 0.05 | 0.04 | 1.19 | 0.07 | 0.05 | 1.20 | 0.05 | 1.20 |
| text: to_chars int | 27.00 | 20.04 | 1.35 | 17.73 | 17.73 | 1.00 | 12.38 | 1.43 |
| text: to_chars double (shortest) | 71.48 | 57.73 | 1.24 | 66.85 | 45.53 | 1.47 | 34.44 | **1.94** |
| text: to_chars double fixed .6 | 37.13 | 75.39 | 0.49 | 28.39 | 56.87 | 0.50 | 51.37 | 0.55 |
| text: from_chars int | 28.36 | 28.66 | 0.99 | 18.90 | 21.61 | 0.87 | 25.21 | 0.75 |
| text: from_chars double | 40.46 | 31.38 | 1.29 | 26.20 | 23.04 | 1.14 | 40.35 | 0.65 |
| text: to_chars float (shortest) | 60.92 | 64.82 | 0.94 | 45.22 | 45.55 | 0.99 | 53.31 | 0.85 |
| text: from_chars float | 43.10 | 31.82 | 1.35 | 38.14 | 23.98 | **1.59** | 92.74 | 0.41 |
| text: to_chars double scientific .3 | 73.28 | 61.76 | 1.19 | 55.91 | 42.54 | 1.31 | 38.85 | 1.44 |
| text: format {} int | 55.86 | 65.12 | 0.86 | 36.21 | 34.54 | 1.05 | 64.03 | 0.57 |
| text: format {} double | 159.85 | 149.60 | 1.07 | 140.55 | 97.75 | 1.44 | 110.96 | 1.27 |
| text: format {:.3f} double | 147.73 | 165.17 | 0.89 | 100.23 | 132.48 | 0.76 | 103.50 | 0.97 |
| text: format mixed (str, int, pad) | 254.52 | 295.33 | 0.86 | 180.27 | 197.51 | 0.91 | 200.28 | 0.90 |
| text: format_to buffer int | 52.63 | 51.41 | 1.02 | 32.59 | 27.38 | 1.19 | 58.89 | 0.55 |
| text: to_string int | 22.00 | 21.57 | 1.02 | 16.22 | 21.23 | 0.76 | 18.92 | 0.86 |
| text: to_string double | 91.33 | 118.10 | 0.77 | 76.05 | 86.82 | 0.88 | 304.58 | 0.25 |
| text: format_to back_inserter (3 args) | 155.31 | 268.33 | 0.58 | 106.10 | 143.94 | 0.74 | 131.93 | 0.80 |
| text: print to FILE (int, string) | 122.38 | 151.91 | 0.81 | 81.12 | 110.18 | 0.74 | 105.08 | 0.77 |
| text: format chrono {:%F %T} | 268.70 | 175.70 | **1.53** | 163.02 | 125.44 | 1.30 | 629.70 | 0.26 |
| text: ostringstream << int | 114.36 | 67.94 | **1.68** | 69.89 | 49.20 | 1.42 | 90.01 | 0.78 |
| text: ostringstream << double | 188.29 | 386.10 | 0.49 | 179.60 | 348.82 | 0.51 | 329.03 | 0.55 |
| text: ostringstream << string | 15.34 | 16.41 | 0.94 | 13.30 | 12.74 | 1.04 | 18.14 | 0.73 |
| text: ostringstream construct+str | 129.39 | 161.34 | 0.80 | 98.85 | 118.73 | 0.83 | 118.58 | 0.83 |
| text: istringstream >> int | 90.83 | 101.71 | 0.89 | 93.79 | 63.75 | 1.47 | 120.74 | 0.78 |
| text: istringstream >> double | 101.82 | 150.28 | 0.68 | 68.91 | 138.80 | 0.50 | 184.56 | 0.37 |
| text: getline (istringstream) | 24.62 | 23.84 | 1.03 | 19.78 | 16.61 | 1.19 | 35.67 | 0.55 |
| text: ofstream << line (file write) | 145.14 | 119.55 | 1.21 | 120.92 | 122.02 | 0.99 | 168.51 | 0.72 |
| text: ifstream getline (file read) | 29.24 | 24.22 | 1.21 | 23.15 | 25.02 | 0.93 | 49.87 | 0.46 |
| text: regex_search literal (per char) | 14.73 | 36.02 | 0.41 | 10.99 | 65.73 | 0.17 | 55.83 | 0.20 |
| text: regex_search email (per char) | 65.32 | 221.97 | 0.29 | 44.15 | 364.74 | 0.12 | 455.58 | 0.10 |
| text: regex_search date (per char) | 29.46 | 53.65 | 0.55 | 24.81 | 71.38 | 0.35 | 96.98 | 0.26 |
| text: regex_search alternation (per char) | 130.07 | 114.42 | 1.14 | 103.95 | 141.65 | 0.73 | 305.02 | 0.34 |
| text: regex construct | 2847.66 | 3755.12 | 0.76 | 2619.46 | 4621.66 | 0.57 | 987.96 | **2.65** |
| text: regex_match short | 180.58 | 449.24 | 0.40 | 199.81 | 761.20 | 0.26 | 696.43 | 0.29 |

## Performance pass 1 (2026-10-06): against libstdc++

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

### Before / after (ratio libycxx / libstdc++)

"Before" is commit c649c71 (the harness alone), "after" the end of this work. Rows renamed or
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

### Instruction counts (callgrind, whole small programs; lower is better)

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

### Remaining ratios above 1.5

- `deque` push at either end (1.5-2.3x): the end position is recomputed from the start index and
  size (division/modulo by the block size and a map lookup) on every push.
- `from_chars(double)` (1.9-2.2x) and `to_chars` fixed with a precision (1.4-1.7x): exact
  integer-only Eisel-Lemire with 128/192-bit arithmetic and a generic rounding step; there is
  no floating-point fast path (the result must not depend on the rounding mode).
- `dynamic_cast` across virtual bases or to a sibling, Clang only (1.6-2.1x): Clang names
  anonymous-namespace classes without the `*` uniqueness marker, so type comparisons fall back
  to `strcmp` past the long common `N12_GLOBAL__N_1` prefix.
- `string.operator+ small` on GCC (1.2-1.7x, noisy; 1.0 on Clang).

### Full results (after)

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
