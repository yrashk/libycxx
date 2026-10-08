// libycxx core: the mathematical special functions of <cmath> ([sf.cmath]).
//
// Not constexpr (the draft does not make them so). Each function computes in a working type
// wider than its result where one exists (float in double, double in x87 long double; long double
// and binary128 in themselves, the 16-bit types in double), using:
// - three-term recurrences for the polynomials and the (spherical) Legendre functions (the
//   spherical ones normalised at every step, so they neither overflow nor underflow early);
// - Carlson's symmetric integrals R_F, R_D, R_J (duplication algorithm) for the elliptic
//   integrals, with phi reduced modulo pi;
// - for the Bessel functions, the continued fraction for J'/J (or I'/I) with downward
//   recurrence, Temme's series (x < 2) or Steed's complex continued fraction (x >= 2) for the
//   order mu in [-1/2, 1/2], normalised by the Wronskian; Hankel's asymptotic expansions for
//   x > 1000 when x also exceeds nu^2 / 2; reflection formulas for negative orders;
// - the power series / continued fraction / asymptotic series of Ei; Euler-Maclaurin summation
//   and the functional equation for zeta; Gamma through lgamma/tgamma of the working type.
// A domain error ([sf.cmath.general]) is reported the way the C library reports one: the
// result is a NaN, errno is set to EDOM (if math_errhandling & MATH_ERRNO) and FE_INVALID is
// raised. Orders and degrees of 128 or more ("implementation-defined") are computed with the
// same algorithms.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/cmath_impl.hpp>
#include <ycxx/core/cmath_tables.hpp>
#include <ycxx/core/math_constants.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__sf {

template <class _Tp>
consteval auto __work_of() {
  using _Cp = __ycxx::__detail::__cm::__carrier_t<_Tp>;
  if constexpr (__fp_format<_Tp>.digits <= 24)
    return std::type_identity<double>{};
  else if constexpr (__fp_format<_Tp>.digits <= 53 && __fp_format<long double>.digits > 53)
    return std::type_identity<long double>{};
  else
    return std::type_identity<_Cp>{};
}
template <class _Tp>
using __work_t = typename decltype(__ycxx::__detail::__sf::__work_of<_Tp>())::type;

template <class _Tp>
[[__gnu__::__cold__]] _Tp domain_error() noexcept {
  volatile double __minus_one = -1.0;
  volatile double r = __builtin_sqrt(__minus_one); // EDOM and FE_INVALID, as the C library does
  (void)r;
  return __ycxx::__detail::__fpm::__fp_quiet_nan<_Tp>();
}
template <class _Tp>
_Tp __nan_result() noexcept {
  return __ycxx::__detail::__fpm::__fp_quiet_nan<_Tp>();
}

// ---- elementary functions of the working type (run time) ------------------------------------------
namespace __w {
template <class _Wp>
_Wp sqrt(_Wp __x) noexcept {
  return __ycxx::__detail::__cm::sqrt<_Wp>(__x);
}
template <class _Wp>
_Wp abs(_Wp __x) noexcept {
  return __ycxx::__detail::__fpm::__fp_abs(__x);
}
template <class _Wp>
_Wp exp(_Wp __x) noexcept {
  return __ycxx::__detail::__cm::__transcendental<__ycxx::__detail::__cm::op::exp, _Wp>(__x);
}
template <class _Wp>
_Wp log(_Wp __x) noexcept {
  return __ycxx::__detail::__cm::__transcendental<__ycxx::__detail::__cm::op::log, _Wp>(__x);
}
template <class _Wp>
_Wp sin(_Wp __x) noexcept {
  return __ycxx::__detail::__cm::__transcendental<__ycxx::__detail::__cm::op::sin, _Wp>(__x);
}
template <class _Wp>
_Wp cos(_Wp __x) noexcept {
  return __ycxx::__detail::__cm::__transcendental<__ycxx::__detail::__cm::op::cos, _Wp>(__x);
}
template <class _Wp>
_Wp sinh(_Wp __x) noexcept {
  return __ycxx::__detail::__cm::__transcendental<__ycxx::__detail::__cm::op::sinh, _Wp>(__x);
}
template <class _Wp>
_Wp cosh(_Wp __x) noexcept {
  return __ycxx::__detail::__cm::__transcendental<__ycxx::__detail::__cm::op::cosh, _Wp>(__x);
}
template <class _Wp>
_Wp pow(_Wp __x, _Wp y) noexcept {
  return __ycxx::__detail::__cm::__transcendental<__ycxx::__detail::__cm::op::pow, _Wp>(__x, y);
}
template <class _Wp>
_Wp lgamma(_Wp __x) noexcept {
  return __ycxx::__detail::__cm::__transcendental<__ycxx::__detail::__cm::op::lgamma, _Wp>(__x);
}
template <class _Wp>
_Wp tgamma(_Wp __x) noexcept {
  return __ycxx::__detail::__cm::__transcendental<__ycxx::__detail::__cm::op::tgamma, _Wp>(__x);
}
template <class _Wp>
_Wp floor(_Wp __x) noexcept {
  return __ycxx::__detail::__cm::__round_integral<__ycxx::__detail::__cm::__rint_op::floor, _Wp>(__x);
}
template <class _Wp>
_Wp round(_Wp __x) noexcept {
  return __ycxx::__detail::__cm::__round_integral<__ycxx::__detail::__cm::__rint_op::round, _Wp>(__x);
}
template <class _Wp>
_Wp fmod(_Wp __x, _Wp y) noexcept {
  return __ycxx::__detail::__cm::fmod<_Wp>(__x, y);
}
template <class _Wp>
inline constexpr _Wp pi = __ycxx::__detail::__math_constant_value<_Wp>(__math_constant::pi);
template <class _Wp>
inline constexpr _Wp egamma = __ycxx::__detail::__math_constant_value<_Wp>(__math_constant::egamma);
template <class _Wp>
inline constexpr _Wp __eps = _Wp(1) / __ycxx::__detail::__fpm::__fp_scale(_Wp(1), __ycxx::__detail::__fp_format<_Wp>.digits - 1);
template <class _Wp>
inline constexpr _Wp __tiny = __ycxx::__detail::__fpm::__fp_scale(_Wp(1), __ycxx::__detail::__fp_format<_Wp>.__min_exp + 16);
// sin(pi x), cos(pi x) with x reduced exactly first.
template <class _Wp>
_Wp __sinpi(_Wp __x) noexcept {
  const _Wp r = __ycxx::__detail::__sf::__w::fmod(__x, _Wp(2)); // exact
  if (r == _Wp(0) || r == _Wp(1) || r == _Wp(-1)) return _Wp(0);
  return __ycxx::__detail::__sf::__w::sin(__ycxx::__detail::__sf::__w::pi<_Wp> * r);
}
template <class _Wp>
_Wp __cospi(_Wp __x) noexcept {
  const _Wp r = __ycxx::__detail::__sf::__w::fmod(__ycxx::__detail::__sf::__w::abs(__x), _Wp(2));
  if (r == _Wp(0.5) || r == _Wp(1.5)) return _Wp(0);
  return __ycxx::__detail::__sf::__w::cos(__ycxx::__detail::__sf::__w::pi<_Wp> * r);
}
} // namespace w

