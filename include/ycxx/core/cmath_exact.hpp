// libycxx core: constexpr software implementations of the <cmath> functions whose results are
// exact or need only correct rounding of an exactly computable value (fma, sqrt, cbrt, hypot,
// fmod, remainder, nextafter, ...), for every binary floating-point format (cmath_fp.hpp).
//
// They are what constant evaluation uses (the compilers fold few of these builtins), and what
// the extended types without a C library counterpart use at run time. Results are correctly
// rounded; the floating-point exceptions of ISO/IEC 9899:2024 Annex F other than "inexact"
// go through fp_report, which makes a constant evaluation non-constant ([library.c]/3).
// Arguments are finite unless a function says otherwise; NaN and infinity handling that the
// C standard specifies is done here too.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/cmath_fp.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::fpm {

// ---- NaN handling ----------------------------------------------------------------------------
template <class T>
constexpr bool fp_isnan(T x) noexcept {
  return __builtin_isnan(x);
}
template <class T>
constexpr bool fp_signbit(T x) noexcept {
  return __builtin_signbit(x);
}
template <class T>
constexpr bool fp_issignaling(T x) noexcept {
  return __builtin_issignaling(x);
}
// A NaN operand: a signaling NaN raises "invalid" and becomes quiet.
template <class T>
constexpr T fp_nan_operand(T x) noexcept {
  if (ycxx::detail::fpm::fp_issignaling(x)) {
    ycxx::detail::fpm::fp_report(fe_invalid);
    return ycxx::detail::fpm::fp_quiet_nan<T>(ycxx::detail::fpm::fp_signbit(x));
  }
  return x;
}
template <class T>
constexpr T fp_nan_operands(T x, T y) noexcept {
  if (ycxx::detail::fpm::fp_issignaling(x) || ycxx::detail::fpm::fp_issignaling(y))
    ycxx::detail::fpm::fp_report(fe_invalid);
  return ycxx::detail::fpm::fp_isnan(x) ? (ycxx::detail::fpm::fp_issignaling(x) ? ycxx::detail::fpm::fp_quiet_nan<T>() : x)
                                        : (ycxx::detail::fpm::fp_issignaling(y) ? ycxx::detail::fpm::fp_quiet_nan<T>() : y);
}
template <class T>
constexpr T fp_invalid() noexcept { // a new NaN with "invalid"
  ycxx::detail::fpm::fp_report(fe_invalid);
  return ycxx::detail::fpm::fp_quiet_nan<T>();
}
template <class T>
constexpr T fp_finish(fp_result<T> r) noexcept {
  ycxx::detail::fpm::fp_report(r.flags);
  return r.value;
}

template <class T>
constexpr T fp_abs(T x) noexcept {
  return ycxx::detail::fpm::fp_signbit(x) ? -x : x;
}
template <class T>
constexpr T fp_copysign(T x, T y) noexcept {
  return ycxx::detail::fpm::fp_signbit(x) != ycxx::detail::fpm::fp_signbit(y) ? -x : x;
}

// ---- rounding to integers ---------------------------------------------------------------------
enum class fp_rint_mode : unsigned char { trunc, floor, ceil, half_away, half_even };

// The integer part of a finite decoded value, rounded by `mode`. `inexact` tells whether a
// fraction was dropped.
template <class T>
constexpr wide<2> fp_integer_part(const fp_value& v, fp_rint_mode mode, bool& inexact) noexcept {
  if (v.exp >= 0) {
    inexact = false;
    return ycxx::detail::fpm::wide_shl(v.sig, v.exp > 127 ? 128 : v.exp); // callers check the range
  }
  const int s = -v.exp;
  const bool half = ycxx::detail::fpm::wide_bit(v.sig, s - 1);
  bool rest = false;
  wide<2> ip = ycxx::detail::fpm::wide_shr(ycxx::detail::fpm::wide_shr(v.sig, s - 1, rest), 1);
  inexact = half || rest;
  bool up = false;
  switch (mode) {
  case fp_rint_mode::trunc:
    break;
  case fp_rint_mode::floor:
    up = v.neg && inexact;
    break;
  case fp_rint_mode::ceil:
    up = !v.neg && inexact;
    break;
  case fp_rint_mode::half_away:
    up = half;
    break;
  case fp_rint_mode::half_even:
    up = half && (rest || (ip.w[0] & 1));
    break;
  }
  if (up) ycxx::detail::fpm::wide_add_small(ip, 1);
  return ip;
}

