// A reference mersenne_twister_engine written from [rand.eng.mers] (for comparisons only).
#pragma once
#include <cstddef>
#include <cstdint>

template <class E>
struct ref_mt {
  using T = typename E::result_type;
  static constexpr std::size_t w = E::word_size, n = E::state_size, m = E::shift_size, r = E::mask_bits;
  static constexpr std::uint64_t wmask = w == 64 ? ~0ull : (1ull << w) - 1;
  static constexpr std::uint64_t lower = r == 64 ? ~0ull : (1ull << r) - 1;  // lower r bits
  static constexpr std::uint64_t upper = wmask & ~lower;                     // upper w - r bits
  std::uint64_t x[n];  // x[(p + j) % n] is X_{i-n+j}
  std::size_t p = 0;

  explicit ref_mt(std::uint64_t value) {  // [rand.eng.mers]/6
    x[0] = value & wmask;
    for (std::size_t i = 1; i < n; ++i)
      x[i] = (std::uint64_t(E::initialization_multiplier) * (x[i - 1] ^ (x[i - 1] >> (w - 2))) + i) & wmask;
  }
  template <class Q> explicit ref_mt(Q& q, int) {  // [rand.eng.mers]/8
    constexpr std::size_t k = (w + 31) / 32;
    std::uint32_t a[n * k];
    q.generate(a + 0, a + n * k);
    for (std::size_t i = 0; i < n; ++i) {
      std::uint64_t v = 0;
      for (std::size_t j = k; j-- > 0;) v = (k > 1 ? (v << 32) : 0) + a[k * i + j];
      x[i] = v & wmask;
    }
    bool zero = (x[0] & upper) == 0;
    for (std::size_t i = 1; i < n; ++i) zero = zero && x[i] == 0;
    if (zero) x[0] = 1ull << (w - 1);
  }
  T operator()() {
    std::uint64_t y = (x[p] & upper) | (x[(p + 1) % n] & lower);
    std::uint64_t alpha = (y & 1) ? std::uint64_t(E::xor_mask) : 0;
    x[p] = x[(p + m) % n] ^ (y >> 1) ^ alpha;
    std::uint64_t z = x[p];
    p = (p + 1) % n;
    // Shifts by w or more give 0 (lshift_w / rshift of a w-bit value).
    auto shl = [](std::uint64_t v, std::size_t k) { return k >= 64 ? 0 : v << k; };
    auto shr = [](std::uint64_t v, std::size_t k) { return k >= 64 ? 0 : v >> k; };
    z = z ^ (shr(z, E::tempering_u) & E::tempering_d);
    z = z ^ (shl(z, E::tempering_s) & E::tempering_b & wmask);
    z = z ^ (shl(z, E::tempering_t) & E::tempering_c & wmask);
    z = z ^ shr(z, E::tempering_l);
    return T(z & wmask);
  }
};
