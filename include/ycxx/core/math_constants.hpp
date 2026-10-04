// libycxx core: the mathematical constants of <numbers> ([math.constants]) to 192 bits, and
// their correctly rounded values in any floating-point format. The soft floating-point math of
// <cmath> (cmath_mp.hpp) uses the same table.
#pragma once

#include <ycxx/config.hpp>

namespace ycxx::detail {

// value = 0.m[0]m[1]m[2] (binary, top bit set) * 2^(exp + 1), truncated to 192 bits. The
// constants are irrational, so the truncated tail is never zero (rounding needs no sticky bit).
struct math_constant_bits {
  int exp;
  unsigned long long m[3];
};

enum class math_constant : unsigned char {
  e, log2e, log10e, pi, inv_pi, inv_sqrtpi, ln2, ln10, sqrt2, sqrt3, inv_sqrt3, egamma, phi
};

// Generated with 400-bit arithmetic: floor(c * 2^(191 - exp)), exp = floor(log2(c)).
inline constexpr math_constant_bits math_constant_table[] = {
    {1, {0xadf85458a2bb4a9aull, 0xafdc5620273d3cf1ull, 0xd8b9c583ce2d3695ull}},  // e
    {0, {0xb8aa3b295c17f0bbull, 0xbe87fed0691d3e88ull, 0xeb577aa8dd695a58ull}},  // log2(e)
    {-2, {0xde5bd8a937287195ull, 0x355baaafad33dc32ull, 0x3ee3460245c9a202ull}}, // log10(e)
    {1, {0xc90fdaa22168c234ull, 0xc4c6628b80dc1cd1ull, 0x29024e088a67cc74ull}},  // pi
    {-2, {0xa2f9836e4e441529ull, 0xfc2757d1f534ddc0ull, 0xdb6295993c439041ull}}, // 1/pi
    {-1, {0x906eba8214db688dull, 0x71d48a7f6bfec344ull, 0x1409a0ebac3e7517ull}}, // 1/sqrt(pi)
    {-1, {0xb17217f7d1cf79abull, 0xc9e3b39803f2f6afull, 0x40f343267298b62dull}}, // ln(2)
    {1, {0x935d8dddaaa8ac16ull, 0xea56d62b82d30a28ull, 0xe28fecf9da5df90eull}},  // ln(10)
    {0, {0xb504f333f9de6484ull, 0x597d89b3754abe9full, 0x1d6f60ba893ba84cull}},  // sqrt(2)
    {0, {0xddb3d742c265539dull, 0x92ba16b83c5c1dc4ull, 0x92ec1a6629ed23ccull}},  // sqrt(3)
    {-1, {0x93cd3a2c8198e269ull, 0x0c7c0f257d92be83ull, 0x0c9d66eec69e17ddull}}, // 1/sqrt(3)
    {-1, {0x93c467e37db0c7a4ull, 0xd1be3f810152cb56ull, 0xa1cecc3af65cc019ull}}, // Euler-Mascheroni
    {0, {0xcf1bbcdcbfa53e0aull, 0xf9ce60302e76e41aull, 0x084113b5f9d13928ull}},  // (1 + sqrt(5)) / 2
};

// The nearest value of T (any floating-point format with at most 127 significand bits) to the
// constant `c`.
template <class T>
consteval T math_constant_value(math_constant c) {
  const math_constant_bits& b = ycxx::detail::math_constant_table[static_cast<int>(c)];
  constexpr int p = ycxx::detail::fp_format<T>.digits;
  static_assert(p > 0 && p < 128, "libycxx: unsupported floating-point format");
  // q = the top p bits, rounded by the next bit (the tail beyond it is never zero).
  unsigned long long hi = 0, lo = 0; // q = hi * 2^64 + lo
  for (int i = 0; i <= p; ++i) {     // bit i counts from the top of m
    const unsigned long long bit = (b.m[i / 64] >> (63 - i % 64)) & 1;
    if (i == p) {
      if (bit && ++lo == 0) ++hi;
    } else {
      hi = (hi << 1) | (lo >> 63);
      lo = (lo << 1) | bit;
    }
  }
  int exp = b.exp - (p - 1);
  if (p < 64 ? (lo >> p) != 0 : p == 64 ? hi != 0 : (hi >> (p - 64)) != 0) {
    // Rounded up to 2^p: renormalise.
    lo = (lo >> 1) | (hi << 63);
    hi >>= 1;
    ++exp;
  }
  T v = T(lo); // exact: q has at most p bits
  if constexpr (p > 64) v += T(hi) * T(4294967296.0) * T(4294967296.0);
  for (; exp > 0; --exp) v *= T(2);
  for (; exp < 0; ++exp) v /= T(2);
  return v;
}

} // namespace ycxx::detail