template <class T>
constexpr T fp_rint(T x, fp_rint_mode mode) noexcept {
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  if (v.kind == fp_kind::nan) return ycxx::detail::fpm::fp_nan_operand(x);
  if (v.kind != fp_kind::finite || v.exp >= 0) return x;
  bool inexact = false;
  const wide<2> ip = ycxx::detail::fpm::fp_integer_part<T>(v, mode, inexact);
  return ycxx::detail::fpm::fp_round<T>(v.neg, ip, 0).value; // exact; zero keeps the sign
}

// lround/llround (I = long or long long), and the exactly-rounded integer for lrint at
// constant evaluation (not used: lrint is not constexpr).
template <class I, class T>
constexpr I fp_to_integer(T x, fp_rint_mode mode) noexcept {
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  if (v.kind == fp_kind::zero) return 0;
  if (v.kind != fp_kind::finite) {
    ycxx::detail::fpm::fp_report(fe_invalid);
    return static_cast<I>(-__LONG_LONG_MAX__ - 1);
  }
  constexpr int bits = 8 * int(sizeof(I)) - 1;
  bool inexact = false;
  if (v.exp + ycxx::detail::fpm::wide_bitlen(v.sig) > bits + 1) { // |x| >= 2^(bits + 1)
    ycxx::detail::fpm::fp_report(fe_invalid);
    return static_cast<I>(-__LONG_LONG_MAX__ - 1);
  }
  const wide<2> ip = ycxx::detail::fpm::fp_integer_part<T>(v, mode, inexact);
  const u64 limit = (u64(1) << bits) - (v.neg ? 0 : 1); // magnitude limit
  if (ip.w[1] != 0 || ip.w[0] > limit) {
    ycxx::detail::fpm::fp_report(fe_invalid);
    return static_cast<I>(-__LONG_LONG_MAX__ - 1);
  }
  return v.neg ? static_cast<I>(-static_cast<I>(ip.w[0] - 1) - 1) : static_cast<I>(ip.w[0]);
}

template <class T>
constexpr T fp_modf(T x, T* iptr) noexcept {
  if (ycxx::detail::fpm::fp_isnan(x)) {
    x = ycxx::detail::fpm::fp_nan_operand(x);
    *iptr = x;
    return x;
  }
  if (__builtin_isinf(x)) {
    *iptr = x;
    return ycxx::detail::fpm::fp_zero<T>(ycxx::detail::fpm::fp_signbit(x));
  }
  const T ip = ycxx::detail::fpm::fp_rint(x, fp_rint_mode::trunc);
  *iptr = ip;
  return ycxx::detail::fpm::fp_copysign(T(x - ip), x); // exact
}

// ---- exponent manipulation ---------------------------------------------------------------------
template <class T>
constexpr T fp_frexp(T x, int* e) noexcept {
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  *e = 0;
  if (v.kind == fp_kind::nan) return ycxx::detail::fpm::fp_nan_operand(x);
  if (v.kind != fp_kind::finite) return x;
  const int len = ycxx::detail::fpm::wide_bitlen(v.sig);
  *e = v.exp + len;
  return ycxx::detail::fpm::fp_round<T>(v.neg, v.sig, -len).value;
}

template <class T>
constexpr T fp_scale(T x, long n) noexcept {
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  if (v.kind == fp_kind::nan) return ycxx::detail::fpm::fp_nan_operand(x);
  if (v.kind != fp_kind::finite) return x;
  constexpr long lim = 1L << 20; // far beyond every format's exponent range
  if (n > lim) n = lim;
  if (n < -lim) n = -lim;
  using L = fp_layout<T>;
  const long e = long(v.exp) + n;
  if (v.exp > L::qmin && e > L::qmin && e <= L::emax - (L::p - 1)) // normal in, normal out: exact
    return ycxx::detail::fpm::fp_encode_finite<T>(v.neg, v.sig, static_cast<int>(e));
  return ycxx::detail::fpm::fp_finish(ycxx::detail::fpm::fp_round<T>(v.neg, v.sig, static_cast<int>(e)));
}

// ilogb: the exponent; FP_ILOGB0 / INT_MAX / FP_ILOGBNAN (with "invalid", F.10.3.5) otherwise.
template <class T>
constexpr int fp_ilogb(T x, int ilogb0, int ilogbnan) noexcept {
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  if (v.kind != fp_kind::finite) {
    ycxx::detail::fpm::fp_report(fe_invalid);
    return v.kind == fp_kind::zero ? ilogb0 : v.kind == fp_kind::inf ? __INT_MAX__ : ilogbnan;
  }
  return v.exp + ycxx::detail::fpm::wide_bitlen(v.sig) - 1;
}
template <class T>
constexpr T fp_logb(T x) noexcept {
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  switch (v.kind) {
  case fp_kind::nan:
    return ycxx::detail::fpm::fp_nan_operand(x);
  case fp_kind::inf:
    return ycxx::detail::fpm::fp_infinity<T>(false);
  case fp_kind::zero:
    ycxx::detail::fpm::fp_report(fe_divbyzero);
    return ycxx::detail::fpm::fp_infinity<T>(true);
  default:
    return T(v.exp + ycxx::detail::fpm::wide_bitlen(v.sig) - 1);
  }
}

