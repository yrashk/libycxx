// Helpers for the [rand] tests of libycxx's own suite, written from the working draft only.
#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include "check.hpp"

namespace rs {

// A seed sequence ([rand.req.seedseq]) whose generate() writes a fixed, recognisable pattern
// (value i of call c is mix(c, i) mod 2^32) and records how it was called.
struct pattern_seq {
  using result_type = std::uint_least32_t;
  int calls = 0;
  std::size_t last_length = 0;
  std::uint32_t salt = 0;
  bool zeros = false;  // write zeros instead of the pattern

  pattern_seq() = default;
  template <class It> pattern_seq(It, It) {}
  template <class T> pattern_seq(std::initializer_list<T>) {}

  static constexpr std::uint32_t mix(std::uint32_t salt, std::uint32_t i) {
    std::uint32_t x = salt * 0x9e3779b9u + i * 0x85ebca6bu + 0x165667b1u;
    x ^= x >> 15; x *= 0x2c1b3c6du; x ^= x >> 12; x *= 0x297a2d39u; x ^= x >> 15;
    return x;
  }
  template <class RA> void generate(RA b, RA e) {
    ++calls;
    last_length = static_cast<std::size_t>(e - b);
    for (std::uint32_t i = 0; b != e; ++b, ++i) *b = zeros ? 0u : mix(salt, i);
  }
  std::size_t size() const { return 0; }
  template <class O> void param(O) const {}
};

// A uniform random bit generator that replays a fixed list of values (cycling) and counts calls.
template <class UInt, UInt Min, UInt Max>
struct replay_urbg {
  using result_type = UInt;
  static constexpr result_type min() { return Min; }
  static constexpr result_type max() { return Max; }
  const UInt* values;
  std::size_t count;
  std::size_t pos = 0;
  std::size_t calls = 0;
  std::size_t limit = 100000;  // more calls than this means a runaway loop: fail
  constexpr replay_urbg(const UInt* v, std::size_t n) : values(v), count(n) {}
  constexpr result_type operator()() {
    ++calls;
    CHECK(calls <= limit);
    UInt v = values[pos];
    pos = (pos + 1) % count;
    return v;
  }
};

// Small statistics helpers (no <cmath> needed).
constexpr double absd(double x) { return x < 0 ? -x : x; }
constexpr bool near(double x, double y, double tol) { return absd(x - y) <= tol; }
constexpr double sqrtd(double x) {
  if (x <= 0) return 0;
  double r = x > 1 ? x : 1;
  for (int i = 0; i < 200; ++i) r = 0.5 * (r + x / r);
  return r;
}

}  // namespace rs
