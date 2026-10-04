// A reference subtract_with_carry_engine written from [rand.eng.sub] (for comparisons only).
#pragma once
#include <cstddef>
#include <cstdint>

template <class E>
struct ref_swc {
  using T = typename E::result_type;
  static constexpr std::size_t w = E::word_size, s = E::short_lag, r = E::long_lag;
  static constexpr std::uint64_t mask = w == 64 ? ~0ull : (1ull << w) - 1;
  static constexpr std::size_t nwords = (w + 31) / 32;
  std::uint64_t x[r];  // x[(p + j) % r] is X_{i-r+j}
  std::size_t p = 0;
  int carry = 0;

  explicit ref_swc(std::uint64_t value) {  // [rand.eng.sub]/7
    std::uint64_t lcg = value == 0 ? 19780503u : std::uint32_t(value % 2147483563u);
    lcg %= 2147483563u;
    if (lcg == 0) lcg = 1;
    for (std::size_t k = 0; k < r; ++k) {
      std::uint64_t v = 0;
      for (std::size_t j = 0; j < nwords; ++j) {
        lcg = lcg * 40014u % 2147483563u;
        v += j < 2 ? lcg << (32 * j) : 0;
      }
      x[k] = v & mask;
    }
    carry = x[r - 1] == 0;
  }
  template <class Q> ref_swc(Q& q, int) {  // [rand.eng.sub]/9
    std::uint32_t a[r * nwords];
    q.generate(a + 0, a + r * nwords);
    for (std::size_t i = 0; i < r; ++i) {
      std::uint64_t v = 0;
      for (std::size_t j = 0; j < nwords; ++j) v += std::uint64_t(a[nwords * i + j]) << (32 * j);
      x[i] = v & mask;
    }
    carry = x[r - 1] == 0;
  }
  T operator()() {
    std::uint64_t xs = x[(p + r - s) % r], xr = x[p];
    // Y = X_{i-s} - X_{i-r} - c; X_i = Y mod m; c = (Y < 0)
    bool neg = xs < xr || (xs - xr) < std::uint64_t(carry);
    std::uint64_t y = (xs - xr - std::uint64_t(carry)) & mask;
    x[p] = y;
    carry = neg;
    p = (p + 1) % r;
    return T(y);
  }
};