// ---- neighbours ---------------------------------------------------------------------------------
// The next representable value from finite or infinite x towards +inf (up) or -inf.
template <class T>
constexpr T fp_step(T x, bool up) noexcept {
  using L = fp_layout<T>;
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  if (v.kind == fp_kind::inf) {
    if (v.neg == up) { // towards zero: the largest finite value
      wide<2> m = ycxx::detail::fpm::wide_from<2>(1);
      m = ycxx::detail::fpm::wide_shl(m, L::p);
      wide<2> one = ycxx::detail::fpm::wide_from<2>(1);
      ycxx::detail::fpm::wide_sub(m, one);
      return ycxx::detail::fpm::fp_round<T>(v.neg, m, L::emax - (L::p - 1)).value;
    }
    return x;
  }
  if (v.kind == fp_kind::zero) return ycxx::detail::fpm::fp_round<T>(!up, ycxx::detail::fpm::wide_from<2>(1), L::qmin).value;
  wide<2> m = v.sig;
  int e = v.exp;
  if (up != v.neg) { // magnitude grows
    ycxx::detail::fpm::wide_add_small(m, 1);
  } else if (e > L::qmin && ycxx::detail::fpm::wide_bitlen(m) == L::p && ycxx::detail::fpm::wide_ctz(m) == L::p - 1) {
    // A power of two above the subnormal range: the next smaller magnitude has a finer ulp.
    m = ycxx::detail::fpm::wide_shl(m, 1);
    wide<2> one = ycxx::detail::fpm::wide_from<2>(1);
    ycxx::detail::fpm::wide_sub(m, one);
    --e;
  } else {
    wide<2> one = ycxx::detail::fpm::wide_from<2>(1);
    ycxx::detail::fpm::wide_sub(m, one);
  }
  return ycxx::detail::fpm::fp_round<T>(v.neg, m, e).value; // exact, or the overflow to infinity
}

template <class T>
constexpr T fp_nextup(T x) noexcept {
  if (ycxx::detail::fpm::fp_isnan(x)) return ycxx::detail::fpm::fp_nan_operand(x);
  return ycxx::detail::fpm::fp_step(x, true);
}
template <class T>
constexpr T fp_nextdown(T x) noexcept {
  if (ycxx::detail::fpm::fp_isnan(x)) return ycxx::detail::fpm::fp_nan_operand(x);
  return ycxx::detail::fpm::fp_step(x, false);
}
// nextafter / nexttoward: `dir` is the sign of (y - x) (0: equal), y already compared in the
// right type. F.10.8.3: overflow for a finite x with an infinite result, underflow for a
// subnormal or zero result.
template <class T>
constexpr T fp_next_toward(T x, int dir) noexcept {
  const T r = ycxx::detail::fpm::fp_step(x, dir > 0);
  if (__builtin_isinf(r) && !__builtin_isinf(x))
    ycxx::detail::fpm::fp_report(fe_overflow | fe_inexact);
  else if (!__builtin_isnormal(r))
    ycxx::detail::fpm::fp_report(fe_underflow | fe_inexact);
  return r;
}
template <class T>
constexpr T fp_nextafter(T x, T y) noexcept {
  if (ycxx::detail::fpm::fp_isnan(x) || ycxx::detail::fpm::fp_isnan(y)) return ycxx::detail::fpm::fp_nan_operands(x, y);
  if (x == y) return y;
  return ycxx::detail::fpm::fp_next_toward(x, y > x ? 1 : -1);
}
template <class T>
constexpr T fp_nexttoward(T x, long double y) noexcept {
  if (ycxx::detail::fpm::fp_isnan(x)) return ycxx::detail::fpm::fp_nan_operand(x);
  if (ycxx::detail::fpm::fp_isnan(y)) {
    if (ycxx::detail::fpm::fp_issignaling(y)) ycxx::detail::fpm::fp_report(fe_invalid);
    return ycxx::detail::fpm::fp_quiet_nan<T>();
  }
  const long double lx = static_cast<long double>(x); // exact: T is a standard type
  if (lx == y) return static_cast<T>(y);
  return ycxx::detail::fpm::fp_next_toward(x, y > lx ? 1 : -1);
}

