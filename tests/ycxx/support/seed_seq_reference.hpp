// The seed_seq::generate algorithm of [rand.util.seedseq]/9, transcribed for comparisons.
#pragma once
#include <cstddef>
#include <cstdint>

inline void ref_seed_generate(const std::uint32_t* v, std::size_t s, std::uint32_t* b, std::size_t n) {
  if (n == 0) return;
  for (std::size_t i = 0; i < n; ++i) b[i] = 0x8b8b8b8bu;
  std::size_t t = n >= 623 ? 11 : n >= 68 ? 7 : n >= 39 ? 5 : n >= 7 ? 3 : (n - 1) / 2;
  std::size_t p = (n - t) / 2, q = p + t;
  std::size_t m = s + 1 > n ? s + 1 : n;
  auto T = [](std::uint32_t x) { return std::uint32_t(x ^ (x >> 27)); };
  auto at = [&](std::size_t k) -> std::uint32_t& { return b[k % n]; };
  for (std::size_t k = 0; k < m; ++k) {
    std::uint32_t r1 = 1664525u * T(at(k) ^ at(k + p) ^ at(k + n - 1));
    std::uint32_t r2 = r1 + (k == 0 ? std::uint32_t(s) : k <= s ? std::uint32_t(k % n) + v[k - 1] : std::uint32_t(k % n));
    at(k + p) += r1;
    at(k + q) += r2;
    at(k) = r2;
  }
  for (std::size_t k = m; k < m + n; ++k) {
    std::uint32_t r3 = 1566083941u * T(at(k) + at(k + p) + at(k + n - 1));
    std::uint32_t r4 = r3 - std::uint32_t(k % n);
    at(k + p) ^= r3;
    at(k + q) ^= r4;
    at(k) = r4;
  }
}
