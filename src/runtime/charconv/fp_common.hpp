// libycxx runtime: floating-point <charconv>, shared internals.
//
// Every binary format is handled by the same templates, parameterised by fp_kind. Values are
// decoded into an integer significand m and exponent e (value = m * 2^e). Exact work is done on
// fixed-capacity big integers (no allocation, so the code is usable freestanding); the capacity
// of each format is derived below from its exponent range.
#pragma once

#include <charconv>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::fpconv {

using u32 = unsigned int;
using u64 = unsigned long long;
using u128 = ycxx::detail::uint128;
using kind = ycxx::detail::fp_kind;

// ---- format descriptions ---------------------------------------------------------------------

struct format {
  int p;         // significand bits, including the leading one
  int exp_bits;  // width of the biased exponent field
  int emin;      // the smallest normal number is 2^emin
  int emax;      // the largest finite number is (2 - 2^(1-p)) * 2^emax
  bool explicit_bit; // the leading significand bit is stored (x87)
  constexpr int field_bits() const { return explicit_bit ? p : p - 1; } // width of the significand field
  constexpr int qmin() const { return emin - p + 1; } // exponent of the smallest subnormal
};

template <kind K>
inline constexpr format fmt_of = [] {
  if constexpr (K == kind::binary16)
    return format{11, 5, -14, 15, false};
  else if constexpr (K == kind::bfloat16)
    return format{8, 8, -126, 127, false};
  else if constexpr (K == kind::binary32)
    return format{24, 8, -126, 127, false};
  else if constexpr (K == kind::binary64)
    return format{53, 11, -1022, 1023, false};
  else if constexpr (K == kind::x87_extended)
    return format{64, 15, -16382, 16383, true};
  else
    return format{113, 15, -16382, 16383, false};
}();

// floor(x * log10(2)) and floor(x * log2(5)) / floor(x * log2(10)) for the exponent ranges used
// here (|x| < 2^17), by fixed-point multiplication; the table builder and the verification
// programs check them against exact computation.
constexpr int floor_log10_pow2(int x) { return static_cast<int>((static_cast<long long>(x) * 661971961083LL) >> 41); }
// floor(log10(3/4 * 2^x)), for the asymmetric interval of a power of two (Schubfach).
constexpr int floor_log10_three_quarters_pow2(int x) {
  return static_cast<int>((static_cast<long long>(x) * 661971961083LL - 274743187321LL) >> 41);
}
constexpr int floor_log2_pow5(int x) { return static_cast<int>((static_cast<long long>(x) * 1217359) >> 19); }

// Capacity planning. Upper bounds use rational over-approximations of the logarithms.
struct limits {
  int max_digits;  // decimal significand digits a parser must keep (any halfway point fits)
  int int_digits;  // decimal digits of the largest finite value
  int sig_digits;  // significant digits of the exact decimal expansion of any value
  int limbs_in;    // 32-bit limbs of the parser's big integers
  int limbs_out;   // 32-bit limbs of the formatter's big integers
};
constexpr int ceil_div(long long a, long long b) { return static_cast<int>((a + b - 1) / b); }
template <kind K>
inline constexpr limits limits_of = [] {
  constexpr format f = fmt_of<K>;
  limits l{};
  // A halfway point (2c + 1) * 2^(q - 1), q >= qmin, has at most (p + 1) log10 2 + (1 - qmin) log10 5
  // significant digits.
  l.max_digits = ceil_div((f.p + 1) * 30103LL + (1 - f.qmin()) * 69898LL, 100000) + 2;
  l.int_digits = ceil_div((f.emax + 1) * 30103LL, 100000) + 1;
  l.sig_digits = l.max_digits;
  // Decimal exponent below which a value is certainly below half the smallest subnormal.
  int low10 = (f.qmin() - 1) * 30103 / 100000 - 2; // negative
  long long k = l.max_digits + 1 - low10;           // largest power of 5 a parser divides by
  long long bits_parse = k * 23220 / 10000 + 1 + f.p + 8;
  long long bits_digits = (l.max_digits + 1) * 33220LL / 10000 + 8;
  long long bits_int = (l.int_digits + 2) * 33220LL / 10000 + f.p + 8; // m * 10^E or the integer part
  long long bits_frac = (1 - f.qmin()) + 40 + f.p;                     // fractions of the exact expansion
  // m * 10^-k in the shortest-digit loop, whose bounds grow by a factor 10 per digit (at most
  // p * log10(2) + 2 digits).
  long long bits_scaled = 2 * f.p + 24 + static_cast<long long>(-low10 + 2) * 33220 / 10000;
  auto max = [](long long a, long long b) { return a > b ? a : b; };
  l.limbs_in = ceil_div(max(max(bits_parse, bits_digits), bits_int), 32) + 4;
  l.limbs_out = ceil_div(max(max(bits_int, bits_frac), bits_scaled), 32) + 4;
  return l;
}();

