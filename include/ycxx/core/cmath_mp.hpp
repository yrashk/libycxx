// libycxx core: constexpr transcendental functions of <cmath> (soft floating point).
//
// Used during constant evaluation where the compiler does not fold the corresponding builtin
// (Clang folds none of them; GCC folds all but lgamma). Every function works in a binary
// floating-point number of 64*N bits (mpf<N>: N = 2 for formats of up to 64 significand bits,
// 3 for binary128) with an error of a few units in its last place, then rounds once to the
// result type. The result is therefore correctly rounded unless the exact value lies within
// about 2^-(64N - p - 8) (relative) of a rounding boundary of the p-bit result: at least
// faithfully rounded, and in practice correctly rounded. Known weaker spots: lgamma close to
// its zeros near -2.457..., -2.747..., ... (absolute, not relative, accuracy there), and pow
// with exponents beyond 2^14 for binary128 (one or two bits fewer).
//
// Exceptions follow ISO/IEC 9899:2024 Annex F; flags other than "inexact" go through
// fp_report (non-constant during constant evaluation, [library.c]/3). A subnormal result of a
// transcendental function is taken to be inexact (it always is, except for the exact cases
// that are computed exactly: exp2 of integers, pow with exact results).
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/cmath_exact.hpp>
#include <ycxx/core/cmath_fp.hpp>
#include <ycxx/core/cmath_tables.hpp>
#include <ycxx/core/math_constants.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::fpm {

// ---- mpf<N> -------------------------------------------------------------------------------------
// value = (neg ? -1 : 1) * m * 2^exp, with m normalised (bit 64N - 1 set) or zero.
template <int N>
struct mpf {
  bool neg = false;
  int exp = 0;
  wide<N> m;
};

template <int N>
constexpr bool mp_is_zero(const mpf<N>& x) noexcept {
  return ycxx::detail::fpm::wide_is_zero(x.m);
}
// 2^mp_ilog(x) <= |x| < 2^(mp_ilog(x) + 1)
template <int N>
constexpr int mp_ilog(const mpf<N>& x) noexcept {
  return x.exp + 64 * N - 1;
}

// Normalises +-mag * 2^exp into mpf<N>, rounding to nearest.
template <int N, int K>
constexpr mpf<N> mp_make(bool neg, const wide<K>& mag, int exp) noexcept {
  mpf<N> r;
  const int len = ycxx::detail::fpm::wide_bitlen(mag);
  if (len == 0) return r;
  r.neg = neg;
  const int shift = len - 64 * N;
  if (shift > 0) {
    const bool round = ycxx::detail::fpm::wide_bit(mag, shift - 1);
    r.m = ycxx::detail::fpm::wide_resize<N>(ycxx::detail::fpm::wide_shr(mag, shift));
    r.exp = exp + shift;
    if (round) {
      ycxx::detail::fpm::wide_add_small(r.m, 1);
      if (ycxx::detail::fpm::wide_is_zero(r.m)) { // carried out of the top
        ycxx::detail::fpm::wide_set_bit(r.m, 64 * N - 1);
        ++r.exp;
      }
    }
  } else {
    r.m = ycxx::detail::fpm::wide_shl(ycxx::detail::fpm::wide_resize<N>(mag), -shift);
    r.exp = exp + shift;
  }
  return r;
}

template <int N>
constexpr mpf<N> mp_from_u64(u64 v, bool neg = false) noexcept {
  return ycxx::detail::fpm::mp_make<N>(neg, ycxx::detail::fpm::wide_from<1>(v), 0);
}
template <int N>
constexpr mpf<N> mp_from_int(long long v) noexcept {
  return ycxx::detail::fpm::mp_from_u64<N>(v < 0 ? u64(0) - u64(v) : u64(v), v < 0);
}
template <int N>
constexpr mpf<N> mp_from_value(const fp_value& v) noexcept { // finite or zero
  return ycxx::detail::fpm::mp_make<N>(v.neg, v.sig, v.exp);
}
template <int N, class T>
constexpr mpf<N> mp_from(T x) noexcept {
  return ycxx::detail::fpm::mp_from_value<N>(ycxx::detail::fpm::fp_decode(x));
}
template <int N>
constexpr mpf<N> mp_from_bits(bool neg, int exp, const unsigned long long (&m)[3]) noexcept {
  wide<3> w;
  w.w[0] = m[2];
  w.w[1] = m[1];
  w.w[2] = m[0];
  return ycxx::detail::fpm::mp_make<N>(neg, w, exp + 1 - 192);
}
template <int N>
constexpr mpf<N> mp_const(math_constant c) noexcept {
  const math_constant_bits& b = ycxx::detail::math_constant_table[static_cast<int>(c)];
  return ycxx::detail::fpm::mp_from_bits<N>(false, b.exp, b.m);
}
template <int N>
constexpr mpf<N> mp_const(const mp_const_bits& b) noexcept {
  return ycxx::detail::fpm::mp_from_bits<N>(b.neg, b.exp, b.m);
}
template <int N>
constexpr mpf<N> mp_ldexp(mpf<N> x, int k) noexcept {
  if (!ycxx::detail::fpm::mp_is_zero(x)) x.exp += k;
  return x;
}
template <int N>
constexpr mpf<N> mp_neg(mpf<N> x) noexcept {
  x.neg = !x.neg;
  return x;
}
template <int N>
constexpr mpf<N> mp_abs(mpf<N> x) noexcept {
  x.neg = false;
  return x;
}

template <class T, int N>
constexpr fp_result<T> mp_round(const mpf<N>& x) noexcept {
  if (ycxx::detail::fpm::mp_is_zero(x)) return {T(0), 0};
  return ycxx::detail::fpm::fp_round<T>(x.neg, x.m, x.exp, true);
}
template <class T, int N>
constexpr T mp_to(const mpf<N>& x) noexcept {
  return ycxx::detail::fpm::fp_finish(ycxx::detail::fpm::mp_round<T>(x));
}

// Compares |a| with |b|.
template <int N>
constexpr int mp_cmp_abs(const mpf<N>& a, const mpf<N>& b) noexcept {
  const bool za = ycxx::detail::fpm::mp_is_zero(a), zb = ycxx::detail::fpm::mp_is_zero(b);
  if (za || zb) return za && zb ? 0 : za ? -1 : 1;
  if (a.exp != b.exp) return a.exp < b.exp ? -1 : 1;
  return ycxx::detail::fpm::wide_cmp(a.m, b.m);
}
template <int N>
constexpr int mp_cmp(const mpf<N>& a, const mpf<N>& b) noexcept {
  const bool za = ycxx::detail::fpm::mp_is_zero(a), zb = ycxx::detail::fpm::mp_is_zero(b);
  const int sa = za ? 0 : a.neg ? -1 : 1, sb = zb ? 0 : b.neg ? -1 : 1;
  if (sa != sb) return sa < sb ? -1 : 1;
  if (sa == 0) return 0;
  const int c = ycxx::detail::fpm::mp_cmp_abs(a, b);
  return sa > 0 ? c : -c;
}