// ---- min / max -------------------------------------------------------------------------------
// num: C's fmax/fmin and fmaximum_num/fminimum_num (a NaN operand is ignored); otherwise
// fmaximum/fminimum (NaN wins). All order -0 below +0 (allowed for fmax/fmin).
template <class T>
constexpr T fp_minmax(T x, T y, bool max, bool num) noexcept {
  const bool nx = ycxx::detail::fpm::fp_isnan(x), ny = ycxx::detail::fpm::fp_isnan(y);
  if (nx || ny) {
    if (ycxx::detail::fpm::fp_issignaling(x) || ycxx::detail::fpm::fp_issignaling(y))
      ycxx::detail::fpm::fp_report(fe_invalid);
    if (num && !(nx && ny)) return nx ? y : x;
    return ycxx::detail::fpm::fp_quiet_nan<T>(); // the payload is not preserved
  }
  if (x == y) { // equal values, or zeros of either sign
    const bool sx = ycxx::detail::fpm::fp_signbit(x);
    return (sx == max) ? y : x;
  }
  return (x < y) == max ? y : x;
}

template <class T>
constexpr T fp_fdim(T x, T y) noexcept {
  if (ycxx::detail::fpm::fp_isnan(x) || ycxx::detail::fpm::fp_isnan(y)) return ycxx::detail::fpm::fp_nan_operands(x, y);
  if (!(x > y)) return T(0);
  const T r = x - y;
  if (__builtin_isinf(r) && !__builtin_isinf(x) && !__builtin_isinf(y)) ycxx::detail::fpm::fp_report(fe_overflow | fe_inexact);
  return r;
}

// ---- fmod, remainder, remquo ------------------------------------------------------------------
// |x| mod |y| for finite nonzero x, y, with the low three bits of the quotient.
struct fp_mod_result {
  wide<2> rem; // remainder * 2^exp
  int exp;
  unsigned quo; // the low bits of the integral quotient
};
template <class T>
constexpr fp_mod_result fp_mod(const fp_value& x, const fp_value& y) noexcept {
  // |x| = mx * 2^ex, |y| = my * 2^ey. With d = ex - ey >= 0: rem = (mx * 2^d) mod my, at 2^ey.
  // With d < 0: scale my up instead (exact; |x| >= |y| was checked by the caller or not).
  wide<2> mx = x.sig, my = y.sig;
  int e = y.exp;
  int d = x.exp - y.exp;
  if (d < 0) {
    // Bring both to the exponent of x: my * 2^-d at 2^ex.
    const int len = ycxx::detail::fpm::wide_bitlen(my);
    if (len - d > 127) return {mx, x.exp, 0}; // |y| is far larger than |x|
    my = ycxx::detail::fpm::wide_shl(my, -d);
    e = x.exp;
    d = 0;
  }
  unsigned quo = 0;
  // r = mx mod my, then repeatedly r = (r * 2^k) mod my.
  wide<2> r = mx;
  {
    // Initial reduction: mx < 2^128, so do it bitwise from the top.
    unsigned q = 0;
    if (ycxx::detail::fpm::wide_cmp(r, my) >= 0) {
      const int shift = ycxx::detail::fpm::wide_bitlen(r) - ycxx::detail::fpm::wide_bitlen(my);
      for (int s = shift; s >= 0; --s) {
        const wide<2> t = ycxx::detail::fpm::wide_shl(my, s);
        q <<= 1;
        if (ycxx::detail::fpm::wide_cmp(r, t) >= 0) {
          ycxx::detail::fpm::wide_sub(r, t);
          q |= 1;
        }
      }
    }
    quo = q;
  }
  if (my.w[1] == 0) {
    // 64-bit modulus: 63 bits at a time with a 128 / 64 division.
    const u64 m = my.w[0];
    u64 rr = r.w[0];
    while (d > 0) {
      const int k = d > 63 ? 63 : d;
      const u64 hi = rr >> (64 - k), lo = rr << k;
      u64 rem = 0;
      const u64 q = ycxx::detail::fpm::div128(hi, lo, m, rem);
      quo = k >= 3 ? static_cast<unsigned>(q & 7) : static_cast<unsigned>(((quo << k) | q) & 7);
      rr = rem;
      d -= k;
    }
    r = ycxx::detail::fpm::wide_from<2>(rr);
  } else {
    while (d > 0) { // one bit at a time
      const bool top = ycxx::detail::fpm::wide_bit(r, 127);
      r = ycxx::detail::fpm::wide_shl(r, 1);
      quo <<= 1;
      if (top || ycxx::detail::fpm::wide_cmp(r, my) >= 0) {
        ycxx::detail::fpm::wide_sub(r, my);
        quo |= 1;
      }
      --d;
    }
  }
  return {r, e, quo & 7};
}

