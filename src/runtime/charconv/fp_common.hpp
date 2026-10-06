// libycxx runtime: floating-point <charconv>, shared internals.
//
// Every binary format is handled by the same templates, parameterised by fp_kind. Values are
// decoded into an integer significand m and exponent e (value = m * 2^e). Exact work is done on
// fixed-capacity big integers (no allocation, so the code is usable freestanding); the capacity
// of each format is derived below from its exponent range.
#pragma once

#include <charconv>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__fpconv {

using __y_u32 = unsigned int;
using __y_u64 = unsigned long long;
using __u128 = __ycxx::__detail::__uint128;
using kind = __ycxx::__detail::__fp_kind;

// ---- format descriptions ---------------------------------------------------------------------

struct format {
  int p;         // significand bits, including the leading one
  int __exp_bits;  // width of the biased exponent field
  int __emin;      // the smallest normal number is 2^emin
  int __emax;      // the largest finite number is (2 - 2^(1-p)) * 2^emax
  bool __explicit_bit; // the leading significand bit is stored (x87)
  constexpr int __field_bits() const { return __explicit_bit ? p : p - 1; } // width of the significand field
  constexpr int __qmin() const { return __emin - p + 1; } // exponent of the smallest subnormal
};

template <kind _Kp>
inline constexpr format __fmt_of = [] {
  if constexpr (_Kp == kind::__binary16)
    return format{11, 5, -14, 15, false};
  else if constexpr (_Kp == kind::__bfloat16)
    return format{8, 8, -126, 127, false};
  else if constexpr (_Kp == kind::__binary32)
    return format{24, 8, -126, 127, false};
  else if constexpr (_Kp == kind::__binary64)
    return format{53, 11, -1022, 1023, false};
  else if constexpr (_Kp == kind::__x87_extended)
    return format{64, 15, -16382, 16383, true};
  else
    return format{113, 15, -16382, 16383, false};
}();

// floor(x * log10(2)) and floor(x * log2(5)) / floor(x * log2(10)) for the exponent ranges used
// here (|x| < 2^17), by fixed-point multiplication; the table builder and the verification
// programs check them against exact computation.
constexpr int __floor_log10_pow2(int __x) { return static_cast<int>((static_cast<long long>(__x) * 661971961083LL) >> 41); }
// floor(log10(3/4 * 2^x)), for the asymmetric interval of a power of two (Schubfach).
constexpr int __floor_log10_three_quarters_pow2(int __x) {
  return static_cast<int>((static_cast<long long>(__x) * 661971961083LL - 274743187321LL) >> 41);
}
constexpr int __floor_log2_pow5(int __x) { return static_cast<int>((static_cast<long long>(__x) * 1217359) >> 19); }

// Capacity planning. Upper bounds use rational over-approximations of the logarithms.
struct __limits {
  int __max_digits;  // decimal significand digits a parser must keep (any halfway point fits)
  int __int_digits;  // decimal digits of the largest finite value
  int __sig_digits;  // significant digits of the exact decimal expansion of any value
  int __limbs_in;    // 32-bit limbs of the parser's big integers
  int __limbs_out;   // 32-bit limbs of the formatter's big integers
};
constexpr int __ceil_div(long long a, long long b) { return static_cast<int>((a + b - 1) / b); }
template <kind _Kp>
inline constexpr __limits __limits_of = [] {
  constexpr format __f = __fmt_of<_Kp>;
  __limits __l{};
  // A halfway point (2c + 1) * 2^(q - 1), q >= qmin, has at most (p + 1) log10 2 + (1 - qmin) log10 5
  // significant digits.
  __l.__max_digits = __ceil_div((__f.p + 1) * 30103LL + (1 - __f.__qmin()) * 69898LL, 100000) + 2;
  __l.__int_digits = __ceil_div((__f.__emax + 1) * 30103LL, 100000) + 1;
  __l.__sig_digits = __l.__max_digits;
  // Decimal exponent below which a value is certainly below half the smallest subnormal.
  int __low10 = (__f.__qmin() - 1) * 30103 / 100000 - 2; // negative
  long long k = __l.__max_digits + 1 - __low10;           // largest power of 5 a parser divides by
  long long __bits_parse = k * 23220 / 10000 + 1 + __f.p + 8;
  long long __bits_digits = (__l.__max_digits + 1) * 33220LL / 10000 + 8;
  long long __bits_int = (__l.__int_digits + 2) * 33220LL / 10000 + __f.p + 8; // m * 10^E or the integer part
  long long __bits_frac = (1 - __f.__qmin()) + 40 + __f.p;                     // fractions of the exact expansion
  // m * 10^-k in the shortest-digit loop, whose bounds grow by a factor 10 per digit (at most
  // p * log10(2) + 2 digits).
  long long __bits_scaled = 2 * __f.p + 24 + static_cast<long long>(-__low10 + 2) * 33220 / 10000;
  auto max = [](long long a, long long b) { return a > b ? a : b; };
  __l.__limbs_in = __ceil_div(max(max(__bits_parse, __bits_digits), __bits_int), 32) + 4;
  __l.__limbs_out = __ceil_div(max(max(__bits_int, __bits_frac), __bits_scaled), 32) + 4;
  return __l;
}();