template <class _Wp, int _Kp>
struct __w_table {
  _Wp __v[_Kp];
};
template <class _Wp, int _Kp>
consteval __w_table<_Wp, _Kp> __make_w_table(const __ycxx::__detail::__fpm::__mp_const_bits (&__src)[_Kp]) {
  __w_table<_Wp, _Kp> t{};
  for (int i = 0; i < _Kp; ++i)
    t.__v[i] = __ycxx::__detail::__fpm::__mp_round<_Wp>(__ycxx::__detail::__fpm::__mp_const<__ycxx::__detail::__fpm::__mp_limbs<_Wp> + 1>(__src[i])).value;
  return t;
}
template <class _Wp>
inline constexpr auto __inv_gamma_w = __ycxx::__detail::__sf::__make_w_table<_Wp>(__ycxx::__detail::__fpm::__inv_gamma_taylor);
template <class _Wp>
inline constexpr auto __stirling_w = __ycxx::__detail::__sf::__make_w_table<_Wp>(__ycxx::__detail::__fpm::__stirling_coefficients);

// ---- polynomials ([sf.cmath.hermite], [sf.cmath.laguerre], [sf.cmath.assoc.laguerre]) ------------
template <class _Tp>
_Tp hermite(unsigned n, _Tp __x) noexcept {
  using _Wp = __work_t<_Tp>;
  if (__builtin_isnan(__x)) return __x;
  if (n == 0) return _Tp(1);
  if (__builtin_isinf(__x)) return (__x < _Tp(0) && n % 2 == 1) ? __x : __ycxx::__detail::__fpm::__fp_abs(__x); // the sign of x^n
  const _Wp __wx = __x;
  _Wp __h0 = 1, __h1 = 2 * __wx;
  for (unsigned k = 1; k < n; ++k) {
    const _Wp __h2 = 2 * __wx * __h1 - 2 * _Wp(k) * __h0;
    // Overflow (where the working type is the result's own, as double on Arm Darwin): the next
    // step would be inf - inf. |x| is then far beyond the zeros, where H_n has the sign of x^n
    // and grows with n.
    if (__builtin_isinf(__h2))
      return (__x < _Tp(0) && n % 2 == 1) ? -__builtin_huge_valf() : __builtin_huge_valf();
    __h0 = __h1;
    __h1 = __h2;
  }
  return static_cast<_Tp>(__h1);
}
template <class _Tp>
_Tp assoc_laguerre(unsigned n, unsigned m, _Tp __x) noexcept {
  using _Wp = __work_t<_Tp>;
  if (__builtin_isnan(__x)) return __x;
  if (__x < _Tp(0)) return __ycxx::__detail::__sf::domain_error<_Tp>();
  const _Wp __wx = __x, __wm = m;
  _Wp __l0 = 1, __l1 = 1 + __wm - __wx;
  if (n == 0) return _Tp(1);
  for (unsigned k = 1; k < n; ++k) {
    const _Wp __l2 = ((2 * _Wp(k) + 1 + __wm - __wx) * __l1 - (_Wp(k) + __wm) * __l0) / _Wp(k + 1);
    __l0 = __l1;
    __l1 = __l2;
  }
  return static_cast<_Tp>(__l1);
}
template <class _Tp>
_Tp laguerre(unsigned n, _Tp __x) noexcept {
  return __ycxx::__detail::__sf::assoc_laguerre<_Tp>(n, 0, __x);
}

// ---- Legendre ([sf.cmath.legendre], [sf.cmath.assoc.legendre], [sf.cmath.sph.legendre]) -----------
template <class _Tp>
_Tp assoc_legendre(unsigned __l, unsigned m, _Tp __x) noexcept {
  using _Wp = __work_t<_Tp>;
  if (__builtin_isnan(__x)) return __x;
  if (__x > _Tp(1) || __x < _Tp(-1)) return __ycxx::__detail::__sf::domain_error<_Tp>();
  if (m > __l) return _Tp(0);
  const _Wp __wx = __x;
  // P_m^m = (2m - 1)!! (1 - x^2)^(m/2) (no Condon-Shortley phase, Formula 29.23)
  _Wp __pmm = 1;
  if (m > 0) {
    const _Wp s = __ycxx::__detail::__sf::__w::sqrt((1 - __wx) * (1 + __wx));
    _Wp __f = 1;
    for (unsigned i = 1; i <= m; ++i) {
      __pmm *= __f * s;
      __f += 2;
    }
  }
  if (__l == m) return static_cast<_Tp>(__pmm);
  _Wp __p1 = __wx * _Wp(2 * m + 1) * __pmm;
  _Wp __p0 = __pmm;
  for (unsigned k = m + 1; k < __l; ++k) {
    const _Wp __p2 = (_Wp(2 * k + 1) * __wx * __p1 - _Wp(k + m) * __p0) / _Wp(k + 1 - m);
    __p0 = __p1;
    __p1 = __p2;
  }
  return static_cast<_Tp>(__p1);
}
template <class _Tp>
_Tp legendre(unsigned __l, _Tp __x) noexcept {
  return __ycxx::__detail::__sf::assoc_legendre<_Tp>(__l, 0, __x);
}
template <class _Tp>
_Tp sph_legendre(unsigned __l, unsigned m, _Tp __theta) noexcept {
  using _Wp = __work_t<_Tp>;
  if (__builtin_isnan(__theta)) return __theta;
  if (m > __l) return _Tp(0);
  const _Wp __th = __theta;
  const _Wp c = __ycxx::__detail::__sf::__w::cos(__th), s = __ycxx::__detail::__sf::__w::sin(__th);
  // Y_m^m = (-1)^m sqrt((2m + 1)/(4 pi) (2m - 1)!! / (2m)!!) sin^m(theta), built factor by factor.
  _Wp y = 1 / __ycxx::__detail::__sf::__w::sqrt(4 * __ycxx::__detail::__sf::__w::pi<_Wp>);
  for (unsigned i = 1; i <= m; ++i) y *= -__ycxx::__detail::__sf::__w::sqrt(_Wp(2 * i + 1) / _Wp(2 * i)) * s;
  if (__l == m) return static_cast<_Tp>(y);
  _Wp __y1 = c * __ycxx::__detail::__sf::__w::sqrt(_Wp(2 * m + 3)) * y;
  _Wp __y0 = y;
  for (unsigned k = m + 2; k <= __l; ++k) {
    const _Wp a = __ycxx::__detail::__sf::__w::sqrt(_Wp(4 * k * k - 1) / _Wp((k - m) * (k + m)));
    const _Wp b = __ycxx::__detail::__sf::__w::sqrt(_Wp(2 * k + 1) * _Wp(k + m - 1) * _Wp(k - m - 1) / (_Wp(2 * k - 3) * _Wp(k - m) * _Wp(k + m)));
    const _Wp __y2 = a * c * __y1 - b * __y0;
    __y0 = __y1;
    __y1 = __y2;
  }
  return static_cast<_Tp>(__y1);
}