template <class T>
constexpr T fp_fmod(T x, T y) noexcept {
  if (ycxx::detail::fpm::fp_isnan(x) || ycxx::detail::fpm::fp_isnan(y)) return ycxx::detail::fpm::fp_nan_operands(x, y);
  const fp_value vx = ycxx::detail::fpm::fp_decode(x), vy = ycxx::detail::fpm::fp_decode(y);
  if (vx.kind == fp_kind::inf || vy.kind == fp_kind::zero) return ycxx::detail::fpm::fp_invalid<T>();
  if (vx.kind == fp_kind::zero || vy.kind == fp_kind::inf) return x;
  if (ycxx::detail::fpm::fp_abs(x) < ycxx::detail::fpm::fp_abs(y)) return x;
  const fp_mod_result r = ycxx::detail::fpm::fp_mod<T>(vx, vy);
  return ycxx::detail::fpm::fp_round<T>(vx.neg, r.rem, r.exp).value; // exact
}

template <class T>
constexpr T fp_remquo(T x, T y, int* quo) noexcept {
  *quo = 0;
  if (ycxx::detail::fpm::fp_isnan(x) || ycxx::detail::fpm::fp_isnan(y)) return ycxx::detail::fpm::fp_nan_operands(x, y);
  const fp_value vx = ycxx::detail::fpm::fp_decode(x), vy = ycxx::detail::fpm::fp_decode(y);
  if (vx.kind == fp_kind::inf || vy.kind == fp_kind::zero) return ycxx::detail::fpm::fp_invalid<T>();
  if (vx.kind == fp_kind::zero || vy.kind == fp_kind::inf) return x;
  fp_mod_result r{vx.sig, vx.exp, 0};
  const bool small = ycxx::detail::fpm::fp_abs(x) < ycxx::detail::fpm::fp_abs(y);
  if (!small) r = ycxx::detail::fpm::fp_mod<T>(vx, vy);
  // Round the quotient to nearest (ties to even): compare 2 * rem with |y|, at a common scale.
  // rem is at 2^r.exp and |y| = sig * 2^vy.exp with r.exp <= vy.exp (or rem = |x| < |y|).
  wide<4> two_rem = ycxx::detail::fpm::wide_resize<4>(r.rem);
  wide<4> ym = ycxx::detail::fpm::wide_resize<4>(vy.sig);
  int er = r.exp + 1, ey = vy.exp; // 2 * rem = rem * 2^(exp + 1)
  if (er > ey)
    two_rem = ycxx::detail::fpm::wide_shl(two_rem, er - ey);
  else if (ey - er < 200)
    ym = ycxx::detail::fpm::wide_shl(ym, ey - er);
  else
    ym = ycxx::detail::fpm::wide_shl(ycxx::detail::fpm::wide_from<4>(1), 255); // |y| >> 2 rem
  const int c = ycxx::detail::fpm::wide_cmp(two_rem, ym);
  unsigned q = r.quo;
  T rem = ycxx::detail::fpm::fp_round<T>(false, r.rem, r.exp).value; // exact, >= 0
  if (c > 0 || (c == 0 && (q & 1))) {
    rem = rem - ycxx::detail::fpm::fp_abs(y); // exact (Sterbenz-like: |y|/2 <= rem < |y|)
    ++q;
  }
  if (vx.neg) rem = -rem;
  const int sq = static_cast<int>(q & 7);
  *quo = vx.neg != vy.neg ? -sq : sq;
  return rem == 0 ? ycxx::detail::fpm::fp_zero<T>(vx.neg) : rem;
}
template <class T>
constexpr T fp_remainder(T x, T y) noexcept {
  int q = 0;
  return ycxx::detail::fpm::fp_remquo(x, y, &q);
}