template <int N>
constexpr mpf<N> mp_add(const mpf<N>& a, const mpf<N>& b) noexcept {
  if (ycxx::detail::fpm::mp_is_zero(a)) return b;
  if (ycxx::detail::fpm::mp_is_zero(b)) return a;
  const bool a_big = ycxx::detail::fpm::mp_cmp_abs(a, b) >= 0;
  const mpf<N>& x = a_big ? a : b;
  const mpf<N>& y = a_big ? b : a;
  const int d = x.exp - y.exp;
  if (d > 64 * N + 64) return x;
  // One guard limb below each operand.
  wide<N + 1> X = ycxx::detail::fpm::wide_shl(ycxx::detail::fpm::wide_resize<N + 1>(x.m), 64);
  bool sticky = false;
  wide<N + 1> Y = ycxx::detail::fpm::wide_shr(ycxx::detail::fpm::wide_shl(ycxx::detail::fpm::wide_resize<N + 1>(y.m), 64), d, sticky);
  if (x.neg == y.neg) {
    if (ycxx::detail::fpm::wide_add(X, Y)) {
      X = ycxx::detail::fpm::wide_shr(X, 1);
      ycxx::detail::fpm::wide_set_bit(X, 64 * (N + 1) - 1);
      return ycxx::detail::fpm::mp_make<N>(x.neg, X, x.exp - 64 + 1);
    }
  } else {
    ycxx::detail::fpm::wide_sub(X, Y);
    if (sticky) { // the true difference is a little smaller
      wide<N + 1> one = ycxx::detail::fpm::wide_from<N + 1>(1);
      ycxx::detail::fpm::wide_sub(X, one);
    }
  }
  return ycxx::detail::fpm::mp_make<N>(x.neg, X, x.exp - 64);
}
template <int N>
constexpr mpf<N> mp_sub(const mpf<N>& a, const mpf<N>& b) noexcept {
  return ycxx::detail::fpm::mp_add(a, ycxx::detail::fpm::mp_neg(b));
}
template <int N>
constexpr mpf<N> mp_mul(const mpf<N>& a, const mpf<N>& b) noexcept {
  if (ycxx::detail::fpm::mp_is_zero(a) || ycxx::detail::fpm::mp_is_zero(b)) return {};
  return ycxx::detail::fpm::mp_make<N>(a.neg != b.neg, ycxx::detail::fpm::wide_mul(a.m, b.m), a.exp + b.exp);
}
template <int N>
constexpr mpf<N> mp_mul_u64(const mpf<N>& a, u64 k) noexcept {
  wide<N + 1> x = ycxx::detail::fpm::wide_resize<N + 1>(a.m);
  ycxx::detail::fpm::wide_mul_small(x, k);
  return ycxx::detail::fpm::mp_make<N>(a.neg, x, a.exp);
}
template <int N>
constexpr mpf<N> mp_div_u64(const mpf<N>& a, u64 k) noexcept {
  wide<N + 1> x = ycxx::detail::fpm::wide_shl(ycxx::detail::fpm::wide_resize<N + 1>(a.m), 64);
  ycxx::detail::fpm::wide_div_small(x, k);
  return ycxx::detail::fpm::mp_make<N>(a.neg, x, a.exp - 64);
}
template <int N>
constexpr mpf<N> mp_one() noexcept {
  return ycxx::detail::fpm::mp_from_u64<N>(1);
}

// The leading bits of a normalised mantissa as a double in [0.5, 1) (exact).
template <int N>
constexpr double mp_lead(const mpf<N>& x) noexcept {
  return static_cast<double>(x.m.w[N - 1] >> 11) * 0x1p-53;
}
template <int N>
constexpr mpf<N> mp_from_double(double d) noexcept {
  return ycxx::detail::fpm::mp_from<N>(d);
}

// 1 / b by Newton's iteration from a double approximation.
template <int N>
constexpr mpf<N> mp_recip(const mpf<N>& b) noexcept {
  // b = f * 2^(b.exp + 64N), f in [0.5, 1).
  mpf<N> f = b;
  f.neg = false;
  f.exp = -64 * N;
  mpf<N> y = ycxx::detail::fpm::mp_from_double<N>(1.0 / ycxx::detail::fpm::mp_lead(f));
  const mpf<N> one = ycxx::detail::fpm::mp_one<N>();
  for (int bits = 50; bits < 64 * N + 8; bits *= 2) {
    const mpf<N> e = ycxx::detail::fpm::mp_sub(one, ycxx::detail::fpm::mp_mul(f, y));
    y = ycxx::detail::fpm::mp_add(y, ycxx::detail::fpm::mp_mul(y, e));
  }
  y = ycxx::detail::fpm::mp_ldexp(y, -(b.exp + 64 * N));
  y.neg = b.neg;
  return y;
}
template <int N>
constexpr mpf<N> mp_div(const mpf<N>& a, const mpf<N>& b) noexcept {
  return ycxx::detail::fpm::mp_mul(a, ycxx::detail::fpm::mp_recip(b));
}

// sqrt(a), a >= 0, through 1 / sqrt by Newton's iteration.
template <int N>
constexpr mpf<N> mp_sqrt(const mpf<N>& a) noexcept {
  if (ycxx::detail::fpm::mp_is_zero(a)) return a;
  // a = f * 2^E with f in [0.25, 1) and E even.
  int E = a.exp + 64 * N;
  mpf<N> f = a;
  f.exp = -64 * N;
  double fd = ycxx::detail::fpm::mp_lead(f);
  if (E & 1) {
    f.exp -= 1;
    fd *= 0.5;
    E += 1;
  }
  double yd = 1.5;
  for (int i = 0; i < 8; ++i) yd = yd * (3.0 - fd * yd * yd) * 0.5;
  mpf<N> y = ycxx::detail::fpm::mp_from_double<N>(yd);
  const mpf<N> one = ycxx::detail::fpm::mp_one<N>();
  for (int bits = 48; bits < 64 * N + 8; bits *= 2) {
    const mpf<N> e = ycxx::detail::fpm::mp_sub(one, ycxx::detail::fpm::mp_mul(f, ycxx::detail::fpm::mp_mul(y, y)));
    y = ycxx::detail::fpm::mp_add(y, ycxx::detail::fpm::mp_ldexp(ycxx::detail::fpm::mp_mul(y, e), -1));
  }
  return ycxx::detail::fpm::mp_ldexp(ycxx::detail::fpm::mp_mul(f, y), E / 2);
}

// The nearest integer to x (|x| < 2^62) and x minus it.
template <int N>
constexpr long long mp_round_int(const mpf<N>& x, mpf<N>* rest = nullptr) noexcept {
  if (ycxx::detail::fpm::mp_is_zero(x) || ycxx::detail::fpm::mp_ilog(x) < -1) {
    if (rest) *rest = x;
    return 0;
  }
  // x = m * 2^exp with exp < 0 here; k = round(m / 2^-exp).
  const int s = -x.exp;
  bool sticky = false;
  wide<N> ip = ycxx::detail::fpm::wide_shr(x.m, s - 1, sticky);
  const bool half = ip.w[0] & 1;
  ip = ycxx::detail::fpm::wide_shr(ip, 1);
  u64 k = ip.w[0] + (half ? 1 : 0);
  const long long sk = x.neg ? -static_cast<long long>(k) : static_cast<long long>(k);
  if (rest) *rest = ycxx::detail::fpm::mp_sub(x, ycxx::detail::fpm::mp_from_int<N>(sk));
  return sk;
}

// Stops a series once |term| < 2^-(64N + 8) relative to an O(1) sum.
template <int N>
constexpr bool mp_negligible(const mpf<N>& term, int scale = 0) noexcept {
  return ycxx::detail::fpm::mp_is_zero(term) || ycxx::detail::fpm::mp_ilog(term) < scale - 64 * N - 8;
}

