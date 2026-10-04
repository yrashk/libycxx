// Minimal benchmark harness shared by the programs in bench/. It uses only the standard library,
// so every program builds unchanged against libycxx (tools/ycxx-cxx) and the toolchain's
// libstdc++ (tools/ref-cxx); bench/run compiles each twice and compares.
//
// A benchmark is a callable that performs `ops` operations. The harness runs it once to warm up,
// then picks a repeat count so that one sample takes at least ~10 ms, takes BENCH_SAMPLES (default
// 9) samples and prints the median time per operation:
//     <name>\t<ns per op>
// Command-line arguments are substring filters on the names.
#pragma once
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace bench {

// Make the optimizer assume `v` is read (and, through the memory clobber, that memory changed).
template <class T> inline void sink(const T& v) { asm volatile("" : : "r,m"(v) : "memory"); }
template <class T> inline void sink_mut(T& v) { asm volatile("" : "+m"(v) : : "memory"); }
inline void clobber() { asm volatile("" : : : "memory"); }
// Hide a value from the optimizer (no constant propagation through it).
template <class T> inline T opaque(T v) {
  asm volatile("" : "+m"(v));
  return v;
}

inline int argc_ = 0;
inline char** argv_ = nullptr;

inline bool selected(const char* name) {
  if (argc_ <= 1) return true;
  for (int i = 1; i < argc_; ++i)
    if (std::strstr(name, argv_[i])) return true;
  return false;
}

inline int samples() {
  const char* s = std::getenv("BENCH_SAMPLES");
  int n = s ? std::atoi(s) : 9;
  return n < 1 ? 1 : n;
}

// Runs `f()` (which performs `ops` operations) and prints the median ns per operation.
template <class F> void run(const char* name, double ops, F f) {
  if (!selected(name)) return;
  using clock = std::chrono::steady_clock;
  auto once = [&](long reps) {
    auto t0 = clock::now();
    for (long i = 0; i < reps; ++i) f();
    return std::chrono::duration<double, std::nano>(clock::now() - t0).count();
  };
  double first = once(1);  // warm-up
  long reps = 1;
  if (first < 1e7) reps = static_cast<long>(1e7 / (first > 1 ? first : 1)) + 1;
  std::vector<double> v;
  for (int i = 0, n = samples(); i < n; ++i) v.push_back(once(reps) / (static_cast<double>(reps) * ops));
  std::sort(v.begin(), v.end());
  std::printf("%s\t%.3f\n", name, v[v.size() / 2]);
  std::fflush(stdout);
}

// As run, but `prep()` runs untimed before every timed `body()` (for instance to restore an
// unsorted input), so only `body` is measured.
template <class P, class F> void run_prepared(const char* name, double ops, P prep, F body) {
  if (!selected(name)) return;
  using clock = std::chrono::steady_clock;
  auto once = [&](long reps) {
    double t = 0;
    for (long i = 0; i < reps; ++i) {
      prep();
      auto t0 = clock::now();
      body();
      t += std::chrono::duration<double, std::nano>(clock::now() - t0).count();
    }
    return t;
  };
  double first = once(1);
  long reps = 1;
  if (first < 1e7) reps = static_cast<long>(1e7 / (first > 1 ? first : 1)) + 1;
  std::vector<double> v;
  for (int i = 0, n = samples(); i < n; ++i) v.push_back(once(reps) / (static_cast<double>(reps) * ops));
  std::sort(v.begin(), v.end());
  std::printf("%s\t%.3f\n", name, v[v.size() / 2]);
  std::fflush(stdout);
}

// Deterministic data (xorshift), identical under both libraries.
struct rng {
  unsigned long long s = 0x9E3779B97F4A7C15ull;
  unsigned long long operator()() {
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    return s;
  }
};

inline void init(int argc, char** argv) {
  argc_ = argc;
  argv_ = argv;
}

}  // namespace bench
