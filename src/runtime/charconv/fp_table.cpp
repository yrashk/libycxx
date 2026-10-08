// libycxx runtime: the 128-bit power-of-ten table of floating-point <charconv>, computed at
// compile time.
//
// Entry j holds floor(5^j * 2^(127 - floor(log2(5^j)))), the top 128 bits of 5^j normalised so
// that bit 127 is set. For j >= 0 these are the leading bits of the exact power. For j < 0,
// R = floor(2^1024 / 5^-j) is maintained by exact division by 5 (floor(floor(x / 5) / 5) =
// floor(x / 25)); its leading 128 bits are the entry, because 2^1024 exceeds 5^342 * 2^128.
#include "fp_common.hpp"

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail::__fpconv {
void table_check_failed(); // not constexpr and never defined: reaching it fails table generation
namespace {

struct table_bignum {
  static constexpr int __limbs = 36; // 1152 bits: 2^1024 and 5^325 (755 bits) fit
  __y_u32 __w[__limbs] = {};
  constexpr int __bit_length() const {
    for (int i = __limbs - 1; i >= 0; --i)
      if (__w[i] != 0)
        return 32 * i + 32 - __builtin_clz(__w[i]);
    return 0;
  }
  constexpr void mul5() {
    __y_u64 __carry = 0;
    for (__y_u32& __x : __w) {
      __y_u64 t = static_cast<__y_u64>(__x) * 5 + __carry;
      __x = static_cast<__y_u32>(t);
      __carry = t >> 32;
    }
  }
  constexpr void div5() {
    __y_u64 rem = 0;
    for (int i = __limbs - 1; i >= 0; --i) {
      __y_u64 cur = (rem << 32) | __w[i];
      __w[i] = static_cast<__y_u32>(cur / 5);
      rem = cur % 5;
    }
  }
  constexpr __y_u32 __limb(int i) const { return i >= 0 && i < __limbs ? __w[i] : 0; }
  // The 128 bits starting at the leading one, as (hi, lo); zero bits below the number.
  constexpr __pow10_entry top128() const {
    int from = __bit_length() - 128; // index of the lowest bit taken (may be negative)
    __y_u32 part[4] = {};
    for (int k = 0; k < 4; ++k) {
      int b = from + 32 * k; // lowest bit of part k
      int i = b >= 0 ? b / 32 : -((-b + 31) / 32);
      int s = b - 32 * i; // 0..31
      __y_u64 __two = (static_cast<__y_u64>(__limb(i + 1)) << 32) | __limb(i);
      part[k] = static_cast<__y_u32>(__two >> s);
    }
    return {(static_cast<__y_u64>(part[3]) << 32) | part[2], (static_cast<__y_u64>(part[1]) << 32) | part[0]};
  }
};

consteval __pow10_table_t make_table() {
  __pow10_table_t t{};
  table_bignum p{}; // 5^j for j >= 0
  p.__w[0] = 1;
  for (int __j = 0; __j <= __pow10_max; ++__j) {
    t.e[__j - __pow10_min] = p.top128();
    // floor_log2_pow5 must agree with the exact bit length.
    if (__ycxx::__detail::__fpconv::__floor_log2_pow5(__j) != p.__bit_length() - 1)
      table_check_failed();
    p.mul5();
  }
  table_bignum r{}; // floor(2^1024 / 5^n)
  r.__w[1024 / 32] = 1;
  table_bignum __q{}; // 5^n, for the exponent check
  __q.__w[0] = 1;
  for (int n = 1; n <= -__pow10_min; ++n) {
    r.div5();
    __q.mul5();
    t.e[-n - __pow10_min] = r.top128();
    // floor(log2(5^-n)) = -bit_length(5^n), 5^n being no power of two.
    if (__ycxx::__detail::__fpconv::__floor_log2_pow5(-n) != -__q.__bit_length())
      table_check_failed();
  }
  return t;
}

} // namespace

constinit const __pow10_table_t __pow10_table = make_table();

}} // namespace __ycxx::__detail::__fpconv