// ---- exp, log -------------------------------------------------------------------------------------
// e^r - 1 by its Taylor series (|r| small).
template <int N>
constexpr mpf<N> mp_expm1_series(const mpf<N>& r) noexcept {
  mpf<N> sum = r, term = r;
  const int scale = ycxx::detail::fpm::mp_ilog(r);
  for (u64 k = 2;; ++k) {
    term = ycxx::detail::fpm::mp_div_u64(ycxx::detail::fpm::mp_mul(term, r), k);
    if (ycxx::detail::fpm::mp_negligible(term, scale)) break;
    sum = ycxx::detail::fpm::mp_add(sum, term);
  }
  return sum;
}
// e^x for |x| < 2^20.
template <int N>
constexpr mpf<N> mp_exp(const mpf<N>& x) noexcept {
  const mpf<N> ln2 = ycxx::detail::fpm::mp_const<N>(math_constant::ln2);
  mpf<N> r;
  const long long k =
      ycxx::detail::fpm::mp_round_int(ycxx::detail::fpm::mp_mul(x, ycxx::detail::fpm::mp_const<N>(math_constant::log2e)));
  r = ycxx::detail::fpm::mp_sub(x, ycxx::detail::fpm::mp_mul(ln2, ycxx::detail::fpm::mp_from_int<N>(k)));
  mpf<N> e = ycxx::detail::fpm::mp_one<N>();
  if (!ycxx::detail::fpm::mp_is_zero(r)) e = ycxx::detail::fpm::mp_add(e, ycxx::detail::fpm::mp_expm1_series(r));
  return ycxx::detail::fpm::mp_ldexp(e, static_cast<int>(k));
}
template <int N>
constexpr mpf<N> mp_expm1(const mpf<N>& x) noexcept {
  if (ycxx::detail::fpm::mp_is_zero(x) || ycxx::detail::fpm::mp_ilog(x) < -2) return ycxx::detail::fpm::mp_expm1_series(x);
  return ycxx::detail::fpm::mp_sub(ycxx::detail::fpm::mp_exp(x), ycxx::detail::fpm::mp_one<N>());
}

// 2 atanh(z) = 2 (z + z^3/3 + z^5/5 + ...), |z| <= 0.18.
template <int N>
constexpr mpf<N> mp_two_atanh_series(const mpf<N>& z) noexcept {
  const mpf<N> z2 = ycxx::detail::fpm::mp_mul(z, z);
  mpf<N> pow = z, sum = z;
  const int scale = ycxx::detail::fpm::mp_ilog(z);
  for (u64 k = 3;; k += 2) {
    pow = ycxx::detail::fpm::mp_mul(pow, z2);
    const mpf<N> term = ycxx::detail::fpm::mp_div_u64(pow, k);
    if (ycxx::detail::fpm::mp_negligible(term, scale)) break;
    sum = ycxx::detail::fpm::mp_add(sum, term);
  }
  return ycxx::detail::fpm::mp_ldexp(sum, 1);
}
// ln x for x > 0.
template <int N>
constexpr mpf<N> mp_log(const mpf<N>& x) noexcept {
  // x = f * 2^e with f in [1/sqrt 2, sqrt 2).
  int e = x.exp + 64 * N;
  mpf<N> f = x;
  f.exp = -64 * N; // [0.5, 1)
  if (ycxx::detail::fpm::mp_lead(f) < 0.70710678118654752) {
    f.exp += 1;
    e -= 1;
  }
  const mpf<N> one = ycxx::detail::fpm::mp_one<N>();
  const mpf<N> z = ycxx::detail::fpm::mp_div(ycxx::detail::fpm::mp_sub(f, one), ycxx::detail::fpm::mp_add(f, one));
  mpf<N> r = ycxx::detail::fpm::mp_is_zero(z) ? z : ycxx::detail::fpm::mp_two_atanh_series(z);
  if (e != 0)
    r = ycxx::detail::fpm::mp_add(
        r, ycxx::detail::fpm::mp_mul(ycxx::detail::fpm::mp_const<N>(math_constant::ln2), ycxx::detail::fpm::mp_from_int<N>(e)));
  return r;
}
// ln(1 + x) for x > -1.
template <int N>
constexpr mpf<N> mp_log1p(const mpf<N>& x) noexcept {
  const mpf<N> one = ycxx::detail::fpm::mp_one<N>();
  if (ycxx::detail::fpm::mp_is_zero(x)) return x;
  if (ycxx::detail::fpm::mp_ilog(x) < -2) { // |x| < 1/4: 2 atanh(x / (2 + x))
    const mpf<N> z = ycxx::detail::fpm::mp_div(x, ycxx::detail::fpm::mp_add(ycxx::detail::fpm::mp_ldexp(one, 1), x));
    return ycxx::detail::fpm::mp_two_atanh_series(z);
  }
  return ycxx::detail::fpm::mp_log(ycxx::detail::fpm::mp_add(one, x));
}

// ---- trigonometric ----------------------------------------------------------------------------------
// sin and cos of |r| <= pi/4 (Taylor).
template <int N>
constexpr void mp_sin_cos_small(const mpf<N>& r, mpf<N>& s, mpf<N>& c) noexcept {
  const mpf<N> r2 = ycxx::detail::fpm::mp_mul(r, r);
  s = r;
  c = ycxx::detail::fpm::mp_one<N>();
  mpf<N> ts = r, tc = ycxx::detail::fpm::mp_one<N>();
  const int scale = ycxx::detail::fpm::mp_is_zero(r) ? 0 : ycxx::detail::fpm::mp_ilog(r);
  bool done_s = ycxx::detail::fpm::mp_is_zero(r), done_c = ycxx::detail::fpm::mp_is_zero(r);
  for (u64 k = 1; !(done_s && done_c); ++k) {
    // ts: r^(2k+1) / (2k+1)!, tc: r^(2k) / (2k)!, alternating.
    tc = ycxx::detail::fpm::mp_neg(ycxx::detail::fpm::mp_div_u64(ycxx::detail::fpm::mp_mul(tc, r2), (2 * k - 1) * (2 * k)));
    ts = ycxx::detail::fpm::mp_neg(ycxx::detail::fpm::mp_div_u64(ycxx::detail::fpm::mp_mul(ts, r2), (2 * k) * (2 * k + 1)));
    if (!done_c) {
      if (ycxx::detail::fpm::mp_negligible(tc, -1))
        done_c = true;
      else
        c = ycxx::detail::fpm::mp_add(c, tc);
    }
    if (!done_s) {
      if (ycxx::detail::fpm::mp_negligible(ts, scale))
        done_s = true;
      else
        s = ycxx::detail::fpm::mp_add(s, ts);
    }
  }
}

// x - q pi/2 with |result| <= pi/4 (Payne-Hanek: the bits of 2/pi that matter for x).
template <int N>
constexpr mpf<N> mp_reduce_pio2(const fp_value& v, int& quadrant) noexcept {
  quadrant = 0;
  const mpf<N> x = ycxx::detail::fpm::mp_from_value<N>(v);
  if (ycxx::detail::fpm::mp_ilog(x) < -1) return x; // |x| < 1/2 < pi/4
  constexpr int F = 64 * N + 128;                  // fraction bits kept
  constexpr int W = 2 + 113 + F + 64;              // window bits (upper bound)
  constexpr int WL = (W + 63) / 64;
  const int E = v.exp, lenM = ycxx::detail::fpm::wide_bitlen(v.sig);
  // x * 2/pi = M * sum b_i 2^(E - i); terms with E - i >= 2 are multiples of 4.
  const int start = E - 1 > 1 ? E - 1 : 1;
  const int end = lenM + E + F;
  // B = bits start..end of 2/pi as an integer (bit `end` is its least significant bit).
  wide<WL> B;
  for (int i = start; i <= end;) {
    // Bits i .. i + k - 1 of the table (k <= 64, within one word) go to positions end - i down.
    const int word = (i - 1) / 64, off = (i - 1) % 64;
    int k = 64 - off;
    if (k > end - i + 1) k = end - i + 1;
    const u64 chunk = (ycxx::detail::fpm::two_over_pi_bits[word] << off) >> (64 - k); // k bits, msb first
    const int pos = end - (i + k - 1);                                                // lsb position in B
    B.w[pos / 64] |= chunk << (pos % 64);
    if (pos % 64 != 0 && pos % 64 + k > 64) B.w[pos / 64 + 1] |= chunk >> (64 - pos % 64);
    i += k;
  }
  const wide<WL + 2> P = ycxx::detail::fpm::wide_mul(v.sig, B); // x * 2/pi = P * 2^(E - end)
  const int point = end - E;                                    // binary point position in P
  int q = (ycxx::detail::fpm::wide_bit(P, point + 1) ? 2 : 0) + (ycxx::detail::fpm::wide_bit(P, point) ? 1 : 0);
  wide<WL + 2> frac = ycxx::detail::fpm::wide_low_bits(P, point);
  bool neg = false;
  if (ycxx::detail::fpm::wide_bit(frac, point - 1)) { // frac >= 1/2: take frac - 1
    wide<WL + 2> one;
    ycxx::detail::fpm::wide_set_bit(one, point);
    ycxx::detail::fpm::wide_sub(one, frac);
    frac = one;
    neg = true;
    q = (q + 1) & 3;
  }
  mpf<N> f = ycxx::detail::fpm::mp_make<N>(neg, frac, -point);
  if (v.neg) {
    f.neg = !f.neg;
    q = (4 - q) & 3;
  }
  quadrant = q;
  return ycxx::detail::fpm::mp_mul(f, ycxx::detail::fpm::mp_ldexp(ycxx::detail::fpm::mp_const<N>(math_constant::pi), -1));
}