// ---- integer roots ----------------------------------------------------------------------------
// floor(sqrt(n)); `exact` tells whether n is a perfect square.
template <int N>
constexpr wide<N> wide_isqrt(wide<N> n, bool& exact) noexcept {
  wide<N> res;
  int top = ycxx::detail::fpm::wide_bitlen(n);
  if (top == 0) {
    exact = true;
    return res;
  }
  int b = (top - 1) & ~1; // highest even bit position <= top bit
  for (; b >= 0; b -= 2) {
    wide<N> t = res;
    wide<N> bit;
    ycxx::detail::fpm::wide_set_bit(bit, b);
    ycxx::detail::fpm::wide_add(t, bit); // res + bit
    res = ycxx::detail::fpm::wide_shr(res, 1);
    if (ycxx::detail::fpm::wide_cmp(n, t) >= 0) {
      ycxx::detail::fpm::wide_sub(n, t);
      ycxx::detail::fpm::wide_add(res, bit);
    }
  }
  exact = ycxx::detail::fpm::wide_is_zero(n);
  return res;
}

// Rounds sqrt(S * 2^e + tail) for an exact integer S (tail nonzero iff `sticky`, below 2^e).
template <class T, int N>
constexpr fp_result<T> fp_sqrt_round(wide<N> s, int e, bool sticky) noexcept {
  using L = fp_layout<T>;
  // Make e even, then give S at least 2 (p + 2) bits (or drop extra low bits into sticky).
  if (e & 1) {
    s = ycxx::detail::fpm::wide_shl(s, 1);
    --e;
  }
  const int want = 2 * (L::p + 2);
  int len = ycxx::detail::fpm::wide_bitlen(s);
  if (len < want) {
    const int k = (want - len + 1) / 2;
    s = ycxx::detail::fpm::wide_shl(s, 2 * k);
    e -= 2 * k;
  } else if (len > want + 1) {
    const int k = (len - want) / 2;
    s = ycxx::detail::fpm::wide_shr(s, 2 * k, sticky);
    e += 2 * k;
  }
  bool exact = true;
  const wide<N> r = ycxx::detail::fpm::wide_isqrt(s, exact);
  return ycxx::detail::fpm::fp_round<T>(false, r, e / 2, sticky || !exact);
}

template <class T>
constexpr T fp_sqrt(T x) noexcept {
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  if (v.kind == fp_kind::nan) return ycxx::detail::fpm::fp_nan_operand(x);
  if (v.kind == fp_kind::zero) return x;
  if (v.neg) return ycxx::detail::fpm::fp_invalid<T>();
  if (v.kind == fp_kind::inf) return x;
  return ycxx::detail::fpm::fp_finish(ycxx::detail::fpm::fp_sqrt_round<T>(ycxx::detail::fpm::wide_resize<4>(v.sig), v.exp, false));
}

template <class T>
constexpr T fp_cbrt(T x) noexcept {
  using L = fp_layout<T>;
  const fp_value v = ycxx::detail::fpm::fp_decode(x);
  if (v.kind == fp_kind::nan) return ycxx::detail::fpm::fp_nan_operand(x);
  if (v.kind != fp_kind::finite) return x;
  // n = sig * 2^(3k) with at least 3 (p + 2) bits, exponent a multiple of 3.
  wide<6> n = ycxx::detail::fpm::wide_resize<6>(v.sig);
  int e = v.exp;
  int len = ycxx::detail::fpm::wide_bitlen(n);
  int sh = 3 * (L::p + 2) - len;
  if (sh < 0) sh = 0;
  sh += ((e - sh) % 3 + 3) % 3; // make e - sh a multiple of 3
  n = ycxx::detail::fpm::wide_shl(n, sh);
  e -= sh;
  len = ycxx::detail::fpm::wide_bitlen(n);
  // Digit by digit: r, r^2, r^3 kept exactly; compare r^3 with the top 3i bits of n.
  wide<6> r, r2, r3;
  const int digits = (len + 2) / 3;
  for (int i = digits - 1; i >= 0; --i) {
    // Candidate (2r + 1): (2r+1)^3 = 8 r^3 + 12 r^2 + 6 r + 1, (2r+1)^2 = 4 r^2 + 4 r + 1.
    wide<6> c3 = ycxx::detail::fpm::wide_shl(r3, 3);
    wide<6> t = r2;
    ycxx::detail::fpm::wide_mul_small(t, 12);
    ycxx::detail::fpm::wide_add(c3, t);
    t = r;
    ycxx::detail::fpm::wide_mul_small(t, 6);
    ycxx::detail::fpm::wide_add(c3, t);
    ycxx::detail::fpm::wide_add_small(c3, 1);
    const wide<6> top = ycxx::detail::fpm::wide_shr(n, 3 * i);
    wide<6> two_r = ycxx::detail::fpm::wide_shl(r, 1);
    if (ycxx::detail::fpm::wide_cmp(c3, top) <= 0) {
      wide<6> c2 = ycxx::detail::fpm::wide_shl(r2, 2);
      wide<6> fr = ycxx::detail::fpm::wide_shl(r, 2);
      ycxx::detail::fpm::wide_add(c2, fr);
      ycxx::detail::fpm::wide_add_small(c2, 1);
      r = two_r;
      ycxx::detail::fpm::wide_add_small(r, 1);
      r2 = c2;
      r3 = c3;
    } else {
      r = two_r;
      r2 = ycxx::detail::fpm::wide_shl(r2, 2);
      r3 = ycxx::detail::fpm::wide_shl(r3, 3);
    }
  }
  const bool exact = ycxx::detail::fpm::wide_cmp(r3, n) == 0;
  return ycxx::detail::fpm::fp_round<T>(v.neg, r, e / 3, !exact).value; // never over/underflows
}

