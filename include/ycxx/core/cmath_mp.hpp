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

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__fpm {

// ---- mpf<N> -------------------------------------------------------------------------------------
// value = (neg ? -1 : 1) * m * 2^exp, with m normalised (bit 64N - 1 set) or zero.
template <int _Np>
struct __mpf {
  bool __neg = false;
  int exp = 0;
  __wide<_Np> m;
};

template <int _Np>
constexpr bool __mp_is_zero(const __mpf<_Np>& __x) noexcept {
  return __ycxx::__detail::__fpm::__wide_is_zero(__x.m);
}
// 2^mp_ilog(x) <= |x| < 2^(mp_ilog(x) + 1)
template <int _Np>
constexpr int __mp_ilog(const __mpf<_Np>& __x) noexcept {
  return __x.exp + 64 * _Np - 1;
}

// Normalises +-mag * 2^exp into mpf<N>, rounding to nearest.
template <int _Np, int _Kp>
constexpr __mpf<_Np> __mp_make(bool __neg, const __wide<_Kp>& __mag, int exp) noexcept {
  __mpf<_Np> r;
  const int __len = __ycxx::__detail::__fpm::__wide_bitlen(__mag);
  if (__len == 0) return r;
  r.__neg = __neg;
  const int shift = __len - 64 * _Np;
  if (shift > 0) {
    const bool round = __ycxx::__detail::__fpm::__wide_bit(__mag, shift - 1);
    r.m = __ycxx::__detail::__fpm::__wide_resize<_Np>(__ycxx::__detail::__fpm::__wide_shr(__mag, shift));
    r.exp = exp + shift;
    if (round) {
      __ycxx::__detail::__fpm::__wide_add_small(r.m, 1);
      if (__ycxx::__detail::__fpm::__wide_is_zero(r.m)) { // carried out of the top
        __ycxx::__detail::__fpm::__wide_set_bit(r.m, 64 * _Np - 1);
        ++r.exp;
      }
    }
  } else {
    r.m = __ycxx::__detail::__fpm::__wide_shl(__ycxx::__detail::__fpm::__wide_resize<_Np>(__mag), -shift);
    r.exp = exp + shift;
  }
  return r;
}

template <int _Np>
constexpr __mpf<_Np> __mp_from_u64(__y_u64 __v, bool __neg = false) noexcept {
  return __ycxx::__detail::__fpm::__mp_make<_Np>(__neg, __ycxx::__detail::__fpm::__wide_from<1>(__v), 0);
}
template <int _Np>
constexpr __mpf<_Np> __mp_from_int(long long __v) noexcept {
  return __ycxx::__detail::__fpm::__mp_from_u64<_Np>(__v < 0 ? __y_u64(0) - __y_u64(__v) : __y_u64(__v), __v < 0);
}
template <int _Np>
constexpr __mpf<_Np> __mp_from_value(const __fp_value& __v) noexcept { // finite or zero
  return __ycxx::__detail::__fpm::__mp_make<_Np>(__v.__neg, __v.__sig, __v.exp);
}
template <int _Np, class _Tp>
constexpr __mpf<_Np> __mp_from(_Tp __x) noexcept {
  return __ycxx::__detail::__fpm::__mp_from_value<_Np>(__ycxx::__detail::__fpm::__fp_decode(__x));
}
template <int _Np>
constexpr __mpf<_Np> __mp_from_bits(bool __neg, int exp, const unsigned long long (&m)[3]) noexcept {
  __wide<3> __w;
  __w.__w[0] = m[2];
  __w.__w[1] = m[1];
  __w.__w[2] = m[0];
  return __ycxx::__detail::__fpm::__mp_make<_Np>(__neg, __w, exp + 1 - 192);
}
template <int _Np>
constexpr __mpf<_Np> __mp_const(__math_constant c) noexcept {
  const __math_constant_bits& b = __ycxx::__detail::__math_constant_table[static_cast<int>(c)];
  return __ycxx::__detail::__fpm::__mp_from_bits<_Np>(false, b.exp, b.m);
}
template <int _Np>
constexpr __mpf<_Np> __mp_const(const __mp_const_bits& b) noexcept {
  return __ycxx::__detail::__fpm::__mp_from_bits<_Np>(b.__neg, b.exp, b.m);
}
template <int _Np>
constexpr __mpf<_Np> __mp_ldexp(__mpf<_Np> __x, int k) noexcept {
  if (!__ycxx::__detail::__fpm::__mp_is_zero(__x)) __x.exp += k;
  return __x;
}
template <int _Np>
constexpr __mpf<_Np> __mp_neg(__mpf<_Np> __x) noexcept {
  __x.__neg = !__x.__neg;
  return __x;
}
template <int _Np>
constexpr __mpf<_Np> __mp_abs(__mpf<_Np> __x) noexcept {
  __x.__neg = false;
  return __x;
}

template <class _Tp, int _Np>
constexpr __fp_result<_Tp> __mp_round(const __mpf<_Np>& __x) noexcept {
  if (__ycxx::__detail::__fpm::__mp_is_zero(__x)) return {_Tp(0), 0};
  return __ycxx::__detail::__fpm::__fp_round<_Tp>(__x.__neg, __x.m, __x.exp, true);
}
template <class _Tp, int _Np>
constexpr _Tp __mp_to(const __mpf<_Np>& __x) noexcept {
  return __ycxx::__detail::__fpm::__fp_finish(__ycxx::__detail::__fpm::__mp_round<_Tp>(__x));
}

// Compares |a| with |b|.
template <int _Np>
constexpr int __mp_cmp_abs(const __mpf<_Np>& a, const __mpf<_Np>& b) noexcept {
  const bool __za = __ycxx::__detail::__fpm::__mp_is_zero(a), __zb = __ycxx::__detail::__fpm::__mp_is_zero(b);
  if (__za || __zb) return __za && __zb ? 0 : __za ? -1 : 1;
  if (a.exp != b.exp) return a.exp < b.exp ? -1 : 1;
  return __ycxx::__detail::__fpm::__wide_cmp(a.m, b.m);
}
template <int _Np>
constexpr int __mp_cmp(const __mpf<_Np>& a, const __mpf<_Np>& b) noexcept {
  const bool __za = __ycxx::__detail::__fpm::__mp_is_zero(a), __zb = __ycxx::__detail::__fpm::__mp_is_zero(b);
  const int __sa = __za ? 0 : a.__neg ? -1 : 1, __sb = __zb ? 0 : b.__neg ? -1 : 1;
  if (__sa != __sb) return __sa < __sb ? -1 : 1;
  if (__sa == 0) return 0;
  const int c = __ycxx::__detail::__fpm::__mp_cmp_abs(a, b);
  return __sa > 0 ? c : -c;
}

template <int _Np>
constexpr __mpf<_Np> __mp_add(const __mpf<_Np>& a, const __mpf<_Np>& b) noexcept {
  if (__ycxx::__detail::__fpm::__mp_is_zero(a)) return b;
  if (__ycxx::__detail::__fpm::__mp_is_zero(b)) return a;
  const bool __a_big = __ycxx::__detail::__fpm::__mp_cmp_abs(a, b) >= 0;
  const __mpf<_Np>& __x = __a_big ? a : b;
  const __mpf<_Np>& y = __a_big ? b : a;
  const int d = __x.exp - y.exp;
  if (d > 64 * _Np + 64) return __x;
  // One guard limb below each operand.
  __wide<_Np + 1> _Xp = __ycxx::__detail::__fpm::__wide_shl(__ycxx::__detail::__fpm::__wide_resize<_Np + 1>(__x.m), 64);
  bool __sticky = false;
  __wide<_Np + 1> _Yp = __ycxx::__detail::__fpm::__wide_shr(__ycxx::__detail::__fpm::__wide_shl(__ycxx::__detail::__fpm::__wide_resize<_Np + 1>(y.m), 64), d, __sticky);
  if (__x.__neg == y.__neg) {
    if (__ycxx::__detail::__fpm::__wide_add(_Xp, _Yp)) {
      _Xp = __ycxx::__detail::__fpm::__wide_shr(_Xp, 1);
      __ycxx::__detail::__fpm::__wide_set_bit(_Xp, 64 * (_Np + 1) - 1);
      return __ycxx::__detail::__fpm::__mp_make<_Np>(__x.__neg, _Xp, __x.exp - 64 + 1);
    }
  } else {
    __ycxx::__detail::__fpm::__wide_sub(_Xp, _Yp);
    if (__sticky) { // the true difference is a little smaller
      __wide<_Np + 1> __one = __ycxx::__detail::__fpm::__wide_from<_Np + 1>(1);
      __ycxx::__detail::__fpm::__wide_sub(_Xp, __one);
    }
  }
  return __ycxx::__detail::__fpm::__mp_make<_Np>(__x.__neg, _Xp, __x.exp - 64);
}
template <int _Np>
constexpr __mpf<_Np> __mp_sub(const __mpf<_Np>& a, const __mpf<_Np>& b) noexcept {
  return __ycxx::__detail::__fpm::__mp_add(a, __ycxx::__detail::__fpm::__mp_neg(b));
}
template <int _Np>
constexpr __mpf<_Np> __mp_mul(const __mpf<_Np>& a, const __mpf<_Np>& b) noexcept {
  if (__ycxx::__detail::__fpm::__mp_is_zero(a) || __ycxx::__detail::__fpm::__mp_is_zero(b)) return {};
  return __ycxx::__detail::__fpm::__mp_make<_Np>(a.__neg != b.__neg, __ycxx::__detail::__fpm::__wide_mul(a.m, b.m), a.exp + b.exp);
}
template <int _Np>
constexpr __mpf<_Np> __mp_mul_u64(const __mpf<_Np>& a, __y_u64 k) noexcept {
  __wide<_Np + 1> __x = __ycxx::__detail::__fpm::__wide_resize<_Np + 1>(a.m);
  __ycxx::__detail::__fpm::__wide_mul_small(__x, k);
  return __ycxx::__detail::__fpm::__mp_make<_Np>(a.__neg, __x, a.exp);
}
template <int _Np>
constexpr __mpf<_Np> __mp_div_u64(const __mpf<_Np>& a, __y_u64 k) noexcept {
  __wide<_Np + 1> __x = __ycxx::__detail::__fpm::__wide_shl(__ycxx::__detail::__fpm::__wide_resize<_Np + 1>(a.m), 64);
  __ycxx::__detail::__fpm::__wide_div_small(__x, k);
  return __ycxx::__detail::__fpm::__mp_make<_Np>(a.__neg, __x, a.exp - 64);
}
template <int _Np>
constexpr __mpf<_Np> __mp_one() noexcept {
  return __ycxx::__detail::__fpm::__mp_from_u64<_Np>(1);
}

// The leading bits of a normalised mantissa as a double in [0.5, 1) (exact).
template <int _Np>
constexpr double __mp_lead(const __mpf<_Np>& __x) noexcept {
  return static_cast<double>(__x.m.__w[_Np - 1] >> 11) * 0x1p-53;
}
template <int _Np>
constexpr __mpf<_Np> __mp_from_double(double d) noexcept {
  return __ycxx::__detail::__fpm::__mp_from<_Np>(d);
}

// 1 / b by Newton's iteration from a double approximation.
template <int _Np>
constexpr __mpf<_Np> __mp_recip(const __mpf<_Np>& b) noexcept {
  // b = f * 2^(b.exp + 64N), f in [0.5, 1).
  __mpf<_Np> __f = b;
  __f.__neg = false;
  __f.exp = -64 * _Np;
  __mpf<_Np> y = __ycxx::__detail::__fpm::__mp_from_double<_Np>(1.0 / __ycxx::__detail::__fpm::__mp_lead(__f));
  const __mpf<_Np> __one = __ycxx::__detail::__fpm::__mp_one<_Np>();
  for (int bits = 50; bits < 64 * _Np + 8; bits *= 2) {
    const __mpf<_Np> e = __ycxx::__detail::__fpm::__mp_sub(__one, __ycxx::__detail::__fpm::__mp_mul(__f, y));
    y = __ycxx::__detail::__fpm::__mp_add(y, __ycxx::__detail::__fpm::__mp_mul(y, e));
  }
  y = __ycxx::__detail::__fpm::__mp_ldexp(y, -(b.exp + 64 * _Np));
  y.__neg = b.__neg;
  return y;
}
template <int _Np>
constexpr __mpf<_Np> __mp_div(const __mpf<_Np>& a, const __mpf<_Np>& b) noexcept {
  return __ycxx::__detail::__fpm::__mp_mul(a, __ycxx::__detail::__fpm::__mp_recip(b));
}

// sqrt(a), a >= 0, through 1 / sqrt by Newton's iteration.
template <int _Np>
constexpr __mpf<_Np> __mp_sqrt(const __mpf<_Np>& a) noexcept {
  if (__ycxx::__detail::__fpm::__mp_is_zero(a)) return a;
  // a = f * 2^E with f in [0.25, 1) and E even.
  int _Ep = a.exp + 64 * _Np;
  __mpf<_Np> __f = a;
  __f.exp = -64 * _Np;
  double __fd = __ycxx::__detail::__fpm::__mp_lead(__f);
  if (_Ep & 1) {
    __f.exp -= 1;
    __fd *= 0.5;
    _Ep += 1;
  }
  double __yd = 1.5;
  for (int i = 0; i < 8; ++i) __yd = __yd * (3.0 - __fd * __yd * __yd) * 0.5;
  __mpf<_Np> y = __ycxx::__detail::__fpm::__mp_from_double<_Np>(__yd);
  const __mpf<_Np> __one = __ycxx::__detail::__fpm::__mp_one<_Np>();
  for (int bits = 48; bits < 64 * _Np + 8; bits *= 2) {
    const __mpf<_Np> e = __ycxx::__detail::__fpm::__mp_sub(__one, __ycxx::__detail::__fpm::__mp_mul(__f, __ycxx::__detail::__fpm::__mp_mul(y, y)));
    y = __ycxx::__detail::__fpm::__mp_add(y, __ycxx::__detail::__fpm::__mp_ldexp(__ycxx::__detail::__fpm::__mp_mul(y, e), -1));
  }
  return __ycxx::__detail::__fpm::__mp_ldexp(__ycxx::__detail::__fpm::__mp_mul(__f, y), _Ep / 2);
}

// The nearest integer to x (|x| < 2^62) and x minus it.
template <int _Np>
constexpr long long __mp_round_int(const __mpf<_Np>& __x, __mpf<_Np>* __rest = nullptr) noexcept {
  if (__ycxx::__detail::__fpm::__mp_is_zero(__x) || __ycxx::__detail::__fpm::__mp_ilog(__x) < -1) {
    if (__rest) *__rest = __x;
    return 0;
  }
  // x = m * 2^exp with exp < 0 here; k = round(m / 2^-exp).
  const int s = -__x.exp;
  bool __sticky = false;
  __wide<_Np> __ip = __ycxx::__detail::__fpm::__wide_shr(__x.m, s - 1, __sticky);
  const bool __half = __ip.__w[0] & 1;
  __ip = __ycxx::__detail::__fpm::__wide_shr(__ip, 1);
  __y_u64 k = __ip.__w[0] + (__half ? 1 : 0);
  const long long __sk = __x.__neg ? -static_cast<long long>(k) : static_cast<long long>(k);
  if (__rest) *__rest = __ycxx::__detail::__fpm::__mp_sub(__x, __ycxx::__detail::__fpm::__mp_from_int<_Np>(__sk));
  return __sk;
}

// Stops a series once |term| < 2^-(64N + 8) relative to an O(1) sum.
template <int _Np>
constexpr bool __mp_negligible(const __mpf<_Np>& __term, int scale = 0) noexcept {
  return __ycxx::__detail::__fpm::__mp_is_zero(__term) || __ycxx::__detail::__fpm::__mp_ilog(__term) < scale - 64 * _Np - 8;
}

// ---- exp, log -------------------------------------------------------------------------------------
// e^r - 1 by its Taylor series (|r| small).
template <int _Np>
constexpr __mpf<_Np> __mp_expm1_series(const __mpf<_Np>& r) noexcept {
  __mpf<_Np> sum = r, __term = r;
  const int scale = __ycxx::__detail::__fpm::__mp_ilog(r);
  for (__y_u64 k = 2;; ++k) {
    __term = __ycxx::__detail::__fpm::__mp_div_u64(__ycxx::__detail::__fpm::__mp_mul(__term, r), k);
    if (__ycxx::__detail::__fpm::__mp_negligible(__term, scale)) break;
    sum = __ycxx::__detail::__fpm::__mp_add(sum, __term);
  }
  return sum;
}
// e^x for |x| < 2^20.
template <int _Np>
constexpr __mpf<_Np> __mp_exp(const __mpf<_Np>& __x) noexcept {
  const __mpf<_Np> ln2 = __ycxx::__detail::__fpm::__mp_const<_Np>(__math_constant::ln2);
  __mpf<_Np> r;
  const long long k =
      __ycxx::__detail::__fpm::__mp_round_int(__ycxx::__detail::__fpm::__mp_mul(__x, __ycxx::__detail::__fpm::__mp_const<_Np>(__math_constant::log2e)));
  r = __ycxx::__detail::__fpm::__mp_sub(__x, __ycxx::__detail::__fpm::__mp_mul(ln2, __ycxx::__detail::__fpm::__mp_from_int<_Np>(k)));
  __mpf<_Np> e = __ycxx::__detail::__fpm::__mp_one<_Np>();
  if (!__ycxx::__detail::__fpm::__mp_is_zero(r)) e = __ycxx::__detail::__fpm::__mp_add(e, __ycxx::__detail::__fpm::__mp_expm1_series(r));
  return __ycxx::__detail::__fpm::__mp_ldexp(e, static_cast<int>(k));
}
template <int _Np>
constexpr __mpf<_Np> __mp_expm1(const __mpf<_Np>& __x) noexcept {
  if (__ycxx::__detail::__fpm::__mp_is_zero(__x) || __ycxx::__detail::__fpm::__mp_ilog(__x) < -2) return __ycxx::__detail::__fpm::__mp_expm1_series(__x);
  return __ycxx::__detail::__fpm::__mp_sub(__ycxx::__detail::__fpm::__mp_exp(__x), __ycxx::__detail::__fpm::__mp_one<_Np>());
}

// 2 atanh(z) = 2 (z + z^3/3 + z^5/5 + ...), |z| <= 0.18.
template <int _Np>
constexpr __mpf<_Np> __mp_two_atanh_series(const __mpf<_Np>& __z) noexcept {
  const __mpf<_Np> __z2 = __ycxx::__detail::__fpm::__mp_mul(__z, __z);
  __mpf<_Np> pow = __z, sum = __z;
  const int scale = __ycxx::__detail::__fpm::__mp_ilog(__z);
  for (__y_u64 k = 3;; k += 2) {
    pow = __ycxx::__detail::__fpm::__mp_mul(pow, __z2);
    const __mpf<_Np> __term = __ycxx::__detail::__fpm::__mp_div_u64(pow, k);
    if (__ycxx::__detail::__fpm::__mp_negligible(__term, scale)) break;
    sum = __ycxx::__detail::__fpm::__mp_add(sum, __term);
  }
  return __ycxx::__detail::__fpm::__mp_ldexp(sum, 1);
}
// ln x for x > 0.
template <int _Np>
constexpr __mpf<_Np> __mp_log(const __mpf<_Np>& __x) noexcept {
  // x = f * 2^e with f in [1/sqrt 2, sqrt 2).
  int e = __x.exp + 64 * _Np;
  __mpf<_Np> __f = __x;
  __f.exp = -64 * _Np; // [0.5, 1)
  if (__ycxx::__detail::__fpm::__mp_lead(__f) < 0.70710678118654752) {
    __f.exp += 1;
    e -= 1;
  }
  const __mpf<_Np> __one = __ycxx::__detail::__fpm::__mp_one<_Np>();
  const __mpf<_Np> __z = __ycxx::__detail::__fpm::__mp_div(__ycxx::__detail::__fpm::__mp_sub(__f, __one), __ycxx::__detail::__fpm::__mp_add(__f, __one));
  __mpf<_Np> r = __ycxx::__detail::__fpm::__mp_is_zero(__z) ? __z : __ycxx::__detail::__fpm::__mp_two_atanh_series(__z);
  if (e != 0)
    r = __ycxx::__detail::__fpm::__mp_add(
        r, __ycxx::__detail::__fpm::__mp_mul(__ycxx::__detail::__fpm::__mp_const<_Np>(__math_constant::ln2), __ycxx::__detail::__fpm::__mp_from_int<_Np>(e)));
  return r;
}
// ln(1 + x) for x > -1.
template <int _Np>
constexpr __mpf<_Np> __mp_log1p(const __mpf<_Np>& __x) noexcept {
  const __mpf<_Np> __one = __ycxx::__detail::__fpm::__mp_one<_Np>();
  if (__ycxx::__detail::__fpm::__mp_is_zero(__x)) return __x;
  if (__ycxx::__detail::__fpm::__mp_ilog(__x) < -2) { // |x| < 1/4: 2 atanh(x / (2 + x))
    const __mpf<_Np> __z = __ycxx::__detail::__fpm::__mp_div(__x, __ycxx::__detail::__fpm::__mp_add(__ycxx::__detail::__fpm::__mp_ldexp(__one, 1), __x));
    return __ycxx::__detail::__fpm::__mp_two_atanh_series(__z);
  }
  return __ycxx::__detail::__fpm::__mp_log(__ycxx::__detail::__fpm::__mp_add(__one, __x));
}

// ---- trigonometric ----------------------------------------------------------------------------------
// sin and cos of |r| <= pi/4 (Taylor).
template <int _Np>
constexpr void __mp_sin_cos_small(const __mpf<_Np>& r, __mpf<_Np>& s, __mpf<_Np>& c) noexcept {
  const __mpf<_Np> __r2 = __ycxx::__detail::__fpm::__mp_mul(r, r);
  s = r;
  c = __ycxx::__detail::__fpm::__mp_one<_Np>();
  __mpf<_Np> __ts = r, __tc = __ycxx::__detail::__fpm::__mp_one<_Np>();
  const int scale = __ycxx::__detail::__fpm::__mp_is_zero(r) ? 0 : __ycxx::__detail::__fpm::__mp_ilog(r);
  bool __done_s = __ycxx::__detail::__fpm::__mp_is_zero(r), __done_c = __ycxx::__detail::__fpm::__mp_is_zero(r);
  for (__y_u64 k = 1; !(__done_s && __done_c); ++k) {
    // ts: r^(2k+1) / (2k+1)!, tc: r^(2k) / (2k)!, alternating.
    __tc = __ycxx::__detail::__fpm::__mp_neg(__ycxx::__detail::__fpm::__mp_div_u64(__ycxx::__detail::__fpm::__mp_mul(__tc, __r2), (2 * k - 1) * (2 * k)));
    __ts = __ycxx::__detail::__fpm::__mp_neg(__ycxx::__detail::__fpm::__mp_div_u64(__ycxx::__detail::__fpm::__mp_mul(__ts, __r2), (2 * k) * (2 * k + 1)));
    if (!__done_c) {
      if (__ycxx::__detail::__fpm::__mp_negligible(__tc, -1))
        __done_c = true;
      else
        c = __ycxx::__detail::__fpm::__mp_add(c, __tc);
    }
    if (!__done_s) {
      if (__ycxx::__detail::__fpm::__mp_negligible(__ts, scale))
        __done_s = true;
      else
        s = __ycxx::__detail::__fpm::__mp_add(s, __ts);
    }
  }
}

// x - q pi/2 with |result| <= pi/4 (Payne-Hanek: the bits of 2/pi that matter for x).
template <int _Np>
constexpr __mpf<_Np> __mp_reduce_pio2(const __fp_value& __v, int& __quadrant) noexcept {
  __quadrant = 0;
  const __mpf<_Np> __x = __ycxx::__detail::__fpm::__mp_from_value<_Np>(__v);
  if (__ycxx::__detail::__fpm::__mp_ilog(__x) < -1) return __x; // |x| < 1/2 < pi/4
  constexpr int _Fp = 64 * _Np + 128;                  // fraction bits kept
  constexpr int _Wp = 2 + 113 + _Fp + 64;              // window bits (upper bound)
  constexpr int _WL = (_Wp + 63) / 64;
  const int _Ep = __v.exp, __lenM = __ycxx::__detail::__fpm::__wide_bitlen(__v.__sig);
  // x * 2/pi = M * sum b_i 2^(E - i); terms with E - i >= 2 are multiples of 4.
  const int start = _Ep - 1 > 1 ? _Ep - 1 : 1;
  const int end = __lenM + _Ep + _Fp;
  // B = bits start..end of 2/pi as an integer (bit `end` is its least significant bit).
  __wide<_WL> _Bp;
  for (int i = start; i <= end;) {
    // Bits i .. i + k - 1 of the table (k <= 64, within one word) go to positions end - i down.
    const int __word = (i - 1) / 64, __off = (i - 1) % 64;
    int k = 64 - __off;
    if (k > end - i + 1) k = end - i + 1;
    const __y_u64 chunk = (__ycxx::__detail::__fpm::__two_over_pi_bits[__word] << __off) >> (64 - k); // k bits, msb first
    const int __pos = end - (i + k - 1);                                                // lsb position in B
    _Bp.__w[__pos / 64] |= chunk << (__pos % 64);
    if (__pos % 64 != 0 && __pos % 64 + k > 64) _Bp.__w[__pos / 64 + 1] |= chunk >> (64 - __pos % 64);
    i += k;
  }
  const __wide<_WL + 2> _Pp = __ycxx::__detail::__fpm::__wide_mul(__v.__sig, _Bp); // x * 2/pi = P * 2^(E - end)
  const int __point = end - _Ep;                                    // binary point position in P
  int __q = (__ycxx::__detail::__fpm::__wide_bit(_Pp, __point + 1) ? 2 : 0) + (__ycxx::__detail::__fpm::__wide_bit(_Pp, __point) ? 1 : 0);
  __wide<_WL + 2> __frac = __ycxx::__detail::__fpm::__wide_low_bits(_Pp, __point);
  bool __neg = false;
  if (__ycxx::__detail::__fpm::__wide_bit(__frac, __point - 1)) { // frac >= 1/2: take frac - 1
    __wide<_WL + 2> __one;
    __ycxx::__detail::__fpm::__wide_set_bit(__one, __point);
    __ycxx::__detail::__fpm::__wide_sub(__one, __frac);
    __frac = __one;
    __neg = true;
    __q = (__q + 1) & 3;
  }
  __mpf<_Np> __f = __ycxx::__detail::__fpm::__mp_make<_Np>(__neg, __frac, -__point);
  if (__v.__neg) {
    __f.__neg = !__f.__neg;
    __q = (4 - __q) & 3;
  }
  __quadrant = __q;
  return __ycxx::__detail::__fpm::__mp_mul(__f, __ycxx::__detail::__fpm::__mp_ldexp(__ycxx::__detail::__fpm::__mp_const<_Np>(__math_constant::pi), -1));
}

// sin(x) (which == 0), cos(x) (1) or tan(x) (2) of a finite nonzero x.
template <int _Np>
constexpr __mpf<_Np> __mp_trig(const __fp_value& __v, int __which) noexcept {
  int __q = 0;
  const __mpf<_Np> r = __ycxx::__detail::__fpm::__mp_reduce_pio2<_Np>(__v, __q);
  __mpf<_Np> s, c;
  __ycxx::__detail::__fpm::__mp_sin_cos_small(r, s, c);
  // sin(r + q pi/2), cos(r + q pi/2)
  __mpf<_Np> __sn = s, __cs = c;
  switch (__q) {
  case 1:
    __sn = c;
    __cs = __ycxx::__detail::__fpm::__mp_neg(s);
    break;
  case 2:
    __sn = __ycxx::__detail::__fpm::__mp_neg(s);
    __cs = __ycxx::__detail::__fpm::__mp_neg(c);
    break;
  case 3:
    __sn = __ycxx::__detail::__fpm::__mp_neg(c);
    __cs = s;
    break;
  default:
    break;
  }
  if (__which == 0) return __sn;
  if (__which == 1) return __cs;
  return __ycxx::__detail::__fpm::__mp_div(__sn, __cs);
}

// sin(pi x) for an exactly represented finite x (lgamma/tgamma reflection).
template <int _Np>
constexpr __mpf<_Np> __mp_sinpi(const __fp_value& __v) noexcept {
  // 2|x| = k + 2f with k an integer and |f| <= 1/4: sin(pi |x|) = sin(k pi/2 + pi f).
  const int e = __v.exp + 1; // 2|x| = sig * 2^e
  unsigned k = 0;
  __mpf<_Np> __f;
  if (e >= 0) {
    if (e < 2) k = static_cast<unsigned>(__ycxx::__detail::__fpm::__wide_shl(__v.__sig, e).__w[0] & 3);
  } else {
    const int s = -e;
    bool __rest = false;
    __wide<2> __ip = __ycxx::__detail::__fpm::__wide_shr(__v.__sig, s, __rest);
    __wide<2> __low = __ycxx::__detail::__fpm::__wide_low_bits(__v.__sig, s);
    bool __neg = false;
    if (__ycxx::__detail::__fpm::__wide_bit(__low, s - 1)) { // fraction >= 1/2: round k up
      __ycxx::__detail::__fpm::__wide_add_small(__ip, 1);
      __wide<2> __one;
      __ycxx::__detail::__fpm::__wide_set_bit(__one, s);
      __ycxx::__detail::__fpm::__wide_sub(__one, __low);
      __low = __one;
      __neg = true;
    }
    k = static_cast<unsigned>(__ip.__w[0] & 3);
    __f = __ycxx::__detail::__fpm::__mp_make<_Np>(__neg, __low, e - 1); // (2|x| - k) / 2
  }
  __mpf<_Np> s, c;
  __ycxx::__detail::__fpm::__mp_sin_cos_small(__ycxx::__detail::__fpm::__mp_mul(__f, __ycxx::__detail::__fpm::__mp_const<_Np>(__math_constant::pi)), s, c);
  __mpf<_Np> r = (k & 1) ? c : s;
  if (k & 2) r = __ycxx::__detail::__fpm::__mp_neg(r);
  if (__v.__neg) r = __ycxx::__detail::__fpm::__mp_neg(r);
  return r;
}

// atan(x), any finite x.
template <int _Np>
constexpr __mpf<_Np> __mp_atan(const __mpf<_Np>& __x) noexcept {
  if (__ycxx::__detail::__fpm::__mp_is_zero(__x)) return __x;
  const __mpf<_Np> __one = __ycxx::__detail::__fpm::__mp_one<_Np>();
  __mpf<_Np> __z = __ycxx::__detail::__fpm::__mp_abs(__x);
  const bool __invert = __ycxx::__detail::__fpm::__mp_cmp_abs(__z, __one) > 0;
  if (__invert) __z = __ycxx::__detail::__fpm::__mp_recip(__z);
  // atan(z) = 2 atan(z / (1 + sqrt(1 + z^2))): three halvings bring z below tan(pi/32).
  int __halvings = 0;
  for (; __halvings < 3 && __ycxx::__detail::__fpm::__mp_ilog(__z) >= -4; ++__halvings)
    __z = __ycxx::__detail::__fpm::__mp_div(
        __z, __ycxx::__detail::__fpm::__mp_add(__one, __ycxx::__detail::__fpm::__mp_sqrt(__ycxx::__detail::__fpm::__mp_add(__one, __ycxx::__detail::__fpm::__mp_mul(__z, __z)))));
  const __mpf<_Np> __z2 = __ycxx::__detail::__fpm::__mp_mul(__z, __z);
  __mpf<_Np> pow = __z, sum = __z;
  const int scale = __ycxx::__detail::__fpm::__mp_ilog(__z);
  for (__y_u64 k = 3;; k += 2) {
    pow = __ycxx::__detail::__fpm::__mp_neg(__ycxx::__detail::__fpm::__mp_mul(pow, __z2));
    const __mpf<_Np> __term = __ycxx::__detail::__fpm::__mp_div_u64(pow, k);
    if (__ycxx::__detail::__fpm::__mp_negligible(__term, scale)) break;
    sum = __ycxx::__detail::__fpm::__mp_add(sum, __term);
  }
  sum = __ycxx::__detail::__fpm::__mp_ldexp(sum, __halvings);
  if (__invert) sum = __ycxx::__detail::__fpm::__mp_sub(__ycxx::__detail::__fpm::__mp_ldexp(__ycxx::__detail::__fpm::__mp_const<_Np>(__math_constant::pi), -1), sum);
  sum.__neg = __x.__neg;
  return sum;
}
// atan2(y, x) for finite y, x, not both zero.
template <int _Np>
constexpr __mpf<_Np> __mp_atan2(const __mpf<_Np>& y, const __mpf<_Np>& __x) noexcept {
  const __mpf<_Np> pi = __ycxx::__detail::__fpm::__mp_const<_Np>(__math_constant::pi);
  __mpf<_Np> r;
  if (__ycxx::__detail::__fpm::__mp_is_zero(__x)) {
    r = __ycxx::__detail::__fpm::__mp_ldexp(pi, -1);
  } else if (__ycxx::__detail::__fpm::__mp_is_zero(y)) {
    r = __x.__neg ? pi : __mpf<_Np>{};
  } else {
    r = __ycxx::__detail::__fpm::__mp_atan(__ycxx::__detail::__fpm::__mp_div(__ycxx::__detail::__fpm::__mp_abs(y), __ycxx::__detail::__fpm::__mp_abs(__x)));
    if (__x.__neg) r = __ycxx::__detail::__fpm::__mp_sub(pi, r);
  }
  r.__neg = y.__neg && !__ycxx::__detail::__fpm::__mp_is_zero(r);
  return r;
}

// ---- erf, erfc --------------------------------------------------------------------------------------
// erf(x) for 0 <= x < 3.
template <int _Np>
constexpr __mpf<_Np> __mp_erf_small(const __mpf<_Np>& __x) noexcept {
  const __mpf<_Np> __two_over_sqrtpi = __ycxx::__detail::__fpm::__mp_ldexp(__ycxx::__detail::__fpm::__mp_const<_Np>(__math_constant::inv_sqrtpi), 1);
  const __mpf<_Np> __x2 = __ycxx::__detail::__fpm::__mp_mul(__x, __x);
  if (__ycxx::__detail::__fpm::__mp_ilog(__x) < -1) {
    // Maclaurin: sum (-1)^n x^(2n+1) / (n! (2n+1)).
    __mpf<_Np> pow = __x, sum = __x;
    const int scale = __ycxx::__detail::__fpm::__mp_ilog(__x);
    for (__y_u64 n = 1;; ++n) {
      pow = __ycxx::__detail::__fpm::__mp_neg(__ycxx::__detail::__fpm::__mp_div_u64(__ycxx::__detail::__fpm::__mp_mul(pow, __x2), n));
      const __mpf<_Np> __term = __ycxx::__detail::__fpm::__mp_div_u64(pow, 2 * n + 1);
      if (__ycxx::__detail::__fpm::__mp_negligible(__term, scale)) break;
      sum = __ycxx::__detail::__fpm::__mp_add(sum, __term);
    }
    return __ycxx::__detail::__fpm::__mp_mul(sum, __two_over_sqrtpi);
  }
  // erf(x) = 2/sqrt(pi) e^(-x^2) sum 2^n x^(2n+1) / (1 3 5 ... (2n+1)): positive terms.
  const __mpf<_Np> __two_x2 = __ycxx::__detail::__fpm::__mp_ldexp(__x2, 1);
  __mpf<_Np> __term = __x, sum = __x;
  for (__y_u64 n = 1;; ++n) {
    __term = __ycxx::__detail::__fpm::__mp_div_u64(__ycxx::__detail::__fpm::__mp_mul(__term, __two_x2), 2 * n + 1);
    if (__ycxx::__detail::__fpm::__mp_negligible(__term, __ycxx::__detail::__fpm::__mp_ilog(sum))) break;
    sum = __ycxx::__detail::__fpm::__mp_add(sum, __term);
  }
  return __ycxx::__detail::__fpm::__mp_mul(__ycxx::__detail::__fpm::__mp_mul(sum, __two_over_sqrtpi), __ycxx::__detail::__fpm::__mp_exp(__ycxx::__detail::__fpm::__mp_neg(__x2)));
}
// erfc(x) for x >= 3 (continued fraction).
template <int _Np>
constexpr __mpf<_Np> __mp_erfc_large(const __mpf<_Np>& __x, double __xd) noexcept {
  const int depth = static_cast<int>((_Np <= 2 ? 2000.0 : 3600.0) / (__xd * __xd)) + (_Np <= 2 ? 20 : 30);
  __mpf<_Np> t = __x;
  for (int k = depth; k >= 1; --k)
    t = __ycxx::__detail::__fpm::__mp_add(__x, __ycxx::__detail::__fpm::__mp_div(__ycxx::__detail::__fpm::__mp_ldexp(__ycxx::__detail::__fpm::__mp_from_u64<_Np>(__y_u64(k)), -1), t));
  const __mpf<_Np> e = __ycxx::__detail::__fpm::__mp_exp(__ycxx::__detail::__fpm::__mp_neg(__ycxx::__detail::__fpm::__mp_mul(__x, __x)));
  return __ycxx::__detail::__fpm::__mp_div(__ycxx::__detail::__fpm::__mp_mul(e, __ycxx::__detail::__fpm::__mp_const<_Np>(__math_constant::inv_sqrtpi)), t);
}

// ---- gamma ---------------------------------------------------------------------------------------------
template <int _Np>
inline constexpr int __stirling_min_x = _Np <= 2 ? 20 : 36;
template <int _Np>
inline constexpr int __stirling_terms = _Np <= 2 ? 25 : 32;

// ln Gamma(x) for x >= stirling_min_x.
template <int _Np>
constexpr __mpf<_Np> __mp_lgamma_stirling(const __mpf<_Np>& __x) noexcept {
  const __mpf<_Np> __half = __ycxx::__detail::__fpm::__mp_ldexp(__ycxx::__detail::__fpm::__mp_one<_Np>(), -1);
  __mpf<_Np> r = __ycxx::__detail::__fpm::__mp_sub(__ycxx::__detail::__fpm::__mp_mul(__ycxx::__detail::__fpm::__mp_sub(__x, __half), __ycxx::__detail::__fpm::__mp_log(__x)), __x);
  r = __ycxx::__detail::__fpm::__mp_add(r, __ycxx::__detail::__fpm::__mp_const<_Np>(__ycxx::__detail::__fpm::__half_ln_2pi));
  const __mpf<_Np> __inv = __ycxx::__detail::__fpm::__mp_recip(__x), __inv2 = __ycxx::__detail::__fpm::__mp_mul(__inv, __inv);
  __mpf<_Np> pow = __inv;
  for (int k = 0; k < __stirling_terms<_Np>; ++k) {
    const __mpf<_Np> __term = __ycxx::__detail::__fpm::__mp_mul(__ycxx::__detail::__fpm::__mp_const<_Np>(__ycxx::__detail::__fpm::__stirling_coefficients[k]), pow);
    if (__ycxx::__detail::__fpm::__mp_negligible(__term, 0)) break;
    r = __ycxx::__detail::__fpm::__mp_add(r, __term);
    pow = __ycxx::__detail::__fpm::__mp_mul(pow, __inv2);
  }
  return r;
}
// ln Gamma(x) for x > 0, and (for x below the Stirling range) the shift product.
template <int _Np>
constexpr __mpf<_Np> __mp_lgamma_pos(const __mpf<_Np>& __x) noexcept {
  const __mpf<_Np> __lim = __ycxx::__detail::__fpm::__mp_from_u64<_Np>(__stirling_min_x<_Np>);
  if (__ycxx::__detail::__fpm::__mp_cmp(__x, __lim) >= 0) return __ycxx::__detail::__fpm::__mp_lgamma_stirling(__x);
  // Gamma(x) = Gamma(x + n) / (x (x + 1) ... (x + n - 1))
  __mpf<_Np> __prod = __x, y = __x;
  const __mpf<_Np> __one = __ycxx::__detail::__fpm::__mp_one<_Np>();
  for (;;) {
    y = __ycxx::__detail::__fpm::__mp_add(y, __one);
    if (__ycxx::__detail::__fpm::__mp_cmp(y, __lim) >= 0) break;
    __prod = __ycxx::__detail::__fpm::__mp_mul(__prod, y);
  }
  return __ycxx::__detail::__fpm::__mp_sub(__ycxx::__detail::__fpm::__mp_lgamma_stirling(y), __ycxx::__detail::__fpm::__mp_log(__prod));
}

// ---- the functions on T ----------------------------------------------------------------------------
template <class _Tp>
inline constexpr int __mp_limbs = __fp_layout<_Tp>::p <= 64 ? 2 : 3;

template <class _Tp>
constexpr bool __fp_is_int(const __fp_value& __v) noexcept {
  return __v.kind == __fp_kind::zero || (__v.kind == __fp_kind::__finite && (__v.exp >= 0 || __ycxx::__detail::__fpm::__wide_ctz(__v.__sig) >= -__v.exp));
}
template <class _Tp>
constexpr bool __fp_is_odd_int(const __fp_value& __v) noexcept {
  if (__v.kind != __fp_kind::__finite || __v.exp > 0) return false;
  return __ycxx::__detail::__fpm::__fp_is_int<_Tp>(__v) && __ycxx::__detail::__fpm::__wide_bit(__v.__sig, -__v.exp);
}

// Overflow / underflow screens for results e^w: w > (emax + 1) ln 2 overflows; w below
// (qmin - 2) ln 2 rounds to zero.
template <class _Tp, int _Np>
constexpr int __mp_exp_range(const __mpf<_Np>& __w) noexcept {
  using _Lp = __fp_layout<_Tp>;
  const __mpf<_Np> ln2 = __ycxx::__detail::__fpm::__mp_const<_Np>(__math_constant::ln2);
  if (__ycxx::__detail::__fpm::__mp_cmp(__w, __ycxx::__detail::__fpm::__mp_mul_u64(ln2, __y_u64(_Lp::__emax + 1))) > 0) return 1;
  if (__ycxx::__detail::__fpm::__mp_cmp(__w, __ycxx::__detail::__fpm::__mp_neg(__ycxx::__detail::__fpm::__mp_mul_u64(ln2, __y_u64(2 - _Lp::__qmin)))) < 0) return -1;
  return 0;
}
template <class _Tp>
constexpr _Tp __fp_overflow(bool __neg) noexcept {
  __ycxx::__detail::__fpm::__fp_report(__fe_overflow | __fe_inexact);
  return __ycxx::__detail::__fpm::__fp_infinity<_Tp>(__neg);
}
template <class _Tp>
constexpr _Tp __fp_underflow(bool __neg) noexcept {
  __ycxx::__detail::__fpm::__fp_report(__fe_underflow | __fe_inexact);
  return __ycxx::__detail::__fpm::__fp_zero<_Tp>(__neg);
}
template <class _Tp>
constexpr _Tp __fp_pole(bool __neg) noexcept {
  __ycxx::__detail::__fpm::__fp_report(__fe_divbyzero);
  return __ycxx::__detail::__fpm::__fp_infinity<_Tp>(__neg);
}

// e^w with range screening.
template <class _Tp, int _Np>
constexpr _Tp __mp_exp_to(const __mpf<_Np>& __w, bool __neg = false) noexcept {
  const int range = __ycxx::__detail::__fpm::__mp_exp_range<_Tp>(__w);
  if (range > 0) return __ycxx::__detail::__fpm::__fp_overflow<_Tp>(__neg);
  if (range < 0) return __ycxx::__detail::__fpm::__fp_underflow<_Tp>(__neg);
  __mpf<_Np> r = __ycxx::__detail::__fpm::__mp_exp(__w);
  r.__neg = __neg;
  return __ycxx::__detail::__fpm::__mp_to<_Tp>(r);
}

template <class _Tp>
constexpr _Tp __fp_exp(_Tp __x) noexcept {
  constexpr int _Np = __mp_limbs<_Tp>;
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  switch (__v.kind) {
  case __fp_kind::nan:
    return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  case __fp_kind::__inf:
    return __v.__neg ? _Tp(0) : __x;
  case __fp_kind::zero:
    return _Tp(1);
  default:
    if (__v.exp + __ycxx::__detail::__fpm::__wide_bitlen(__v.__sig) > 20) return __v.__neg ? __ycxx::__detail::__fpm::__fp_underflow<_Tp>(false) : __ycxx::__detail::__fpm::__fp_overflow<_Tp>(false);
    return __ycxx::__detail::__fpm::__mp_exp_to<_Tp>(__ycxx::__detail::__fpm::__mp_from_value<_Np>(__v));
  }
}
template <class _Tp>
constexpr _Tp __fp_exp2(_Tp __x) noexcept {
  constexpr int _Np = __mp_limbs<_Tp>;
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  switch (__v.kind) {
  case __fp_kind::nan:
    return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  case __fp_kind::__inf:
    return __v.__neg ? _Tp(0) : __x;
  case __fp_kind::zero:
    return _Tp(1);
  default:
    if (__v.exp + __ycxx::__detail::__fpm::__wide_bitlen(__v.__sig) > 20) return __v.__neg ? __ycxx::__detail::__fpm::__fp_underflow<_Tp>(false) : __ycxx::__detail::__fpm::__fp_overflow<_Tp>(false);
    if (__ycxx::__detail::__fpm::__fp_is_int<_Tp>(__v)) // exact
      return __ycxx::__detail::__fpm::__fp_scale(_Tp(1), static_cast<long>(__ycxx::__detail::__fpm::__mp_round_int(__ycxx::__detail::__fpm::__mp_from_value<_Np>(__v))));
    return __ycxx::__detail::__fpm::__mp_exp_to<_Tp>(
        __ycxx::__detail::__fpm::__mp_mul(__ycxx::__detail::__fpm::__mp_from_value<_Np>(__v), __ycxx::__detail::__fpm::__mp_const<_Np>(__math_constant::ln2)));
  }
}
template <class _Tp>
constexpr _Tp __fp_expm1(_Tp __x) noexcept {
  constexpr int _Np = __mp_limbs<_Tp>;
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  switch (__v.kind) {
  case __fp_kind::nan:
    return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  case __fp_kind::__inf:
    return __v.__neg ? _Tp(-1) : __x;
  case __fp_kind::zero:
    return __x;
  default:
    if (__v.exp + __ycxx::__detail::__fpm::__wide_bitlen(__v.__sig) > 20) {
      if (__v.__neg) return __ycxx::__detail::__fpm::__mp_to<_Tp>(__ycxx::__detail::__fpm::__mp_from_int<_Np>(-1)); // -1, inexact
      return __ycxx::__detail::__fpm::__fp_overflow<_Tp>(false);
    }
    const __mpf<_Np> __w = __ycxx::__detail::__fpm::__mp_from_value<_Np>(__v);
    if (__ycxx::__detail::__fpm::__mp_exp_range<_Tp>(__w) > 0) return __ycxx::__detail::__fpm::__fp_overflow<_Tp>(false);
    return __ycxx::__detail::__fpm::__mp_to<_Tp>(__ycxx::__detail::__fpm::__mp_expm1(__w));
  }
}

// log (base 0: e, 2, 10)
template <class _Tp>
constexpr _Tp __fp_log_base(_Tp __x, int base) noexcept {
  constexpr int _Np = __mp_limbs<_Tp>;
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  if (__v.kind == __fp_kind::nan) return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  if (__v.kind == __fp_kind::zero) return __ycxx::__detail::__fpm::__fp_pole<_Tp>(true);
  if (__v.__neg) return __ycxx::__detail::__fpm::__fp_invalid<_Tp>();
  if (__v.kind == __fp_kind::__inf) return __x;
  if (__x == _Tp(1)) return _Tp(0);
  if (base == 2 && __ycxx::__detail::__fpm::__wide_bitlen(__v.__sig) == __ycxx::__detail::__fpm::__wide_ctz(__v.__sig) + 1) // a power of 2
    return _Tp(__v.exp + __ycxx::__detail::__fpm::__wide_ctz(__v.__sig));
  __mpf<_Np> r = __ycxx::__detail::__fpm::__mp_log(__ycxx::__detail::__fpm::__mp_from_value<_Np>(__v));
  if (base == 2) r = __ycxx::__detail::__fpm::__mp_mul(r, __ycxx::__detail::__fpm::__mp_const<_Np>(__math_constant::log2e));
  if (base == 10) r = __ycxx::__detail::__fpm::__mp_mul(r, __ycxx::__detail::__fpm::__mp_const<_Np>(__math_constant::log10e));
  return __ycxx::__detail::__fpm::__mp_to<_Tp>(r);
}
template <class _Tp>
constexpr _Tp __fp_log1p(_Tp __x) noexcept {
  constexpr int _Np = __mp_limbs<_Tp>;
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  if (__v.kind == __fp_kind::nan) return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  if (__v.kind == __fp_kind::zero) return __x;
  if (__x == _Tp(-1)) return __ycxx::__detail::__fpm::__fp_pole<_Tp>(true);
  if (__x < _Tp(-1)) return __ycxx::__detail::__fpm::__fp_invalid<_Tp>();
  if (__v.kind == __fp_kind::__inf) return __x;
  return __ycxx::__detail::__fpm::__mp_to<_Tp>(__ycxx::__detail::__fpm::__mp_log1p(__ycxx::__detail::__fpm::__mp_from_value<_Np>(__v)));
}

template <class _Tp>
constexpr _Tp __fp_pow(_Tp __x, _Tp y) noexcept {
  constexpr int _Np = __mp_limbs<_Tp>;
  const __fp_value __vx = __ycxx::__detail::__fpm::__fp_decode(__x), __vy = __ycxx::__detail::__fpm::__fp_decode(y);
  // F.10.4.5
  if (__vy.kind == __fp_kind::zero) {
    if (__vx.__signaling) __ycxx::__detail::__fpm::__fp_report(__fe_invalid);
    return _Tp(1);
  }
  if (__x == _Tp(1)) {
    if (__vy.__signaling) __ycxx::__detail::__fpm::__fp_report(__fe_invalid);
    return _Tp(1);
  }
  if (__vx.kind == __fp_kind::nan || __vy.kind == __fp_kind::nan) return __ycxx::__detail::__fpm::__fp_nan_operands(__x, y);
  const bool __y_odd = __ycxx::__detail::__fpm::__fp_is_odd_int<_Tp>(__vy), __y_int = __ycxx::__detail::__fpm::__fp_is_int<_Tp>(__vy) || __vy.kind == __fp_kind::__inf;
  if (__vx.kind == __fp_kind::zero) {
    if (__vy.__neg) {
      if (__vy.kind == __fp_kind::__inf) return __ycxx::__detail::__fpm::__fp_infinity<_Tp>(false);
      return __ycxx::__detail::__fpm::__fp_pole<_Tp>(__y_odd && __vx.__neg);
    }
    return __ycxx::__detail::__fpm::__fp_zero<_Tp>(__y_odd && __vx.__neg);
  }
  if (__vy.kind == __fp_kind::__inf) {
    if (__x == _Tp(-1)) return _Tp(1);
    const bool big = __ycxx::__detail::__fpm::__fp_abs(__x) > _Tp(1);
    return big != __vy.__neg ? __ycxx::__detail::__fpm::__fp_infinity<_Tp>(false) : _Tp(0);
  }
  if (__vx.kind == __fp_kind::__inf) {
    if (__vx.__neg) return __vy.__neg ? __ycxx::__detail::__fpm::__fp_zero<_Tp>(__y_odd) : __ycxx::__detail::__fpm::__fp_infinity<_Tp>(__y_odd);
    return __vy.__neg ? _Tp(0) : __x;
  }
  if (__vx.__neg && !__y_int) return __ycxx::__detail::__fpm::__fp_invalid<_Tp>();
  const bool __neg = __vx.__neg && __y_odd;
  // Exact cases: integral y with |y| small and an exactly computable power.
  if (__y_int && __vy.exp + __ycxx::__detail::__fpm::__wide_bitlen(__vy.__sig) <= 8) {
    const long long n = __ycxx::__detail::__fpm::__mp_round_int(__ycxx::__detail::__fpm::__mp_from_value<_Np>(__vy));
    const int __tz = __ycxx::__detail::__fpm::__wide_ctz(__vx.__sig);
    const __wide<2> __odd = __ycxx::__detail::__fpm::__wide_shr(__vx.__sig, __tz);
    const long long __e2 = static_cast<long long>(__vx.exp + __tz);
    if (__ycxx::__detail::__fpm::__wide_bitlen(__odd) == 1) { // a power of two
      const long long e = __e2 * n;
      return __ycxx::__detail::__fpm::__fp_scale(__neg ? _Tp(-1) : _Tp(1), e > (1L << 20) ? (1L << 20) : e < -(1L << 20) ? -(1L << 20) : static_cast<long>(e));
    }
    if (n > 0 && static_cast<long long>(__ycxx::__detail::__fpm::__wide_bitlen(__odd)) * n <= 128) {
      __wide<4> p = __ycxx::__detail::__fpm::__wide_resize<4>(__odd);
      for (long long i = 1; i < n; ++i) p = __ycxx::__detail::__fpm::__wide_resize<4>(__ycxx::__detail::__fpm::__wide_mul(p, __ycxx::__detail::__fpm::__wide_resize<2>(__odd)));
      const long long e = __e2 * n;
      if (e > -(1L << 20) && e < (1L << 20))
        return __ycxx::__detail::__fpm::__fp_finish(__ycxx::__detail::__fpm::__fp_round<_Tp>(__neg, p, static_cast<int>(e)));
    }
  }
  // General: e^(y ln|x|).
  __mpf<_Np> __ax = __ycxx::__detail::__fpm::__mp_from_value<_Np>(__vx);
  __ax.__neg = false;
  const __mpf<_Np> __w = __ycxx::__detail::__fpm::__mp_mul(__ycxx::__detail::__fpm::__mp_from_value<_Np>(__vy), __ycxx::__detail::__fpm::__mp_log(__ax));
  return __ycxx::__detail::__fpm::__mp_exp_to<_Tp>(__w, __neg);
}

template <class _Tp>
constexpr _Tp __fp_trig(_Tp __x, int __which) noexcept {
  constexpr int _Np = __mp_limbs<_Tp>;
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  if (__v.kind == __fp_kind::nan) return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  if (__v.kind == __fp_kind::__inf) return __ycxx::__detail::__fpm::__fp_invalid<_Tp>();
  if (__v.kind == __fp_kind::zero) return __which == 1 ? _Tp(1) : __x;
  return __ycxx::__detail::__fpm::__mp_to<_Tp>(__ycxx::__detail::__fpm::__mp_trig<_Np>(__v, __which));
}

template <class _Tp>
constexpr _Tp __fp_atan(_Tp __x) noexcept {
  constexpr int _Np = __mp_limbs<_Tp>;
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  if (__v.kind == __fp_kind::nan) return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  if (__v.kind == __fp_kind::zero) return __x;
  if (__v.kind == __fp_kind::__inf) {
    __mpf<_Np> h = __ycxx::__detail::__fpm::__mp_ldexp(__ycxx::__detail::__fpm::__mp_const<_Np>(__math_constant::pi), -1);
    h.__neg = __v.__neg;
    return __ycxx::__detail::__fpm::__mp_to<_Tp>(h);
  }
  return __ycxx::__detail::__fpm::__mp_to<_Tp>(__ycxx::__detail::__fpm::__mp_atan(__ycxx::__detail::__fpm::__mp_from_value<_Np>(__v)));
}
template <class _Tp>
constexpr _Tp __fp_atan2(_Tp y, _Tp __x) noexcept {
  constexpr int _Np = __mp_limbs<_Tp>;
  const __fp_value __vy = __ycxx::__detail::__fpm::__fp_decode(y), __vx = __ycxx::__detail::__fpm::__fp_decode(__x);
  if (__vy.kind == __fp_kind::nan || __vx.kind == __fp_kind::nan) return __ycxx::__detail::__fpm::__fp_nan_operands(y, __x);
  const __mpf<_Np> pi = __ycxx::__detail::__fpm::__mp_const<_Np>(__math_constant::pi);
  __mpf<_Np> r;
  if (__vy.kind == __fp_kind::zero) {
    if (!__vx.__neg) return y; // +-0
    r = pi;
  } else if (__vy.kind == __fp_kind::__inf) {
    if (__vx.kind == __fp_kind::__inf)
      r = __vx.__neg ? __ycxx::__detail::__fpm::__mp_ldexp(__ycxx::__detail::__fpm::__mp_mul_u64(pi, 3), -2) : __ycxx::__detail::__fpm::__mp_ldexp(pi, -2);
    else
      r = __ycxx::__detail::__fpm::__mp_ldexp(pi, -1);
  } else if (__vx.kind == __fp_kind::__inf) {
    if (!__vx.__neg) return __ycxx::__detail::__fpm::__fp_zero<_Tp>(__vy.__neg);
    r = pi;
  } else if (__vx.kind == __fp_kind::zero) {
    r = __ycxx::__detail::__fpm::__mp_ldexp(pi, -1);
  } else {
    r = __ycxx::__detail::__fpm::__mp_atan2(__ycxx::__detail::__fpm::__mp_from_value<_Np>(__vy), __ycxx::__detail::__fpm::__mp_from_value<_Np>(__vx));
    r.__neg = false;
  }
  r.__neg = __vy.__neg;
  return __ycxx::__detail::__fpm::__mp_to<_Tp>(r);
}
// asin (acos: which == 1)
template <class _Tp>
constexpr _Tp __fp_asin_acos(_Tp __x, bool acos) noexcept {
  constexpr int _Np = __mp_limbs<_Tp>;
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  if (__v.kind == __fp_kind::nan) return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  if (__v.kind == __fp_kind::__inf || __ycxx::__detail::__fpm::__fp_abs(__x) > _Tp(1)) return __ycxx::__detail::__fpm::__fp_invalid<_Tp>();
  if (acos && __x == _Tp(1)) return _Tp(0);
  if (!acos && __v.kind == __fp_kind::zero) return __x;
  const __mpf<_Np> __one = __ycxx::__detail::__fpm::__mp_one<_Np>();
  const __mpf<_Np> a = __ycxx::__detail::__fpm::__mp_from_value<_Np>(__v);
  // sqrt(1 - x^2) = sqrt((1 - x)(1 + x)), exact factors
  const __mpf<_Np> c = __ycxx::__detail::__fpm::__mp_sqrt(__ycxx::__detail::__fpm::__mp_mul(__ycxx::__detail::__fpm::__mp_sub(__one, a), __ycxx::__detail::__fpm::__mp_add(__one, a)));
  if (acos) return __ycxx::__detail::__fpm::__mp_to<_Tp>(__ycxx::__detail::__fpm::__mp_atan2(c, a));
  if (__ycxx::__detail::__fpm::__mp_is_zero(c)) {
    __mpf<_Np> h = __ycxx::__detail::__fpm::__mp_ldexp(__ycxx::__detail::__fpm::__mp_const<_Np>(__math_constant::pi), -1);
    h.__neg = __v.__neg;
    return __ycxx::__detail::__fpm::__mp_to<_Tp>(h);
  }
  return __ycxx::__detail::__fpm::__mp_to<_Tp>(__ycxx::__detail::__fpm::__mp_atan(__ycxx::__detail::__fpm::__mp_div(a, c)));
}

// sinh (0), cosh (1), tanh (2)
template <class _Tp>
constexpr _Tp __fp_hyperbolic(_Tp __x, int __which) noexcept {
  constexpr int _Np = __mp_limbs<_Tp>;
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  if (__v.kind == __fp_kind::nan) return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  if (__v.kind == __fp_kind::zero) return __which == 1 ? _Tp(1) : __x;
  if (__v.kind == __fp_kind::__inf) return __which == 1 ? __ycxx::__detail::__fpm::__fp_abs(__x) : __which == 0 ? __x : (__v.__neg ? _Tp(-1) : _Tp(1));
  __mpf<_Np> a = __ycxx::__detail::__fpm::__mp_from_value<_Np>(__v);
  a.__neg = false;
  const __mpf<_Np> __one = __ycxx::__detail::__fpm::__mp_one<_Np>();
  if (__which == 2) {
    if (__ycxx::__detail::__fpm::__mp_ilog(a) >= 7) return __v.__neg ? __ycxx::__detail::__fpm::__mp_to<_Tp>(__ycxx::__detail::__fpm::__mp_neg(__one)) : __ycxx::__detail::__fpm::__mp_to<_Tp>(__one); // 1 - tiny
    const __mpf<_Np> e = __ycxx::__detail::__fpm::__mp_expm1(__ycxx::__detail::__fpm::__mp_ldexp(a, 1));
    __mpf<_Np> r = __ycxx::__detail::__fpm::__mp_div(e, __ycxx::__detail::__fpm::__mp_add(e, __ycxx::__detail::__fpm::__mp_ldexp(__one, 1)));
    r.__neg = __v.__neg;
    return __ycxx::__detail::__fpm::__mp_to<_Tp>(r);
  }
  if (__ycxx::__detail::__fpm::__mp_ilog(a) >= 20) return __ycxx::__detail::__fpm::__fp_overflow<_Tp>(__which == 0 && __v.__neg);
  // e^|x| / 2 overflows exactly when sinh/cosh do (up to the last ulp, decided by rounding).
  const __mpf<_Np> __half_e = __ycxx::__detail::__fpm::__mp_sub(a, __ycxx::__detail::__fpm::__mp_const<_Np>(__math_constant::ln2));
  if (__ycxx::__detail::__fpm::__mp_exp_range<_Tp>(__half_e) > 0) return __ycxx::__detail::__fpm::__fp_overflow<_Tp>(__which == 0 && __v.__neg);
  __mpf<_Np> r;
  if (__which == 0) {
    // sinh = (E + E / (E + 1)) / 2, E = expm1(|x|): accurate for small |x| too
    const __mpf<_Np> e = __ycxx::__detail::__fpm::__mp_expm1(a);
    r = __ycxx::__detail::__fpm::__mp_ldexp(__ycxx::__detail::__fpm::__mp_add(e, __ycxx::__detail::__fpm::__mp_div(e, __ycxx::__detail::__fpm::__mp_add(e, __one))), -1);
    r.__neg = __v.__neg;
  } else {
    const __mpf<_Np> e = __ycxx::__detail::__fpm::__mp_exp(a);
    r = __ycxx::__detail::__fpm::__mp_ldexp(__ycxx::__detail::__fpm::__mp_add(e, __ycxx::__detail::__fpm::__mp_recip(e)), -1);
  }
  return __ycxx::__detail::__fpm::__mp_to<_Tp>(r);
}
// asinh (0), acosh (1), atanh (2)
template <class _Tp>
constexpr _Tp __fp_inverse_hyperbolic(_Tp __x, int __which) noexcept {
  constexpr int _Np = __mp_limbs<_Tp>;
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  if (__v.kind == __fp_kind::nan) return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  const __mpf<_Np> __one = __ycxx::__detail::__fpm::__mp_one<_Np>();
  if (__which == 0) {
    if (__v.kind != __fp_kind::__finite) return __x;
    __mpf<_Np> a = __ycxx::__detail::__fpm::__mp_from_value<_Np>(__v);
    a.__neg = false;
    // log1p(a + a^2 / (1 + sqrt(1 + a^2)))
    const __mpf<_Np> __a2 = __ycxx::__detail::__fpm::__mp_mul(a, a);
    __mpf<_Np> r = __ycxx::__detail::__fpm::__mp_log1p(__ycxx::__detail::__fpm::__mp_add(
        a, __ycxx::__detail::__fpm::__mp_div(__a2, __ycxx::__detail::__fpm::__mp_add(__one, __ycxx::__detail::__fpm::__mp_sqrt(__ycxx::__detail::__fpm::__mp_add(__one, __a2))))));
    r.__neg = __v.__neg;
    return __ycxx::__detail::__fpm::__mp_to<_Tp>(r);
  }
  if (__which == 1) {
    if (__x < _Tp(1)) return __ycxx::__detail::__fpm::__fp_invalid<_Tp>();
    if (__x == _Tp(1)) return _Tp(0);
    if (__v.kind == __fp_kind::__inf) return __x;
    const __mpf<_Np> a = __ycxx::__detail::__fpm::__mp_from_value<_Np>(__v);
    const __mpf<_Np> t = __ycxx::__detail::__fpm::__mp_sub(a, __one); // exact
    return __ycxx::__detail::__fpm::__mp_to<_Tp>(__ycxx::__detail::__fpm::__mp_log1p(
        __ycxx::__detail::__fpm::__mp_add(t, __ycxx::__detail::__fpm::__mp_sqrt(__ycxx::__detail::__fpm::__mp_mul(t, __ycxx::__detail::__fpm::__mp_add(a, __one))))));
  }
  if (__v.kind == __fp_kind::zero) return __x;
  const _Tp __ax = __ycxx::__detail::__fpm::__fp_abs(__x);
  if (__ax > _Tp(1)) return __ycxx::__detail::__fpm::__fp_invalid<_Tp>();
  if (__ax == _Tp(1)) return __ycxx::__detail::__fpm::__fp_pole<_Tp>(__v.__neg);
  __mpf<_Np> a = __ycxx::__detail::__fpm::__mp_from_value<_Np>(__v);
  a.__neg = false;
  // atanh = log1p(2a / (1 - a)) / 2
  __mpf<_Np> r = __ycxx::__detail::__fpm::__mp_ldexp(
      __ycxx::__detail::__fpm::__mp_log1p(__ycxx::__detail::__fpm::__mp_div(__ycxx::__detail::__fpm::__mp_ldexp(a, 1), __ycxx::__detail::__fpm::__mp_sub(__one, a))), -1);
  r.__neg = __v.__neg;
  return __ycxx::__detail::__fpm::__mp_to<_Tp>(r);
}

template <class _Tp>
constexpr _Tp __fp_erf(_Tp __x, bool __complement) noexcept {
  constexpr int _Np = __mp_limbs<_Tp>;
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  if (__v.kind == __fp_kind::nan) return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  if (__v.kind == __fp_kind::__inf) return __complement ? (__v.__neg ? _Tp(2) : _Tp(0)) : (__v.__neg ? _Tp(-1) : _Tp(1));
  if (__v.kind == __fp_kind::zero) return __complement ? _Tp(1) : __x;
  __mpf<_Np> a = __ycxx::__detail::__fpm::__mp_from_value<_Np>(__v);
  a.__neg = false;
  const __mpf<_Np> __one = __ycxx::__detail::__fpm::__mp_one<_Np>();
  const bool __large = __ycxx::__detail::__fpm::__mp_cmp(a, __ycxx::__detail::__fpm::__mp_from_u64<_Np>(3)) >= 0;
  __mpf<_Np> r;
  if (!__large) {
    const __mpf<_Np> e = __ycxx::__detail::__fpm::__mp_erf_small(a);
    if (!__complement) {
      r = e;
      r.__neg = __v.__neg;
    } else {
      r = __v.__neg ? __ycxx::__detail::__fpm::__mp_add(__one, e) : __ycxx::__detail::__fpm::__mp_sub(__one, e);
    }
    return __ycxx::__detail::__fpm::__mp_to<_Tp>(r);
  }
  // |x| >= 3: through erfc(|x|) = e^(-x^2) / sqrt(pi) * CF.
  if (__ycxx::__detail::__fpm::__mp_ilog(a) >= 16) { // erfc(|x|) underflows to 0 in every format
    if (__complement) return __v.__neg ? __ycxx::__detail::__fpm::__mp_to<_Tp>(__ycxx::__detail::__fpm::__mp_ldexp(__one, 1)) : __ycxx::__detail::__fpm::__fp_underflow<_Tp>(false);
    return __ycxx::__detail::__fpm::__mp_to<_Tp>(__v.__neg ? __ycxx::__detail::__fpm::__mp_neg(__one) : __one);
  }
  double __ad = 0;
  {
    const __mpf<_Np> t = a;
    __ad = __ycxx::__detail::__fpm::__mp_lead(t);
    for (int e = t.exp + 64 * _Np; e > 0; --e) __ad *= 2;
  }
  const __mpf<_Np> c = __ycxx::__detail::__fpm::__mp_erfc_large(a, __ad);
  if (__complement) {
    if (__v.__neg) return __ycxx::__detail::__fpm::__mp_to<_Tp>(__ycxx::__detail::__fpm::__mp_sub(__ycxx::__detail::__fpm::__mp_ldexp(__one, 1), c));
    return __ycxx::__detail::__fpm::__mp_to<_Tp>(c);
  }
  r = __ycxx::__detail::__fpm::__mp_sub(__one, c);
  r.__neg = __v.__neg;
  return __ycxx::__detail::__fpm::__mp_to<_Tp>(r);
}

// lgamma (log of |Gamma|) and tgamma.
template <class _Tp>
constexpr _Tp __fp_gamma(_Tp __x, bool log) noexcept {
  constexpr int _Np = __mp_limbs<_Tp>;
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  if (__v.kind == __fp_kind::nan) return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  if (__v.kind == __fp_kind::__inf) {
    if (log) return __ycxx::__detail::__fpm::__fp_infinity<_Tp>(false);
    return __v.__neg ? __ycxx::__detail::__fpm::__fp_invalid<_Tp>() : __x;
  }
  if (__v.kind == __fp_kind::zero) return __ycxx::__detail::__fpm::__fp_pole<_Tp>(!log && __v.__neg);
  const bool __is_int = __ycxx::__detail::__fpm::__fp_is_int<_Tp>(__v);
  if (__v.__neg && __is_int) return log ? __ycxx::__detail::__fpm::__fp_pole<_Tp>(false) : __ycxx::__detail::__fpm::__fp_invalid<_Tp>();
  if (log && (__x == _Tp(1) || __x == _Tp(2))) return _Tp(0);
  const __mpf<_Np> a = __ycxx::__detail::__fpm::__mp_from_value<_Np>(__v);
  if (__ycxx::__detail::__fpm::__mp_ilog(a) >= 30) { // huge |x|
    if (!__v.__neg) return log ? __ycxx::__detail::__fpm::__mp_to<_Tp>(__ycxx::__detail::__fpm::__mp_lgamma_pos(a)) : __ycxx::__detail::__fpm::__fp_overflow<_Tp>(false);
  }
  const __mpf<_Np> __one = __ycxx::__detail::__fpm::__mp_one<_Np>();
  __mpf<_Np> __lg; // ln |Gamma(x)|
  bool __neg = false;
  if (!__v.__neg) {
    __lg = __ycxx::__detail::__fpm::__mp_lgamma_pos(a);
  } else {
    // Gamma(x) Gamma(1 - x) = pi / sin(pi x)
    const __mpf<_Np> s = __ycxx::__detail::__fpm::__mp_sinpi<_Np>(__v);
    __neg = s.__neg;
    const __mpf<_Np> pi = __ycxx::__detail::__fpm::__mp_const<_Np>(__math_constant::pi);
    __lg = __ycxx::__detail::__fpm::__mp_sub(__ycxx::__detail::__fpm::__mp_log(__ycxx::__detail::__fpm::__mp_div(pi, __ycxx::__detail::__fpm::__mp_abs(s))),
                                   __ycxx::__detail::__fpm::__mp_lgamma_pos(__ycxx::__detail::__fpm::__mp_sub(__one, a)));
  }
  if (log) return __ycxx::__detail::__fpm::__mp_to<_Tp>(__lg);
  return __ycxx::__detail::__fpm::__mp_exp_to<_Tp>(__lg, __neg);
}

}} // namespace __ycxx::__detail::__fpm