// sin(x) (which == 0), cos(x) (1) or tan(x) (2) of a finite nonzero x.
template <int N>
constexpr mpf<N> mp_trig(const fp_value& v, int which) noexcept {
  int q = 0;
  const mpf<N> r = ycxx::detail::fpm::mp_reduce_pio2<N>(v, q);
  mpf<N> s, c;
  ycxx::detail::fpm::mp_sin_cos_small(r, s, c);
  // sin(r + q pi/2), cos(r + q pi/2)
  mpf<N> sn = s, cs = c;
  switch (q) {
  case 1:
    sn = c;
    cs = ycxx::detail::fpm::mp_neg(s);
    break;
  case 2:
    sn = ycxx::detail::fpm::mp_neg(s);
    cs = ycxx::detail::fpm::mp_neg(c);
    break;
  case 3:
    sn = ycxx::detail::fpm::mp_neg(c);
    cs = s;
    break;
  default:
    break;
  }
  if (which == 0) return sn;
  if (which == 1) return cs;
  return ycxx::detail::fpm::mp_div(sn, cs);
}

// sin(pi x) for an exactly represented finite x (lgamma/tgamma reflection).
template <int N>
constexpr mpf<N> mp_sinpi(const fp_value& v) noexcept {
  // 2|x| = k + 2f with k an integer and |f| <= 1/4: sin(pi |x|) = sin(k pi/2 + pi f).
  const int e = v.exp + 1; // 2|x| = sig * 2^e
  unsigned k = 0;
  mpf<N> f;
  if (e >= 0) {
    if (e < 2) k = static_cast<unsigned>(ycxx::detail::fpm::wide_shl(v.sig, e).w[0] & 3);
  } else {
    const int s = -e;
    bool rest = false;
    wide<2> ip = ycxx::detail::fpm::wide_shr(v.sig, s, rest);
    wide<2> low = ycxx::detail::fpm::wide_low_bits(v.sig, s);
    bool neg = false;
    if (ycxx::detail::fpm::wide_bit(low, s - 1)) { // fraction >= 1/2: round k up
      ycxx::detail::fpm::wide_add_small(ip, 1);
      wide<2> one;
      ycxx::detail::fpm::wide_set_bit(one, s);
      ycxx::detail::fpm::wide_sub(one, low);
      low = one;
      neg = true;
    }
    k = static_cast<unsigned>(ip.w[0] & 3);
    f = ycxx::detail::fpm::mp_make<N>(neg, low, e - 1); // (2|x| - k) / 2
  }
  mpf<N> s, c;
  ycxx::detail::fpm::mp_sin_cos_small(ycxx::detail::fpm::mp_mul(f, ycxx::detail::fpm::mp_const<N>(math_constant::pi)), s, c);
  mpf<N> r = (k & 1) ? c : s;
  if (k & 2) r = ycxx::detail::fpm::mp_neg(r);
  if (v.neg) r = ycxx::detail::fpm::mp_neg(r);
  return r;
}

// atan(x), any finite x.
template <int N>
constexpr mpf<N> mp_atan(const mpf<N>& x) noexcept {
  if (ycxx::detail::fpm::mp_is_zero(x)) return x;
  const mpf<N> one = ycxx::detail::fpm::mp_one<N>();
  mpf<N> z = ycxx::detail::fpm::mp_abs(x);
  const bool invert = ycxx::detail::fpm::mp_cmp_abs(z, one) > 0;
  if (invert) z = ycxx::detail::fpm::mp_recip(z);
  // atan(z) = 2 atan(z / (1 + sqrt(1 + z^2))): three halvings bring z below tan(pi/32).
  int halvings = 0;
  for (; halvings < 3 && ycxx::detail::fpm::mp_ilog(z) >= -4; ++halvings)
    z = ycxx::detail::fpm::mp_div(
        z, ycxx::detail::fpm::mp_add(one, ycxx::detail::fpm::mp_sqrt(ycxx::detail::fpm::mp_add(one, ycxx::detail::fpm::mp_mul(z, z)))));
  const mpf<N> z2 = ycxx::detail::fpm::mp_mul(z, z);
  mpf<N> pow = z, sum = z;
  const int scale = ycxx::detail::fpm::mp_ilog(z);
  for (u64 k = 3;; k += 2) {
    pow = ycxx::detail::fpm::mp_neg(ycxx::detail::fpm::mp_mul(pow, z2));
    const mpf<N> term = ycxx::detail::fpm::mp_div_u64(pow, k);
    if (ycxx::detail::fpm::mp_negligible(term, scale)) break;
    sum = ycxx::detail::fpm::mp_add(sum, term);
  }
  sum = ycxx::detail::fpm::mp_ldexp(sum, halvings);
  if (invert) sum = ycxx::detail::fpm::mp_sub(ycxx::detail::fpm::mp_ldexp(ycxx::detail::fpm::mp_const<N>(math_constant::pi), -1), sum);
  sum.neg = x.neg;
  return sum;
}
// atan2(y, x) for finite y, x, not both zero.
template <int N>
constexpr mpf<N> mp_atan2(const mpf<N>& y, const mpf<N>& x) noexcept {
  const mpf<N> pi = ycxx::detail::fpm::mp_const<N>(math_constant::pi);
  mpf<N> r;
  if (ycxx::detail::fpm::mp_is_zero(x)) {
    r = ycxx::detail::fpm::mp_ldexp(pi, -1);
  } else if (ycxx::detail::fpm::mp_is_zero(y)) {
    r = x.neg ? pi : mpf<N>{};
  } else {
    r = ycxx::detail::fpm::mp_atan(ycxx::detail::fpm::mp_div(ycxx::detail::fpm::mp_abs(y), ycxx::detail::fpm::mp_abs(x)));
    if (x.neg) r = ycxx::detail::fpm::mp_sub(pi, r);
  }
  r.neg = y.neg && !ycxx::detail::fpm::mp_is_zero(r);
  return r;
}