// ---- beta ([sf.cmath.beta]) -------------------------------------------------------------------------
template <class _Tp>
_Tp beta(_Tp __x, _Tp y) noexcept {
  using _Wp = __work_t<_Tp>;
  if (__builtin_isnan(__x) || __builtin_isnan(y)) return __ycxx::__detail::__sf::__nan_result<_Tp>();
  if (!(__x > _Tp(0)) || !(y > _Tp(0))) return __ycxx::__detail::__sf::domain_error<_Tp>();
  const _Wp a = __x, b = y;
  const _Wp __g = __ycxx::__detail::__sf::__w::tgamma(a + b);
  if (__builtin_isfinite(__g)) {
    const _Wp __ga = __ycxx::__detail::__sf::__w::tgamma(a), __gb = __ycxx::__detail::__sf::__w::tgamma(b);
    const _Wp r = a > b ? __ga / __g * __gb : __gb / __g * __ga;
    if (__builtin_isfinite(r) && r != _Wp(0)) return static_cast<_Tp>(r);
  }
  return static_cast<_Tp>(__ycxx::__detail::__sf::__w::exp(__ycxx::__detail::__sf::__w::lgamma(a) + __ycxx::__detail::__sf::__w::lgamma(b) -
                                                  __ycxx::__detail::__sf::__w::lgamma(a + b)));
}

// ---- Carlson's symmetric elliptic integrals -------------------------------------------------------
template <class _Wp>
_Wp __carlson_tol() noexcept {
  return __ycxx::__detail::__sf::__w::pow(4 * __ycxx::__detail::__sf::__w::__eps<_Wp>, _Wp(1) / _Wp(6));
}
template <class _Wp>
_Wp __carlson_rc(_Wp __x, _Wp y) noexcept { // y > 0
  const _Wp __tol = __ycxx::__detail::__sf::__carlson_tol<_Wp>();
  _Wp __ave, s;
  for (int i = 0; i < 200; ++i) {
    const _Wp __lam = 2 * __ycxx::__detail::__sf::__w::sqrt(__x) * __ycxx::__detail::__sf::__w::sqrt(y) + y;
    __x = (__x + __lam) / 4;
    y = (y + __lam) / 4;
    __ave = (__x + y + y) / 3;
    s = (y - __ave) / __ave;
    if (__ycxx::__detail::__sf::__w::abs(s) < __tol) break;
  }
  return (1 + s * s * (_Wp(3) / 10 + s * (_Wp(1) / 7 + s * (_Wp(3) / 8 + s * _Wp(9) / 22)))) / __ycxx::__detail::__sf::__w::sqrt(__ave);
}
template <class _Wp>
_Wp __carlson_rf(_Wp __x, _Wp y, _Wp __z) noexcept {
  const _Wp __tol = __ycxx::__detail::__sf::__carlson_tol<_Wp>();
  _Wp __ave = 0, __dx = 0, __dy = 0, __dz = 0;
  for (int i = 0; i < 200; ++i) {
    const _Wp __sx = __ycxx::__detail::__sf::__w::sqrt(__x), __sy = __ycxx::__detail::__sf::__w::sqrt(y), __sz = __ycxx::__detail::__sf::__w::sqrt(__z);
    const _Wp __lam = __sx * (__sy + __sz) + __sy * __sz;
    __x = (__x + __lam) / 4;
    y = (y + __lam) / 4;
    __z = (__z + __lam) / 4;
    __ave = (__x + y + __z) / 3;
    __dx = (__ave - __x) / __ave;
    __dy = (__ave - y) / __ave;
    __dz = (__ave - __z) / __ave;
    if (__ycxx::__detail::__sf::__w::abs(__dx) < __tol && __ycxx::__detail::__sf::__w::abs(__dy) < __tol && __ycxx::__detail::__sf::__w::abs(__dz) < __tol) break;
  }
  const _Wp __e2 = __dx * __dy - __dz * __dz, __e3 = __dx * __dy * __dz;
  return (1 + (__e2 / 24 - _Wp(1) / 10 - _Wp(3) * __e3 / 44) * __e2 + __e3 / 14) / __ycxx::__detail::__sf::__w::sqrt(__ave);
}
template <class _Wp>
_Wp __carlson_rd(_Wp __x, _Wp y, _Wp __z) noexcept {
  const _Wp __tol = __ycxx::__detail::__sf::__carlson_tol<_Wp>();
  _Wp sum = 0, __fac = 1, __ave = 0, __dx = 0, __dy = 0, __dz = 0;
  for (int i = 0; i < 200; ++i) {
    const _Wp __sx = __ycxx::__detail::__sf::__w::sqrt(__x), __sy = __ycxx::__detail::__sf::__w::sqrt(y), __sz = __ycxx::__detail::__sf::__w::sqrt(__z);
    const _Wp __lam = __sx * (__sy + __sz) + __sy * __sz;
    sum += __fac / (__sz * (__z + __lam));
    __fac /= 4;
    __x = (__x + __lam) / 4;
    y = (y + __lam) / 4;
    __z = (__z + __lam) / 4;
    __ave = (__x + y + 3 * __z) / 5;
    __dx = (__ave - __x) / __ave;
    __dy = (__ave - y) / __ave;
    __dz = (__ave - __z) / __ave;
    if (__ycxx::__detail::__sf::__w::abs(__dx) < __tol && __ycxx::__detail::__sf::__w::abs(__dy) < __tol && __ycxx::__detail::__sf::__w::abs(__dz) < __tol) break;
  }
  const _Wp __ea = __dx * __dy, __eb = __dz * __dz, ec = __ea - __eb, __ed = __ea - 6 * __eb, __ee = __ed + ec + ec;
  const _Wp __c1 = _Wp(3) / 14, __c2 = _Wp(1) / 6, __c3 = _Wp(9) / 22, __c4 = _Wp(3) / 26, __c5 = __c3 / 4, __c6 = _Wp(3) * __c4 / 2;
  return 3 * sum + __fac * (1 + __ed * (-__c1 + __c5 * __ed - __c6 * __dz * __ee) + __dz * (__c2 * __ee + __dz * (-__c3 * ec + __dz * __c4 * __ea))) /
                       (__ave * __ycxx::__detail::__sf::__w::sqrt(__ave));
}
template <class _Wp>
_Wp __carlson_rj(_Wp __x, _Wp y, _Wp __z, _Wp p) noexcept { // p > 0
  const _Wp __tol = __ycxx::__detail::__sf::__carlson_tol<_Wp>();
  _Wp sum = 0, __fac = 1, __ave = 0, __dx = 0, __dy = 0, __dz = 0, __dp = 0;
  for (int i = 0; i < 200; ++i) {
    const _Wp __sx = __ycxx::__detail::__sf::__w::sqrt(__x), __sy = __ycxx::__detail::__sf::__w::sqrt(y), __sz = __ycxx::__detail::__sf::__w::sqrt(__z);
    const _Wp __lam = __sx * (__sy + __sz) + __sy * __sz;
    const _Wp __alpha0 = p * (__sx + __sy + __sz) + __sx * __sy * __sz;
    const _Wp alpha = __alpha0 * __alpha0, beta = p * (p + __lam) * (p + __lam);
    sum += __fac * __ycxx::__detail::__sf::__carlson_rc(alpha, beta);
    __fac /= 4;
    __x = (__x + __lam) / 4;
    y = (y + __lam) / 4;
    __z = (__z + __lam) / 4;
    p = (p + __lam) / 4;
    __ave = (__x + y + __z + p + p) / 5;
    __dx = (__ave - __x) / __ave;
    __dy = (__ave - y) / __ave;
    __dz = (__ave - __z) / __ave;
    __dp = (__ave - p) / __ave;
    if (__ycxx::__detail::__sf::__w::abs(__dx) < __tol && __ycxx::__detail::__sf::__w::abs(__dy) < __tol && __ycxx::__detail::__sf::__w::abs(__dz) < __tol &&
        __ycxx::__detail::__sf::__w::abs(__dp) < __tol)
      break;
  }
  const _Wp __ea = __dx * (__dy + __dz) + __dy * __dz, __eb = __dx * __dy * __dz, ec = __dp * __dp, __ed = __ea - 3 * ec, __ee = __eb + 2 * __dp * (__ea - ec);
  const _Wp __c1 = _Wp(3) / 14, __c2 = _Wp(1) / 3, __c3 = _Wp(3) / 22, __c4 = _Wp(3) / 26, __c5 = _Wp(3) * __c3 / 4, __c6 = _Wp(3) * __c4 / 2, __c7 = __c2 / 2,
          __c8 = __c3 + __c3;
  return 3 * sum + __fac * (1 + __ed * (-__c1 + __c5 * __ed - __c6 * __ee) + __eb * (__c7 + __dp * (-__c8 + __dp * __c4)) + __dp * __ea * (__c2 - __dp * __c3) -
                          __c2 * __dp * ec) /
                       (__ave * __ycxx::__detail::__sf::__w::sqrt(__ave));
}

