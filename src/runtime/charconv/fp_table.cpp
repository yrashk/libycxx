// libycxx runtime: the 128-bit power-of-ten table of floating-point <charconv>, computed at
// compile time.
//
// Entry j holds floor(5^j * 2^(127 - floor(log2(5^j)))), the top 128 bits of 5^j normalised so
// that bit 127 is set. For j >= 0 these are the leading bits of the exact power. For j < 0,
// R = floor(2^1024 / 5^-j) is maintained by exact division by 5 (floor(floor(x / 5) / 5) =
// floor(x / 25)); its leading 128 bits are the entry, because 2^1024 exceeds 5^342 * 2^128.
#include "fp_common.hpp"

namespace ycxx::detail::fpconv {
void table_check_failed(); // not constexpr and never defined: reaching it fails table generation
namespace {

struct table_bignum {
  static constexpr int limbs = 36; // 1152 bits: 2^1024 and 5^325 (755 bits) fit
  u32 w[limbs] = {};
  constexpr int bit_length() const {
    for (int i = limbs - 1; i >= 0; --i)
      if (w[i] != 0)
        return 32 * i + 32 - __builtin_clz(w[i]);
    return 0;
  }
  constexpr void mul5() {
    u64 carry = 0;
    for (u32& x : w) {
      u64 t = static_cast<u64>(x) * 5 + carry;
      x = static_cast<u32>(t);
      carry = t >> 32;
    }
  }
  constexpr void div5() {
    u64 rem = 0;
    for (int i = limbs - 1; i >= 0; --i) {
      u64 cur = (rem << 32) | w[i];
      w[i] = static_cast<u32>(cur / 5);
      rem = cur % 5;
    }
  }
  constexpr u32 limb(int i) const { return i >= 0 && i < limbs ? w[i] : 0; }
  // The 128 bits starting at the leading one, as (hi, lo); zero bits below the number.
  constexpr pow10_entry top128() const {
    int from = bit_length() - 128; // index of the lowest bit taken (may be negative)
    u32 part[4] = {};
    for (int k = 0; k < 4; ++k) {
      int b = from + 32 * k; // lowest bit of part k
      int i = b >= 0 ? b / 32 : -((-b + 31) / 32);
      int s = b - 32 * i; // 0..31
      u64 two = (static_cast<u64>(limb(i + 1)) << 32) | limb(i);
      part[k] = static_cast<u32>(two >> s);
    }
    return {(static_cast<u64>(part[3]) << 32) | part[2], (static_cast<u64>(part[1]) << 32) | part[0]};
  }
};

consteval pow10_table_t make_table() {
  pow10_table_t t{};
  table_bignum p{}; // 5^j for j >= 0
  p.w[0] = 1;
  for (int j = 0; j <= pow10_max; ++j) {
    t.e[j - pow10_min] = p.top128();
    // floor_log2_pow5 must agree with the exact bit length.
    if (ycxx::detail::fpconv::floor_log2_pow5(j) != p.bit_length() - 1)
      table_check_failed();
    p.mul5();
  }
  table_bignum r{}; // floor(2^1024 / 5^n)
  r.w[1024 / 32] = 1;
  table_bignum q{}; // 5^n, for the exponent check
  q.w[0] = 1;
  for (int n = 1; n <= -pow10_min; ++n) {
    r.div5();
    q.mul5();
    t.e[-n - pow10_min] = r.top128();
    // floor(log2(5^-n)) = -bit_length(5^n), 5^n being no power of two.
    if (ycxx::detail::fpconv::floor_log2_pow5(-n) != -q.bit_length())
      table_check_failed();
  }
  return t;
}

} // namespace

constinit const pow10_table_t pow10_table = make_table();

} // namespace ycxx::detail::fpconv