// ---- hypot ---------------------------------------------------------------------------------------
// sqrt(sum of squares) of finite values, correctly rounded.
template <class T, int K>
constexpr T fp_hypot_finite(const T (&in)[K]) noexcept {
  using L = fp_layout<T>;
  fp_value v[K];
  int big = -1;
  long top_big = 0;
  for (int i = 0; i < K; ++i) {
    v[i] = ycxx::detail::fpm::fp_decode(in[i]);
    if (v[i].kind != fp_kind::finite) continue;
    const long top = long(v[i].exp) + ycxx::detail::fpm::wide_bitlen(v[i].sig);
    if (big < 0 || top > top_big) {
      big = i;
      top_big = top;
    }
  }
  if (big < 0) return T(0);
  // Terms more than p + 3 binades below the largest only make the result inexact.
  bool sticky = false;
  int lsb = v[big].exp;
  for (int i = 0; i < K; ++i) {
    if (v[i].kind != fp_kind::finite || i == big) continue;
    const long top = long(v[i].exp) + ycxx::detail::fpm::wide_bitlen(v[i].sig);
    if (top_big - top > L::p + 3) {
      v[i].kind = fp_kind::zero;
      sticky = true;
    } else if (v[i].exp < lsb) {
      lsb = v[i].exp;
    }
  }
  wide<8> sum;
  for (int i = 0; i < K; ++i) {
    if (v[i].kind != fp_kind::finite) continue;
    const wide<4> m = ycxx::detail::fpm::wide_shl(ycxx::detail::fpm::wide_resize<4>(v[i].sig), v[i].exp - lsb);
    ycxx::detail::fpm::wide_add(sum, ycxx::detail::fpm::wide_mul(m, m));
  }
  return ycxx::detail::fpm::fp_finish(ycxx::detail::fpm::fp_sqrt_round<T>(sum, 2 * lsb, sticky));
}

template <class T>
constexpr T fp_hypot(T x, T y) noexcept {
  // F.10.4.3: hypot(+-inf, y) is +inf even for a NaN y.
  if (__builtin_isinf(x) || __builtin_isinf(y)) {
    if (ycxx::detail::fpm::fp_issignaling(x) || ycxx::detail::fpm::fp_issignaling(y)) ycxx::detail::fpm::fp_report(fe_invalid);
    return ycxx::detail::fpm::fp_infinity<T>(false);
  }
  if (ycxx::detail::fpm::fp_isnan(x) || ycxx::detail::fpm::fp_isnan(y)) return ycxx::detail::fpm::fp_nan_operands(x, y);
  const T in[2] = {x, y};
  return ycxx::detail::fpm::fp_hypot_finite(in);
}
template <class T>
constexpr T fp_hypot3(T x, T y, T z) noexcept {
  if (__builtin_isinf(x) || __builtin_isinf(y) || __builtin_isinf(z)) {
    if (ycxx::detail::fpm::fp_issignaling(x) || ycxx::detail::fpm::fp_issignaling(y) || ycxx::detail::fpm::fp_issignaling(z))
      ycxx::detail::fpm::fp_report(fe_invalid);
    return ycxx::detail::fpm::fp_infinity<T>(false);
  }
  if (ycxx::detail::fpm::fp_isnan(x) || ycxx::detail::fpm::fp_isnan(y)) return ycxx::detail::fpm::fp_nan_operands(x, y);
  if (ycxx::detail::fpm::fp_isnan(z)) return ycxx::detail::fpm::fp_nan_operand(z);
  const T in[3] = {x, y, z};
  return ycxx::detail::fpm::fp_hypot_finite(in);
}