// ---- erf, erfc --------------------------------------------------------------------------------------
// erf(x) for 0 <= x < 3.
template <int N>
constexpr mpf<N> mp_erf_small(const mpf<N>& x) noexcept {
  const mpf<N> two_over_sqrtpi = ycxx::detail::fpm::mp_ldexp(ycxx::detail::fpm::mp_const<N>(math_constant::inv_sqrtpi), 1);
  const mpf<N> x2 = ycxx::detail::fpm::mp_mul(x, x);
  if (ycxx::detail::fpm::mp_ilog(x) < -1) {
    // Maclaurin: sum (-1)^n x^(2n+1) / (n! (2n+1)).
    mpf<N> pow = x, sum = x;
    const int scale = ycxx::detail::fpm::mp_ilog(x);
    for (u64 n = 1;; ++n) {
      pow = ycxx::detail::fpm::mp_neg(ycxx::detail::fpm::mp_div_u64(ycxx::detail::fpm::mp_mul(pow, x2), n));
      const mpf<N> term = ycxx::detail::fpm::mp_div_u64(pow, 2 * n + 1);
      if (ycxx::detail::fpm::mp_negligible(term, scale)) break;
      sum = ycxx::detail::fpm::mp_add(sum, term);
    }
    return ycxx::detail::fpm::mp_mul(sum, two_over_sqrtpi);
  }
  // erf(x) = 2/sqrt(pi) e^(-x^2) sum 2^n x^(2n+1) / (1 3 5 ... (2n+1)): positive terms.
  const mpf<N> two_x2 = ycxx::detail::fpm::mp_ldexp(x2, 1);
  mpf<N> term = x, sum = x;
  for (u64 n = 1;; ++n) {
    term = ycxx::detail::fpm::mp_div_u64(ycxx::detail::fpm::mp_mul(term, two_x2), 2 * n + 1);
    if (ycxx::detail::fpm::mp_negligible(term, ycxx::detail::fpm::mp_ilog(sum))) break;
    sum = ycxx::detail::fpm::mp_add(sum, term);
  }
  return ycxx::detail::fpm::mp_mul(ycxx::detail::fpm::mp_mul(sum, two_over_sqrtpi), ycxx::detail::fpm::mp_exp(ycxx::detail::fpm::mp_neg(x2)));
}
// erfc(x) for x >= 3 (continued fraction).
template <int N>
constexpr mpf<N> mp_erfc_large(const mpf<N>& x, double xd) noexcept {
  const int depth = static_cast<int>((N <= 2 ? 2000.0 : 3600.0) / (xd * xd)) + (N <= 2 ? 20 : 30);
  mpf<N> t = x;
  for (int k = depth; k >= 1; --k)
    t = ycxx::detail::fpm::mp_add(x, ycxx::detail::fpm::mp_div(ycxx::detail::fpm::mp_ldexp(ycxx::detail::fpm::mp_from_u64<N>(u64(k)), -1), t));
  const mpf<N> e = ycxx::detail::fpm::mp_exp(ycxx::detail::fpm::mp_neg(ycxx::detail::fpm::mp_mul(x, x)));
  return ycxx::detail::fpm::mp_div(ycxx::detail::fpm::mp_mul(e, ycxx::detail::fpm::mp_const<N>(math_constant::inv_sqrtpi)), t);
}

// ---- gamma ---------------------------------------------------------------------------------------------
template <int N>
inline constexpr int stirling_min_x = N <= 2 ? 20 : 36;
template <int N>
inline constexpr int stirling_terms = N <= 2 ? 25 : 32;

// ln Gamma(x) for x >= stirling_min_x.
template <int N>
constexpr mpf<N> mp_lgamma_stirling(const mpf<N>& x) noexcept {
  const mpf<N> half = ycxx::detail::fpm::mp_ldexp(ycxx::detail::fpm::mp_one<N>(), -1);
  mpf<N> r = ycxx::detail::fpm::mp_sub(ycxx::detail::fpm::mp_mul(ycxx::detail::fpm::mp_sub(x, half), ycxx::detail::fpm::mp_log(x)), x);
  r = ycxx::detail::fpm::mp_add(r, ycxx::detail::fpm::mp_const<N>(ycxx::detail::fpm::half_ln_2pi));
  const mpf<N> inv = ycxx::detail::fpm::mp_recip(x), inv2 = ycxx::detail::fpm::mp_mul(inv, inv);
  mpf<N> pow = inv;
  for (int k = 0; k < stirling_terms<N>; ++k) {
    const mpf<N> term = ycxx::detail::fpm::mp_mul(ycxx::detail::fpm::mp_const<N>(ycxx::detail::fpm::stirling_coefficients[k]), pow);
    if (ycxx::detail::fpm::mp_negligible(term, 0)) break;
    r = ycxx::detail::fpm::mp_add(r, term);
    pow = ycxx::detail::fpm::mp_mul(pow, inv2);
  }
  return r;
}
// ln Gamma(x) for x > 0, and (for x below the Stirling range) the shift product.
template <int N>
constexpr mpf<N> mp_lgamma_pos(const mpf<N>& x) noexcept {
  const mpf<N> lim = ycxx::detail::fpm::mp_from_u64<N>(stirling_min_x<N>);
  if (ycxx::detail::fpm::mp_cmp(x, lim) >= 0) return ycxx::detail::fpm::mp_lgamma_stirling(x);
  // Gamma(x) = Gamma(x + n) / (x (x + 1) ... (x + n - 1))
  mpf<N> prod = x, y = x;
  const mpf<N> one = ycxx::detail::fpm::mp_one<N>();
  for (;;) {
    y = ycxx::detail::fpm::mp_add(y, one);
    if (ycxx::detail::fpm::mp_cmp(y, lim) >= 0) break;
    prod = ycxx::detail::fpm::mp_mul(prod, y);
  }
  return ycxx::detail::fpm::mp_sub(ycxx::detail::fpm::mp_lgamma_stirling(y), ycxx::detail::fpm::mp_log(prod));
}

// ---- the functions on T ----------------------------------------------------------------------------
template <class T>
inline constexpr int mp_limbs = fp_layout<T>::p <= 64 ? 2 : 3;

template <class T>
constexpr bool fp_is_int(const fp_value& v) noexcept {
  return v.kind == fp_kind::zero || (v.kind == fp_kind::finite && (v.exp >= 0 || ycxx::detail::fpm::wide_ctz(v.sig) >= -v.exp));
}
template <class T>
constexpr bool fp_is_odd_int(const fp_value& v) noexcept {
  if (v.kind != fp_kind::finite || v.exp > 0) return false;
  return ycxx::detail::fpm::fp_is_int<T>(v) && ycxx::detail::fpm::wide_bit(v.sig, -v.exp);
}

// Overflow / underflow screens for results e^w: w > (emax + 1) ln 2 overflows; w below
// (qmin - 2) ln 2 rounds to zero.
template <class T, int N>
constexpr int mp_exp_range(const mpf<N>& w) noexcept {
  using L = fp_layout<T>;
  const mpf<N> ln2 = ycxx::detail::fpm::mp_const<N>(math_constant::ln2);
  if (ycxx::detail::fpm::mp_cmp(w, ycxx::detail::fpm::mp_mul_u64(ln2, u64(L::emax + 1))) > 0) return 1;
  if (ycxx::detail::fpm::mp_cmp(w, ycxx::detail::fpm::mp_neg(ycxx::detail::fpm::mp_mul_u64(ln2, u64(2 - L::qmin)))) < 0) return -1;
  return 0;
}
template <class T>
constexpr T fp_overflow(bool neg) noexcept {
  ycxx::detail::fpm::fp_report(fe_overflow | fe_inexact);
  return ycxx::detail::fpm::fp_infinity<T>(neg);
}
template <class T>
constexpr T fp_underflow(bool neg) noexcept {
  ycxx::detail::fpm::fp_report(fe_underflow | fe_inexact);
  return ycxx::detail::fpm::fp_zero<T>(neg);
}
template <class T>
constexpr T fp_pole(bool neg) noexcept {
  ycxx::detail::fpm::fp_report(fe_divbyzero);
  return ycxx::detail::fpm::fp_infinity<T>(neg);
}

// e^w with range screening.
template <class T, int N>
constexpr T mp_exp_to(const mpf<N>& w, bool neg = false) noexcept {
  const int range = ycxx::detail::fpm::mp_exp_range<T>(w);
  if (range > 0) return ycxx::detail::fpm::fp_overflow<T>(neg);
  if (range < 0) return ycxx::detail::fpm::fp_underflow<T>(neg);
  mpf<N> r = ycxx::detail::fpm::mp_exp(w);
  r.neg = neg;
  return ycxx::detail::fpm::mp_to<T>(r);
}