// ---- decoding and encoding -------------------------------------------------------------------

enum class __fp_class { zero, __finite, infinity, nan };

struct __decoded {
  bool __negative;
  __fp_class __cls;
  __u128 m; // value = m * 2^e; for normal numbers m has exactly p bits
  int e;
};

inline __u128 __to_u128(__fp_raw r) { return (static_cast<__u128>(r.__hi) << 64) | r.__lo; }
inline __fp_raw __from_u128(__u128 __v) { return __fp_raw{static_cast<__y_u64>(__v), static_cast<__y_u64>(__v >> 64)}; }
inline __u128 __low_mask(int bits) { return bits >= 128 ? ~__u128(0) : (__u128(1) << bits) - 1; }

template <kind _Kp>
__decoded __decode(__fp_raw __raw) {
  constexpr format __f = __fmt_of<_Kp>;
  __u128 bits = __ycxx::__detail::__fpconv::__to_u128(__raw);
  const int __fb = __f.__field_bits();
  const int __bias = __f.__emax;
  __decoded d{};
  d.__negative = ((bits >> (__fb + __f.__exp_bits)) & 1) != 0;
  int __bexp = static_cast<int>((bits >> __fb) & ((__u128(1) << __f.__exp_bits) - 1));
  __u128 field = bits & __ycxx::__detail::__fpconv::__low_mask(__fb);
  if (__bexp == (1 << __f.__exp_bits) - 1) {
    __u128 __payload = field & __ycxx::__detail::__fpconv::__low_mask(__f.p - 1); // x87: ignore the explicit bit
    d.__cls = __payload == 0 ? __fp_class::infinity : __fp_class::nan;
    return d;
  }
  if (__bexp == 0) {
    if (field == 0) {
      d.__cls = __fp_class::zero;
      return d;
    }
    d.__cls = __fp_class::__finite;
    d.m = field;
    d.e = __f.__qmin();
    return d;
  }
  d.__cls = __fp_class::__finite;
  d.m = __f.__explicit_bit ? field : (field | (__u128(1) << (__f.p - 1)));
  d.e = __bexp - __bias - (__f.p - 1);
  return d;
}

// m: the significand (p bits for a normal number, fewer for a subnormal one); biased: the
// biased exponent field (0 for subnormal numbers and zero).
template <kind _Kp>
__fp_raw __encode(bool __negative, __u128 m, int __biased) {
  constexpr format __f = __fmt_of<_Kp>;
  const int __fb = __f.__field_bits();
  __u128 field = m & __ycxx::__detail::__fpconv::__low_mask(__fb);
  __u128 bits = field | (static_cast<__u128>(__biased) << __fb) | (static_cast<__u128>(__negative ? 1 : 0) << (__fb + __f.__exp_bits));
  return __ycxx::__detail::__fpconv::__from_u128(bits);
}
template <kind _Kp>
__fp_raw __encode_infinity(bool __negative) {
  constexpr format __f = __fmt_of<_Kp>;
  __u128 m = __f.__explicit_bit ? (__u128(1) << (__f.p - 1)) : 0;
  return __ycxx::__detail::__fpconv::__encode<_Kp>(__negative, m, (1 << __f.__exp_bits) - 1);
}
template <kind _Kp>
__fp_raw __encode_nan(bool __negative) {
  constexpr format __f = __fmt_of<_Kp>;
  __u128 __quiet = __u128(1) << (__f.p - 2);
  __u128 m = __f.__explicit_bit ? (__quiet | (__u128(1) << (__f.p - 1))) : __quiet;
  return __ycxx::__detail::__fpconv::__encode<_Kp>(__negative, m, (1 << __f.__exp_bits) - 1);
}

