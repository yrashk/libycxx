// libycxx runtime: floating-point std::from_chars ([charconv.from.chars]).
//
// The pattern is strtod's subject sequence with the restrictions of [charconv.from.chars]/6.
// A decimal significand keeps enough digits that every halfway point between two values of the
// format is represented exactly; further digits only matter through whether any is nonzero,
// which is recorded as a trailing digit 1. Conversion is correctly rounded (to nearest, ties to
// even) whatever the floating-point environment's rounding mode, using integer arithmetic only:
//   * Eisel-Lemire (D. Lemire, "Number parsing at a gigabyte per second", 2021): the leading 19
//     digits w times the 128-bit truncated 5^q. The exact product lies in a known interval of
//     width below w; if both ends of the interval round to the same value, that is the result.
//   * otherwise an exact big-integer computation: D * 10^E, or floor(D * 2^s / 5^k) with its
//     remainder, rounded once.
// Hexadecimal significands are exact up to 120 bits, plus a sticky bit.
#include "fp_common.hpp"

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__fpconv {
namespace {

bool __is_digit(char c) { return c >= '0' && c <= '9'; }
int hex_value(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}
char lower(char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; }

// Matches `__word` (lower case) case-insensitively at p.
bool match_word(const char* p, const char* last, const char* __word) {
  for (; *__word != 0; ++__word, ++p)
    if (p == last || __ycxx::__detail::__fpconv::lower(*p) != *__word)
      return false;
  return true;
}

// Parses an exponent "[+-]digits" at p (after the 'e' or 'p'). Returns the end of the match or
// nullptr if there is none. The value saturates far beyond any format's range.
const char* parse_exponent(const char* p, const char* last, long long& value) {
  bool __negative = false;
  if (p != last && (*p == '+' || *p == '-')) {
    __negative = *p == '-';
    ++p;
  }
  if (p == last || !__ycxx::__detail::__fpconv::__is_digit(*p))
    return nullptr;
  long long __v = 0;
  for (; p != last && __ycxx::__detail::__fpconv::__is_digit(*p); ++p)
    if (__v < 1'000'000'000'000LL)
      __v = __v * 10 + (*p - '0');
  value = __negative ? -__v : __v;
  return p;
}

// ---- decimal to binary -----------------------------------------------------------------------

struct u192 {
  __u128 __hi; // bits 64..191
  __y_u64 __lo;  // bits 0..63
};
u192 mul_192(__y_u64 __w, __u128 t) {
  __u128 __p0 = static_cast<__u128>(__w) * static_cast<__y_u64>(t);
  __u128 __p1 = static_cast<__u128>(__w) * static_cast<__y_u64>(t >> 64);
  return {__p1 + (__p0 >> 64), static_cast<__y_u64>(__p0)};
}
u192 add_192(u192 a, __y_u64 b) {
  __y_u64 __lo = a.__lo + b;
  return {a.__hi + (__lo < a.__lo ? 1 : 0), __lo};
}
u192 sub1_192(u192 a) { return {a.__hi - (a.__lo == 0 ? 1 : 0), a.__lo - 1}; }

// Rounds the 192-bit value a * 2^z (a > 0, top bit at 190 or 191) plus `above` (an extra amount
// strictly between 0 and 1 unit).
template <kind _Kp>
__rounded round_192(u192 a, long long __z, bool above) {
  return __ycxx::__detail::__fpconv::__round_to<_Kp>(a.__hi, __z + 64, above || a.__lo != 0);
}

// Normalises w * t (w != 0) so that its top bit is bit 190 or 191; returns the shift applied.
u192 product(__y_u64 __w, __u128 t, int& shift) {
  shift = __builtin_clzll(__w);
  u192 p = __ycxx::__detail::__fpconv::mul_192(__w, t);
  if (shift != 0) {
    p.__hi = (p.__hi << shift) | (p.__lo >> (64 - shift));
    p.__lo <<= shift;
  }
  return p;
}

// Whether the whole interval eisel_lemire computes rounds like its lower end, so that the upper
// end need not be rounded. The value lies strictly between lo and (hi + 1), where lo = (w * t) <<
// shift (round_192 exponent z) and hi - lo = D << shift for the D below. round_to keeps the top p
// bits of lo.hi, s bits above the 64 of lo.lo; the result can differ only if the interval
// contains a halfway point (a carry into the kept bits rounds to the same value), which is
// checked conservatively in units of 2^64. Anything uncertain answers false: a subnormal result,
// an upper end normalised differently, an overflowing bound.
template <kind _Kp>
bool interval_rounds_alike(const u192& __lo, long long __z, __y_u64 __w, __u128 t, int shift, bool __exact, bool truncated) {
  constexpr format __f = __fmt_of<_Kp>;
  const int __len = __ycxx::__detail::__fpconv::__bit_length(__lo.__hi);
  const long long __lsb = __len - 1 + __z + 64 - (__f.p - 1);
  if (__lsb < __f.__qmin())
    return false;
  const int s = __len - __f.p;
  if (s < 1)
    return false;
  __u128 d; // hi - lo before the shift (exact && !truncated never gets here)
  if (!truncated) {
    d = __w - 1;
  } else {
    if (__builtin_clzll(__w + 1) != shift) // w has at most 19 digits: w + 1 does not wrap
      return false;
    if (__exact)
      d = t - 1;
    else if (__builtin_add_overflow(t, static_cast<__u128>(__w), &d))
      return false;
  }
  const __u128 d_hi = (shift == 0 ? d >> 64 : d >> (64 - shift)) + 2; // covers lo.lo, the low bits and the + 1
  const __u128 rem = __lo.__hi & __ycxx::__detail::__fpconv::__low_mask(s);
  const __u128 __half = __u128(1) << (s - 1);
  __u128 end;
  if (__builtin_add_overflow(rem, d_hi, &end))
    return false;
  // Below the halfway point: the whole interval must stay below it. At or above it (lo itself
  // rounds up, the value being above lo): below the next halfway point.
  return rem >= __half ? end <= (__u128(1) << s) + __half : end <= __half;
}

// Eisel-Lemire: w * 10^q, or (truncated) a value strictly between w * 10^q and (w + 1) * 10^q.
// Returns false when the result cannot be decided this way.
template <kind _Kp>
bool eisel_lemire(__y_u64 __w, int __q, bool truncated, __rounded& out) {
  if (__q < __pow10_min || __q > __pow10_max)
    return false;
  const __u128 t = __ycxx::__detail::__fpconv::__pow10_significand(__q);
  const bool __exact = __q >= 0 && __q <= 55; // t is 5^q itself, shifted
  // value = w * t_exact * 2^(q + floor(log2 5^q) - 127)
  const long long base = static_cast<long long>(__q) + __ycxx::__detail::__fpconv::__floor_log2_pow5(__q) - 127;
  int shift;
  u192 __lo = __ycxx::__detail::__fpconv::product(__w, t, shift);
  if (__exact && !truncated) {
    out = __ycxx::__detail::__fpconv::round_192<_Kp>(__lo, base - shift, false);
    return true;
  }
  // The value is above lo (t truncated, or digits dropped).
  __rounded a = __ycxx::__detail::__fpconv::round_192<_Kp>(__lo, base - shift, true);
  if (a.status == __round_status::ok && __ycxx::__detail::__fpconv::interval_rounds_alike<_Kp>(__lo, base - shift, __w, t, shift, __exact, truncated)) {
    out = a;
    return true;
  }
  // An upper end: below x * t_exact with x = w + 1 (truncated) or x = w, and x * t_exact is
  // below x * t + x when t is truncated.
  const __y_u64 __x = truncated ? __w + 1 : __w;
  int shift_hi;
  u192 __hi = __ycxx::__detail::__fpconv::mul_192(__x, t);
  if (!__exact)
    __hi = __ycxx::__detail::__fpconv::add_192(__hi, __x);
  __hi = __ycxx::__detail::__fpconv::sub1_192(__hi); // the value lies in (hi, hi + 1) or below
  shift_hi = __builtin_clzll(__x);
  if (shift_hi != 0) {
    // x * (t + 1) < 2^(192 - shift_hi), so the shifted value still fits.
    __hi.__hi = (__hi.__hi << shift_hi) | (__hi.__lo >> (64 - shift_hi));
    __hi.__lo <<= shift_hi;
  }
  __rounded b = __ycxx::__detail::__fpconv::round_192<_Kp>(__hi, base - shift_hi, true);
  if (!(a == b))
    return false;
  out = a;
  return true;
}

// Exact conversion of D * 10^e10, D given by its decimal digits (no leading zeros; n >= 1).
template <kind _Kp>
__rounded decimal_exact(const char* d, int n, long long e10) {
  constexpr format __f = __fmt_of<_Kp>;
  constexpr __limits __lim = __limits_of<_Kp>;
  constexpr int _Np = __lim.__limbs_in;
  __rounded r;
  // value is in [10^(n - 1 + e10), 10^(n + e10)).
  if (n - 1 + e10 > __lim.__int_digits) {
    r.status = __round_status::overflow;
    return r;
  }
  const long long __low10 = (__f.__qmin() - 1) * 30103LL / 100000 - 2;
  if (n + e10 < __low10) {
    r.status = __round_status::underflow;
    return r;
  }
  __bignum<_Np> a;
  a.n = 0;
  for (int i = 0; i < n;) {
    __y_u32 chunk = 0, scale = 1;
    for (int __j = 0; __j < 9 && i < n; ++__j, ++i) {
      chunk = chunk * 10 + static_cast<__y_u32>(d[i] - '0');
      scale *= 10;
    }
    a.__mul_small(scale);
    a.__add_small(chunk);
  }
  constexpr int __keep = __f.p + 3;
  if (e10 >= 0) {
    a.__mul_pow10(static_cast<int>(e10));
    int __len = a.__bit_length();
    if (__len <= __keep)
      return __ycxx::__detail::__fpconv::__round_to<_Kp>(a.__to_u128(), 0, false);
    bool __rest;
    __u128 __q = a.__top_bits(__keep, __rest);
    return __ycxx::__detail::__fpconv::__round_to<_Kp>(__q, __len - __keep, __rest);
  }
  // value = D / (5^k * 2^k): quotient floor(D * 2^s / 5^k) with keep + 1 bits, and remainder.
  const int k = static_cast<int>(-e10);
  __bignum<_Np> b;
  b.set(1);
  b.__mul_pow5(k);
  int s = b.__bit_length() - a.__bit_length() + __keep;
  if (s > 0)
    a.shift_left(s);
  else
    b.shift_left(-s);
  // D * 2^s / 5^k (or D / (5^k * 2^-s)) lies in (2^(keep - 1), 2^(keep + 1)).
  b.shift_left(__keep);
  __u128 __q = 0;
  for (int i = __keep; i >= 0; --i) {
    if (__ycxx::__detail::__fpconv::compare(a, b) >= 0) {
      a.__sub(b);
      __q |= __u128(1) << i;
    }
    b.__shift_right1();
  }
  return __ycxx::__detail::__fpconv::__round_to<_Kp>(__q, -static_cast<long long>(s) - k, !a.__is_zero());
}

// A value that overflows, or is nonzero and rounds to zero, is outside the range of the type
// ([charconv.from.chars]/1): result_out_of_range, value unmodified.
template <kind _Kp>
std::from_chars_result finish(const char* end, const __rounded& r, bool __negative, __fp_raw& out) {
  if (r.status != __round_status::ok)
    return {end, std::errc::result_out_of_range};
  out = __ycxx::__detail::__fpconv::__encode<_Kp>(__negative, r.m, r.__biased);
  return {end, std::errc{}};
}

template <kind _Kp>
std::from_chars_result parse_decimal_digits(const char* first, const char* p, const char* last, bool __negative,
                                            int __fmt, __fp_raw& out) {
  constexpr int __max_digits = __limits_of<_Kp>.__max_digits;
  char d[__max_digits + 1];
  int n = 0;            // digits stored
  long long e10 = 0;    // value = d * 10^e10 (before the exponent part)
  bool __sticky = false;  // a dropped digit was nonzero
  bool any = false;     // a digit was seen
  for (; p != last && __ycxx::__detail::__fpconv::__is_digit(*p); ++p) {
    any = true;
    if (n == 0 && *p == '0')
      continue;
    if (n < __max_digits)
      d[n++] = *p;
    else {
      ++e10;
      __sticky |= *p != '0';
    }
  }
  if (p != last && *p == '.') {
    ++p;
    for (; p != last && __ycxx::__detail::__fpconv::__is_digit(*p); ++p) {
      any = true;
      if (n == 0 && *p == '0') {
        --e10;
        continue;
      }
      if (n < __max_digits) {
        d[n++] = *p;
        --e10;
      } else {
        __sticky |= *p != '0';
      }
    }
  }
  if (!any)
    return {first, std::errc::invalid_argument};
  const auto __cf = static_cast<std::chars_format>(__fmt);
  const bool sci = (__cf & std::chars_format::scientific) == std::chars_format::scientific;
  const bool fix = (__cf & std::chars_format::fixed) == std::chars_format::fixed;
  if (sci && p != last && (*p == 'e' || *p == 'E')) {
    long long __x = 0;
    if (const char* __q = __ycxx::__detail::__fpconv::parse_exponent(p + 1, last, __x)) {
      p = __q;
      e10 += __x;
    } else if (!fix) {
      return {first, std::errc::invalid_argument};
    }
  } else if (sci && !fix) {
    return {first, std::errc::invalid_argument};
  }
  if (n == 0) { // zero
    out = __ycxx::__detail::__fpconv::__encode<_Kp>(__negative, 0, 0);
    return {p, std::errc{}};
  }
  if (!__sticky)
    while (d[n - 1] == '0') {
      --n;
      ++e10;
    }
  // Eisel-Lemire on the leading 19 digits.
  {
    __y_u64 __w = 0;
    int take = n < 19 ? n : 19;
    for (int i = 0; i < take; ++i)
      __w = __w * 10 + static_cast<__y_u64>(d[i] - '0');
    // Whether a nonzero digit lies beyond the 19 taken: a dropped one (sticky), or else any kept
    // digit past them, since the trailing zeros were removed above, so d[n - 1] is nonzero.
    const bool truncated = __sticky || n > take;
    long long __q = e10 + (n - take);
    __rounded r;
    if (__q >= __pow10_min && __q <= __pow10_max &&
        __ycxx::__detail::__fpconv::eisel_lemire<_Kp>(__w, static_cast<int>(__q), truncated, r))
      return __ycxx::__detail::__fpconv::finish<_Kp>(p, r, __negative, out);
  }
  if (__sticky) {
    d[n++] = '1';
    --e10;
  }
  __rounded r = __ycxx::__detail::__fpconv::decimal_exact<_Kp>(d, n, e10);
  return __ycxx::__detail::__fpconv::finish<_Kp>(p, r, __negative, out);
}

// If the 8 characters at p are all decimal digits, stores their value in v. The characters are
// loaded as one little-endian word (the first one least significant) and converted in three
// multiply steps: digit pairs, then groups of four, then all eight.
bool eight_digits(const char* p, __y_u64& __v) {
  __y_u64 __x = 0;
  for (int i = 0; i < 8; ++i) // one load on little-endian targets
    __x |= static_cast<__y_u64>(static_cast<unsigned char>(p[i])) << (8 * i);
  // Every byte in 0x30..0x39: its high nibble is 3, and still 3 after adding 6.
  if (((__x & 0xF0F0F0F0F0F0F0F0ull) | (((__x + 0x0606060606060606ull) & 0xF0F0F0F0F0F0F0F0ull) >> 4)) !=
      0x3333333333333333ull)
    return false;
  __x -= 0x3030303030303030ull;
  __x = __x * 10 + (__x >> 8); // byte 2k: the pair (digit 2k, digit 2k + 1)
  __x = (((__x & 0x000000FF000000FFull) * (100 + (1000000ull << 32))) +
       (((__x >> 16) & 0x000000FF000000FFull) * (1 + (10000ull << 32)))) >>
      32;
  __v = __x & 0xFFFFFFFFull;
  return true;
}

// The digits of a decimal significand as w * 10^e10, keeping the leading 19 significant digits
// in w and only whether the others are nonzero.
struct decimal_scan {
  const char* p;
  const char* last;
  __y_u64 __w = 0;
  int taken = 0;        // significant digits in w
  long long e10 = 0;
  bool dropped = false; // a digit after the first 19 significant ones was nonzero
  bool any = false;     // a digit was seen

  // One run of digits (fraction: after the point, where each digit taken scales by 1/10): the
  // leading zeros and the first significant digit one at a time, then 8 at a time while they fit
  // in w, then one at a time again.
  [[__gnu__::__always_inline__]] void run(bool __fraction) {
    for (; p != last && __w == 0 && __ycxx::__detail::__fpconv::__is_digit(*p); ++p) {
      any = true;
      __w = static_cast<__y_u64>(*p - '0');
      taken = __w != 0;
      e10 -= __fraction;
    }
    __y_u64 eight;
    while (__w != 0 && taken <= 11 && last - p >= 8 && __ycxx::__detail::__fpconv::eight_digits(p, eight)) {
      __w = __w * 100000000 + eight;
      taken += 8;
      e10 -= __fraction ? 8 : 0;
      p += 8;
    }
    for (; p != last && __ycxx::__detail::__fpconv::__is_digit(*p); ++p) {
      any = true;
      if (taken < 19) {
        __w = __w * 10 + static_cast<__y_u64>(*p - '0');
        ++taken;
        e10 -= __fraction;
      } else {
        e10 += !__fraction;
        dropped |= *p != '0';
      }
    }
  }
};

// The usual case first: the leading 19 significant digits are accumulated directly into w and
// Eisel-Lemire decides. Anything else (an undecided product, an exponent beyond the table, a
// malformed exponent) starts over in parse_decimal_digits, which keeps every digit.
template <kind _Kp>
std::from_chars_result parse_decimal(const char* first, const char* p, const char* last, bool __negative, int __fmt,
                                     __fp_raw& out) {
  const char* const start = p;
  decimal_scan d{p, last};
  d.run(false);
  if (d.p != last && *d.p == '.') {
    ++d.p;
    d.run(true);
  }
  p = d.p;
  const __y_u64 __w = d.__w;
  long long e10 = d.e10;
  const bool dropped = d.dropped;
  const bool any = d.any;
  if (!any || __w == 0)
    return __ycxx::__detail::__fpconv::parse_decimal_digits<_Kp>(first, start, last, __negative, __fmt, out);
  const auto __cf = static_cast<std::chars_format>(__fmt);
  const bool sci = (__cf & std::chars_format::scientific) == std::chars_format::scientific;
  const bool fix = (__cf & std::chars_format::fixed) == std::chars_format::fixed;
  if (sci && p != last && (*p == 'e' || *p == 'E')) {
    long long __x = 0;
    const char* __q = __ycxx::__detail::__fpconv::parse_exponent(p + 1, last, __x);
    if (__q == nullptr)
      return __ycxx::__detail::__fpconv::parse_decimal_digits<_Kp>(first, start, last, __negative, __fmt, out);
    p = __q;
    e10 += __x;
  } else if (sci && !fix) {
    return {first, std::errc::invalid_argument};
  }
  __rounded r;
  if (e10 >= __pow10_min && e10 <= __pow10_max &&
      __ycxx::__detail::__fpconv::eisel_lemire<_Kp>(__w, static_cast<int>(e10), dropped, r))
    return __ycxx::__detail::__fpconv::finish<_Kp>(p, r, __negative, out);
  return __ycxx::__detail::__fpconv::parse_decimal_digits<_Kp>(first, start, last, __negative, __fmt, out);
}

template <kind _Kp>
std::from_chars_result parse_hex(const char* first, const char* p, const char* last, bool __negative, __fp_raw& out) {
  __u128 __q = 0;
  int bits = 0;          // significant bits in q
  long long __e2 = 0;      // value = q * 2^e2 (before the exponent part)
  bool __sticky = false;
  bool any = false;
  auto take = [&](int __v, bool __fraction) {
    any = true;
    if (bits == 0 && __v == 0) {
      if (__fraction)
        __e2 -= 4;
      return;
    }
    if (bits <= 116) {
      __q = (__q << 4) | static_cast<__u128>(__v);
      bits = __ycxx::__detail::__fpconv::__bit_length(__q);
      if (__fraction)
        __e2 -= 4;
    } else {
      __sticky |= __v != 0;
      if (!__fraction)
        __e2 += 4;
    }
  };
  for (int __v; p != last && (__v = __ycxx::__detail::__fpconv::hex_value(*p)) >= 0; ++p)
    take(__v, false);
  if (p != last && *p == '.') {
    ++p;
    for (int __v; p != last && (__v = __ycxx::__detail::__fpconv::hex_value(*p)) >= 0; ++p)
      take(__v, true);
  }
  if (!any)
    return {first, std::errc::invalid_argument};
  if (p != last && (*p == 'p' || *p == 'P')) {
    long long __x = 0;
    if (const char* e = __ycxx::__detail::__fpconv::parse_exponent(p + 1, last, __x)) {
      p = e;
      __e2 += __x;
    }
  }
  if (__q == 0) {
    out = __ycxx::__detail::__fpconv::__encode<_Kp>(__negative, 0, 0);
    return {p, std::errc{}};
  }
  __rounded r = __ycxx::__detail::__fpconv::__round_to<_Kp>(__q, __e2, __sticky);
  return __ycxx::__detail::__fpconv::finish<_Kp>(p, r, __negative, out);
}

template <kind _Kp>
std::from_chars_result from_chars_impl(const char* first, const char* last, __fp_raw& out, int __fmt) {
  const char* p = first;
  bool __negative = false;
  if (p != last && *p == '-') {
    __negative = true;
    ++p;
  }
  if (p != last && (__ycxx::__detail::__fpconv::lower(*p) == 'i' || __ycxx::__detail::__fpconv::lower(*p) == 'n')) {
    if (__ycxx::__detail::__fpconv::match_word(p, last, "inf")) {
      p += __ycxx::__detail::__fpconv::match_word(p, last, "infinity") ? 8 : 3;
      out = __ycxx::__detail::__fpconv::__encode_infinity<_Kp>(__negative);
      return {p, std::errc{}};
    }
    if (__ycxx::__detail::__fpconv::match_word(p, last, "nan")) {
      p += 3;
      if (p != last && *p == '(') {
        const char* __q = p + 1;
        while (__q != last && (__ycxx::__detail::__fpconv::__is_digit(*__q) || (*__q >= 'a' && *__q <= 'z') ||
                             (*__q >= 'A' && *__q <= 'Z') || *__q == '_'))
          ++__q;
        if (__q != last && *__q == ')')
          p = __q + 1;
      }
      out = __ycxx::__detail::__fpconv::__encode_nan<_Kp>(__negative);
      return {p, std::errc{}};
    }
    return {first, std::errc::invalid_argument};
  }
  if (static_cast<std::chars_format>(__fmt) == std::chars_format::hex)
    return __ycxx::__detail::__fpconv::parse_hex<_Kp>(first, p, last, __negative, out);
  return __ycxx::__detail::__fpconv::parse_decimal<_Kp>(first, p, last, __negative, __fmt, out);
}

} // namespace
}} // namespace __ycxx::__detail::__fpconv

std::from_chars_result __ycxx::__detail::__fp_from_chars(const char* first, const char* last, __fp_kind kind, __fp_raw& bits,
                                                   int __fmt) noexcept {
  using enum __ycxx::__detail::__fp_kind;
  switch (kind) {
  case __binary16:
    return __ycxx::__detail::__fpconv::from_chars_impl<__binary16>(first, last, bits, __fmt);
  case __bfloat16:
    return __ycxx::__detail::__fpconv::from_chars_impl<__bfloat16>(first, last, bits, __fmt);
  case __binary32:
    return __ycxx::__detail::__fpconv::from_chars_impl<__binary32>(first, last, bits, __fmt);
  case __binary64:
    return __ycxx::__detail::__fpconv::from_chars_impl<__binary64>(first, last, bits, __fmt);
  case __x87_extended:
    return __ycxx::__detail::__fpconv::from_chars_impl<__x87_extended>(first, last, bits, __fmt);
  case __binary128:
    return __ycxx::__detail::__fpconv::from_chars_impl<__binary128>(first, last, bits, __fmt);
  }
  __builtin_unreachable();
}