template <class T>
constexpr T fp_exp(T x) noexcept {
  constexpr int N = mp_limbs<T>;
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  switch (v.kind) {
  case fp_kind::nan:
    return ycxx::detail::fpm::fp_nan_operand(x);
  case fp_kind::inf:
    return v.neg ? T(0) : x;
  case fp_kind::zero:
    return T(1);
  default:
    if (v.exp + ycxx::detail::fpm::wide_bitlen(v.sig) > 20) return v.neg ? ycxx::detail::fpm::fp_underflow<T>(false) : ycxx::detail::fpm::fp_overflow<T>(false);
    return ycxx::detail::fpm::mp_exp_to<T>(ycxx::detail::fpm::mp_from_value<N>(v));
  }
}
template <class T>
constexpr T fp_exp2(T x) noexcept {
  constexpr int N = mp_limbs<T>;
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  switch (v.kind) {
  case fp_kind::nan:
    return ycxx::detail::fpm::fp_nan_operand(x);
  case fp_kind::inf:
    return v.neg ? T(0) : x;
  case fp_kind::zero:
    return T(1);
  default:
    if (v.exp + ycxx::detail::fpm::wide_bitlen(v.sig) > 20) return v.neg ? ycxx::detail::fpm::fp_underflow<T>(false) : ycxx::detail::fpm::fp_overflow<T>(false);
    if (ycxx::detail::fpm::fp_is_int<T>(v)) // exact
      return ycxx::detail::fpm::fp_scale(T(1), static_cast<long>(ycxx::detail::fpm::mp_round_int(ycxx::detail::fpm::mp_from_value<N>(v))));
    return ycxx::detail::fpm::mp_exp_to<T>(
        ycxx::detail::fpm::mp_mul(ycxx::detail::fpm::mp_from_value<N>(v), ycxx::detail::fpm::mp_const<N>(math_constant::ln2)));
  }
}
template <class T>
constexpr T fp_expm1(T x) noexcept {
  constexpr int N = mp_limbs<T>;
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  switch (v.kind) {
  case fp_kind::nan:
    return ycxx::detail::fpm::fp_nan_operand(x);
  case fp_kind::inf:
    return v.neg ? T(-1) : x;
  case fp_kind::zero:
    return x;
  default:
    if (v.exp + ycxx::detail::fpm::wide_bitlen(v.sig) > 20) {
      if (v.neg) return ycxx::detail::fpm::mp_to<T>(ycxx::detail::fpm::mp_from_int<N>(-1)); // -1, inexact
      return ycxx::detail::fpm::fp_overflow<T>(false);
    }
    const mpf<N> w = ycxx::detail::fpm::mp_from_value<N>(v);
    if (ycxx::detail::fpm::mp_exp_range<T>(w) > 0) return ycxx::detail::fpm::fp_overflow<T>(false);
    return ycxx::detail::fpm::mp_to<T>(ycxx::detail::fpm::mp_expm1(w));
  }
}

// log (base 0: e, 2, 10)
template <class T>
constexpr T fp_log_base(T x, int base) noexcept {
  constexpr int N = mp_limbs<T>;
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  if (v.kind == fp_kind::nan) return ycxx::detail::fpm::fp_nan_operand(x);
  if (v.kind == fp_kind::zero) return ycxx::detail::fpm::fp_pole<T>(true);
  if (v.neg) return ycxx::detail::fpm::fp_invalid<T>();
  if (v.kind == fp_kind::inf) return x;
  if (x == T(1)) return T(0);
  if (base == 2 && ycxx::detail::fpm::wide_bitlen(v.sig) == ycxx::detail::fpm::wide_ctz(v.sig) + 1) // a power of 2
    return T(v.exp + ycxx::detail::fpm::wide_ctz(v.sig));
  mpf<N> r = ycxx::detail::fpm::mp_log(ycxx::detail::fpm::mp_from_value<N>(v));
  if (base == 2) r = ycxx::detail::fpm::mp_mul(r, ycxx::detail::fpm::mp_const<N>(math_constant::log2e));
  if (base == 10) r = ycxx::detail::fpm::mp_mul(r, ycxx::detail::fpm::mp_const<N>(math_constant::log10e));
  return ycxx::detail::fpm::mp_to<T>(r);
}
template <class T>
constexpr T fp_log1p(T x) noexcept {
  constexpr int N = mp_limbs<T>;
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  if (v.kind == fp_kind::nan) return ycxx::detail::fpm::fp_nan_operand(x);
  if (v.kind == fp_kind::zero) return x;
  if (x == T(-1)) return ycxx::detail::fpm::fp_pole<T>(true);
  if (x < T(-1)) return ycxx::detail::fpm::fp_invalid<T>();
  if (v.kind == fp_kind::inf) return x;
  return ycxx::detail::fpm::mp_to<T>(ycxx::detail::fpm::mp_log1p(ycxx::detail::fpm::mp_from_value<N>(v)));
}

template <class T>
constexpr T fp_pow(T x, T y) noexcept {
  constexpr int N = mp_limbs<T>;
  const fp_value vx = ycxx::detail::fpm::fp_decode(x), vy = ycxx::detail::fpm::fp_decode(y);
  // F.10.4.5
  if (vy.kind == fp_kind::zero) {
    if (vx.signaling) ycxx::detail::fpm::fp_report(fe_invalid);
    return T(1);
  }
  if (x == T(1)) {
    if (vy.signaling) ycxx::detail::fpm::fp_report(fe_invalid);
    return T(1);
  }
  if (vx.kind == fp_kind::nan || vy.kind == fp_kind::nan) return ycxx::detail::fpm::fp_nan_operands(x, y);
  const bool y_odd = ycxx::detail::fpm::fp_is_odd_int<T>(vy), y_int = ycxx::detail::fpm::fp_is_int<T>(vy) || vy.kind == fp_kind::inf;
  if (vx.kind == fp_kind::zero) {
    if (vy.neg) {
      if (vy.kind == fp_kind::inf) return ycxx::detail::fpm::fp_infinity<T>(false);
      return ycxx::detail::fpm::fp_pole<T>(y_odd && vx.neg);
    }
    return ycxx::detail::fpm::fp_zero<T>(y_odd && vx.neg);
  }
  if (vy.kind == fp_kind::inf) {
    if (x == T(-1)) return T(1);
    const bool big = ycxx::detail::fpm::fp_abs(x) > T(1);
    return big != vy.neg ? ycxx::detail::fpm::fp_infinity<T>(false) : T(0);
  }
  if (vx.kind == fp_kind::inf) {
    if (vx.neg) return vy.neg ? ycxx::detail::fpm::fp_zero<T>(y_odd) : ycxx::detail::fpm::fp_infinity<T>(y_odd);
    return vy.neg ? T(0) : x;
  }
  if (vx.neg && !y_int) return ycxx::detail::fpm::fp_invalid<T>();
  const bool neg = vx.neg && y_odd;
  // Exact cases: integral y with |y| small and an exactly computable power.
  if (y_int && vy.exp + ycxx::detail::fpm::wide_bitlen(vy.sig) <= 8) {
    const long long n = ycxx::detail::fpm::mp_round_int(ycxx::detail::fpm::mp_from_value<N>(vy));
    const int tz = ycxx::detail::fpm::wide_ctz(vx.sig);
    const wide<2> odd = ycxx::detail::fpm::wide_shr(vx.sig, tz);
    const long long e2 = static_cast<long long>(vx.exp + tz);
    if (ycxx::detail::fpm::wide_bitlen(odd) == 1) { // a power of two
      const long long e = e2 * n;
      return ycxx::detail::fpm::fp_scale(neg ? T(-1) : T(1), e > (1L << 20) ? (1L << 20) : e < -(1L << 20) ? -(1L << 20) : static_cast<long>(e));
    }
    if (n > 0 && static_cast<long long>(ycxx::detail::fpm::wide_bitlen(odd)) * n <= 128) {
      wide<4> p = ycxx::detail::fpm::wide_resize<4>(odd);
      for (long long i = 1; i < n; ++i) p = ycxx::detail::fpm::wide_resize<4>(ycxx::detail::fpm::wide_mul(p, ycxx::detail::fpm::wide_resize<2>(odd)));
      const long long e = e2 * n;
      if (e > -(1L << 20) && e < (1L << 20))
        return ycxx::detail::fpm::fp_finish(ycxx::detail::fpm::fp_round<T>(neg, p, static_cast<int>(e)));
    }
  }
  // General: e^(y ln|x|).
  mpf<N> ax = ycxx::detail::fpm::mp_from_value<N>(vx);
  ax.neg = false;
  const mpf<N> w = ycxx::detail::fpm::mp_mul(ycxx::detail::fpm::mp_from_value<N>(vy), ycxx::detail::fpm::mp_log(ax));
  return ycxx::detail::fpm::mp_exp_to<T>(w, neg);
}