inline int __bit_length(__u128 __v) {
  __y_u64 __hi = static_cast<__y_u64>(__v >> 64);
  if (__hi != 0)
    return 128 - __builtin_clzll(__hi);
  __y_u64 __lo = static_cast<__y_u64>(__v);
  return __lo == 0 ? 0 : 64 - __builtin_clzll(__lo);
}

// Rounds Q * 2^z (plus "a little more" when sticky) to the nearest value of format K, ties to
// even. When sticky is set, Q must have at least p + 2 bits, so that the rounding position lies
// inside Q.
enum class __round_status { ok, overflow, underflow };
struct __rounded {
  __u128 m = 0;
  int __biased = 0;
  __round_status status = __round_status::ok;
  friend bool operator==(const __rounded&, const __rounded&) = default;
};
template <kind _Kp>
__rounded __round_to(__u128 __q, long long __z, bool __sticky) {
  constexpr format __f = __fmt_of<_Kp>;
  __rounded r;
  long long top = __ycxx::__detail::__fpconv::__bit_length(__q) - 1 + __z;
  long long __lsb = top - (__f.p - 1);
  if (__lsb < __f.__qmin())
    __lsb = __f.__qmin();
  if (top > __f.__emax + 1) { // certainly too large, also for absurd exponents
    r.status = __round_status::overflow;
    return r;
  }
  long long shift = __lsb - __z;
  __u128 m;
  if (shift <= 0) {
    m = __q << -shift; // exact: fits in p bits
  } else if (shift > 128) {
    m = 0; // below half the smallest subnormal
  } else {
    m = shift == 128 ? 0 : __q >> shift;
    __u128 rem = __q & __ycxx::__detail::__fpconv::__low_mask(static_cast<int>(shift));
    __u128 __half = __u128(1) << (shift - 1);
    if (rem > __half || (rem == __half && (__sticky || (m & 1) != 0)))
      ++m;
    if (m >> __f.p) {
      m >>= 1;
      ++__lsb;
    }
  }
  if (m == 0) {
    r.status = __round_status::underflow;
    return r;
  }
  if (__lsb + __f.p - 1 > __f.__emax) {
    r.status = __round_status::overflow;
    return r;
  }
  r.m = m;
  r.__biased = (m >> (__f.p - 1)) != 0 ? static_cast<int>(__lsb + (__f.p - 1) - __f.__emin + 1) : 0;
  return r;
}

// ---- big integers ----------------------------------------------------------------------------

// Fixed-capacity unsigned big integer, 32-bit limbs, little-endian. The callers size N so that
// no operation can exceed it (limits_of above).
template <int _Np>
struct __bignum {
  __y_u32 __w[_Np];
  int n = 0; // limbs in use; w[n - 1] != 0 when n > 0

