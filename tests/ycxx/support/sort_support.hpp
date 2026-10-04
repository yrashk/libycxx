// Helpers for the [alg.sorting] tests of libycxx's own suite: counting comparators and
// projections, a deterministic pseudo-random generator, keyed elements for stability checks
// and a move-only element type. Written from [alg.sorting]; independent of every other suite.
#pragma once
#include <compare>
#include <cstddef>

// Comparator that counts its applications through a pointer (algorithms copy comparators).
struct CountingLess {
  int* n;
  template <class A, class B>
  constexpr bool operator()(const A& a, const B& b) const {
    ++*n;
    return a < b;
  }
};

// Identity-like projection that counts its applications.
struct CountingProj {
  int* n;
  template <class T>
  constexpr const T& operator()(const T& t) const {
    ++*n;
    return t;
  }
};

// Deterministic linear congruential generator (values in [0, 32768)).
struct Lcg {
  unsigned s;
  constexpr unsigned operator()() {
    s = s * 1103515245u + 12345u;
    return (s >> 16) & 0x7fffu;
  }
};

// Input patterns for the sorting complexity tests.
enum class Pattern { random, sorted, reversed, equal, organ_pipe, sawtooth, few_values };
inline constexpr Pattern all_patterns[] = {Pattern::random,     Pattern::sorted,   Pattern::reversed,
                                           Pattern::equal,      Pattern::organ_pipe, Pattern::sawtooth,
                                           Pattern::few_values};

constexpr void fill_pattern(int* a, int n, Pattern p, unsigned seed = 1) {
  Lcg g{seed};
  for (int i = 0; i < n; ++i) {
    switch (p) {
      case Pattern::random: a[i] = static_cast<int>(g()); break;
      case Pattern::sorted: a[i] = i; break;
      case Pattern::reversed: a[i] = n - i; break;
      case Pattern::equal: a[i] = 7; break;
      case Pattern::organ_pipe: a[i] = i < n / 2 ? i : n - i; break;
      case Pattern::sawtooth: a[i] = i % 16; break;
      case Pattern::few_values: a[i] = static_cast<int>(g() % 4); break;
    }
  }
}

// floor(log2(n)) and ceil(log2(n)) for n >= 1.
constexpr int floor_log2(long long n) {
  int r = 0;
  while (n > 1) {
    n /= 2;
    ++r;
  }
  return r;
}
constexpr int ceil_log2(long long n) {
  int r = 0;
  long long v = 1;
  while (v < n) {
    v *= 2;
    ++r;
  }
  return r;
}

// Element with a sort key and an identity, for stability checks; ordered by key only.
struct KV {
  int key;
  int id;
  friend constexpr bool operator<(const KV& a, const KV& b) { return a.key < b.key; }
  friend constexpr bool operator==(const KV&, const KV&) = default;
};

// Move-only element ordered by value.
struct MoveOnly {
  int v = 0;
  constexpr MoveOnly() = default;
  constexpr MoveOnly(int x) : v(x) {}
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;
  constexpr MoveOnly(MoveOnly&& o) noexcept : v(o.v) { o.v = -1; }
  constexpr MoveOnly& operator=(MoveOnly&& o) noexcept {
    v = o.v;
    o.v = -1;
    return *this;
  }
  friend constexpr bool operator==(const MoveOnly& a, const MoveOnly& b) { return a.v == b.v; }
  friend constexpr auto operator<=>(const MoveOnly& a, const MoveOnly& b) { return a.v <=> b.v; }
};

template <class It>
constexpr bool sorted_by(It first, It last) {
  if (first == last) return true;
  It prev = first;
  for (++first; first != last; ++first, ++prev)
    if (*first < *prev) return false;
  return true;
}

// Multiset equality of two int arrays of length n (n <= 4096, values arbitrary).
constexpr bool same_multiset(const int* a, const int* b, int n) {
  for (int i = 0; i < n; ++i) {
    int ca = 0, cb = 0;
    for (int j = 0; j < n; ++j) {
      ca += a[j] == a[i];
      cb += b[j] == a[i];
    }
    if (ca != cb) return false;
  }
  return true;
}