// ---- decoding and encoding -------------------------------------------------------------------

enum class fp_class { zero, finite, infinity, nan };

struct decoded {
  bool negative;
  fp_class cls;
  u128 m; // value = m * 2^e; for normal numbers m has exactly p bits
  int e;
};

inline u128 to_u128(fp_raw r) { return (static_cast<u128>(r.hi) << 64) | r.lo; }
inline fp_raw from_u128(u128 v) { return fp_raw{static_cast<u64>(v), static_cast<u64>(v >> 64)}; }
inline u128 low_mask(int bits) { return bits >= 128 ? ~u128(0) : (u128(1) << bits) - 1; }

template <kind K>
decoded decode(fp_raw raw) {
  constexpr format f = fmt_of<K>;
  u128 bits = ycxx::detail::fpconv::to_u128(raw);
  const int fb = f.field_bits();
  const int bias = f.emax;
  decoded d{};
  d.negative = ((bits >> (fb + f.exp_bits)) & 1) != 0;
  int bexp = static_cast<int>((bits >> fb) & ((u128(1) << f.exp_bits) - 1));
  u128 field = bits & ycxx::detail::fpconv::low_mask(fb);
  if (bexp == (1 << f.exp_bits) - 1) {
    u128 payload = field & ycxx::detail::fpconv::low_mask(f.p - 1); // x87: ignore the explicit bit
    d.cls = payload == 0 ? fp_class::infinity : fp_class::nan;
    return d;
  }
  if (bexp == 0) {
    if (field == 0) {
      d.cls = fp_class::zero;
      return d;
    }
    d.cls = fp_class::finite;
    d.m = field;
    d.e = f.qmin();
    return d;
  }
  d.cls = fp_class::finite;
  d.m = f.explicit_bit ? field : (field | (u128(1) << (f.p - 1)));
  d.e = bexp - bias - (f.p - 1);
  return d;
}

// m: the significand (p bits for a normal number, fewer for a subnormal one); biased: the
// biased exponent field (0 for subnormal numbers and zero).
template <kind K>
fp_raw encode(bool negative, u128 m, int biased) {
  constexpr format f = fmt_of<K>;
  const int fb = f.field_bits();
  u128 field = m & ycxx::detail::fpconv::low_mask(fb);
  u128 bits = field | (static_cast<u128>(biased) << fb) | (static_cast<u128>(negative ? 1 : 0) << (fb + f.exp_bits));
  return ycxx::detail::fpconv::from_u128(bits);
}
template <kind K>
fp_raw encode_infinity(bool negative) {
  constexpr format f = fmt_of<K>;
  u128 m = f.explicit_bit ? (u128(1) << (f.p - 1)) : 0;
  return ycxx::detail::fpconv::encode<K>(negative, m, (1 << f.exp_bits) - 1);
}
template <kind K>
fp_raw encode_nan(bool negative) {
  constexpr format f = fmt_of<K>;
  u128 quiet = u128(1) << (f.p - 2);
  u128 m = f.explicit_bit ? (quiet | (u128(1) << (f.p - 1))) : quiet;
  return ycxx::detail::fpconv::encode<K>(negative, m, (1 << f.exp_bits) - 1);
}

inline int bit_length(u128 v) {
  u64 hi = static_cast<u64>(v >> 64);
  if (hi != 0)
    return 128 - __builtin_clzll(hi);
  u64 lo = static_cast<u64>(v);
  return lo == 0 ? 0 : 64 - __builtin_clzll(lo);
}

// Rounds Q * 2^z (plus "a little more" when sticky) to the nearest value of format K, ties to
// even. When sticky is set, Q must have at least p + 2 bits, so that the rounding position lies
// inside Q.
enum class round_status { ok, overflow, underflow };
struct rounded {
  u128 m = 0;
  int biased = 0;
  round_status status = round_status::ok;
  friend bool operator==(const rounded&, const rounded&) = default;
};
template <kind K>
rounded round_to(u128 q, long long z, bool sticky) {
  constexpr format f = fmt_of<K>;
  rounded r;
  long long top = ycxx::detail::fpconv::bit_length(q) - 1 + z;
  long long lsb = top - (f.p - 1);
  if (lsb < f.qmin())
    lsb = f.qmin();
  if (top > f.emax + 1) { // certainly too large, also for absurd exponents
    r.status = round_status::overflow;
    return r;
  }
  long long shift = lsb - z;
  u128 m;
  if (shift <= 0) {
    m = q << -shift; // exact: fits in p bits
  } else if (shift > 128) {
    m = 0; // below half the smallest subnormal
  } else {
    m = shift == 128 ? 0 : q >> shift;
    u128 rem = q & ycxx::detail::fpconv::low_mask(static_cast<int>(shift));
    u128 half = u128(1) << (shift - 1);
    if (rem > half || (rem == half && (sticky || (m & 1) != 0)))
      ++m;
    if (m >> f.p) {
      m >>= 1;
      ++lsb;
    }
  }
  if (m == 0) {
    r.status = round_status::underflow;
    return r;
  }
  if (lsb + f.p - 1 > f.emax) {
    r.status = round_status::overflow;
    return r;
  }
  r.m = m;
  r.biased = (m >> (f.p - 1)) != 0 ? static_cast<int>(lsb + (f.p - 1) - f.emin + 1) : 0;
  return r;
}