// ---- elliptic integrals ([sf.cmath.comp.ellint.*], [sf.cmath.ellint.*]) ------------------------------
// which: 1 (F, K), 2 (E), 3 (Pi). Complete integrals of |k| <= 1, nu < 1 (nu >= 1: callers).
template <class _Wp>
_Wp __ellint_complete(int __which, _Wp k, _Wp __nu) noexcept {
  const _Wp __kc2 = (1 - k) * (1 + k);
  if (__which == 1) return __ycxx::__detail::__sf::__carlson_rf(_Wp(0), __kc2, _Wp(1));
  if (__which == 2) {
    if (__kc2 == _Wp(0)) return _Wp(1);
    return __ycxx::__detail::__sf::__carlson_rf(_Wp(0), __kc2, _Wp(1)) - k * k / 3 * __ycxx::__detail::__sf::__carlson_rd(_Wp(0), __kc2, _Wp(1));
  }
  return __ycxx::__detail::__sf::__carlson_rf(_Wp(0), __kc2, _Wp(1)) + __nu / 3 * __ycxx::__detail::__sf::__carlson_rj(_Wp(0), __kc2, _Wp(1), 1 - __nu);
}
template <class _Tp>
_Tp __ellint(int __which, _Tp k, _Tp __nu, _Tp phi, bool complete) noexcept {
  using _Wp = __work_t<_Tp>;
  if (__builtin_isnan(k) || __builtin_isnan(__nu) || __builtin_isnan(phi)) return __ycxx::__detail::__sf::__nan_result<_Tp>();
  if (k > _Tp(1) || k < _Tp(-1)) return __ycxx::__detail::__sf::domain_error<_Tp>();
  const _Wp __wk = k, __wnu = __nu;
  const bool __k_one = k == _Tp(1) || k == _Tp(-1);
  if (complete) {
    if (__which == 1 && __k_one) return __ycxx::__detail::__fpm::__fp_infinity<_Tp>(false);
    if (__which == 3) {
      if (__nu == _Tp(1)) return __ycxx::__detail::__fpm::__fp_infinity<_Tp>(false);
      if (__nu > _Tp(1)) return __ycxx::__detail::__sf::domain_error<_Tp>(); // only a principal value exists
      if (__k_one) return __ycxx::__detail::__fpm::__fp_infinity<_Tp>(false);
    }
    return static_cast<_Tp>(__ycxx::__detail::__sf::__ellint_complete<_Wp>(__which, __wk, __wnu));
  }
  if (__builtin_isinf(phi)) return __ycxx::__detail::__sf::domain_error<_Tp>();
  // phi = n pi + r, |r| <= pi/2; F(k, n pi + r) = 2n K(k) + F(k, r) (likewise E, Pi).
  const _Wp __wphi = phi;
  const _Wp n = __ycxx::__detail::__sf::__w::round(__wphi / __ycxx::__detail::__sf::__w::pi<_Wp>);
  const _Wp r = __wphi - n * __ycxx::__detail::__sf::__w::pi<_Wp>;
  const _Wp s = __ycxx::__detail::__sf::__w::sin(r), c = __ycxx::__detail::__sf::__w::cos(r);
  const _Wp __s2 = s * s, __c2 = c * c, __d2 = (1 - __wk * s) * (1 + __wk * s);
  if (__which == 3 && !(1 - __wnu * __s2 > _Wp(0))) return __ycxx::__detail::__sf::domain_error<_Tp>();
  _Wp __v;
  if (__d2 == _Wp(0) && __c2 == _Wp(0)) {
    if (__which == 2) {
      __v = s;
    } else {
      return __ycxx::__detail::__fpm::__fp_infinity<_Tp>(s < _Wp(0));
    }
  } else {
    const _Wp __rf = __ycxx::__detail::__sf::__carlson_rf(__c2, __d2, _Wp(1));
    if (__which == 1)
      __v = s * __rf;
    else if (__which == 2)
      __v = s * __rf - __wk * __wk / 3 * s * __s2 * __ycxx::__detail::__sf::__carlson_rd(__c2, __d2, _Wp(1));
    else
      __v = s * __rf + __wnu / 3 * s * __s2 * __ycxx::__detail::__sf::__carlson_rj(__c2, __d2, _Wp(1), 1 - __wnu * __s2);
  }
  if (n != _Wp(0)) {
    if (__which == 1 && __k_one) return __ycxx::__detail::__fpm::__fp_infinity<_Tp>(n < _Wp(0));
    if (__which == 3 && (__wnu >= _Wp(1) || __k_one)) return __ycxx::__detail::__sf::domain_error<_Tp>();
    __v += 2 * n * __ycxx::__detail::__sf::__ellint_complete<_Wp>(__which, __wk, __wnu);
  }
  return static_cast<_Tp>(__v);
}
template <class _Tp>
_Tp comp_ellint_1(_Tp k) noexcept {
  return __ycxx::__detail::__sf::__ellint<_Tp>(1, k, _Tp(0), _Tp(0), true);
}
template <class _Tp>
_Tp comp_ellint_2(_Tp k) noexcept {
  return __ycxx::__detail::__sf::__ellint<_Tp>(2, k, _Tp(0), _Tp(0), true);
}
template <class _Tp>
_Tp comp_ellint_3(_Tp k, _Tp __nu) noexcept {
  return __ycxx::__detail::__sf::__ellint<_Tp>(3, k, __nu, _Tp(0), true);
}
template <class _Tp>
_Tp ellint_1(_Tp k, _Tp phi) noexcept {
  return __ycxx::__detail::__sf::__ellint<_Tp>(1, k, _Tp(0), phi, false);
}
template <class _Tp>
_Tp ellint_2(_Tp k, _Tp phi) noexcept {
  return __ycxx::__detail::__sf::__ellint<_Tp>(2, k, _Tp(0), phi, false);
}
template <class _Tp>
_Tp ellint_3(_Tp k, _Tp __nu, _Tp phi) noexcept {
  return __ycxx::__detail::__sf::__ellint<_Tp>(3, k, __nu, phi, false);
}

