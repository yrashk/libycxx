// libycxx runtime: floating-point std::to_chars ([charconv.to.chars]).
//
// Shortest round-trip digits:
//   * binary32 and binary64: Schubfach (R. Giulietti, "The Schubfach way to render doubles",
//     2020). The rounding interval is scaled by 10^-k with k = floor(log10(2^q)), so that its
//     width lies in [1, 10); the scaled bounds are computed rounded to odd, which makes the
//     comparisons with multiples of 4 exact. At most one multiple of 10 can lie inside; if one
//     does it is the unique shortest candidate, otherwise the closer of floor(v) and ceil(v)
//     that lies inside wins (ties to even). The 126-bit constants are 10^-k rounded up, or exact
//     where 10^-k * 2^r is an integer; scaled values that are integers are computed exactly.
//   * all other formats, and the smallest subnormals: the exact free-format algorithm of
//     Steele & White / Burger & Dybvig ("Printing floating-point numbers quickly and
//     accurately", 1996) on big integers.
// Precision forms (%e, %f, %g with a precision) are produced from the exact decimal expansion of
// the value, generated nine digits at a time, and rounded half to even, as printf does in the
// default rounding mode. %a works on the bits directly.
#include "fp_common.hpp"

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__fpconv {
namespace {

// ---- shortest digits ---------------------------------------------------------------------------

// value = 0.d[0] d[1] ... d[n-1] * 10^(x + 1), i.e. d[0].d[1]... * 10^x; no trailing zeros.
struct digits {
  char d[48];
  int n = 0;
  int __x = 0;
};

void set_digits(digits& out, __y_u64 __v, int exp10) {
  while (__v % 10 == 0) {
    __v /= 10;
    ++exp10;
  }
  char __tmp[24];
  const char* p = __ycxx::__detail::__charconv_write_unsigned(__tmp + sizeof __tmp, __v, 10); // two digits per step
  const int n = static_cast<int>(__tmp + sizeof __tmp - p);
  __builtin_memcpy(out.d, p, static_cast<std::size_t>(n));
  out.n = n;
  out.__x = exp10 + n - 1;
}

// floor(g * cp / 2^127), with the lowest bit set when the division is inexact (round to odd).
__y_u64 round_to_odd(__u128 __g, __y_u64 __cp) {
  __u128 __p0 = static_cast<__u128>(static_cast<__y_u64>(__g)) * __cp;
  __u128 __p1 = static_cast<__u128>(static_cast<__y_u64>(__g >> 64)) * __cp;
  __y_u64 __low = static_cast<__y_u64>(__p0);
  __u128 __mid = (__p0 >> 64) + static_cast<__y_u64>(__p1);
  __u128 __high = (__p1 >> 64) + (__mid >> 64);
  __y_u64 mid_lo = static_cast<__y_u64>(__mid);
  __y_u64 r = (static_cast<__y_u64>(__high) << 1) | (mid_lo >> 63);
  bool __inexact = (mid_lo << 1) != 0 || __low != 0;
  return r | (__inexact ? 1 : 0);
}

// Schubfach for value = c * 2^q (c >= 1000). Returns false only if no candidate was found, which
// the analysis rules out; the caller then uses the exact algorithm.
template <kind _Kp>
bool schubfach(__y_u64 c, int __q, digits& out) {
  constexpr format __f = __fmt_of<_Kp>;
  const bool asymmetric = c == (__y_u64(1) << (__f.p - 1)) && __q > __f.__qmin();
  const __y_u64 open = c & 1; // odd significand: the interval excludes its bounds
  const __y_u64 __cb = c << 2;
  const __y_u64 cbr = __cb + 2;
  __y_u64 cbl;
  int k;
  if (!asymmetric) {
    cbl = __cb - 2;
    k = __ycxx::__detail::__fpconv::__floor_log10_pow2(__q);
  } else {
    cbl = __cb - 1;
    k = __ycxx::__detail::__fpconv::__floor_log10_three_quarters_pow2(__q);
  }
  const int __j = -k;
  const __u128 t = __ycxx::__detail::__fpconv::__pow10_significand(__j);
  const bool __exact = __j >= 0 && __j <= 55 && (t & 3) == 0;
  const __u128 __g = (t >> 2) + (__exact ? 0 : 1); // 10^-k * 2^(125 - floor(log2(10^-k))), rounded up
  const int h = __q + __j + __ycxx::__detail::__fpconv::__floor_log2_pow5(__j) + 2;
  // x * 2^q * 10^-k, rounded to odd. With g rounded up, the product is exact only where the true
  // value is not an integer; it is one exactly when 5^k divides x (k > 0; q >= k there), and a
  // bound of the interval can be such an integer, so that case is computed exactly.
  const auto scaled = [&](__y_u64 __x) {
    if (k > 0 && k <= 23) {
      __y_u64 p5 = 1;
      for (int i = 0; i < k; ++i)
        p5 *= 5;
      if (__x % p5 == 0)
        return (__x / p5) << (__q - k);
    }
    return __ycxx::__detail::__fpconv::round_to_odd(__g, __x << h);
  };
  const __y_u64 vbl = scaled(cbl);
  const __y_u64 vb = scaled(__cb);
  const __y_u64 vbr = scaled(cbr);
  const __y_u64 s = vb >> 2; // floor(v * 10^-k)
  {
    const __y_u64 sp10 = s / 10 * 10;
    const __y_u64 tp10 = sp10 + 10;
    const bool u_in = vbl + open <= sp10 << 2;
    const bool w_in = (tp10 << 2) + open <= vbr;
    if (u_in != w_in) {
      __ycxx::__detail::__fpconv::set_digits(out, u_in ? sp10 : tp10, k);
      return true;
    }
  }
  const __y_u64 __t1 = s + 1;
  const bool u_in = vbl + open <= s << 2;
  const bool w_in = (__t1 << 2) + open <= vbr;
  if (u_in != w_in) {
    __ycxx::__detail::__fpconv::set_digits(out, u_in ? s : __t1, k);
    return true;
  }
  if (!u_in)
    return false;
  const __y_u64 __mid = (s << 2) + 2; // 4 * (s + 1/2)
  __ycxx::__detail::__fpconv::set_digits(out, vb < __mid || (vb == __mid && (s & 1) == 0) ? s : __t1, k);
  return true;
}

// Exact shortest digits (Burger & Dybvig's free-format algorithm) for value = m * 2^e.
template <kind _Kp>
void shortest_exact(__u128 m, int e, digits& out) {
  constexpr format __f = __fmt_of<_Kp>;
  constexpr int _Np = __limits_of<_Kp>.__limbs_out;
  const bool asymmetric = m == (__u128(1) << (__f.p - 1)) && e > __f.__qmin();
  const bool inclusive = (m & 1) == 0;
  // value = r / s; the rounding interval is ((r - mm) / s, (r + mp) / s).
  __bignum<_Np> r, s, __mp, mm;
  r.set(m);
  if (e >= 0) {
    r.shift_left(e + (asymmetric ? 2 : 1));
    s.set(asymmetric ? 4 : 2);
    __mp.set(1);
    __mp.shift_left(e + (asymmetric ? 1 : 0));
    mm.set(1);
    mm.shift_left(e);
  } else {
    r.shift_left(asymmetric ? 2 : 1);
    s.set(1);
    s.shift_left(-e + (asymmetric ? 2 : 1));
    __mp.set(asymmetric ? 2 : 1);
    mm.set(1);
  }
  // Scale so that 10^(k-1) <= value < 10^k: the first digit is nonzero, and the candidates with
  // one digit are d * 10^(k-1) and, when d = 9 rounds up, 10^k. (Scaling by the upper bound
  // instead would miss 9 * 10^(k-1) when the interval straddles 10^(k-1) and the value lies
  // below it.)
  int k = __ycxx::__detail::__fpconv::__floor_log10_pow2(__ycxx::__detail::__fpconv::__bit_length(m) - 1 + e) + 1;
  if (k >= 0) {
    s.__mul_pow10(k);
  } else {
    r.__mul_pow10(-k);
    __mp.__mul_pow10(-k);
    mm.__mul_pow10(-k);
  }
  while (__ycxx::__detail::__fpconv::compare(r, s) >= 0) {
    s.__mul_small(10);
    ++k;
  }
  __bignum<_Np> __tmp;
  int n = 0;
  for (;;) {
    r.__mul_small(10);
    __mp.__mul_small(10);
    mm.__mul_small(10);
    int d = 0;
    while (__ycxx::__detail::__fpconv::compare(r, s) >= 0) {
      r.__sub(s);
      ++d;
    }
    int __cl = __ycxx::__detail::__fpconv::compare(r, mm);
    bool __low = inclusive ? __cl <= 0 : __cl < 0;
    __tmp = r;
    __tmp.add(__mp);
    int __ch = __ycxx::__detail::__fpconv::compare(__tmp, s);
    bool __high = inclusive ? __ch >= 0 : __ch > 0;
    if (!__low && !__high) {
      out.d[n++] = static_cast<char>('0' + d);
      continue;
    }
    if (__low && __high) {
      __tmp = r;
      __tmp.shift_left(1);
      int c = __ycxx::__detail::__fpconv::compare(__tmp, s);
      if (c > 0 || (c == 0 && d % 2 == 1))
        ++d;
    } else if (__high) {
      ++d;
    }
    // Rounding up can carry (9 -> 10); it cannot reach past the first digit except to 10^k.
    while (d == 10) {
      if (n == 0) {
        d = 1;
        ++k;
        break;
      }
      d = out.d[--n] - '0' + 1;
    }
    out.d[n++] = static_cast<char>('0' + d);
    break;
  }
  while (n > 1 && out.d[n - 1] == '0')
    --n;
  out.n = n;
  out.__x = k - 1;
}

template <kind _Kp>
void __shortest(const __decoded& __v, digits& out) {
  if constexpr (_Kp == kind::__binary32 || _Kp == kind::__binary64) {
    if (__v.m >= 1000 && __ycxx::__detail::__fpconv::schubfach<_Kp>(static_cast<__y_u64>(__v.m), __v.e, out))
      return;
  }
  __ycxx::__detail::__fpconv::shortest_exact<_Kp>(__v.m, __v.e, out);
}

// ---- layouts -----------------------------------------------------------------------------------

// d[0..n) with exponent x in the f style; integer digits beyond n are zeros.
std::to_chars_result layout_fixed(char* first, char* last, bool __negative, const char* d, int n, int __x) {
  long long __len = __negative ? 1 : 0;
  if (__x >= n - 1)
    __len += __x + 1;
  else if (__x >= 0)
    __len += n + 1;
  else
    __len += 2 + (-__x - 1) + n;
  if (__len > last - first)
    return __ycxx::__detail::__fpconv::__too_large(last);
  char* p = first;
  if (__negative)
    *p++ = '-';
  if (__x >= n - 1) {
    for (int i = 0; i < n; ++i)
      *p++ = d[i];
    for (int i = n; i <= __x; ++i)
      *p++ = '0';
  } else if (__x >= 0) {
    __builtin_memcpy(p, d, static_cast<std::size_t>(__x + 1));
    p += __x + 1;
    *p++ = '.';
    __builtin_memcpy(p, d + __x + 1, static_cast<std::size_t>(n - __x - 1));
    p += n - __x - 1;
  } else {
    *p++ = '0';
    *p++ = '.';
    for (int i = 0; i < -__x - 1; ++i)
      *p++ = '0';
    for (int i = 0; i < n; ++i)
      *p++ = d[i];
  }
  return {p, std::errc{}};
}

int scientific_length(bool __negative, int n, int __x) {
  return (__negative ? 1 : 0) + n + (n > 1 ? 1 : 0) + __ycxx::__detail::__fpconv::__exponent_length(__x);
}

std::to_chars_result layout_scientific(char* first, char* last, bool __negative, const char* d, int n, int __x) {
  if (__ycxx::__detail::__fpconv::scientific_length(__negative, n, __x) > last - first)
    return __ycxx::__detail::__fpconv::__too_large(last);
  char* p = first;
  if (__negative)
    *p++ = '-';
  *p++ = d[0];
  if (n > 1) {
    *p++ = '.';
    for (int i = 1; i < n; ++i)
      *p++ = d[i];
  }
  return {__ycxx::__detail::__fpconv::__write_exponent(p, __x), std::errc{}};
}

// ---- exact decimal expansion -------------------------------------------------------------------

// The decimal digits of value = m * 2^e: the integer part, all at once, then the fraction, nine
// digits at a time on demand.
template <kind _Kp>
struct expansion {
  static constexpr int _Np = __limits_of<_Kp>.__limbs_out;
  char* __int_digits = nullptr; // the integer part, no leading zeros
  int int_count = 0;
  __bignum<_Np> __frac; // fraction = frac / 2^frac_bits
  int frac_bits = 0;
  int low_limb = 0; // limbs of frac below this one are zero
  char queue[9];
  int queue_size = 0, queue_pos = 0;

  // The integer part has at most limits_of<K>.int_digits digits; buf holds 8 more, a whole chunk
  // past that bound, which keeps the bound visible to the compiler.
  void init(__u128 m, int e, char* __buf) {
    __int_digits = __buf;
    __bignum<_Np> __ip;
    if (e >= 0) {
      __ip.set(m);
      __ip.shift_left(e);
      __frac.n = 0;
    } else {
      int s = -e;
      __ip.set(s >= 128 ? 0 : m >> s);
      __frac.set(s >= 128 ? m : m & __ycxx::__detail::__fpconv::__low_mask(s));
      frac_bits = s;
    }
    // Integer part: base-10^9 chunks, least significant first.
    __y_u32 __chunks[__limits_of<_Kp>.__int_digits / 9 + 2];
    int __nc = 0;
    while (!__ip.__is_zero())
      __chunks[__nc++] = __ip.__div_small(1000000000u);
    int_count = 0;
    for (int i = __nc - 1; i >= 0; --i) {
      char __tmp[9];
      __y_u32 c = __chunks[i];
      for (int __j = 8; __j >= 0; --__j, c /= 10)
        __tmp[__j] = static_cast<char>('0' + c % 10);
      int __skip = 0;
      if (i == __nc - 1)
        while (__skip < 8 && __tmp[__skip] == '0')
          ++__skip;
      for (int __j = __skip; __j < 9; ++__j)
        __int_digits[int_count++] = __tmp[__j];
    }
  }
  bool fraction_exhausted() const { return queue_pos == queue_size && __frac.__is_zero(); }
  // The next fraction digit (0 once the expansion has ended).
  int next() {
    if (queue_pos == queue_size) {
      if (__frac.__is_zero())
        return 0;
      __frac.__mul_small(1000000000u, low_limb);
      int __li = frac_bits / 32, __y_sh = frac_bits % 32;
      __y_u64 __two = (__li < __frac.n ? __frac.__w[__li] : 0) | (__li + 1 < __frac.n ? static_cast<__y_u64>(__frac.__w[__li + 1]) << 32 : 0);
      __y_u32 chunk = static_cast<__y_u32>(__two >> __y_sh);
      if (__li < __frac.n) {
        __frac.__w[__li] &= (__y_u32(1) << __y_sh) - 1;
        __frac.n = __li + 1;
        __frac.__trim();
      }
      while (low_limb < __frac.n && __frac.__w[low_limb] == 0)
        ++low_limb;
      for (int __j = 8; __j >= 0; --__j, chunk /= 10)
        queue[__j] = static_cast<char>(chunk % 10);
      queue_size = 9;
      queue_pos = 0;
    }
    return queue[queue_pos++];
  }
  bool rest_zero() const {
    for (int i = queue_pos; i < queue_size; ++i)
      if (queue[i] != 0)
        return false;
    return __frac.__is_zero();
  }
};

// Adds one unit in the last place to the digits in [begin, end), skipping a '.'. Returns false if
// the carry ran out of the first digit (all digits were 9 and are now 0).
bool increment(char* begin, char* end) {
  for (char* c = end; c != begin;) {
    --c;
    if (*c == '.')
      continue;
    if (*c == '9') {
      *c = '0';
      continue;
    }
    ++*c;
    return true;
  }
  return false;
}

bool round_up(int next_digit, bool __sticky, char last_digit) {
  return next_digit > 5 || (next_digit == 5 && (__sticky || (last_digit - '0') % 2 == 1));
}

// %.Pf
template <kind _Kp>
std::to_chars_result fixed_precision(char* first, char* last, const __decoded& __v, int precision) {
  char int_buf[__limits_of<_Kp>.__int_digits + 9];
  expansion<_Kp> __x;
  const bool zero = __v.__cls == __fp_class::zero;
  if (!zero)
    __x.init(__v.m, __v.e, int_buf);
  const int __ni = zero ? 0 : __x.int_count;
  long long __len = (__v.__negative ? 1 : 0) + (__ni == 0 ? 1 : __ni) + (precision > 0 ? 1LL + precision : 0);
  if (__len > last - first)
    return __ycxx::__detail::__fpconv::__too_large(last);
  char* p = first;
  if (__v.__negative)
    *p++ = '-';
  char* digits_begin = p;
  if (__ni == 0)
    *p++ = '0';
  for (int i = 0; i < __ni; ++i)
    *p++ = __x.__int_digits[i];
  if (precision > 0) {
    *p++ = '.';
    char* end = p + precision;
    while (p != end && !(zero || __x.fraction_exhausted()))
      *p++ = static_cast<char>('0' + __x.next());
    while (p != end)
      *p++ = '0';
  }
  if (!zero) {
    int next_digit = __x.next();
    if (__ycxx::__detail::__fpconv::round_up(next_digit, !__x.rest_zero(), p[-1]) &&
        !__ycxx::__detail::__fpconv::increment(digits_begin, p)) {
      // 99.9 -> 100.0: one more integer digit.
      if (p == last)
        return __ycxx::__detail::__fpconv::__too_large(last);
      for (char* c = p; c != digits_begin; --c)
        *c = c[-1];
      *digits_begin = '1';
      ++p;
    }
  }
  return {p, std::errc{}};
}

// The significant digits of a nonzero value, most significant first.
template <kind _Kp>
struct significant_stream {
  expansion<_Kp>& __x;
  int __pos = 0;
  int exponent = 0; // decimal exponent of the first digit
  int first = 0;    // the first digit (nonzero)

  explicit significant_stream(expansion<_Kp>& e) : __x(e) {
    if (__x.int_count > 0) {
      exponent = __x.int_count - 1;
      first = __x.__int_digits[0] - '0';
      __pos = 1;
    } else {
      exponent = -1;
      __pos = 0;
      while ((first = __x.next()) == 0)
        --exponent;
    }
  }
  bool exhausted() const { return __pos >= __x.int_count && __x.fraction_exhausted(); }
  int next() { return __pos < __x.int_count ? __x.__int_digits[__pos++] - '0' : __x.next(); }
  bool rest_zero() const {
    for (int i = __pos; i < __x.int_count; ++i)
      if (__x.__int_digits[i] != '0')
        return false;
    return __x.rest_zero();
  }
};

// %.Pe
template <kind _Kp>
std::to_chars_result scientific_precision(char* first, char* last, const __decoded& __v, int precision) {
  long long mantissa_len = (__v.__negative ? 1 : 0) + 1 + (precision > 0 ? 1LL + precision : 0);
  if (mantissa_len + 4 > last - first)
    return __ycxx::__detail::__fpconv::__too_large(last);
  char* p = first;
  if (__v.__negative)
    *p++ = '-';
  char* digits_begin = p;
  int exponent = 0;
  if (__v.__cls == __fp_class::zero) {
    *p++ = '0';
    if (precision > 0) {
      *p++ = '.';
      for (int i = 0; i < precision; ++i)
        *p++ = '0';
    }
  } else {
    char int_buf[__limits_of<_Kp>.__int_digits + 9];
    expansion<_Kp> __x;
    __x.init(__v.m, __v.e, int_buf);
    significant_stream<_Kp> s(__x);
    exponent = s.exponent;
    *p++ = static_cast<char>('0' + s.first);
    if (precision > 0) {
      *p++ = '.';
      char* end = p + precision;
      while (p != end && !s.exhausted())
        *p++ = static_cast<char>('0' + s.next());
      while (p != end)
        *p++ = '0';
    }
    int next_digit = s.next();
    if (__ycxx::__detail::__fpconv::round_up(next_digit, !s.rest_zero(), p[-1]) &&
        !__ycxx::__detail::__fpconv::increment(digits_begin, p)) {
      *digits_begin = '1'; // 9.99 -> 1.00e+1
      ++exponent;
    }
  }
  if (__ycxx::__detail::__fpconv::__exponent_length(exponent) > last - p)
    return __ycxx::__detail::__fpconv::__too_large(last);
  return {__ycxx::__detail::__fpconv::__write_exponent(p, exponent), std::errc{}};
}

// %.Pg
template <kind _Kp>
std::to_chars_result general_precision(char* first, char* last, const __decoded& __v, int precision) {
  if (precision == 0)
    precision = 1;
  if (__v.__cls == __fp_class::zero) {
    const char zero = '0';
    return __ycxx::__detail::__fpconv::layout_fixed(first, last, __v.__negative, &zero, 1, 0);
  }
  // Digits past the exact expansion are zeros, which %g removes: keep at most sig_digits + 1.
  constexpr int __cap = __limits_of<_Kp>.__sig_digits + 1;
  const int __want = precision < __cap ? precision : __cap;
  char __buf[__cap];
  char int_buf[__limits_of<_Kp>.__int_digits + 9];
  expansion<_Kp> __x;
  __x.init(__v.m, __v.e, int_buf);
  significant_stream<_Kp> s(__x);
  int exponent = s.exponent;
  __buf[0] = static_cast<char>('0' + s.first);
  int n = 1;
  while (n < __want && !s.exhausted())
    __buf[n++] = static_cast<char>('0' + s.next());
  if (n == __want && __want == precision) {
    int next_digit = s.next();
    if (__ycxx::__detail::__fpconv::round_up(next_digit, !s.rest_zero(), __buf[n - 1]) &&
        !__ycxx::__detail::__fpconv::increment(__buf, __buf + n)) {
      __buf[0] = '1';
      ++exponent;
    }
  }
  while (n > 1 && __buf[n - 1] == '0')
    --n;
  if (precision > exponent && exponent >= -4)
    return __ycxx::__detail::__fpconv::layout_fixed(first, last, __v.__negative, __buf, n, exponent);
  return __ycxx::__detail::__fpconv::layout_scientific(first, last, __v.__negative, __buf, n, exponent);
}

// ---- %a ----------------------------------------------------------------------------------------

// One hexadecimal digit before the point (1 for normal numbers, 0 for subnormal ones and zero),
// the fraction bits padded to whole digits, and the binary exponent of that leading digit.
// x87 subnormal numbers follow the C library instead: the 64-bit significand read as one digit
// and fifteen, with exponent emin - 3 (0x0.000000000000001p-16385 is the smallest).
template <kind _Kp>
std::to_chars_result hex(char* first, char* last, const __decoded& __v, int precision) {
  constexpr format __f = __fmt_of<_Kp>;
  const bool nibble_subnormal = __f.__explicit_bit && __v.__cls != __fp_class::zero && (__v.m >> (__f.p - 1)) == 0;
  const int __fbits = nibble_subnormal ? __f.p - 4 : __f.p - 1;
  const int digits_all = (__fbits + 3) / 4;
  __u128 __lead = 0, __frac = 0;
  int exponent = 0;
  if (nibble_subnormal) {
    __lead = __v.m >> __fbits;
    __frac = __v.m & __ycxx::__detail::__fpconv::__low_mask(__fbits);
    exponent = __f.__emin - 3;
  } else if (__v.__cls != __fp_class::zero) {
    if (__v.m >> (__f.p - 1)) {
      __lead = 1;
      __frac = __v.m & __ycxx::__detail::__fpconv::__low_mask(__fbits);
      exponent = __v.e + __f.p - 1;
    } else {
      __frac = __v.m;
      exponent = __f.__emin;
    }
  }
  __frac <<= 4 * digits_all - __fbits;
  int __nd = digits_all;
  if (precision < 0) {
    while (__nd > 0 && (__frac & 0xf) == 0) {
      __frac >>= 4;
      --__nd;
    }
  } else if (precision < digits_all) {
    // Round to `precision` digits, half to even; a carry may reach the leading digit.
    __u128 __g = (__lead << (4 * digits_all)) | __frac;
    int drop = 4 * (digits_all - precision);
    __u128 rem = __g & __ycxx::__detail::__fpconv::__low_mask(drop);
    __u128 __half = __u128(1) << (drop - 1);
    __g >>= drop;
    if (rem > __half || (rem == __half && (__g & 1) != 0))
      ++__g;
    __nd = precision;
    __lead = __g >> (4 * __nd);
    __frac = __g & __ycxx::__detail::__fpconv::__low_mask(4 * __nd);
  }
  const int __pad = precision > __nd ? precision - __nd : 0;
  const int total_frac = __nd + __pad;
  int a = exponent < 0 ? -exponent : exponent;
  long long __len = (__v.__negative ? 1 : 0) + 1 + (total_frac > 0 ? 1LL + total_frac : 0) + 2 +
                  __ycxx::__detail::__fpconv::__decimal_length(static_cast<unsigned>(a));
  if (__len > last - first)
    return __ycxx::__detail::__fpconv::__too_large(last);
  char* p = first;
  if (__v.__negative)
    *p++ = '-';
  *p++ = __ycxx::__detail::__charconv_digits[static_cast<int>(__lead)];
  if (total_frac > 0) {
    *p++ = '.';
    for (int i = __nd - 1; i >= 0; --i)
      *p++ = __ycxx::__detail::__charconv_digits[static_cast<int>((__frac >> (4 * i)) & 0xf)];
    for (int i = 0; i < __pad; ++i)
      *p++ = '0';
  }
  *p++ = 'p';
  *p++ = exponent < 0 ? '-' : '+';
  int d = __ycxx::__detail::__fpconv::__decimal_length(static_cast<unsigned>(a));
  for (int i = d - 1; i >= 0; --i, a /= 10)
    p[i] = static_cast<char>('0' + a % 10);
  return {p + d, std::errc{}};
}

// ---- shortest forms ----------------------------------------------------------------------------

// The exact decimal digits of an integer value m * 2^e (e >= 1) into buf; returns the count.
template <kind _Kp>
int integer_digits(__u128 m, int e, char* __buf) {
  expansion<_Kp> __x;
  __x.init(m, e, __buf);
  return __x.int_count;
}

// %f, shortest. An integer value with a spacing of 2 or more is printed exactly: its exact digits
// round-trip, and among the round-tripping integers with that many digits it is the closest.
// (Where the rounding interval reaches below 10^(L-1), L the value's digit count, L-1 nines would
// also round-trip in one character less; like the other implementations, libycxx prints the
// exact value. See STATUS.md, deliberate divergences.)
template <kind _Kp>
std::to_chars_result fixed_shortest(char* first, char* last, const __decoded& __v, const digits& s) {
  if (__v.e < 1)
    return __ycxx::__detail::__fpconv::layout_fixed(first, last, __v.__negative, s.d, s.n, s.__x);
  char __buf[__limits_of<_Kp>.__int_digits + 9];
  int __len = __ycxx::__detail::__fpconv::integer_digits<_Kp>(__v.m, __v.e, __buf);
  return __ycxx::__detail::__fpconv::layout_fixed(first, last, __v.__negative, __buf, __len, __len - 1);
}

// %g, shortest: the shortest output of %g for any precision P that round-trips.
template <kind _Kp>
std::to_chars_result general_shortest(char* first, char* last, const __decoded& __v, const digits& s) {
  if (s.__x < -4)
    return __ycxx::__detail::__fpconv::layout_scientific(first, last, __v.__negative, s.d, s.n, s.__x);
  if (s.__x < s.n)
    return __ycxx::__detail::__fpconv::layout_fixed(first, last, __v.__negative, s.d, s.n, s.__x);
  // The e style needs P <= x; the f style (P > x) shows every integer digit of the value.
  int f_len;
  if (__v.e >= 1) {
    char __buf[__limits_of<_Kp>.__int_digits + 9];
    f_len = __ycxx::__detail::__fpconv::integer_digits<_Kp>(__v.m, __v.e, __buf);
    if (f_len <= __ycxx::__detail::__fpconv::scientific_length(false, s.n, s.__x))
      return __ycxx::__detail::__fpconv::layout_fixed(first, last, __v.__negative, __buf, f_len, f_len - 1);
  } else {
    f_len = s.__x + 1;
    if (f_len <= __ycxx::__detail::__fpconv::scientific_length(false, s.n, s.__x))
      return __ycxx::__detail::__fpconv::layout_fixed(first, last, __v.__negative, s.d, s.n, s.__x);
  }
  return __ycxx::__detail::__fpconv::layout_scientific(first, last, __v.__negative, s.d, s.n, s.__x);
}

// [charconv.to.chars]/7: f for 10^-4 <= |value| < 10^U, U = floor(log10(2^(p+1))), else e.
template <kind _Kp>
bool plain_uses_fixed(const __decoded& __v) {
  constexpr format __f = __fmt_of<_Kp>;
  constexpr __u128 upper = [] {
    __u128 u = 1;
    for (int i = 0; i < __ycxx::__detail::__fpconv::__floor_log10_pow2(__f.p + 1); ++i)
      u *= 10;
    return u;
  }();
  bool at_least_low = __v.e >= 0 || (-__v.e < 128 && __v.m * 10000 >= (__u128(1) << -__v.e));
  bool below_high;
  if (__v.e >= 0)
    below_high = __ycxx::__detail::__fpconv::__bit_length(__v.m) + __v.e <= 120 && (__v.m << __v.e) < upper;
  else
    below_high = -__v.e >= 128 || (__v.m >> -__v.e) < upper;
  return at_least_low && below_high;
}

template <kind _Kp>
std::to_chars_result __to_chars_shortest(char* first, char* last, const __decoded& __v, int __fmt) {
  const auto __cf = static_cast<std::chars_format>(__fmt);
  if (__fmt != 0 && __cf == std::chars_format::hex)
    return __ycxx::__detail::__fpconv::hex<_Kp>(first, last, __v, -1);
  if (__v.__cls == __fp_class::zero) {
    const char zero = '0';
    if (__cf == std::chars_format::scientific)
      return __ycxx::__detail::__fpconv::layout_scientific(first, last, __v.__negative, &zero, 1, 0);
    return __ycxx::__detail::__fpconv::layout_fixed(first, last, __v.__negative, &zero, 1, 0);
  }
  digits s;
  __ycxx::__detail::__fpconv::__shortest<_Kp>(__v, s);
  if (__fmt == 0)
    return __ycxx::__detail::__fpconv::plain_uses_fixed<_Kp>(__v)
               ? __ycxx::__detail::__fpconv::fixed_shortest<_Kp>(first, last, __v, s)
               : __ycxx::__detail::__fpconv::layout_scientific(first, last, __v.__negative, s.d, s.n, s.__x);
  if (__cf == std::chars_format::scientific)
    return __ycxx::__detail::__fpconv::layout_scientific(first, last, __v.__negative, s.d, s.n, s.__x);
  if (__cf == std::chars_format::fixed)
    return __ycxx::__detail::__fpconv::fixed_shortest<_Kp>(first, last, __v, s);
  return __ycxx::__detail::__fpconv::general_shortest<_Kp>(first, last, __v, s);
}

// to_chars(first, last, value) of a normal binary32 or binary64 value, the common case, on 64-bit
// integers in registers: the same decisions as __to_chars_shortest with __fmt == 0, without the
// general __decoded (whose 128-bit significand GCC stores in halves and copies whole, a store-
// forwarding stall). Returns false for the cases left to the general path.
template <kind _Kp>
bool plain_shortest_normal(char* first, char* last, __y_u64 bits, std::to_chars_result& r) {
  constexpr format __f = __fmt_of<_Kp>;
  const int __fb = __f.p - 1;
  const bool __negative = ((bits >> (__fb + __f.__exp_bits)) & 1) != 0;
  const int __bexp = static_cast<int>((bits >> __fb) & ((1u << __f.__exp_bits) - 1));
  if (__bexp == 0 || __bexp == (1 << __f.__exp_bits) - 1)
    return false;
  const __y_u64 m = (bits & ((__y_u64(1) << __fb) - 1)) | (__y_u64(1) << __fb);
  const int e = __bexp - __f.__emax - __fb;
  digits s;
  if (!__ycxx::__detail::__fpconv::schubfach<_Kp>(m, e, s))
    return false;
  // plain_uses_fixed: 10^-4 <= value < 10^U.
  constexpr __u128 upper = [] {
    __u128 u = 1;
    for (int i = 0; i < __ycxx::__detail::__fpconv::__floor_log10_pow2(__f.p + 1); ++i)
      u *= 10;
    return u;
  }();
  const bool at_least_low = e >= 0 || (-e < 128 && static_cast<__u128>(m) * 10000 >= (__u128(1) << -e));
  const bool below_high = e >= 0 ? (__ycxx::__detail::__fpconv::__bit_length(m) + e <= 120 && (static_cast<__u128>(m) << e) < upper)
                                 : (-e >= 64 || (m >> -e) < upper);
  if (!(at_least_low && below_high)) {
    r = __ycxx::__detail::__fpconv::layout_scientific(first, last, __negative, s.d, s.n, s.__x);
    return true;
  }
  if (e >= 1)
    return false; // an integer with a spacing of 2 or more: fixed_shortest's exact digits
  r = __ycxx::__detail::__fpconv::layout_fixed(first, last, __negative, s.d, s.n, s.__x);
  return true;
}

// %.Pf of a finite binary32 or binary64 value m * 2^e with P <= 18, in 128-bit integers: for
// e < 0, m * 10^P < 2^113 is exact, its quotient by 2^-e is the digits and the remainder decides
// the rounding (half to even, as fixed_precision); for e >= 0 the value is the integer m * 2^e
// (< 2^128 for e <= 74) followed by P zeros. Returns false for the cases left to fixed_precision.
inline constexpr __u128 __pow10_small[19] = {1ull, 10ull, 100ull, 1000ull, 10000ull, 100000ull, 1000000ull, 10000000ull,
                                             100000000ull, 1000000000ull, 10000000000ull, 100000000000ull,
                                             1000000000000ull, 10000000000000ull, 100000000000000ull,
                                             1000000000000000ull, 10000000000000000ull, 100000000000000000ull,
                                             1000000000000000000ull};
template <kind _Kp>
bool fixed_precision_small(char* first, char* last, __y_u64 bits, int precision, std::to_chars_result& r) {
  constexpr format __f = __fmt_of<_Kp>;
  const int __fb = __f.p - 1;
  if (precision > 18)
    return false;
  const bool __negative = ((bits >> (__fb + __f.__exp_bits)) & 1) != 0;
  const int __bexp = static_cast<int>((bits >> __fb) & ((1u << __f.__exp_bits) - 1));
  if (__bexp == (1 << __f.__exp_bits) - 1)
    return false;
  const __y_u64 field = bits & ((__y_u64(1) << __fb) - 1);
  const __y_u64 m = __bexp == 0 ? field : field | (__y_u64(1) << __fb);
  const int e = __bexp == 0 ? __f.__qmin() : __bexp - __f.__emax - __fb;
  __u128 __q;
  int __zeros = 0; // fraction digits that are zeros after q's digits (e >= 0)
  if (e >= 0) {
    if (e > 74)
      return false;
    __q = static_cast<__u128>(m) << e;
    __zeros = precision;
  } else {
    const int s = -e;
    if (s >= 128)
      return false;
    const __u128 __prod = static_cast<__u128>(m) * __pow10_small[precision];
    __q = __prod >> s;
    const __u128 rem = __prod & ((__u128(1) << s) - 1);
    const __u128 __half = __u128(1) << (s - 1);
    if (rem > __half || (rem == __half && (__q & 1) != 0))
      ++__q;
  }
  char __buf[48];
  char* const end = __buf + sizeof __buf;
  char* p = __ycxx::__detail::__charconv_write_unsigned(end, __q, 10);
  const int __frac_in_q = precision - __zeros; // the last digits of q are fraction digits
  while (end - p < __frac_in_q + 1)
    *--p = '0'; // "0.00..." for a value below 1
  const int __nd = static_cast<int>(end - p);
  const long long __len = (__negative ? 1 : 0) + __nd + __zeros + (precision > 0 ? 1 : 0);
  if (__len > last - first) {
    r = __ycxx::__detail::__fpconv::__too_large(last);
    return true;
  }
  char* o = first;
  if (__negative)
    *o++ = '-';
  const int __int_len = __nd - __frac_in_q;
  __builtin_memcpy(o, p, static_cast<std::size_t>(__int_len));
  o += __int_len;
  if (precision > 0) {
    *o++ = '.';
    __builtin_memcpy(o, p + __int_len, static_cast<std::size_t>(__frac_in_q));
    o += __frac_in_q;
    for (int i = 0; i < __zeros; ++i)
      *o++ = '0';
  }
  r = {o, std::errc{}};
  return true;
}

template <kind _Kp>
std::to_chars_result to_chars_impl(char* first, char* last, __fp_raw bits, int __fmt, int precision) {
  if constexpr (_Kp == kind::__binary64 || _Kp == kind::__binary32) {
    std::to_chars_result r;
    if (precision < 0 && __fmt == 0 && __ycxx::__detail::__fpconv::plain_shortest_normal<_Kp>(first, last, bits.__lo, r))
      return r;
    if (precision >= 0 && static_cast<std::chars_format>(__fmt) == std::chars_format::fixed &&
        __ycxx::__detail::__fpconv::fixed_precision_small<_Kp>(first, last, bits.__lo, precision, r))
      return r;
  }
  const __decoded __v = __ycxx::__detail::__fpconv::__decode<_Kp>(bits);
  std::to_chars_result r;
  if (__ycxx::__detail::__fpconv::__write_special(first, last, __v, r))
    return r;
  if (precision < 0)
    return __ycxx::__detail::__fpconv::__to_chars_shortest<_Kp>(first, last, __v, __fmt);
  switch (static_cast<std::chars_format>(__fmt)) {
  case std::chars_format::fixed:
    return __ycxx::__detail::__fpconv::fixed_precision<_Kp>(first, last, __v, precision);
  case std::chars_format::scientific:
    return __ycxx::__detail::__fpconv::scientific_precision<_Kp>(first, last, __v, precision);
  case std::chars_format::hex:
    return __ycxx::__detail::__fpconv::hex<_Kp>(first, last, __v, precision);
  default:
    return __ycxx::__detail::__fpconv::general_precision<_Kp>(first, last, __v, precision);
  }
}

} // namespace
}} // namespace __ycxx::__detail::__fpconv

std::to_chars_result __ycxx::__detail::__fp_to_chars(char* first, char* last, __fp_kind kind, __fp_raw bits, int __fmt,
                                               int precision) noexcept {
  using enum __ycxx::__detail::__fp_kind;
  switch (kind) {
  case __binary16:
    return __ycxx::__detail::__fpconv::to_chars_impl<__binary16>(first, last, bits, __fmt, precision);
  case __bfloat16:
    return __ycxx::__detail::__fpconv::to_chars_impl<__bfloat16>(first, last, bits, __fmt, precision);
  case __binary32:
    return __ycxx::__detail::__fpconv::to_chars_impl<__binary32>(first, last, bits, __fmt, precision);
  case __binary64:
    return __ycxx::__detail::__fpconv::to_chars_impl<__binary64>(first, last, bits, __fmt, precision);
  case __x87_extended:
    return __ycxx::__detail::__fpconv::to_chars_impl<__x87_extended>(first, last, bits, __fmt, precision);
  case __binary128:
    return __ycxx::__detail::__fpconv::to_chars_impl<__binary128>(first, last, bits, __fmt, precision);
  }
  __builtin_unreachable();
}
