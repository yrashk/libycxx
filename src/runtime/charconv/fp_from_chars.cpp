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

namespace ycxx::detail::fpconv {
namespace {

bool is_digit(char c) { return c >= '0' && c <= '9'; }
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

// Matches `word` (lower case) case-insensitively at p.
bool match_word(const char* p, const char* last, const char* word) {
  for (; *word != 0; ++word, ++p)
    if (p == last || ycxx::detail::fpconv::lower(*p) != *word)
      return false;
  return true;
}

// Parses an exponent "[+-]digits" at p (after the 'e' or 'p'). Returns the end of the match or
// nullptr if there is none. The value saturates far beyond any format's range.
const char* parse_exponent(const char* p, const char* last, long long& value) {
  bool negative = false;
  if (p != last && (*p == '+' || *p == '-')) {
    negative = *p == '-';
    ++p;
  }
  if (p == last || !ycxx::detail::fpconv::is_digit(*p))
    return nullptr;
  long long v = 0;
  for (; p != last && ycxx::detail::fpconv::is_digit(*p); ++p)
    if (v < 1'000'000'000'000LL)
      v = v * 10 + (*p - '0');
  value = negative ? -v : v;
  return p;
}

// ---- decimal to binary -----------------------------------------------------------------------

struct u192 {
  u128 hi; // bits 64..191
  u64 lo;  // bits 0..63
};
u192 mul_192(u64 w, u128 t) {
  u128 p0 = static_cast<u128>(w) * static_cast<u64>(t);
  u128 p1 = static_cast<u128>(w) * static_cast<u64>(t >> 64);
  return {p1 + (p0 >> 64), static_cast<u64>(p0)};
}
u192 add_192(u192 a, u64 b) {
  u64 lo = a.lo + b;
  return {a.hi + (lo < a.lo ? 1 : 0), lo};
}
u192 sub1_192(u192 a) { return {a.hi - (a.lo == 0 ? 1 : 0), a.lo - 1}; }

// Rounds the 192-bit value a * 2^z (a > 0, top bit at 190 or 191) plus `above` (an extra amount
// strictly between 0 and 1 unit).
template <kind K>
rounded round_192(u192 a, long long z, bool above) {
  return ycxx::detail::fpconv::round_to<K>(a.hi, z + 64, above || a.lo != 0);
}

// Normalises w * t (w != 0) so that its top bit is bit 190 or 191; returns the shift applied.
u192 product(u64 w, u128 t, int& shift) {
  shift = __builtin_clzll(w);
  u192 p = ycxx::detail::fpconv::mul_192(w, t);
  if (shift != 0) {
    p.hi = (p.hi << shift) | (p.lo >> (64 - shift));
    p.lo <<= shift;
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
template <kind K>
bool interval_rounds_alike(const u192& lo, long long z, u64 w, u128 t, int shift, bool exact, bool truncated) {
  constexpr format f = fmt_of<K>;
  const int len = ycxx::detail::fpconv::bit_length(lo.hi);
  const long long lsb = len - 1 + z + 64 - (f.p - 1);
  if (lsb < f.qmin())
    return false;
  const int s = len - f.p;
  if (s < 1)
    return false;
  u128 d; // hi - lo before the shift (exact && !truncated never gets here)
  if (!truncated) {
    d = w - 1;
  } else {
    if (__builtin_clzll(w + 1) != shift) // w has at most 19 digits: w + 1 does not wrap
      return false;
    if (exact)
      d = t - 1;
    else if (__builtin_add_overflow(t, static_cast<u128>(w), &d))
      return false;
  }
  const u128 d_hi = (shift == 0 ? d >> 64 : d >> (64 - shift)) + 2; // covers lo.lo, the low bits and the + 1
  const u128 rem = lo.hi & ycxx::detail::fpconv::low_mask(s);
  const u128 half = u128(1) << (s - 1);
  u128 end;
  if (__builtin_add_overflow(rem, d_hi, &end))
    return false;
  // Below the halfway point: the whole interval must stay below it. At or above it (lo itself
  // rounds up, the value being above lo): below the next halfway point.
  return rem >= half ? end <= (u128(1) << s) + half : end <= half;
}

// Eisel-Lemire: w * 10^q, or (truncated) a value strictly between w * 10^q and (w + 1) * 10^q.
// Returns false when the result cannot be decided this way.
template <kind K>
bool eisel_lemire(u64 w, int q, bool truncated, rounded& out) {
  if (q < pow10_min || q > pow10_max)
    return false;
  const u128 t = ycxx::detail::fpconv::pow10_significand(q);
  const bool exact = q >= 0 && q <= 55; // t is 5^q itself, shifted
  // value = w * t_exact * 2^(q + floor(log2 5^q) - 127)
  const long long base = static_cast<long long>(q) + ycxx::detail::fpconv::floor_log2_pow5(q) - 127;
  int shift;
  u192 lo = ycxx::detail::fpconv::product(w, t, shift);
  if (exact && !truncated) {
    out = ycxx::detail::fpconv::round_192<K>(lo, base - shift, false);
    return true;
  }
  // The value is above lo (t truncated, or digits dropped).
  rounded a = ycxx::detail::fpconv::round_192<K>(lo, base - shift, true);
  if (a.status == round_status::ok && ycxx::detail::fpconv::interval_rounds_alike<K>(lo, base - shift, w, t, shift, exact, truncated)) {
    out = a;
    return true;
  }
  // An upper end: below x * t_exact with x = w + 1 (truncated) or x = w, and x * t_exact is
  // below x * t + x when t is truncated.
  const u64 x = truncated ? w + 1 : w;
  int shift_hi;
  u192 hi = ycxx::detail::fpconv::mul_192(x, t);
  if (!exact)
    hi = ycxx::detail::fpconv::add_192(hi, x);
  hi = ycxx::detail::fpconv::sub1_192(hi); // the value lies in (hi, hi + 1) or below
  shift_hi = __builtin_clzll(x);
  if (shift_hi != 0) {
    // x * (t + 1) < 2^(192 - shift_hi), so the shifted value still fits.
    hi.hi = (hi.hi << shift_hi) | (hi.lo >> (64 - shift_hi));
    hi.lo <<= shift_hi;
  }
  rounded b = ycxx::detail::fpconv::round_192<K>(hi, base - shift_hi, true);
  if (!(a == b))
    return false;
  out = a;
  return true;
}

// Exact conversion of D * 10^e10, D given by its decimal digits (no leading zeros; n >= 1).
template <kind K>
rounded decimal_exact(const char* d, int n, long long e10) {
  constexpr format f = fmt_of<K>;
  constexpr limits lim = limits_of<K>;
  constexpr int N = lim.limbs_in;
  rounded r;
  // value is in [10^(n - 1 + e10), 10^(n + e10)).
  if (n - 1 + e10 > lim.int_digits) {
    r.status = round_status::overflow;
    return r;
  }
  const long long low10 = (f.qmin() - 1) * 30103LL / 100000 - 2;
  if (n + e10 < low10) {
    r.status = round_status::underflow;
    return r;
  }
  bignum<N> a;
  a.n = 0;
  for (int i = 0; i < n;) {
    u32 chunk = 0, scale = 1;
    for (int j = 0; j < 9 && i < n; ++j, ++i) {
      chunk = chunk * 10 + static_cast<u32>(d[i] - '0');
      scale *= 10;
    }
    a.mul_small(scale);
    a.add_small(chunk);
  }
  constexpr int keep = f.p + 3;
  if (e10 >= 0) {
    a.mul_pow10(static_cast<int>(e10));
    int len = a.bit_length();
    if (len <= keep)
      return ycxx::detail::fpconv::round_to<K>(a.to_u128(), 0, false);
    bool rest;
    u128 q = a.top_bits(keep, rest);
    return ycxx::detail::fpconv::round_to<K>(q, len - keep, rest);
  }
  // value = D / (5^k * 2^k): quotient floor(D * 2^s / 5^k) with keep + 1 bits, and remainder.
  const int k = static_cast<int>(-e10);
  bignum<N> b;
  b.set(1);
  b.mul_pow5(k);
  int s = b.bit_length() - a.bit_length() + keep;
  if (s > 0)
    a.shift_left(s);
  else
    b.shift_left(-s);
  // D * 2^s / 5^k (or D / (5^k * 2^-s)) lies in (2^(keep - 1), 2^(keep + 1)).
  b.shift_left(keep);
  u128 q = 0;
  for (int i = keep; i >= 0; --i) {
    if (ycxx::detail::fpconv::compare(a, b) >= 0) {
      a.sub(b);
      q |= u128(1) << i;
    }
    b.shift_right1();
  }
  return ycxx::detail::fpconv::round_to<K>(q, -static_cast<long long>(s) - k, !a.is_zero());
}

// A value that overflows, or is nonzero and rounds to zero, is outside the range of the type
// ([charconv.from.chars]/1): result_out_of_range, value unmodified.
template <kind K>
std::from_chars_result finish(const char* end, const rounded& r, bool negative, fp_raw& out) {
  if (r.status != round_status::ok)
    return {end, std::errc::result_out_of_range};
  out = ycxx::detail::fpconv::encode<K>(negative, r.m, r.biased);
  return {end, std::errc{}};
}

template <kind K>
std::from_chars_result parse_decimal_digits(const char* first, const char* p, const char* last, bool negative,
                                            int fmt, fp_raw& out) {
  constexpr int max_digits = limits_of<K>.max_digits;
  char d[max_digits + 1];
  int n = 0;            // digits stored
  long long e10 = 0;    // value = d * 10^e10 (before the exponent part)
  bool sticky = false;  // a dropped digit was nonzero
  bool any = false;     // a digit was seen
  for (; p != last && ycxx::detail::fpconv::is_digit(*p); ++p) {
    any = true;
    if (n == 0 && *p == '0')
      continue;
    if (n < max_digits)
      d[n++] = *p;
    else {
      ++e10;
      sticky |= *p != '0';
    }
  }
  if (p != last && *p == '.') {
    ++p;
    for (; p != last && ycxx::detail::fpconv::is_digit(*p); ++p) {
      any = true;
      if (n == 0 && *p == '0') {
        --e10;
        continue;
      }
      if (n < max_digits) {
        d[n++] = *p;
        --e10;
      } else {
        sticky |= *p != '0';
      }
    }
  }
  if (!any)
    return {first, std::errc::invalid_argument};
  const auto cf = static_cast<std::chars_format>(fmt);
  const bool sci = (cf & std::chars_format::scientific) == std::chars_format::scientific;
  const bool fix = (cf & std::chars_format::fixed) == std::chars_format::fixed;
  if (sci && p != last && (*p == 'e' || *p == 'E')) {
    long long x = 0;
    if (const char* q = ycxx::detail::fpconv::parse_exponent(p + 1, last, x)) {
      p = q;
      e10 += x;
    } else if (!fix) {
      return {first, std::errc::invalid_argument};
    }
  } else if (sci && !fix) {
    return {first, std::errc::invalid_argument};
  }
  if (n == 0) { // zero
    out = ycxx::detail::fpconv::encode<K>(negative, 0, 0);
    return {p, std::errc{}};
  }
  if (!sticky)
    while (d[n - 1] == '0') {
      --n;
      ++e10;
    }
  // Eisel-Lemire on the leading 19 digits.
  {
    u64 w = 0;
    int take = n < 19 ? n : 19;
    for (int i = 0; i < take; ++i)
      w = w * 10 + static_cast<u64>(d[i] - '0');
    // Whether a nonzero digit lies beyond the 19 taken: a dropped one (sticky), or else any kept
    // digit past them, since the trailing zeros were removed above, so d[n - 1] is nonzero.
    const bool truncated = sticky || n > take;
    long long q = e10 + (n - take);
    rounded r;
    if (q >= pow10_min && q <= pow10_max &&
        ycxx::detail::fpconv::eisel_lemire<K>(w, static_cast<int>(q), truncated, r))
      return ycxx::detail::fpconv::finish<K>(p, r, negative, out);
  }
  if (sticky) {
    d[n++] = '1';
    --e10;
  }
  rounded r = ycxx::detail::fpconv::decimal_exact<K>(d, n, e10);
  return ycxx::detail::fpconv::finish<K>(p, r, negative, out);
}

// If the 8 characters at p are all decimal digits, stores their value in v. The characters are
// loaded as one little-endian word (the first one least significant) and converted in three
// multiply steps: digit pairs, then groups of four, then all eight.
bool eight_digits(const char* p, u64& v) {
  u64 x = 0;
  for (int i = 0; i < 8; ++i) // one load on little-endian targets
    x |= static_cast<u64>(static_cast<unsigned char>(p[i])) << (8 * i);
  // Every byte in 0x30..0x39: its high nibble is 3, and still 3 after adding 6.
  if (((x & 0xF0F0F0F0F0F0F0F0ull) | (((x + 0x0606060606060606ull) & 0xF0F0F0F0F0F0F0F0ull) >> 4)) !=
      0x3333333333333333ull)
    return false;
  x -= 0x3030303030303030ull;
  x = x * 10 + (x >> 8); // byte 2k: the pair (digit 2k, digit 2k + 1)
  x = (((x & 0x000000FF000000FFull) * (100 + (1000000ull << 32))) +
       (((x >> 16) & 0x000000FF000000FFull) * (1 + (10000ull << 32)))) >>
      32;
  v = x & 0xFFFFFFFFull;
  return true;
}

// The digits of a decimal significand as w * 10^e10, keeping the leading 19 significant digits
// in w and only whether the others are nonzero.
struct decimal_scan {
  const char* p;
  const char* last;
  u64 w = 0;
  int taken = 0;        // significant digits in w
  long long e10 = 0;
  bool dropped = false; // a digit after the first 19 significant ones was nonzero
  bool any = false;     // a digit was seen

  // One run of digits (fraction: after the point, where each digit taken scales by 1/10): the
  // leading zeros and the first significant digit one at a time, then 8 at a time while they fit
  // in w, then one at a time again.
  [[gnu::always_inline]] void run(bool fraction) {
    for (; p != last && w == 0 && ycxx::detail::fpconv::is_digit(*p); ++p) {
      any = true;
      w = static_cast<u64>(*p - '0');
      taken = w != 0;
      e10 -= fraction;
    }
    u64 eight;
    while (w != 0 && taken <= 11 && last - p >= 8 && ycxx::detail::fpconv::eight_digits(p, eight)) {
      w = w * 100000000 + eight;
      taken += 8;
      e10 -= fraction ? 8 : 0;
      p += 8;
    }
    for (; p != last && ycxx::detail::fpconv::is_digit(*p); ++p) {
      any = true;
      if (taken < 19) {
        w = w * 10 + static_cast<u64>(*p - '0');
        ++taken;
        e10 -= fraction;
      } else {
        e10 += !fraction;
        dropped |= *p != '0';
      }
    }
  }
};

// The usual case first: the leading 19 significant digits are accumulated directly into w and
// Eisel-Lemire decides. Anything else (an undecided product, an exponent beyond the table, a
// malformed exponent) starts over in parse_decimal_digits, which keeps every digit.
template <kind K>
std::from_chars_result parse_decimal(const char* first, const char* p, const char* last, bool negative, int fmt,
                                     fp_raw& out) {
  const char* const start = p;
  decimal_scan d{p, last};
  d.run(false);
  if (d.p != last && *d.p == '.') {
    ++d.p;
    d.run(true);
  }
  p = d.p;
  const u64 w = d.w;
  long long e10 = d.e10;
  const bool dropped = d.dropped;
  const bool any = d.any;
  if (!any || w == 0)
    return ycxx::detail::fpconv::parse_decimal_digits<K>(first, start, last, negative, fmt, out);
  const auto cf = static_cast<std::chars_format>(fmt);
  const bool sci = (cf & std::chars_format::scientific) == std::chars_format::scientific;
  const bool fix = (cf & std::chars_format::fixed) == std::chars_format::fixed;
  if (sci && p != last && (*p == 'e' || *p == 'E')) {
    long long x = 0;
    const char* q = ycxx::detail::fpconv::parse_exponent(p + 1, last, x);
    if (q == nullptr)
      return ycxx::detail::fpconv::parse_decimal_digits<K>(first, start, last, negative, fmt, out);
    p = q;
    e10 += x;
  } else if (sci && !fix) {
    return {first, std::errc::invalid_argument};
  }
  rounded r;
  if (e10 >= pow10_min && e10 <= pow10_max &&
      ycxx::detail::fpconv::eisel_lemire<K>(w, static_cast<int>(e10), dropped, r))
    return ycxx::detail::fpconv::finish<K>(p, r, negative, out);
  return ycxx::detail::fpconv::parse_decimal_digits<K>(first, start, last, negative, fmt, out);
}

template <kind K>
std::from_chars_result parse_hex(const char* first, const char* p, const char* last, bool negative, fp_raw& out) {
  u128 q = 0;
  int bits = 0;          // significant bits in q
  long long e2 = 0;      // value = q * 2^e2 (before the exponent part)
  bool sticky = false;
  bool any = false;
  auto take = [&](int v, bool fraction) {
    any = true;
    if (bits == 0 && v == 0) {
      if (fraction)
        e2 -= 4;
      return;
    }
    if (bits <= 116) {
      q = (q << 4) | static_cast<u128>(v);
      bits = ycxx::detail::fpconv::bit_length(q);
      if (fraction)
        e2 -= 4;
    } else {
      sticky |= v != 0;
      if (!fraction)
        e2 += 4;
    }
  };
  for (int v; p != last && (v = ycxx::detail::fpconv::hex_value(*p)) >= 0; ++p)
    take(v, false);
  if (p != last && *p == '.') {
    ++p;
    for (int v; p != last && (v = ycxx::detail::fpconv::hex_value(*p)) >= 0; ++p)
      take(v, true);
  }
  if (!any)
    return {first, std::errc::invalid_argument};
  if (p != last && (*p == 'p' || *p == 'P')) {
    long long x = 0;
    if (const char* e = ycxx::detail::fpconv::parse_exponent(p + 1, last, x)) {
      p = e;
      e2 += x;
    }
  }
  if (q == 0) {
    out = ycxx::detail::fpconv::encode<K>(negative, 0, 0);
    return {p, std::errc{}};
  }
  rounded r = ycxx::detail::fpconv::round_to<K>(q, e2, sticky);
  return ycxx::detail::fpconv::finish<K>(p, r, negative, out);
}

template <kind K>
std::from_chars_result from_chars_impl(const char* first, const char* last, fp_raw& out, int fmt) {
  const char* p = first;
  bool negative = false;
  if (p != last && *p == '-') {
    negative = true;
    ++p;
  }
  if (p != last && (ycxx::detail::fpconv::lower(*p) == 'i' || ycxx::detail::fpconv::lower(*p) == 'n')) {
    if (ycxx::detail::fpconv::match_word(p, last, "inf")) {
      p += ycxx::detail::fpconv::match_word(p, last, "infinity") ? 8 : 3;
      out = ycxx::detail::fpconv::encode_infinity<K>(negative);
      return {p, std::errc{}};
    }
    if (ycxx::detail::fpconv::match_word(p, last, "nan")) {
      p += 3;
      if (p != last && *p == '(') {
        const char* q = p + 1;
        while (q != last && (ycxx::detail::fpconv::is_digit(*q) || (*q >= 'a' && *q <= 'z') ||
                             (*q >= 'A' && *q <= 'Z') || *q == '_'))
          ++q;
        if (q != last && *q == ')')
          p = q + 1;
      }
      out = ycxx::detail::fpconv::encode_nan<K>(negative);
      return {p, std::errc{}};
    }
    return {first, std::errc::invalid_argument};
  }
  if (static_cast<std::chars_format>(fmt) == std::chars_format::hex)
    return ycxx::detail::fpconv::parse_hex<K>(first, p, last, negative, out);
  return ycxx::detail::fpconv::parse_decimal<K>(first, p, last, negative, fmt, out);
}

} // namespace
} // namespace ycxx::detail::fpconv

std::from_chars_result ycxx::detail::fp_from_chars(const char* first, const char* last, fp_kind kind, fp_raw& bits,
                                                   int fmt) noexcept {
  using enum ycxx::detail::fp_kind;
  switch (kind) {
  case binary16:
    return ycxx::detail::fpconv::from_chars_impl<binary16>(first, last, bits, fmt);
  case bfloat16:
    return ycxx::detail::fpconv::from_chars_impl<bfloat16>(first, last, bits, fmt);
  case binary32:
    return ycxx::detail::fpconv::from_chars_impl<binary32>(first, last, bits, fmt);
  case binary64:
    return ycxx::detail::fpconv::from_chars_impl<binary64>(first, last, bits, fmt);
  case x87_extended:
    return ycxx::detail::fpconv::from_chars_impl<x87_extended>(first, last, bits, fmt);
  case binary128:
    return ycxx::detail::fpconv::from_chars_impl<binary128>(first, last, bits, fmt);
  }
  __builtin_unreachable();
}