// ---- Bessel functions -------------------------------------------------------------------------------
// Temme's auxiliary gammas for |mu| <= 1/2: gam1 = (1/G(1-mu) - 1/G(1+mu)) / (2 mu),
// gam2 = (1/G(1-mu) + 1/G(1+mu)) / 2, gplus = 1/G(1+mu), gminus = 1/G(1-mu).
template <class _Wp>
void __temme_gammas(_Wp __mu, _Wp& __gam1, _Wp& __gam2, _Wp& __gplus, _Wp& __gminus) noexcept {
  const auto& a = __ycxx::__detail::__sf::__inv_gamma_w<_Wp>.__v;
  constexpr int _Kp = sizeof(a) / sizeof(a[0]);
  _Wp __even = 0, __odd = 0; // sum of a_2j mu^2j, sum of a_(2j+1) mu^2j
  const _Wp __mu2 = __mu * __mu;
  for (int k = (_Kp - 1) & ~1; k >= 0; k -= 2) __even = __even * __mu2 + a[k];
  for (int k = ((_Kp - 2) | 1); k >= 1; k -= 2) __odd = __odd * __mu2 + a[k];
  __gam1 = -__odd;
  __gam2 = __even;
  __gplus = __even + __mu * __odd;
  __gminus = __even - __mu * __odd;
}

template <class _Wp>
_Wp __hankel_term(_Wp __nu, int k) noexcept { // a_k(nu) / x^k numerator factor helper: (4nu^2 - (2k-1)^2) / (8k)
  return (4 * __nu * __nu - _Wp(2 * k - 1) * _Wp(2 * k - 1)) / _Wp(8 * k);
}

// J_nu(x), Y_nu(x) for nu >= 0, x > 0.
template <class _Wp>
void __bessel_jy(_Wp __nu, _Wp __x, _Wp& _Jp, _Wp& _Yp) noexcept {
  const _Wp __eps = __ycxx::__detail::__sf::__w::__eps<_Wp>, __fpmin = __ycxx::__detail::__sf::__w::__tiny<_Wp>;
  const _Wp pi = __ycxx::__detail::__sf::__w::pi<_Wp>;
  if (__x > _Wp(1000) && __x > __nu * __nu / 2) {
    // Hankel: J = sqrt(2/(pi x)) (P cos w - Q sin w), Y = sqrt(2/(pi x)) (P sin w + Q cos w)
    _Wp _Pp = 1, _Qp = 0, __term = 1;
    for (int k = 1; k < 200; ++k) {
      const _Wp next = __term * __ycxx::__detail::__sf::__hankel_term(__nu, k) / __x;
      if (__ycxx::__detail::__sf::__w::abs(next) >= __ycxx::__detail::__sf::__w::abs(__term) && k > 2) break;
      __term = next;
      // a_k / x^k alternates between Q (odd k) and P (even k), with signs (-1)^(k/2)
      if (k % 2 == 1)
        _Qp += ((k / 2) % 2 == 0 ? __term : -__term);
      else
        _Pp += ((k / 2) % 2 == 0 ? __term : -__term);
      if (__ycxx::__detail::__sf::__w::abs(__term) < __eps) break;
    }
    // w = x - (nu/2 + 1/4) pi: cos w = cos x cos f + sin x sin f, f = (nu/2 + 1/4) pi
    const _Wp __f = __ycxx::__detail::__sf::__w::fmod(__nu / 2 + _Wp(0.25), _Wp(2));
    const _Wp __cf = __ycxx::__detail::__sf::__w::__cospi(__f), __sf = __ycxx::__detail::__sf::__w::__sinpi(__f);
    const _Wp __cx = __ycxx::__detail::__sf::__w::cos(__x), __sx = __ycxx::__detail::__sf::__w::sin(__x);
    const _Wp cw = __cx * __cf + __sx * __sf, __sw = __sx * __cf - __cx * __sf;
    const _Wp __amp = __ycxx::__detail::__sf::__w::sqrt(2 / (pi * __x));
    _Jp = __amp * (_Pp * cw - _Qp * __sw);
    _Yp = __amp * (_Pp * __sw + _Qp * cw);
    return;
  }
  const int __nl = __x < _Wp(2) ? static_cast<int>(__nu + _Wp(0.5)) : (__nu - __x + _Wp(1.5) > _Wp(0) ? static_cast<int>(__nu - __x + _Wp(1.5)) : 0);
  const _Wp __mu = __nu - __nl, __mu2 = __mu * __mu, __xi = 1 / __x, __xi2 = 2 * __xi, __wr = __xi2 / pi;
  // CF1: h = J'_nu / J_nu (modified Lentz); isign tracks the sign of J_nu relative to the start.
  int __isign = 1;
  _Wp h = __nu * __xi;
  if (h < __fpmin) h = __fpmin;
  _Wp b = __xi2 * __nu, d = 0, c = h;
  for (long i = 1; i < 2000000; ++i) {
    b += __xi2;
    d = b - d;
    if (__ycxx::__detail::__sf::__w::abs(d) < __fpmin) d = __fpmin;
    c = b - 1 / c;
    if (__ycxx::__detail::__sf::__w::abs(c) < __fpmin) c = __fpmin;
    d = 1 / d;
    const _Wp __del = c * d;
    h *= __del;
    if (d < _Wp(0)) __isign = -__isign;
    if (__ycxx::__detail::__sf::__w::abs(__del - 1) < __eps) break;
  }
  // Downward recurrence from nu to mu, unnormalised.
  _Wp __rjl = __isign * __fpmin, __rjpl = h * __rjl;
  const _Wp __rjl1 = __rjl;
  _Wp __fact = __nu * __xi;
  for (int __l = __nl; __l >= 1; --__l) {
    const _Wp t = __fact * __rjl + __rjpl;
    __fact -= __xi;
    __rjpl = __fact * t - __rjl;
    __rjl = t;
  }
  if (__rjl == _Wp(0)) __rjl = __eps;
  const _Wp __f = __rjpl / __rjl;
  _Wp __rjmu, __rymu, __ry1;
  if (__x < _Wp(2)) {
    // Temme's series for Y_mu and Y_(mu+1).
    const _Wp __x2 = __x / 2, __pimu = pi * __mu;
    const _Wp __fct = __ycxx::__detail::__sf::__w::abs(__pimu) < __eps ? _Wp(1) : __pimu / __ycxx::__detail::__sf::__w::sin(__pimu);
    _Wp __dd = -__ycxx::__detail::__sf::__w::log(__x2);
    _Wp e = __mu * __dd;
    const _Wp __fct2 = __ycxx::__detail::__sf::__w::abs(e) < __eps ? _Wp(1) : __ycxx::__detail::__sf::__w::sinh(e) / e;
    _Wp __gam1, __gam2, __gpl, __gmi;
    __ycxx::__detail::__sf::__temme_gammas(__mu, __gam1, __gam2, __gpl, __gmi);
    _Wp __ff = 2 / pi * __fct * (__gam1 * __ycxx::__detail::__sf::__w::cosh(e) + __gam2 * __fct2 * __dd);
    e = __ycxx::__detail::__sf::__w::exp(e);
    _Wp p = e / (__gpl * pi);   // (x/2)^-mu Gamma(1 + mu) / pi
    _Wp __q = 1 / (e * pi * __gmi); // (x/2)^mu Gamma(1 - mu) / pi
    const _Wp __pimu2 = __pimu / 2;
    const _Wp __fct3 = __ycxx::__detail::__sf::__w::abs(__pimu2) < __eps ? _Wp(1) : __ycxx::__detail::__sf::__w::sin(__pimu2) / __pimu2;
    const _Wp r = pi * __pimu2 * __fct3 * __fct3;
    _Wp __cc = 1;
    __dd = -__x2 * __x2;
    _Wp sum = __ff + r * __q, __sum1 = p;
    for (int i = 1; i < 100000; ++i) {
      __ff = (i * __ff + p + __q) / (_Wp(i) * _Wp(i) - __mu2);
      __cc *= __dd / i;
      p /= _Wp(i) - __mu;
      __q /= _Wp(i) + __mu;
      const _Wp __del = __cc * (__ff + r * __q);
      sum += __del;
      __sum1 += __cc * p - i * __del;
      if (__ycxx::__detail::__sf::__w::abs(__del) < (1 + __ycxx::__detail::__sf::__w::abs(sum)) * __eps) break;
    }
    __rymu = -sum;
    __ry1 = -__sum1 * __xi2;
    const _Wp __rymup = __mu * __xi * __rymu - __ry1;
    __rjmu = __wr / (__rymup - __f * __rymu);
  } else {
    // Steed's complex continued fraction p + iq = (J' + iY') / (J + iY).
    _Wp a = _Wp(0.25) - __mu2, p = -__xi / 2, __q = 1;
    const _Wp __br = 2 * __x;
    _Wp __bi = 2;
    _Wp __fct = a * __xi / (p * p + __q * __q);
    _Wp __cr = __br + __q * __fct, __ci = __bi + p * __fct;
    _Wp den = __br * __br + __bi * __bi;
    _Wp __dr = __br / den, __di = -__bi / den;
    _Wp __dlr = __cr * __dr - __ci * __di, __dli = __cr * __di + __ci * __dr;
    _Wp t = p * __dlr - __q * __dli;
    __q = p * __dli + __q * __dlr;
    p = t;
    for (int i = 2; i < 100000; ++i) {
      a += 2 * (i - 1);
      __bi += 2;
      __dr = a * __dr + __br;
      __di = a * __di + __bi;
      if (__ycxx::__detail::__sf::__w::abs(__dr) + __ycxx::__detail::__sf::__w::abs(__di) < __fpmin) __dr = __fpmin;
      __fct = a / (__cr * __cr + __ci * __ci);
      __cr = __br + __cr * __fct;
      __ci = __bi - __ci * __fct;
      if (__ycxx::__detail::__sf::__w::abs(__cr) + __ycxx::__detail::__sf::__w::abs(__ci) < __fpmin) __cr = __fpmin;
      den = __dr * __dr + __di * __di;
      __dr /= den;
      __di /= -den;
      __dlr = __cr * __dr - __ci * __di;
      __dli = __cr * __di + __ci * __dr;
      t = p * __dlr - __q * __dli;
      __q = p * __dli + __q * __dlr;
      p = t;
      if (__ycxx::__detail::__sf::__w::abs(__dlr - 1) + __ycxx::__detail::__sf::__w::abs(__dli) < __eps) break;
    }
    const _Wp __gam = (p - __f) / __q;
    __rjmu = __ycxx::__detail::__sf::__w::sqrt(__wr / ((p - __f) * __gam + __q));
    if (__rjl < _Wp(0)) __rjmu = -__rjmu;
    __rymu = __rjmu * __gam;
    const _Wp __rymup = __rymu * (p + __q / __gam);
    __ry1 = __mu * __xi * __rymu - __rymup;
  }
  _Jp = __rjl1 * (__rjmu / __rjl);
  for (int i = 1; i <= __nl; ++i) {
    const _Wp t = (__mu + i) * __xi2 * __ry1 - __rymu;
    __rymu = __ry1;
    __ry1 = t;
  }
  _Yp = __rymu;
}