  void set(__u128 __v) {
    n = 0;
    while (__v != 0) {
      __w[n++] = static_cast<__y_u32>(__v);
      __v >>= 32;
    }
  }
  bool __is_zero() const { return n == 0; }
  void __trim() {
    while (n > 0 && __w[n - 1] == 0)
      --n;
  }
  int __bit_length() const { return n == 0 ? 0 : 32 * (n - 1) + 32 - __builtin_clz(__w[n - 1]); }
  bool __bit(int i) const { return i / 32 < n && ((__w[i / 32] >> (i % 32)) & 1) != 0; }
  void __mul_small(__y_u32 __factor, int from = 0) {
    __y_u64 __carry = 0;
    for (int i = from; i < n; ++i) {
      __y_u64 t = static_cast<__y_u64>(__w[i]) * __factor + __carry;
      __w[i] = static_cast<__y_u32>(t);
      __carry = t >> 32;
    }
    if (__carry != 0)
      __w[n++] = static_cast<__y_u32>(__carry);
  }
  void __add_small(__y_u32 a) {
    __y_u64 __carry = a;
    for (int i = 0; i < n && __carry != 0; ++i) {
      __y_u64 t = static_cast<__y_u64>(__w[i]) + __carry;
      __w[i] = static_cast<__y_u32>(t);
      __carry = t >> 32;
    }
    if (__carry != 0)
      __w[n++] = static_cast<__y_u32>(__carry);
  }
  void __mul_pow5(int k) {
    constexpr __y_u32 __pow5_13 = 1220703125u; // 5^13, the largest power of 5 below 2^32
    for (; k >= 13; k -= 13)
      __mul_small(__pow5_13);
    __y_u32 r = 1;
    for (; k > 0; --k)
      r *= 5;
    if (r != 1)
      __mul_small(r);
  }
  void __mul_pow10(int k) {
    __mul_pow5(k);
    shift_left(k);
  }
  void shift_left(int bits) {
    if (n == 0 || bits == 0)
      return;
    int __limbs = bits / 32, s = bits % 32;
    // Each limb is assigned from its source limbs, highest first (a source limb is never at a
    // higher index than its destination), so no limb is read before it is written.
    if (s == 0) {
      __w[n + __limbs] = 0;
      for (int i = n - 1; i >= 0; --i)
        __w[i + __limbs] = __w[i];
    } else {
      __w[n + __limbs] = __w[n - 1] >> (32 - s);
      for (int i = n - 1; i > 0; --i)
        __w[i + __limbs] = (__w[i] << s) | (__w[i - 1] >> (32 - s));
      __w[__limbs] = __w[0] << s;
    }
    for (int i = 0; i < __limbs; ++i)
      __w[i] = 0;
    n += __limbs + 1;
    __trim();
  }
  void __shift_right1() {
    for (int i = 0; i < n; ++i)
      __w[i] = (__w[i] >> 1) | (i + 1 < n ? __w[i + 1] << 31 : 0);
    __trim();
  }
  // Divides in place by d (nonzero); returns the remainder.
  __y_u32 __div_small(__y_u32 d) {
    __y_u64 rem = 0;
    for (int i = n - 1; i >= 0; --i) {
      __y_u64 cur = (rem << 32) | __w[i];
      __w[i] = static_cast<__y_u32>(cur / d);
      rem = cur % d;
    }
    __trim();
    return static_cast<__y_u32>(rem);
  }
  // *this -= b; requires *this >= b.
  void __sub(const __bignum& b) {
    long long __borrow = 0;
    for (int i = 0; i < n; ++i) {
      long long t = static_cast<long long>(__w[i]) - (i < b.n ? b.__w[i] : 0) - __borrow;
      __borrow = t < 0;
      __w[i] = static_cast<__y_u32>(t);
    }
    __trim();
  }
  // *this += b.
  void add(const __bignum& b) {
    __y_u64 __carry = 0;
    int m = n > b.n ? n : b.n;
    for (int i = 0; i < m; ++i) {
      __y_u64 t = static_cast<__y_u64>(i < n ? __w[i] : 0) + (i < b.n ? b.__w[i] : 0) + __carry;
      __w[i] = static_cast<__y_u32>(t);
      __carry = t >> 32;
    }
    n = m;
    if (__carry != 0)
      __w[n++] = static_cast<__y_u32>(__carry);
  }
  // The `count` bits below bit `from` (count <= 64, from >= count), as an integer.
  __y_u64 __bits_below(int from, int count) const {
    __y_u64 r = 0;
    for (int i = from - 1; i >= from - count; --i)
      r = (r << 1) | (__bit(i) ? 1 : 0);
    return r;
  }
  // True if any bit below `from` is set.
  bool __any_below(int from) const {
    int __limb = from / 32;
    for (int i = 0; i < __limb && i < n; ++i)
      if (__w[i] != 0)
        return true;
    if (__limb < n && (__w[__limb] & ((__y_u32(1) << (from % 32)) - 1)) != 0)
      return true;
    return false;
  }
  // The top `count` (<= 128) bits as an integer, and whether any lower bit is set. Requires
  // bit_length() >= count.
  __u128 __top_bits(int count, bool& __rest_nonzero) const {
    int __len = __bit_length();
    int from = __len - count;
    __u128 r = 0;
    for (int i = __len - 1; i >= from; --i)
      r = (r << 1) | (__bit(i) ? 1 : 0);
    __rest_nonzero = __any_below(from);
    return r;
  }
  __u128 __to_u128() const {
    __u128 r = 0;
    for (int i = n - 1; i >= 0; --i)
      r = (r << 32) | __w[i];
    return r;
  }
};