template <class T>
constexpr T fp_trig(T x, int which) noexcept {
  constexpr int N = mp_limbs<T>;
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  if (v.kind == fp_kind::nan) return ycxx::detail::fpm::fp_nan_operand(x);
  if (v.kind == fp_kind::inf) return ycxx::detail::fpm::fp_invalid<T>();
  if (v.kind == fp_kind::zero) return which == 1 ? T(1) : x;
  return ycxx::detail::fpm::mp_to<T>(ycxx::detail::fpm::mp_trig<N>(v, which));
}

template <class T>
constexpr T fp_atan(T x) noexcept {
  constexpr int N = mp_limbs<T>;
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  if (v.kind == fp_kind::nan) return ycxx::detail::fpm::fp_nan_operand(x);
  if (v.kind == fp_kind::zero) return x;
  if (v.kind == fp_kind::inf) {
    mpf<N> h = ycxx::detail::fpm::mp_ldexp(ycxx::detail::fpm::mp_const<N>(math_constant::pi), -1);
    h.neg = v.neg;
    return ycxx::detail::fpm::mp_to<T>(h);
  }
  return ycxx::detail::fpm::mp_to<T>(ycxx::detail::fpm::mp_atan(ycxx::detail::fpm::mp_from_value<N>(v)));
}
template <class T>
constexpr T fp_atan2(T y, T x) noexcept {
  constexpr int N = mp_limbs<T>;
  const fp_value vy = ycxx::detail::fpm::fp_decode(y), vx = ycxx::detail::fpm::fp_decode(x);
  if (vy.kind == fp_kind::nan || vx.kind == fp_kind::nan) return ycxx::detail::fpm::fp_nan_operands(y, x);
  const mpf<N> pi = ycxx::detail::fpm::mp_const<N>(math_constant::pi);
  mpf<N> r;
  if (vy.kind == fp_kind::zero) {
    if (!vx.neg) return y; // +-0
    r = pi;
  } else if (vy.kind == fp_kind::inf) {
    if (vx.kind == fp_kind::inf)
      r = vx.neg ? ycxx::detail::fpm::mp_ldexp(ycxx::detail::fpm::mp_mul_u64(pi, 3), -2) : ycxx::detail::fpm::mp_ldexp(pi, -2);
    else
      r = ycxx::detail::fpm::mp_ldexp(pi, -1);
  } else if (vx.kind == fp_kind::inf) {
    if (!vx.neg) return ycxx::detail::fpm::fp_zero<T>(vy.neg);
    r = pi;
  } else if (vx.kind == fp_kind::zero) {
    r = ycxx::detail::fpm::mp_ldexp(pi, -1);
  } else {
    r = ycxx::detail::fpm::mp_atan2(ycxx::detail::fpm::mp_from_value<N>(vy), ycxx::detail::fpm::mp_from_value<N>(vx));
    r.neg = false;
  }
  r.neg = vy.neg;
  return ycxx::detail::fpm::mp_to<T>(r);
}
// asin (acos: which == 1)
template <class T>
constexpr T fp_asin_acos(T x, bool acos) noexcept {
  constexpr int N = mp_limbs<T>;
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  if (v.kind == fp_kind::nan) return ycxx::detail::fpm::fp_nan_operand(x);
  if (v.kind == fp_kind::inf || ycxx::detail::fpm::fp_abs(x) > T(1)) return ycxx::detail::fpm::fp_invalid<T>();
  if (acos && x == T(1)) return T(0);
  if (!acos && v.kind == fp_kind::zero) return x;
  const mpf<N> one = ycxx::detail::fpm::mp_one<N>();
  const mpf<N> a = ycxx::detail::fpm::mp_from_value<N>(v);
  // sqrt(1 - x^2) = sqrt((1 - x)(1 + x)), exact factors
  const mpf<N> c = ycxx::detail::fpm::mp_sqrt(ycxx::detail::fpm::mp_mul(ycxx::detail::fpm::mp_sub(one, a), ycxx::detail::fpm::mp_add(one, a)));
  if (acos) return ycxx::detail::fpm::mp_to<T>(ycxx::detail::fpm::mp_atan2(c, a));
  if (ycxx::detail::fpm::mp_is_zero(c)) {
    mpf<N> h = ycxx::detail::fpm::mp_ldexp(ycxx::detail::fpm::mp_const<N>(math_constant::pi), -1);
    h.neg = v.neg;
    return ycxx::detail::fpm::mp_to<T>(h);
  }
  return ycxx::detail::fpm::mp_to<T>(ycxx::detail::fpm::mp_atan(ycxx::detail::fpm::mp_div(a, c)));
}