// I_nu(x), K_nu(x) for nu >= 0, x > 0.
template <class _Wp>
void __bessel_ik(_Wp __nu, _Wp __x, _Wp& _Ip, _Wp& _Kp) noexcept {
  const _Wp __eps = __ycxx::__detail::__sf::__w::__eps<_Wp>, __fpmin = __ycxx::__detail::__sf::__w::__tiny<_Wp>;
  const _Wp pi = __ycxx::__detail::__sf::__w::pi<_Wp>;
  if (__x > _Wp(1000) && __x > __nu * __nu / 2) {
    // K ~ sqrt(pi/(2x)) e^-x sum a_k / x^k; I ~ e^x / sqrt(2 pi x) sum (-1)^k a_k / x^k
    _Wp __sk = 1, __si = 1, __term = 1;
    for (int k = 1; k < 200; ++k) {
      const _Wp next = __term * __ycxx::__detail::__sf::__hankel_term(__nu, k) / __x;
      if (__ycxx::__detail::__sf::__w::abs(next) >= __ycxx::__detail::__sf::__w::abs(__term) && k > 2) break;
      __term = next;
      __sk += __term;
      __si += (k % 2 == 0 ? __term : -__term);
      if (__ycxx::__detail::__sf::__w::abs(__term) < __eps) break;
    }
    _Kp = __ycxx::__detail::__sf::__w::sqrt(pi / (2 * __x)) * __ycxx::__detail::__sf::__w::exp(-__x) * __sk;
    const _Wp __ehalf = __ycxx::__detail::__sf::__w::exp(__x / 2); // e^x in two halves: finite as long as I is
    _Ip = __ehalf / __ycxx::__detail::__sf::__w::sqrt(2 * pi * __x) * __si * __ehalf;
    return;
  }
  const int __nl = static_cast<int>(__nu + _Wp(0.5));
  const _Wp __mu = __nu - __nl, __mu2 = __mu * __mu, __xi = 1 / __x, __xi2 = 2 * __xi;
  // CF1: h = I'_nu / I_nu
  _Wp h = __nu * __xi;
  if (h < __fpmin) h = __fpmin;
  _Wp b = __xi2 * __nu, d = 0, c = h;
  for (long i = 1; i < 2000000; ++i) {
    b += __xi2;
    d = 1 / (b + d);
    c = b + 1 / c;
    const _Wp __del = c * d;
    h *= __del;
    if (__ycxx::__detail::__sf::__w::abs(__del - 1) < __eps) break;
  }
  _Wp __ril = __fpmin, __ripl = h * __ril;
  const _Wp __ril1 = __ril;
  _Wp __fact = __nu * __xi;
  for (int __l = __nl; __l >= 1; --__l) {
    const _Wp t = __fact * __ril + __ripl;
    __fact -= __xi;
    __ripl = __fact * t + __ril;
    __ril = t;
  }
  const _Wp __f = __ripl / __ril;
  _Wp __rkmu, __rk1;
  if (__x < _Wp(2)) {
    const _Wp __x2 = __x / 2, __pimu = pi * __mu;
    const _Wp __fct = __ycxx::__detail::__sf::__w::abs(__pimu) < __eps ? _Wp(1) : __pimu / __ycxx::__detail::__sf::__w::sin(__pimu);
    _Wp __dd = -__ycxx::__detail::__sf::__w::log(__x2);
    _Wp e = __mu * __dd;
    const _Wp __fct2 = __ycxx::__detail::__sf::__w::abs(e) < __eps ? _Wp(1) : __ycxx::__detail::__sf::__w::sinh(e) / e;
    _Wp __gam1, __gam2, __gpl, __gmi;
    __ycxx::__detail::__sf::__temme_gammas(__mu, __gam1, __gam2, __gpl, __gmi);
    _Wp __ff = __fct * (__gam1 * __ycxx::__detail::__sf::__w::cosh(e) + __gam2 * __fct2 * __dd);
    _Wp sum = __ff;
    e = __ycxx::__detail::__sf::__w::exp(e);
    _Wp p = e / (2 * __gpl);   // (x/2)^-mu Gamma(1 + mu) / 2
    _Wp __q = 1 / (2 * e * __gmi); // (x/2)^mu Gamma(1 - mu) / 2
    _Wp __cc = 1;
    __dd = __x2 * __x2;
    _Wp __sum1 = p;
    for (int i = 1; i < 100000; ++i) {
      __ff = (i * __ff + p + __q) / (_Wp(i) * _Wp(i) - __mu2);
      __cc *= __dd / i;
      p /= _Wp(i) - __mu;
      __q /= _Wp(i) + __mu;
      const _Wp __del = __cc * __ff;
      sum += __del;
      __sum1 += __cc * (p - i * __ff);
      if (__ycxx::__detail::__sf::__w::abs(__del) < __ycxx::__detail::__sf::__w::abs(sum) * __eps) break;
    }
    __rkmu = sum;
    __rk1 = __sum1 * __xi2;
  } else {
    // Steed's continued fraction for K (Thompson and Barnett).
    _Wp __bb = 2 * (1 + __x), __dd = 1 / __bb, __hh = __dd, __delh = __dd;
    _Wp __q1 = 0, __q2 = 1;
    const _Wp __a1 = _Wp(0.25) - __mu2;
    _Wp __q = __a1, __cc = __a1, a = -__a1;
    _Wp s = 1 + __q * __delh;
    for (int i = 2; i < 100000; ++i) {
      a -= 2 * (i - 1);
      __cc = -a * __cc / i;
      const _Wp __qnew = (__q1 - __bb * __q2) / a;
      __q1 = __q2;
      __q2 = __qnew;
      __q += __cc * __qnew;
      __bb += 2;
      __dd = 1 / (__bb + a * __dd);
      __delh = (__bb * __dd - 1) * __delh;
      __hh += __delh;
      const _Wp __dels = __q * __delh;
      s += __dels;
      if (__ycxx::__detail::__sf::__w::abs(__dels / s) < __eps) break;
    }
    __hh = __a1 * __hh;
    __rkmu = __ycxx::__detail::__sf::__w::sqrt(pi / (2 * __x)) * __ycxx::__detail::__sf::__w::exp(-__x) / s;
    __rk1 = __rkmu * (__mu + __x + _Wp(0.5) - __hh) * __xi;
  }
  const _Wp __rkmup = __mu * __xi * __rkmu - __rk1;
  const _Wp __rimu = __xi / (__f * __rkmu - __rkmup);
  _Ip = __rimu * __ril1 / __ril;
  for (int i = 1; i <= __nl; ++i) {
    const _Wp t = (__mu + i) * __xi2 * __rk1 + __rkmu;
    __rkmu = __rk1;
    __rk1 = t;
  }
  _Kp = __rkmu;
}