// ---- big integers ----------------------------------------------------------------------------

// Fixed-capacity unsigned big integer, 32-bit limbs, little-endian. The callers size N so that
// no operation can exceed it (limits_of above).
template <int N>
struct bignum {
  u32 w[N];
  int n = 0; // limbs in use; w[n - 1] != 0 when n > 0

  void set(u128 v) {
    n = 0;
    while (v != 0) {
      w[n++] = static_cast<u32>(v);
      v >>= 32;
    }
  }
  bool is_zero() const { return n == 0; }
  void trim() {
    while (n > 0 && w[n - 1] == 0)
      --n;
  }
  int bit_length() const { return n == 0 ? 0 : 32 * (n - 1) + 32 - __builtin_clz(w[n - 1]); }
  bool bit(int i) const { return i / 32 < n && ((w[i / 32] >> (i % 32)) & 1) != 0; }
  void mul_small(u32 factor, int from = 0) {
    u64 carry = 0;
    for (int i = from; i < n; ++i) {
      u64 t = static_cast<u64>(w[i]) * factor + carry;
      w[i] = static_cast<u32>(t);
      carry = t >> 32;
    }
    if (carry != 0)
      w[n++] = static_cast<u32>(carry);
  }
  void add_small(u32 a) {
    u64 carry = a;
    for (int i = 0; i < n && carry != 0; ++i) {
      u64 t = static_cast<u64>(w[i]) + carry;
      w[i] = static_cast<u32>(t);
      carry = t >> 32;
    }
    if (carry != 0)
      w[n++] = static_cast<u32>(carry);
  }
  void mul_pow5(int k) {
    constexpr u32 pow5_13 = 1220703125u; // 5^13, the largest power of 5 below 2^32
    for (; k >= 13; k -= 13)
      mul_small(pow5_13);
    u32 r = 1;
    for (; k > 0; --k)
      r *= 5;
    if (r != 1)
      mul_small(r);
  }
  void mul_pow10(int k) {
    mul_pow5(k);
    shift_left(k);
  }
  void shift_left(int bits) {
    if (n == 0 || bits == 0)
      return;
    int limbs = bits / 32, s = bits % 32;
    // Each limb is assigned from its source limbs, highest first (a source limb is never at a
    // higher index than its destination), so no limb is read before it is written.
    if (s == 0) {
      w[n + limbs] = 0;
      for (int i = n - 1; i >= 0; --i)
        w[i + limbs] = w[i];
    } else {
      w[n + limbs] = w[n - 1] >> (32 - s);
      for (int i = n - 1; i > 0; --i)
        w[i + limbs] = (w[i] << s) | (w[i - 1] >> (32 - s));
      w[limbs] = w[0] << s;
    }
    for (int i = 0; i < limbs; ++i)
      w[i] = 0;
    n += limbs + 1;
    trim();
  }
  void shift_right1() {
    for (int i = 0; i < n; ++i)
      w[i] = (w[i] >> 1) | (i + 1 < n ? w[i + 1] << 31 : 0);
    trim();
  }
  // Divides in place by d (nonzero); returns the remainder.
  u32 div_small(u32 d) {
    u64 rem = 0;
    for (int i = n - 1; i >= 0; --i) {
      u64 cur = (rem << 32) | w[i];
      w[i] = static_cast<u32>(cur / d);
      rem = cur % d;
    }
    trim();
    return static_cast<u32>(rem);
  }
  // *this -= b; requires *this >= b.
  void sub(const bignum& b) {
    long long borrow = 0;
    for (int i = 0; i < n; ++i) {
      long long t = static_cast<long long>(w[i]) - (i < b.n ? b.w[i] : 0) - borrow;
      borrow = t < 0;
      w[i] = static_cast<u32>(t);
    }
    trim();
  }
  // *this += b.
  void add(const bignum& b) {
    u64 carry = 0;
    int m = n > b.n ? n : b.n;
    for (int i = 0; i < m; ++i) {
      u64 t = static_cast<u64>(i < n ? w[i] : 0) + (i < b.n ? b.w[i] : 0) + carry;
      w[i] = static_cast<u32>(t);
      carry = t >> 32;
    }
    n = m;
    if (carry != 0)
      w[n++] = static_cast<u32>(carry);
  }
  // The `count` bits below bit `from` (count <= 64, from >= count), as an integer.
  u64 bits_below(int from, int count) const {
    u64 r = 0;
    for (int i = from - 1; i >= from - count; --i)
      r = (r << 1) | (bit(i) ? 1 : 0);
    return r;
  }
  // True if any bit below `from` is set.
  bool any_below(int from) const {
    int limb = from / 32;
    for (int i = 0; i < limb && i < n; ++i)
      if (w[i] != 0)
        return true;
    if (limb < n && (w[limb] & ((u32(1) << (from % 32)) - 1)) != 0)
      return true;
    return false;
  }
  // The top `count` (<= 128) bits as an integer, and whether any lower bit is set. Requires
  // bit_length() >= count.
  u128 top_bits(int count, bool& rest_nonzero) const {
    int len = bit_length();
    int from = len - count;
    u128 r = 0;
    for (int i = len - 1; i >= from; --i)
      r = (r << 1) | (bit(i) ? 1 : 0);
    rest_nonzero = any_below(from);
    return r;
  }
  u128 to_u128() const {
    u128 r = 0;
    for (int i = n - 1; i >= 0; --i)
      r = (r << 32) | w[i];
    return r;
  }
};