// sinh (0), cosh (1), tanh (2)
template <class T>
constexpr T fp_hyperbolic(T x, int which) noexcept {
  constexpr int N = mp_limbs<T>;
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  if (v.kind == fp_kind::nan) return ycxx::detail::fpm::fp_nan_operand(x);
  if (v.kind == fp_kind::zero) return which == 1 ? T(1) : x;
  if (v.kind == fp_kind::inf) return which == 1 ? ycxx::detail::fpm::fp_abs(x) : which == 0 ? x : (v.neg ? T(-1) : T(1));
  mpf<N> a = ycxx::detail::fpm::mp_from_value<N>(v);
  a.neg = false;
  const mpf<N> one = ycxx::detail::fpm::mp_one<N>();
  if (which == 2) {
    if (ycxx::detail::fpm::mp_ilog(a) >= 7) return v.neg ? ycxx::detail::fpm::mp_to<T>(ycxx::detail::fpm::mp_neg(one)) : ycxx::detail::fpm::mp_to<T>(one); // 1 - tiny
    const mpf<N> e = ycxx::detail::fpm::mp_expm1(ycxx::detail::fpm::mp_ldexp(a, 1));
    mpf<N> r = ycxx::detail::fpm::mp_div(e, ycxx::detail::fpm::mp_add(e, ycxx::detail::fpm::mp_ldexp(one, 1)));
    r.neg = v.neg;
    return ycxx::detail::fpm::mp_to<T>(r);
  }
  if (ycxx::detail::fpm::mp_ilog(a) >= 20) return ycxx::detail::fpm::fp_overflow<T>(which == 0 && v.neg);
  // e^|x| / 2 overflows exactly when sinh/cosh do (up to the last ulp, decided by rounding).
  const mpf<N> half_e = ycxx::detail::fpm::mp_sub(a, ycxx::detail::fpm::mp_const<N>(math_constant::ln2));
  if (ycxx::detail::fpm::mp_exp_range<T>(half_e) > 0) return ycxx::detail::fpm::fp_overflow<T>(which == 0 && v.neg);
  mpf<N> r;
  if (which == 0) {
    // sinh = (E + E / (E + 1)) / 2, E = expm1(|x|): accurate for small |x| too
    const mpf<N> e = ycxx::detail::fpm::mp_expm1(a);
    r = ycxx::detail::fpm::mp_ldexp(ycxx::detail::fpm::mp_add(e, ycxx::detail::fpm::mp_div(e, ycxx::detail::fpm::mp_add(e, one))), -1);
    r.neg = v.neg;
  } else {
    const mpf<N> e = ycxx::detail::fpm::mp_exp(a);
    r = ycxx::detail::fpm::mp_ldexp(ycxx::detail::fpm::mp_add(e, ycxx::detail::fpm::mp_recip(e)), -1);
  }
  return ycxx::detail::fpm::mp_to<T>(r);
}
// asinh (0), acosh (1), atanh (2)
template <class T>
constexpr T fp_inverse_hyperbolic(T x, int which) noexcept {
  constexpr int N = mp_limbs<T>;
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  if (v.kind == fp_kind::nan) return ycxx::detail::fpm::fp_nan_operand(x);
  const mpf<N> one = ycxx::detail::fpm::mp_one<N>();
  if (which == 0) {
    if (v.kind != fp_kind::finite) return x;
    mpf<N> a = ycxx::detail::fpm::mp_from_value<N>(v);
    a.neg = false;
    // log1p(a + a^2 / (1 + sqrt(1 + a^2)))
    const mpf<N> a2 = ycxx::detail::fpm::mp_mul(a, a);
    mpf<N> r = ycxx::detail::fpm::mp_log1p(ycxx::detail::fpm::mp_add(
        a, ycxx::detail::fpm::mp_div(a2, ycxx::detail::fpm::mp_add(one, ycxx::detail::fpm::mp_sqrt(ycxx::detail::fpm::mp_add(one, a2))))));
    r.neg = v.neg;
    return ycxx::detail::fpm::mp_to<T>(r);
  }
  if (which == 1) {
    if (x < T(1)) return ycxx::detail::fpm::fp_invalid<T>();
    if (x == T(1)) return T(0);
    if (v.kind == fp_kind::inf) return x;
    const mpf<N> a = ycxx::detail::fpm::mp_from_value<N>(v);
    const mpf<N> t = ycxx::detail::fpm::mp_sub(a, one); // exact
    return ycxx::detail::fpm::mp_to<T>(ycxx::detail::fpm::mp_log1p(
        ycxx::detail::fpm::mp_add(t, ycxx::detail::fpm::mp_sqrt(ycxx::detail::fpm::mp_mul(t, ycxx::detail::fpm::mp_add(a, one))))));
  }
  if (v.kind == fp_kind::zero) return x;
  const T ax = ycxx::detail::fpm::fp_abs(x);
  if (ax > T(1)) return ycxx::detail::fpm::fp_invalid<T>();
  if (ax == T(1)) return ycxx::detail::fpm::fp_pole<T>(v.neg);
  mpf<N> a = ycxx::detail::fpm::mp_from_value<N>(v);
  a.neg = false;
  // atanh = log1p(2a / (1 - a)) / 2
  mpf<N> r = ycxx::detail::fpm::mp_ldexp(
      ycxx::detail::fpm::mp_log1p(ycxx::detail::fpm::mp_div(ycxx::detail::fpm::mp_ldexp(a, 1), ycxx::detail::fpm::mp_sub(one, a))), -1);
  r.neg = v.neg;
  return ycxx::detail::fpm::mp_to<T>(r);
}

template <class T>
constexpr T fp_erf(T x, bool complement) noexcept {
  constexpr int N = mp_limbs<T>;
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  if (v.kind == fp_kind::nan) return ycxx::detail::fpm::fp_nan_operand(x);
  if (v.kind == fp_kind::inf) return complement ? (v.neg ? T(2) : T(0)) : (v.neg ? T(-1) : T(1));
  if (v.kind == fp_kind::zero) return complement ? T(1) : x;
  mpf<N> a = ycxx::detail::fpm::mp_from_value<N>(v);
  a.neg = false;
  const mpf<N> one = ycxx::detail::fpm::mp_one<N>();
  const bool large = ycxx::detail::fpm::mp_cmp(a, ycxx::detail::fpm::mp_from_u64<N>(3)) >= 0;
  mpf<N> r;
  if (!large) {
    const mpf<N> e = ycxx::detail::fpm::mp_erf_small(a);
    if (!complement) {
      r = e;
      r.neg = v.neg;
    } else {
      r = v.neg ? ycxx::detail::fpm::mp_add(one, e) : ycxx::detail::fpm::mp_sub(one, e);
    }
    return ycxx::detail::fpm::mp_to<T>(r);
  }
  // |x| >= 3: through erfc(|x|) = e^(-x^2) / sqrt(pi) * CF.
  if (ycxx::detail::fpm::mp_ilog(a) >= 16) { // erfc(|x|) underflows to 0 in every format
    if (complement) return v.neg ? ycxx::detail::fpm::mp_to<T>(ycxx::detail::fpm::mp_ldexp(one, 1)) : ycxx::detail::fpm::fp_underflow<T>(false);
    return ycxx::detail::fpm::mp_to<T>(v.neg ? ycxx::detail::fpm::mp_neg(one) : one);
  }
  double ad = 0;
  {
    const mpf<N> t = a;
    ad = ycxx::detail::fpm::mp_lead(t);
    for (int e = t.exp + 64 * N; e > 0; --e) ad *= 2;
  }
  const mpf<N> c = ycxx::detail::fpm::mp_erfc_large(a, ad);
  if (complement) {
    if (v.neg) return ycxx::detail::fpm::mp_to<T>(ycxx::detail::fpm::mp_sub(ycxx::detail::fpm::mp_ldexp(one, 1), c));
    return ycxx::detail::fpm::mp_to<T>(c);
  }
  r = ycxx::detail::fpm::mp_sub(one, c);
  r.neg = v.neg;
  return ycxx::detail::fpm::mp_to<T>(r);
}

// lgamma (log of |Gamma|) and tgamma.
template <class T>
constexpr T fp_gamma(T x, bool log) noexcept {
  constexpr int N = mp_limbs<T>;
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  if (v.kind == fp_kind::nan) return ycxx::detail::fpm::fp_nan_operand(x);
  if (v.kind == fp_kind::inf) {
    if (log) return ycxx::detail::fpm::fp_infinity<T>(false);
    return v.neg ? ycxx::detail::fpm::fp_invalid<T>() : x;
  }
  if (v.kind == fp_kind::zero) return ycxx::detail::fpm::fp_pole<T>(!log && v.neg);
  const bool is_int = ycxx::detail::fpm::fp_is_int<T>(v);
  if (v.neg && is_int) return log ? ycxx::detail::fpm::fp_pole<T>(false) : ycxx::detail::fpm::fp_invalid<T>();
  if (log && (x == T(1) || x == T(2))) return T(0);
  const mpf<N> a = ycxx::detail::fpm::mp_from_value<N>(v);
  if (ycxx::detail::fpm::mp_ilog(a) >= 30) { // huge |x|
    if (!v.neg) return log ? ycxx::detail::fpm::mp_to<T>(ycxx::detail::fpm::mp_lgamma_pos(a)) : ycxx::detail::fpm::fp_overflow<T>(false);
  }
  const mpf<N> one = ycxx::detail::fpm::mp_one<N>();
  mpf<N> lg; // ln |Gamma(x)|
  bool neg = false;
  if (!v.neg) {
    lg = ycxx::detail::fpm::mp_lgamma_pos(a);
  } else {
    // Gamma(x) Gamma(1 - x) = pi / sin(pi x)
    const mpf<N> s = ycxx::detail::fpm::mp_sinpi<N>(v);
    neg = s.neg;
    const mpf<N> pi = ycxx::detail::fpm::mp_const<N>(math_constant::pi);
    lg = ycxx::detail::fpm::mp_sub(ycxx::detail::fpm::mp_log(ycxx::detail::fpm::mp_div(pi, ycxx::detail::fpm::mp_abs(s))),
                                   ycxx::detail::fpm::mp_lgamma_pos(ycxx::detail::fpm::mp_sub(one, a)));
  }
  if (log) return ycxx::detail::fpm::mp_to<T>(lg);
  return ycxx::detail::fpm::mp_exp_to<T>(lg, neg);
}

}} // namespace ycxx::detail::fpm