// which: 0 J, 1 Y (Neumann), 2 I, 3 K
template <class _Tp>
_Tp __cyl_bessel(int __which, _Tp __nu, _Tp __x) noexcept {
  using _Wp = __work_t<_Tp>;
  if (__builtin_isnan(__nu) || __builtin_isnan(__x)) return __ycxx::__detail::__sf::__nan_result<_Tp>();
  if (__x < _Tp(0) || __builtin_isinf(__nu)) return __ycxx::__detail::__sf::domain_error<_Tp>();
  const _Wp __wnu = __ycxx::__detail::__sf::__w::abs(_Wp(__nu)), __wx = __x;
  const bool __neg = __nu < _Tp(0);
  const bool integral = __ycxx::__detail::__sf::__w::floor(__wnu) == __wnu;
  if (__x == _Tp(0)) {
    switch (__which) {
    case 0:
    case 2:
      if (__nu == _Tp(0)) return _Tp(1);
      if (!__neg || integral) return _Tp(0);
      return __ycxx::__detail::__sf::domain_error<_Tp>(); // |J_nu(0)| = inf for negative non-integral nu
    case 1:
      return __ycxx::__detail::__fpm::__fp_infinity<_Tp>(true);
    default:
      return __ycxx::__detail::__fpm::__fp_infinity<_Tp>(false);
    }
  }
  if (__builtin_isinf(__x)) {
    if (__which == 2) return __ycxx::__detail::__fpm::__fp_infinity<_Tp>(false);
    return _Tp(0);
  }
  _Wp a, b;
  if (__which <= 1) {
    __ycxx::__detail::__sf::__bessel_jy(__wnu, __wx, a, b);
    if (__neg) {
      // J_-nu = cos(nu pi) J_nu - sin(nu pi) Y_nu, Y_-nu = sin(nu pi) J_nu + cos(nu pi) Y_nu
      const _Wp c = __ycxx::__detail::__sf::__w::__cospi(__wnu), s = __ycxx::__detail::__sf::__w::__sinpi(__wnu);
      const _Wp __j = c * a - s * b, y = s * a + c * b;
      a = __j;
      b = y;
    }
    return static_cast<_Tp>(__which == 0 ? a : b);
  }
  __ycxx::__detail::__sf::__bessel_ik(__wnu, __wx, a, b);
  if (__which == 3) return static_cast<_Tp>(b);
  if (__neg && !integral) a += 2 / __ycxx::__detail::__sf::__w::pi<_Wp> * __ycxx::__detail::__sf::__w::__sinpi(__wnu) * b;
  return static_cast<_Tp>(a);
}
template <class _Tp>
_Tp cyl_bessel_j(_Tp __nu, _Tp __x) noexcept {
  return __ycxx::__detail::__sf::__cyl_bessel<_Tp>(0, __nu, __x);
}
template <class _Tp>
_Tp cyl_neumann(_Tp __nu, _Tp __x) noexcept {
  return __ycxx::__detail::__sf::__cyl_bessel<_Tp>(1, __nu, __x);
}
template <class _Tp>
_Tp cyl_bessel_i(_Tp __nu, _Tp __x) noexcept {
  return __ycxx::__detail::__sf::__cyl_bessel<_Tp>(2, __nu, __x);
}
template <class _Tp>
_Tp cyl_bessel_k(_Tp __nu, _Tp __x) noexcept {
  return __ycxx::__detail::__sf::__cyl_bessel<_Tp>(3, __nu, __x);
}
template <class _Tp>
_Tp __sph_bessel_impl(bool __neumann, unsigned n, _Tp __x) noexcept {
  using _Wp = __work_t<_Tp>;
  if (__builtin_isnan(__x)) return __x;
  if (__x < _Tp(0)) return __ycxx::__detail::__sf::domain_error<_Tp>();
  if (__x == _Tp(0)) {
    if (__neumann) return __ycxx::__detail::__fpm::__fp_infinity<_Tp>(true);
    return n == 0 ? _Tp(1) : _Tp(0);
  }
  if (__builtin_isinf(__x)) return _Tp(0);
  const _Wp __wx = __x;
  _Wp __j, y;
  __ycxx::__detail::__sf::__bessel_jy(_Wp(n) + _Wp(0.5), __wx, __j, y);
  const _Wp __f = __ycxx::__detail::__sf::__w::sqrt(__ycxx::__detail::__sf::__w::pi<_Wp> / (2 * __wx));
  return static_cast<_Tp>(__f * (__neumann ? y : __j));
}
template <class _Tp>
_Tp sph_bessel(unsigned n, _Tp __x) noexcept {
  return __ycxx::__detail::__sf::__sph_bessel_impl<_Tp>(false, n, __x);
}
template <class _Tp>
_Tp sph_neumann(unsigned n, _Tp __x) noexcept {
  return __ycxx::__detail::__sf::__sph_bessel_impl<_Tp>(true, n, __x);
}