template <int N>
int compare(const bignum<N>& a, const bignum<N>& b) {
  if (a.n != b.n)
    return a.n < b.n ? -1 : 1;
  for (int i = a.n - 1; i >= 0; --i)
    if (a.w[i] != b.w[i])
      return a.w[i] < b.w[i] ? -1 : 1;
  return 0;
}

// ---- powers of ten ---------------------------------------------------------------------------

// pow10_table[j - pow10_min] = floor(5^j * 2^(127 - floor(log2(5^j)))): the 128-bit normalised
// truncation of 5^j (exact for 0 <= j <= 55), for j in [pow10_min, pow10_max]. It serves both
// Schubfach (10^-k for binary32/64 output) and the Eisel-Lemire fast path of the parser.
inline constexpr int pow10_min = -342;
inline constexpr int pow10_max = 325;
struct pow10_entry {
  u64 hi, lo;
};
struct pow10_table_t {
  pow10_entry e[pow10_max - pow10_min + 1];
};
extern const pow10_table_t pow10_table; // fp_table.cpp
inline u128 pow10_significand(int j) {
  const pow10_entry& e = ycxx::detail::fpconv::pow10_table.e[j - pow10_min];
  return (static_cast<u128>(e.hi) << 64) | e.lo;
}

// ---- output helpers --------------------------------------------------------------------------

inline std::to_chars_result too_large(char* last) { return {last, std::errc::value_too_large}; }

// Writes "inf", "nan" (with '-' when negative), or returns false for a finite value.
inline bool write_special(char*& first, char* last, const decoded& d, std::to_chars_result& r) {
  if (d.cls != fp_class::infinity && d.cls != fp_class::nan)
    return false;
  const char* text = d.cls == fp_class::infinity ? "inf" : "nan";
  int len = 3 + (d.negative ? 1 : 0);
  if (last - first < len) {
    r = too_large(last);
    return true;
  }
  if (d.negative)
    *first++ = '-';
  for (int i = 0; i < 3; ++i)
    *first++ = text[i];
  r = {first, std::errc{}};
  return true;
}

inline int decimal_length(unsigned v) {
  int n = 1;
  while (v >= 10) {
    v /= 10;
    ++n;
  }
  return n;
}

// Exponent suffix of the e style: 'e', sign, at least two digits.
inline int exponent_length(int x) {
  int a = x < 0 ? -x : x;
  int d = ycxx::detail::fpconv::decimal_length(static_cast<unsigned>(a));
  return 2 + (d < 2 ? 2 : d);
}
inline char* write_exponent(char* p, int x) {
  *p++ = 'e';
  *p++ = x < 0 ? '-' : '+';
  unsigned a = static_cast<unsigned>(x < 0 ? -x : x);
  int d = ycxx::detail::fpconv::decimal_length(a);
  if (d < 2)
    d = 2;
  for (int i = d - 1; i >= 0; --i, a /= 10)
    p[i] = static_cast<char>('0' + a % 10);
  return p + d;
}

}} // namespace ycxx::detail::fpconv