// ---- fma -----------------------------------------------------------------------------------------
template <class T>
constexpr T fp_fma(T x, T y, T z) noexcept {
  using L = fp_layout<T>;
  if (ycxx::detail::fpm::fp_isnan(x) || ycxx::detail::fpm::fp_isnan(y) || ycxx::detail::fpm::fp_isnan(z)) {
    if (ycxx::detail::fpm::fp_issignaling(x) || ycxx::detail::fpm::fp_issignaling(y) || ycxx::detail::fpm::fp_issignaling(z))
      ycxx::detail::fpm::fp_report(fe_invalid);
    // F.10.10.1: fma(inf, 0, NaN) may raise invalid; libycxx does not.
    return ycxx::detail::fpm::fp_isnan(x) ? ycxx::detail::fpm::fp_nan_operand(x)
         : ycxx::detail::fpm::fp_isnan(y) ? ycxx::detail::fpm::fp_nan_operand(y)
                                          : ycxx::detail::fpm::fp_nan_operand(z);
  }
  const fp_value vx = ycxx::detail::fpm::fp_decode(x), vy = ycxx::detail::fpm::fp_decode(y), vz = ycxx::detail::fpm::fp_decode(z);
  const bool pneg = vx.neg != vy.neg;
  if (vx.kind == fp_kind::inf || vy.kind == fp_kind::inf) {
    if (vx.kind == fp_kind::zero || vy.kind == fp_kind::zero) return ycxx::detail::fpm::fp_invalid<T>();
    if (vz.kind == fp_kind::inf && vz.neg != pneg) return ycxx::detail::fpm::fp_invalid<T>();
    return ycxx::detail::fpm::fp_infinity<T>(pneg);
  }
  if (vz.kind == fp_kind::inf) return z;
  if (vx.kind == fp_kind::zero || vy.kind == fp_kind::zero) {
    if (vz.kind == fp_kind::zero) return ycxx::detail::fpm::fp_zero<T>(pneg && vz.neg);
    return z;
  }
  // Exact product P = mx * my * 2^(ex + ey).
  wide<4> p = ycxx::detail::fpm::wide_mul(vx.sig, vy.sig);
  int ep = vx.exp + vy.exp;
  if (vz.kind == fp_kind::zero) return ycxx::detail::fpm::fp_finish(ycxx::detail::fpm::fp_round<T>(pneg, p, ep));
  wide<4> zm = ycxx::detail::fpm::wide_resize<4>(vz.sig);
  int ez = vz.exp;
  const long tp = long(ep) + ycxx::detail::fpm::wide_bitlen(p), tz = long(ez) + ycxx::detail::fpm::wide_bitlen(zm);
  // An operand far below the other only decides the direction of an inexact rounding: replace
  // it by a single bit well below the rounding position (same rounded result and flags).
  constexpr int far = 2 * L::p + 8;
  if (tz - tp > far) {
    p = ycxx::detail::fpm::wide_from<4>(1);
    ep = static_cast<int>(tz - far);
  } else if (tp - tz > far) {
    zm = ycxx::detail::fpm::wide_from<4>(1);
    ez = static_cast<int>(tp - far);
  }
  const int base = ep < ez ? ep : ez;
  wide<8> a = ycxx::detail::fpm::wide_shl(ycxx::detail::fpm::wide_resize<8>(p), ep - base);
  wide<8> b = ycxx::detail::fpm::wide_shl(ycxx::detail::fpm::wide_resize<8>(zm), ez - base);
  bool neg = pneg;
  if (pneg == vz.neg) {
    ycxx::detail::fpm::wide_add(a, b);
  } else if (ycxx::detail::fpm::wide_cmp(a, b) >= 0) {
    ycxx::detail::fpm::wide_sub(a, b);
  } else {
    ycxx::detail::fpm::wide_sub(b, a);
    a = b;
    neg = vz.neg;
  }
  if (ycxx::detail::fpm::wide_is_zero(a)) return T(0); // exact cancellation: +0
  return ycxx::detail::fpm::fp_finish(ycxx::detail::fpm::fp_round<T>(neg, a, base));
}

// ---- lerp (P0811) ---------------------------------------------------------------------------------
template <class T>
constexpr T fp_lerp(T a, T b, T t) noexcept {
  if ((a <= T(0) && b >= T(0)) || (a >= T(0) && b <= T(0))) {
    // Exact at t == 0 and t == 1; an infinite t would meet 0 * inf here.
    if (__builtin_isinf(t)) return a + t * (b - a);
    return t * b + (T(1) - t) * a;
  }
  if (t == T(1)) return b;
  const T x = a + t * (b - a);
  // Monotonic and exact at t == 1: clamp to b on the far side of it.
  if ((t > T(1)) == (b > a)) return b < x ? x : b;
  return x < b ? x : b;
}

}} // namespace ycxx::detail::fpm