// ---- exponential integral ([sf.cmath.expint]) -------------------------------------------------------
template <class _Tp>
_Tp expint(_Tp __x) noexcept {
  using _Wp = __work_t<_Tp>;
  if (__builtin_isnan(__x)) return __x;
  if (__x == _Tp(0)) return __ycxx::__detail::__fpm::__fp_infinity<_Tp>(true);
  if (__builtin_isinf(__x)) return __x > _Tp(0) ? __x : _Tp(0);
  const _Wp __eps = __ycxx::__detail::__sf::__w::__eps<_Wp>, __wx = __x;
  if (__x > _Tp(0)) {
    const _Wp big = _Wp(__ycxx::__detail::__fp_format<_Wp>.digits) * _Wp(0.75); // asymptotic series accurate beyond
    if (__wx > big) {
      _Wp sum = 1, __term = 1;
      for (int k = 1; k < 1000; ++k) {
        const _Wp next = __term * k / __wx;
        if (next >= __term) break;
        __term = next;
        sum += __term;
        if (__term < __eps * sum) break;
      }
      return static_cast<_Tp>(__ycxx::__detail::__sf::__w::exp(__wx) / __wx * sum);
    }
    _Wp sum = 0, __term = 1;
    for (int k = 1; k < 100000; ++k) {
      __term *= __wx / k;
      const _Wp t = __term / k;
      sum += t;
      if (t < __eps * sum) break;
    }
    return static_cast<_Tp>(__ycxx::__detail::__sf::__w::egamma<_Wp> + __ycxx::__detail::__sf::__w::log(__wx) + sum);
  }
  // Ei(x) = -E1(-x)
  const _Wp t = -__wx;
  if (t <= _Wp(1)) {
    _Wp sum = 0, __term = 1;
    for (int k = 1; k < 100000; ++k) {
      __term *= -t / k;
      const _Wp s = __term / k;
      sum += s;
      if (__ycxx::__detail::__sf::__w::abs(s) < __eps * __ycxx::__detail::__sf::__w::abs(sum)) break;
    }
    const _Wp __e1 = -__ycxx::__detail::__sf::__w::egamma<_Wp> - __ycxx::__detail::__sf::__w::log(t) - sum;
    return static_cast<_Tp>(-__e1);
  }
  // E1(t) = e^-t / (t + 1 - 1/(t + 3 - 4/(t + 5 - ...))) (modified Lentz)
  const _Wp __fpmin = __ycxx::__detail::__sf::__w::__tiny<_Wp>;
  _Wp b = t + 1, c = 1 / __fpmin, d = 1 / b, h = d;
  for (int i = 1; i < 100000; ++i) {
    const _Wp __an = -_Wp(i) * _Wp(i);
    b += 2;
    d = 1 / (__an * d + b);
    c = b + __an / c;
    const _Wp __del = c * d;
    h *= __del;
    if (__ycxx::__detail::__sf::__w::abs(__del - 1) < __eps) break;
  }
  return static_cast<_Tp>(-h * __ycxx::__detail::__sf::__w::exp(-t));
}

// ---- Riemann zeta ([sf.cmath.riemann.zeta]) ---------------------------------------------------------
template <class _Wp>
_Wp __zeta_em(_Wp s) noexcept { // Euler-Maclaurin, s >= 1/2, s != 1
  constexpr int _Np = 24;
  const auto& c = __ycxx::__detail::__sf::__stirling_w<_Wp>.__v; // B_2j / (2j (2j-1))
  constexpr int _Jp = sizeof(c) / sizeof(c[0]);
  const _Wp __eps = __ycxx::__detail::__sf::__w::__eps<_Wp>;
  _Wp sum = 0;
  for (int k = _Np - 1; k >= 1; --k) sum += __ycxx::__detail::__sf::__w::pow(_Wp(k), -s);
  const _Wp _Np_ = __ycxx::__detail::__sf::__w::pow(_Wp(_Np), -s);
  sum += _Np_ * _Wp(_Np) / (s - 1) + _Np_ / 2;
  // sum_j B_2j / (2j)! s (s+1) ... (s + 2j - 2) N^(-s - 2j + 1)
  _Wp __poch = s;            // s (s+1) ... (s+2j-2)
  _Wp __npow = _Np_ / _Wp(_Np);    // N^(-s-1)
  _Wp __fact = 1;            // (2j - 2)!
  for (int __j = 1; __j <= _Jp; ++__j) {
    const _Wp __term = c[__j - 1] / __fact * __poch * __npow;
    sum += __term;
    if (__ycxx::__detail::__sf::__w::abs(__term) < __eps * __ycxx::__detail::__sf::__w::abs(sum)) break;
    __poch *= (s + 2 * __j - 1) * (s + 2 * __j);
    __npow /= _Wp(_Np) * _Wp(_Np);
    __fact *= _Wp(2 * __j - 1) * _Wp(2 * __j);
  }
  return sum;
}
template <class _Tp>
_Tp riemann_zeta(_Tp __x) noexcept {
  using _Wp = __work_t<_Tp>;
  if (__builtin_isnan(__x)) return __x;
  if (__x == _Tp(1) || __x == -__ycxx::__detail::__fpm::__fp_infinity<_Tp>(false)) return __ycxx::__detail::__sf::domain_error<_Tp>();
  if (__builtin_isinf(__x)) return _Tp(1);
  if (__x == _Tp(0)) return _Tp(-0.5);
  const _Wp s = __x;
  if (s >= _Wp(0.5)) return static_cast<_Tp>(__ycxx::__detail::__sf::__zeta_em(s));
  // zeta(s) = 2^s pi^(s-1) sin(pi s / 2) Gamma(1 - s) zeta(1 - s)
  const _Wp __half = s / 2;
  if (__half == __ycxx::__detail::__sf::__w::floor(__half)) return _Tp(0); // the trivial zeros
  const _Wp __sn = __ycxx::__detail::__sf::__w::__sinpi(__half);
  const _Wp __z1 = __ycxx::__detail::__sf::__zeta_em(1 - s);
  const _Wp pi = __ycxx::__detail::__sf::__w::pi<_Wp>;
  if (1 - s < _Wp(150)) {
    return static_cast<_Tp>(__ycxx::__detail::__sf::__w::pow(_Wp(2), s) * __ycxx::__detail::__sf::__w::pow(pi, s - 1) * __sn *
                          __ycxx::__detail::__sf::__w::tgamma(1 - s) * __z1);
  }
  const _Wp __lg = s * __ycxx::__detail::__sf::__w::log(2 * pi) - __ycxx::__detail::__sf::__w::log(pi) + __ycxx::__detail::__sf::__w::log(__ycxx::__detail::__sf::__w::abs(__sn)) +
               __ycxx::__detail::__sf::__w::lgamma(1 - s) + __ycxx::__detail::__sf::__w::log(__z1);
  const _Wp r = __ycxx::__detail::__sf::__w::exp(__lg);
  return static_cast<_Tp>(__sn < _Wp(0) ? -r : r);
}

}} // namespace __ycxx::__detail::__sf
