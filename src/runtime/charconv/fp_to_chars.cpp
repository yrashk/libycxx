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

namespace ycxx::detail::fpconv {
namespace {

// ---- shortest digits ---------------------------------------------------------------------------

// value = 0.d[0] d[1] ... d[n-1] * 10^(x + 1), i.e. d[0].d[1]... * 10^x; no trailing zeros.
struct digits {
  char d[48];
  int n = 0;
  int x = 0;
};

void set_digits(digits& out, u64 v, int exp10) {
  while (v % 10 == 0) {
    v /= 10;
    ++exp10;
  }
  char tmp[24];
  const char* p = ycxx::detail::charconv_write_unsigned(tmp + sizeof tmp, v, 10); // two digits per step
  const int n = static_cast<int>(tmp + sizeof tmp - p);
  __builtin_memcpy(out.d, p, static_cast<std::size_t>(n));
  out.n = n;
  out.x = exp10 + n - 1;
}

// floor(g * cp / 2^127), with the lowest bit set when the division is inexact (round to odd).
u64 round_to_odd(u128 g, u64 cp) {
  u128 p0 = static_cast<u128>(static_cast<u64>(g)) * cp;
  u128 p1 = static_cast<u128>(static_cast<u64>(g >> 64)) * cp;
  u64 low = static_cast<u64>(p0);
  u128 mid = (p0 >> 64) + static_cast<u64>(p1);
  u128 high = (p1 >> 64) + (mid >> 64);
  u64 mid_lo = static_cast<u64>(mid);
  u64 r = (static_cast<u64>(high) << 1) | (mid_lo >> 63);
  bool inexact = (mid_lo << 1) != 0 || low != 0;
  return r | (inexact ? 1 : 0);
}

// Schubfach for value = c * 2^q (c >= 1000). Returns false only if no candidate was found, which
// the analysis rules out; the caller then uses the exact algorithm.
template <kind K>
bool schubfach(u64 c, int q, digits& out) {
  constexpr format f = fmt_of<K>;
  const bool asymmetric = c == (u64(1) << (f.p - 1)) && q > f.qmin();
  const u64 open = c & 1; // odd significand: the interval excludes its bounds
  const u64 cb = c << 2;
  const u64 cbr = cb + 2;
  u64 cbl;
  int k;
  if (!asymmetric) {
    cbl = cb - 2;
    k = ycxx::detail::fpconv::floor_log10_pow2(q);
  } else {
    cbl = cb - 1;
    k = ycxx::detail::fpconv::floor_log10_three_quarters_pow2(q);
  }
  const int j = -k;
  const u128 t = ycxx::detail::fpconv::pow10_significand(j);
  const bool exact = j >= 0 && j <= 55 && (t & 3) == 0;
  const u128 g = (t >> 2) + (exact ? 0 : 1); // 10^-k * 2^(125 - floor(log2(10^-k))), rounded up
  const int h = q + j + ycxx::detail::fpconv::floor_log2_pow5(j) + 2;
  // x * 2^q * 10^-k, rounded to odd. With g rounded up, the product is exact only where the true
  // value is not an integer; it is one exactly when 5^k divides x (k > 0; q >= k there), and a
  // bound of the interval can be such an integer, so that case is computed exactly.
  const auto scaled = [&](u64 x) {
    if (k > 0 && k <= 23) {
      u64 p5 = 1;
      for (int i = 0; i < k; ++i)
        p5 *= 5;
      if (x % p5 == 0)
        return (x / p5) << (q - k);
    }
    return ycxx::detail::fpconv::round_to_odd(g, x << h);
  };
  const u64 vbl = scaled(cbl);
  const u64 vb = scaled(cb);
  const u64 vbr = scaled(cbr);
  const u64 s = vb >> 2; // floor(v * 10^-k)
  {
    const u64 sp10 = s / 10 * 10;
    const u64 tp10 = sp10 + 10;
    const bool u_in = vbl + open <= sp10 << 2;
    const bool w_in = (tp10 << 2) + open <= vbr;
    if (u_in != w_in) {
      ycxx::detail::fpconv::set_digits(out, u_in ? sp10 : tp10, k);
      return true;
    }
  }
  const u64 t1 = s + 1;
  const bool u_in = vbl + open <= s << 2;
  const bool w_in = (t1 << 2) + open <= vbr;
  if (u_in != w_in) {
    ycxx::detail::fpconv::set_digits(out, u_in ? s : t1, k);
    return true;
  }
  if (!u_in)
    return false;
  const u64 mid = (s << 2) + 2; // 4 * (s + 1/2)
  ycxx::detail::fpconv::set_digits(out, vb < mid || (vb == mid && (s & 1) == 0) ? s : t1, k);
  return true;
}

// Exact shortest digits (Burger & Dybvig's free-format algorithm) for value = m * 2^e.
template <kind K>
void shortest_exact(u128 m, int e, digits& out) {
  constexpr format f = fmt_of<K>;
  constexpr int N = limits_of<K>.limbs_out;
  const bool asymmetric = m == (u128(1) << (f.p - 1)) && e > f.qmin();
  const bool inclusive = (m & 1) == 0;
  // value = r / s; the rounding interval is ((r - mm) / s, (r + mp) / s).
  bignum<N> r, s, mp, mm;
  r.set(m);
  if (e >= 0) {
    r.shift_left(e + (asymmetric ? 2 : 1));
    s.set(asymmetric ? 4 : 2);
    mp.set(1);
    mp.shift_left(e + (asymmetric ? 1 : 0));
    mm.set(1);
    mm.shift_left(e);
  } else {
    r.shift_left(asymmetric ? 2 : 1);
    s.set(1);
    s.shift_left(-e + (asymmetric ? 2 : 1));
    mp.set(asymmetric ? 2 : 1);
    mm.set(1);
  }
  // Scale so that 10^(k-1) <= value < 10^k: the first digit is nonzero, and the candidates with
  // one digit are d * 10^(k-1) and, when d = 9 rounds up, 10^k. (Scaling by the upper bound
  // instead would miss 9 * 10^(k-1) when the interval straddles 10^(k-1) and the value lies
  // below it.)
  int k = ycxx::detail::fpconv::floor_log10_pow2(ycxx::detail::fpconv::bit_length(m) - 1 + e) + 1;
  if (k >= 0) {
    s.mul_pow10(k);
  } else {
    r.mul_pow10(-k);
    mp.mul_pow10(-k);
    mm.mul_pow10(-k);
  }
  while (ycxx::detail::fpconv::compare(r, s) >= 0) {
    s.mul_small(10);
    ++k;
  }
  bignum<N> tmp;
  int n = 0;
  for (;;) {
    r.mul_small(10);
    mp.mul_small(10);
    mm.mul_small(10);
    int d = 0;
    while (ycxx::detail::fpconv::compare(r, s) >= 0) {
      r.sub(s);
      ++d;
    }
    int cl = ycxx::detail::fpconv::compare(r, mm);
    bool low = inclusive ? cl <= 0 : cl < 0;
    tmp = r;
    tmp.add(mp);
    int ch = ycxx::detail::fpconv::compare(tmp, s);
    bool high = inclusive ? ch >= 0 : ch > 0;
    if (!low && !high) {
      out.d[n++] = static_cast<char>('0' + d);
      continue;
    }
    if (low && high) {
      tmp = r;
      tmp.shift_left(1);
      int c = ycxx::detail::fpconv::compare(tmp, s);
      if (c > 0 || (c == 0 && d % 2 == 1))
        ++d;
    } else if (high) {
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
  out.x = k - 1;
}

template <kind K>
void shortest(const decoded& v, digits& out) {
  if constexpr (K == kind::binary32 || K == kind::binary64) {
    if (v.m >= 1000 && ycxx::detail::fpconv::schubfach<K>(static_cast<u64>(v.m), v.e, out))
      return;
  }
  ycxx::detail::fpconv::shortest_exact<K>(v.m, v.e, out);
}

// ---- layouts -----------------------------------------------------------------------------------

// d[0..n) with exponent x in the f style; integer digits beyond n are zeros.
std::to_chars_result layout_fixed(char* first, char* last, bool negative, const char* d, int n, int x) {
  long long len = negative ? 1 : 0;
  if (x >= n - 1)
    len += x + 1;
  else if (x >= 0)
    len += n + 1;
  else
    len += 2 + (-x - 1) + n;
  if (len > last - first)
    return ycxx::detail::fpconv::too_large(last);
  char* p = first;
  if (negative)
    *p++ = '-';
  if (x >= n - 1) {
    for (int i = 0; i < n; ++i)
      *p++ = d[i];
    for (int i = n; i <= x; ++i)
      *p++ = '0';
  } else if (x >= 0) {
    __builtin_memcpy(p, d, static_cast<std::size_t>(x + 1));
    p += x + 1;
    *p++ = '.';
    __builtin_memcpy(p, d + x + 1, static_cast<std::size_t>(n - x - 1));
    p += n - x - 1;
  } else {
    *p++ = '0';
    *p++ = '.';
    for (int i = 0; i < -x - 1; ++i)
      *p++ = '0';
    for (int i = 0; i < n; ++i)
      *p++ = d[i];
  }
  return {p, std::errc{}};
}

int scientific_length(bool negative, int n, int x) {
  return (negative ? 1 : 0) + n + (n > 1 ? 1 : 0) + ycxx::detail::fpconv::exponent_length(x);
}

std::to_chars_result layout_scientific(char* first, char* last, bool negative, const char* d, int n, int x) {
  if (ycxx::detail::fpconv::scientific_length(negative, n, x) > last - first)
    return ycxx::detail::fpconv::too_large(last);
  char* p = first;
  if (negative)
    *p++ = '-';
  *p++ = d[0];
  if (n > 1) {
    *p++ = '.';
    for (int i = 1; i < n; ++i)
      *p++ = d[i];
  }
  return {ycxx::detail::fpconv::write_exponent(p, x), std::errc{}};
}

// ---- exact decimal expansion -------------------------------------------------------------------

// The decimal digits of value = m * 2^e: the integer part, all at once, then the fraction, nine
// digits at a time on demand.
template <kind K>
struct expansion {
  static constexpr int N = limits_of<K>.limbs_out;
  char* int_digits = nullptr; // the integer part, no leading zeros
  int int_count = 0;
  bignum<N> frac; // fraction = frac / 2^frac_bits
  int frac_bits = 0;
  int low_limb = 0; // limbs of frac below this one are zero
  char queue[9];
  int queue_size = 0, queue_pos = 0;

  // The integer part has at most limits_of<K>.int_digits digits; buf holds 8 more, a whole chunk
  // past that bound, which keeps the bound visible to the compiler.
  void init(u128 m, int e, char* buf) {
    int_digits = buf;
    bignum<N> ip;
    if (e >= 0) {
      ip.set(m);
      ip.shift_left(e);
      frac.n = 0;
    } else {
      int s = -e;
      ip.set(s >= 128 ? 0 : m >> s);
      frac.set(s >= 128 ? m : m & ycxx::detail::fpconv::low_mask(s));
      frac_bits = s;
    }
    // Integer part: base-10^9 chunks, least significant first.
    u32 chunks[limits_of<K>.int_digits / 9 + 2];
    int nc = 0;
    while (!ip.is_zero())
      chunks[nc++] = ip.div_small(1000000000u);
    int_count = 0;
    for (int i = nc - 1; i >= 0; --i) {
      char tmp[9];
      u32 c = chunks[i];
      for (int j = 8; j >= 0; --j, c /= 10)
        tmp[j] = static_cast<char>('0' + c % 10);
      int skip = 0;
      if (i == nc - 1)
        while (skip < 8 && tmp[skip] == '0')
          ++skip;
      for (int j = skip; j < 9; ++j)
        int_digits[int_count++] = tmp[j];
    }
  }
  bool fraction_exhausted() const { return queue_pos == queue_size && frac.is_zero(); }
  // The next fraction digit (0 once the expansion has ended).
  int next() {
    if (queue_pos == queue_size) {
      if (frac.is_zero())
        return 0;
      frac.mul_small(1000000000u, low_limb);
      int li = frac_bits / 32, sh = frac_bits % 32;
      u64 two = (li < frac.n ? frac.w[li] : 0) | (li + 1 < frac.n ? static_cast<u64>(frac.w[li + 1]) << 32 : 0);
      u32 chunk = static_cast<u32>(two >> sh);
      if (li < frac.n) {
        frac.w[li] &= (u32(1) << sh) - 1;
        frac.n = li + 1;
        frac.trim();
      }
      while (low_limb < frac.n && frac.w[low_limb] == 0)
        ++low_limb;
      for (int j = 8; j >= 0; --j, chunk /= 10)
        queue[j] = static_cast<char>(chunk % 10);
      queue_size = 9;
      queue_pos = 0;
    }
    return queue[queue_pos++];
  }
  bool rest_zero() const {
    for (int i = queue_pos; i < queue_size; ++i)
      if (queue[i] != 0)
        return false;
    return frac.is_zero();
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

bool round_up(int next_digit, bool sticky, char last_digit) {
  return next_digit > 5 || (next_digit == 5 && (sticky || (last_digit - '0') % 2 == 1));
}

// %.Pf
template <kind K>
std::to_chars_result fixed_precision(char* first, char* last, const decoded& v, int precision) {
  char int_buf[limits_of<K>.int_digits + 9];
  expansion<K> x;
  const bool zero = v.cls == fp_class::zero;
  if (!zero)
    x.init(v.m, v.e, int_buf);
  const int ni = zero ? 0 : x.int_count;
  long long len = (v.negative ? 1 : 0) + (ni == 0 ? 1 : ni) + (precision > 0 ? 1LL + precision : 0);
  if (len > last - first)
    return ycxx::detail::fpconv::too_large(last);
  char* p = first;
  if (v.negative)
    *p++ = '-';
  char* digits_begin = p;
  if (ni == 0)
    *p++ = '0';
  for (int i = 0; i < ni; ++i)
    *p++ = x.int_digits[i];
  if (precision > 0) {
    *p++ = '.';
    char* end = p + precision;
    while (p != end && !(zero || x.fraction_exhausted()))
      *p++ = static_cast<char>('0' + x.next());
    while (p != end)
      *p++ = '0';
  }
  if (!zero) {
    int next_digit = x.next();
    if (ycxx::detail::fpconv::round_up(next_digit, !x.rest_zero(), p[-1]) &&
        !ycxx::detail::fpconv::increment(digits_begin, p)) {
      // 99.9 -> 100.0: one more integer digit.
      if (p == last)
        return ycxx::detail::fpconv::too_large(last);
      for (char* c = p; c != digits_begin; --c)
        *c = c[-1];
      *digits_begin = '1';
      ++p;
    }
  }
  return {p, std::errc{}};
}

// The significant digits of a nonzero value, most significant first.
template <kind K>
struct significant_stream {
  expansion<K>& x;
  int pos = 0;
  int exponent = 0; // decimal exponent of the first digit
  int first = 0;    // the first digit (nonzero)

  explicit significant_stream(expansion<K>& e) : x(e) {
    if (x.int_count > 0) {
      exponent = x.int_count - 1;
      first = x.int_digits[0] - '0';
      pos = 1;
    } else {
      exponent = -1;
      pos = 0;
      while ((first = x.next()) == 0)
        --exponent;
    }
  }
  bool exhausted() const { return pos >= x.int_count && x.fraction_exhausted(); }
  int next() { return pos < x.int_count ? x.int_digits[pos++] - '0' : x.next(); }
  bool rest_zero() const {
    for (int i = pos; i < x.int_count; ++i)
      if (x.int_digits[i] != '0')
        return false;
    return x.rest_zero();
  }
};

// %.Pe
template <kind K>
std::to_chars_result scientific_precision(char* first, char* last, const decoded& v, int precision) {
  long long mantissa_len = (v.negative ? 1 : 0) + 1 + (precision > 0 ? 1LL + precision : 0);
  if (mantissa_len + 4 > last - first)
    return ycxx::detail::fpconv::too_large(last);
  char* p = first;
  if (v.negative)
    *p++ = '-';
  char* digits_begin = p;
  int exponent = 0;
  if (v.cls == fp_class::zero) {
    *p++ = '0';
    if (precision > 0) {
      *p++ = '.';
      for (int i = 0; i < precision; ++i)
        *p++ = '0';
    }
  } else {
    char int_buf[limits_of<K>.int_digits + 9];
    expansion<K> x;
    x.init(v.m, v.e, int_buf);
    significant_stream<K> s(x);
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
    if (ycxx::detail::fpconv::round_up(next_digit, !s.rest_zero(), p[-1]) &&
        !ycxx::detail::fpconv::increment(digits_begin, p)) {
      *digits_begin = '1'; // 9.99 -> 1.00e+1
      ++exponent;
    }
  }
  if (ycxx::detail::fpconv::exponent_length(exponent) > last - p)
    return ycxx::detail::fpconv::too_large(last);
  return {ycxx::detail::fpconv::write_exponent(p, exponent), std::errc{}};
}

// %.Pg
template <kind K>
std::to_chars_result general_precision(char* first, char* last, const decoded& v, int precision) {
  if (precision == 0)
    precision = 1;
  if (v.cls == fp_class::zero) {
    const char zero = '0';
    return ycxx::detail::fpconv::layout_fixed(first, last, v.negative, &zero, 1, 0);
  }
  // Digits past the exact expansion are zeros, which %g removes: keep at most sig_digits + 1.
  constexpr int cap = limits_of<K>.sig_digits + 1;
  const int want = precision < cap ? precision : cap;
  char buf[cap];
  char int_buf[limits_of<K>.int_digits + 9];
  expansion<K> x;
  x.init(v.m, v.e, int_buf);
  significant_stream<K> s(x);
  int exponent = s.exponent;
  buf[0] = static_cast<char>('0' + s.first);
  int n = 1;
  while (n < want && !s.exhausted())
    buf[n++] = static_cast<char>('0' + s.next());
  if (n == want && want == precision) {
    int next_digit = s.next();
    if (ycxx::detail::fpconv::round_up(next_digit, !s.rest_zero(), buf[n - 1]) &&
        !ycxx::detail::fpconv::increment(buf, buf + n)) {
      buf[0] = '1';
      ++exponent;
    }
  }
  while (n > 1 && buf[n - 1] == '0')
    --n;
  if (precision > exponent && exponent >= -4)
    return ycxx::detail::fpconv::layout_fixed(first, last, v.negative, buf, n, exponent);
  return ycxx::detail::fpconv::layout_scientific(first, last, v.negative, buf, n, exponent);
}

// ---- %a ----------------------------------------------------------------------------------------

// One hexadecimal digit before the point (1 for normal numbers, 0 for subnormal ones and zero),
// the fraction bits padded to whole digits, and the binary exponent of that leading digit.
// x87 subnormal numbers follow the C library instead: the 64-bit significand read as one digit
// and fifteen, with exponent emin - 3 (0x0.000000000000001p-16385 is the smallest).
template <kind K>
std::to_chars_result hex(char* first, char* last, const decoded& v, int precision) {
  constexpr format f = fmt_of<K>;
  const bool nibble_subnormal = f.explicit_bit && v.cls != fp_class::zero && (v.m >> (f.p - 1)) == 0;
  const int fbits = nibble_subnormal ? f.p - 4 : f.p - 1;
  const int digits_all = (fbits + 3) / 4;
  u128 lead = 0, frac = 0;
  int exponent = 0;
  if (nibble_subnormal) {
    lead = v.m >> fbits;
    frac = v.m & ycxx::detail::fpconv::low_mask(fbits);
    exponent = f.emin - 3;
  } else if (v.cls != fp_class::zero) {
    if (v.m >> (f.p - 1)) {
      lead = 1;
      frac = v.m & ycxx::detail::fpconv::low_mask(fbits);
      exponent = v.e + f.p - 1;
    } else {
      frac = v.m;
      exponent = f.emin;
    }
  }
  frac <<= 4 * digits_all - fbits;
  int nd = digits_all;
  if (precision < 0) {
    while (nd > 0 && (frac & 0xf) == 0) {
      frac >>= 4;
      --nd;
    }
  } else if (precision < digits_all) {
    // Round to `precision` digits, half to even; a carry may reach the leading digit.
    u128 g = (lead << (4 * digits_all)) | frac;
    int drop = 4 * (digits_all - precision);
    u128 rem = g & ycxx::detail::fpconv::low_mask(drop);
    u128 half = u128(1) << (drop - 1);
    g >>= drop;
    if (rem > half || (rem == half && (g & 1) != 0))
      ++g;
    nd = precision;
    lead = g >> (4 * nd);
    frac = g & ycxx::detail::fpconv::low_mask(4 * nd);
  }
  const int pad = precision > nd ? precision - nd : 0;
  const int total_frac = nd + pad;
  int a = exponent < 0 ? -exponent : exponent;
  long long len = (v.negative ? 1 : 0) + 1 + (total_frac > 0 ? 1LL + total_frac : 0) + 2 +
                  ycxx::detail::fpconv::decimal_length(static_cast<unsigned>(a));
  if (len > last - first)
    return ycxx::detail::fpconv::too_large(last);
  char* p = first;
  if (v.negative)
    *p++ = '-';
  *p++ = ycxx::detail::charconv_digits[static_cast<int>(lead)];
  if (total_frac > 0) {
    *p++ = '.';
    for (int i = nd - 1; i >= 0; --i)
      *p++ = ycxx::detail::charconv_digits[static_cast<int>((frac >> (4 * i)) & 0xf)];
    for (int i = 0; i < pad; ++i)
      *p++ = '0';
  }
  *p++ = 'p';
  *p++ = exponent < 0 ? '-' : '+';
  int d = ycxx::detail::fpconv::decimal_length(static_cast<unsigned>(a));
  for (int i = d - 1; i >= 0; --i, a /= 10)
    p[i] = static_cast<char>('0' + a % 10);
  return {p + d, std::errc{}};
}

// ---- shortest forms ----------------------------------------------------------------------------

// The exact decimal digits of an integer value m * 2^e (e >= 1) into buf; returns the count.
template <kind K>
int integer_digits(u128 m, int e, char* buf) {
  expansion<K> x;
  x.init(m, e, buf);
  return x.int_count;
}

// %f, shortest. An integer value with a spacing of 2 or more is printed exactly: its exact digits
// round-trip, and among the round-tripping integers with that many digits it is the closest.
// (Where the rounding interval reaches below 10^(L-1), L the value's digit count, L-1 nines would
// also round-trip in one character less; like the other implementations, libycxx prints the
// exact value. See STATUS.md, deliberate divergences.)
template <kind K>
std::to_chars_result fixed_shortest(char* first, char* last, const decoded& v, const digits& s) {
  if (v.e < 1)
    return ycxx::detail::fpconv::layout_fixed(first, last, v.negative, s.d, s.n, s.x);
  char buf[limits_of<K>.int_digits + 9];
  int len = ycxx::detail::fpconv::integer_digits<K>(v.m, v.e, buf);
  return ycxx::detail::fpconv::layout_fixed(first, last, v.negative, buf, len, len - 1);
}

// %g, shortest: the shortest output of %g for any precision P that round-trips.
template <kind K>
std::to_chars_result general_shortest(char* first, char* last, const decoded& v, const digits& s) {
  if (s.x < -4)
    return ycxx::detail::fpconv::layout_scientific(first, last, v.negative, s.d, s.n, s.x);
  if (s.x < s.n)
    return ycxx::detail::fpconv::layout_fixed(first, last, v.negative, s.d, s.n, s.x);
  // The e style needs P <= x; the f style (P > x) shows every integer digit of the value.
  int f_len;
  if (v.e >= 1) {
    char buf[limits_of<K>.int_digits + 9];
    f_len = ycxx::detail::fpconv::integer_digits<K>(v.m, v.e, buf);
    if (f_len <= ycxx::detail::fpconv::scientific_length(false, s.n, s.x))
      return ycxx::detail::fpconv::layout_fixed(first, last, v.negative, buf, f_len, f_len - 1);
  } else {
    f_len = s.x + 1;
    if (f_len <= ycxx::detail::fpconv::scientific_length(false, s.n, s.x))
      return ycxx::detail::fpconv::layout_fixed(first, last, v.negative, s.d, s.n, s.x);
  }
  return ycxx::detail::fpconv::layout_scientific(first, last, v.negative, s.d, s.n, s.x);
}

// [charconv.to.chars]/7: f for 10^-4 <= |value| < 10^U, U = floor(log10(2^(p+1))), else e.
template <kind K>
bool plain_uses_fixed(const decoded& v) {
  constexpr format f = fmt_of<K>;
  u128 upper = 1;
  for (int i = 0; i < ycxx::detail::fpconv::floor_log10_pow2(f.p + 1); ++i)
    upper *= 10;
  bool at_least_low = v.e >= 0 || (-v.e < 128 && v.m * 10000 >= (u128(1) << -v.e));
  bool below_high;
  if (v.e >= 0)
    below_high = ycxx::detail::fpconv::bit_length(v.m) + v.e <= 120 && (v.m << v.e) < upper;
  else
    below_high = -v.e >= 128 || (v.m >> -v.e) < upper;
  return at_least_low && below_high;
}

template <kind K>
std::to_chars_result to_chars_shortest(char* first, char* last, const decoded& v, int fmt) {
  const auto cf = static_cast<std::chars_format>(fmt);
  if (fmt != 0 && cf == std::chars_format::hex)
    return ycxx::detail::fpconv::hex<K>(first, last, v, -1);
  if (v.cls == fp_class::zero) {
    const char zero = '0';
    if (cf == std::chars_format::scientific)
      return ycxx::detail::fpconv::layout_scientific(first, last, v.negative, &zero, 1, 0);
    return ycxx::detail::fpconv::layout_fixed(first, last, v.negative, &zero, 1, 0);
  }
  digits s;
  ycxx::detail::fpconv::shortest<K>(v, s);
  if (fmt == 0)
    return ycxx::detail::fpconv::plain_uses_fixed<K>(v)
               ? ycxx::detail::fpconv::fixed_shortest<K>(first, last, v, s)
               : ycxx::detail::fpconv::layout_scientific(first, last, v.negative, s.d, s.n, s.x);
  if (cf == std::chars_format::scientific)
    return ycxx::detail::fpconv::layout_scientific(first, last, v.negative, s.d, s.n, s.x);
  if (cf == std::chars_format::fixed)
    return ycxx::detail::fpconv::fixed_shortest<K>(first, last, v, s);
  return ycxx::detail::fpconv::general_shortest<K>(first, last, v, s);
}

template <kind K>
std::to_chars_result to_chars_impl(char* first, char* last, fp_raw bits, int fmt, int precision) {
  const decoded v = ycxx::detail::fpconv::decode<K>(bits);
  std::to_chars_result r;
  if (ycxx::detail::fpconv::write_special(first, last, v, r))
    return r;
  if (precision < 0)
    return ycxx::detail::fpconv::to_chars_shortest<K>(first, last, v, fmt);
  switch (static_cast<std::chars_format>(fmt)) {
  case std::chars_format::fixed:
    return ycxx::detail::fpconv::fixed_precision<K>(first, last, v, precision);
  case std::chars_format::scientific:
    return ycxx::detail::fpconv::scientific_precision<K>(first, last, v, precision);
  case std::chars_format::hex:
    return ycxx::detail::fpconv::hex<K>(first, last, v, precision);
  default:
    return ycxx::detail::fpconv::general_precision<K>(first, last, v, precision);
  }
}

} // namespace
} // namespace ycxx::detail::fpconv

std::to_chars_result ycxx::detail::fp_to_chars(char* first, char* last, fp_kind kind, fp_raw bits, int fmt,
                                               int precision) noexcept {
  using enum ycxx::detail::fp_kind;
  switch (kind) {
  case binary16:
    return ycxx::detail::fpconv::to_chars_impl<binary16>(first, last, bits, fmt, precision);
  case bfloat16:
    return ycxx::detail::fpconv::to_chars_impl<bfloat16>(first, last, bits, fmt, precision);
  case binary32:
    return ycxx::detail::fpconv::to_chars_impl<binary32>(first, last, bits, fmt, precision);
  case binary64:
    return ycxx::detail::fpconv::to_chars_impl<binary64>(first, last, bits, fmt, precision);
  case x87_extended:
    return ycxx::detail::fpconv::to_chars_impl<x87_extended>(first, last, bits, fmt, precision);
  case binary128:
    return ycxx::detail::fpconv::to_chars_impl<binary128>(first, last, bits, fmt, precision);
  }
  __builtin_unreachable();
}