template <int _Np>
int compare(const __bignum<_Np>& a, const __bignum<_Np>& b) {
  if (a.n != b.n)
    return a.n < b.n ? -1 : 1;
  for (int i = a.n - 1; i >= 0; --i)
    if (a.__w[i] != b.__w[i])
      return a.__w[i] < b.__w[i] ? -1 : 1;
  return 0;
}

// ---- powers of ten ---------------------------------------------------------------------------

// pow10_table[j - pow10_min] = floor(5^j * 2^(127 - floor(log2(5^j)))): the 128-bit normalised
// truncation of 5^j (exact for 0 <= j <= 55), for j in [pow10_min, pow10_max]. It serves both
// Schubfach (10^-k for binary32/64 output) and the Eisel-Lemire fast path of the parser.
inline constexpr int __pow10_min = -342;
inline constexpr int __pow10_max = 325;
struct __pow10_entry {
  __y_u64 __hi, __lo;
};
struct __pow10_table_t {
  __pow10_entry e[__pow10_max - __pow10_min + 1];
};
extern const __pow10_table_t __pow10_table; // fp_table.cpp
inline __u128 __pow10_significand(int __j) {
  const __pow10_entry& e = __ycxx::__detail::__fpconv::__pow10_table.e[__j - __pow10_min];
  return (static_cast<__u128>(e.__hi) << 64) | e.__lo;
}

// ---- output helpers --------------------------------------------------------------------------

inline std::to_chars_result __too_large(char* last) { return {last, std::errc::value_too_large}; }

// Writes "inf", "nan" (with '-' when negative), or returns false for a finite value.
inline bool __write_special(char*& first, char* last, const __decoded& d, std::to_chars_result& r) {
  if (d.__cls != __fp_class::infinity && d.__cls != __fp_class::nan)
    return false;
  const char* __text = d.__cls == __fp_class::infinity ? "inf" : "nan";
  int __len = 3 + (d.__negative ? 1 : 0);
  if (last - first < __len) {
    r = __too_large(last);
    return true;
  }
  if (d.__negative)
    *first++ = '-';
  for (int i = 0; i < 3; ++i)
    *first++ = __text[i];
  r = {first, std::errc{}};
  return true;
}

inline int __decimal_length(unsigned __v) {
  int n = 1;
  while (__v >= 10) {
    __v /= 10;
    ++n;
  }
  return n;
}

// Exponent suffix of the e style: 'e', sign, at least two digits.
inline int __exponent_length(int __x) {
  int a = __x < 0 ? -__x : __x;
  int d = __ycxx::__detail::__fpconv::__decimal_length(static_cast<unsigned>(a));
  return 2 + (d < 2 ? 2 : d);
}
inline char* __write_exponent(char* p, int __x) {
  *p++ = 'e';
  *p++ = __x < 0 ? '-' : '+';
  unsigned a = static_cast<unsigned>(__x < 0 ? -__x : __x);
  int d = __ycxx::__detail::__fpconv::__decimal_length(a);
  if (d < 2)
    d = 2;
  for (int i = d - 1; i >= 0; --i, a /= 10)
    p[i] = static_cast<char>('0' + a % 10);
  return p + d;
}

}} // namespace __ycxx::__detail::__fpconv
