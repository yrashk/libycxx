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

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::sf {

template <class T>
consteval auto work_of() {
  using C = ycxx::detail::cm::carrier_t<T>;
  if constexpr (fp_format<T>.digits <= 24)
    return std::type_identity<double>{};
  else if constexpr (fp_format<T>.digits <= 53 && fp_format<long double>.digits > 53)
    return std::type_identity<long double>{};
  else
    return std::type_identity<C>{};
}
template <class T>
using work_t = typename decltype(ycxx::detail::sf::work_of<T>())::type;

template <class T>
[[gnu::cold]] T domain_error() noexcept {
  volatile double minus_one = -1.0;
  volatile double r = __builtin_sqrt(minus_one); // EDOM and FE_INVALID, as the C library does
  (void)r;
  return ycxx::detail::fpm::fp_quiet_nan<T>();
}
template <class T>
T nan_result() noexcept {
  return ycxx::detail::fpm::fp_quiet_nan<T>();
}

// ---- elementary functions of the working type (run time) ------------------------------------------
namespace w {
template <class W>
W sqrt(W x) noexcept {
  return ycxx::detail::cm::sqrt<W>(x);
}
template <class W>
W abs(W x) noexcept {
  return ycxx::detail::fpm::fp_abs(x);
}
template <class W>
W exp(W x) noexcept {
  return ycxx::detail::cm::transcendental<ycxx::detail::cm::op::exp, W>(x);
}
template <class W>
W log(W x) noexcept {
  return ycxx::detail::cm::transcendental<ycxx::detail::cm::op::log, W>(x);
}
template <class W>
W sin(W x) noexcept {
  return ycxx::detail::cm::transcendental<ycxx::detail::cm::op::sin, W>(x);
}
template <class W>
W cos(W x) noexcept {
  return ycxx::detail::cm::transcendental<ycxx::detail::cm::op::cos, W>(x);
}
template <class W>
W sinh(W x) noexcept {
  return ycxx::detail::cm::transcendental<ycxx::detail::cm::op::sinh, W>(x);
}
template <class W>
W cosh(W x) noexcept {
  return ycxx::detail::cm::transcendental<ycxx::detail::cm::op::cosh, W>(x);
}
template <class W>
W pow(W x, W y) noexcept {
  return ycxx::detail::cm::transcendental<ycxx::detail::cm::op::pow, W>(x, y);
}
template <class W>
W lgamma(W x) noexcept {
  return ycxx::detail::cm::transcendental<ycxx::detail::cm::op::lgamma, W>(x);
}
template <class W>
W tgamma(W x) noexcept {
  return ycxx::detail::cm::transcendental<ycxx::detail::cm::op::tgamma, W>(x);
}
template <class W>
W floor(W x) noexcept {
  return ycxx::detail::cm::round_integral<ycxx::detail::cm::rint_op::floor, W>(x);
}
template <class W>
W round(W x) noexcept {
  return ycxx::detail::cm::round_integral<ycxx::detail::cm::rint_op::round, W>(x);
}
template <class W>
W fmod(W x, W y) noexcept {
  return ycxx::detail::cm::fmod<W>(x, y);
}
template <class W>
inline constexpr W pi = ycxx::detail::math_constant_value<W>(math_constant::pi);
template <class W>
inline constexpr W egamma = ycxx::detail::math_constant_value<W>(math_constant::egamma);
template <class W>
inline constexpr W eps = W(1) / ycxx::detail::fpm::fp_scale(W(1), ycxx::detail::fp_format<W>.digits - 1);
template <class W>
inline constexpr W tiny = ycxx::detail::fpm::fp_scale(W(1), ycxx::detail::fp_format<W>.min_exp + 16);
// sin(pi x), cos(pi x) with x reduced exactly first.
template <class W>
W sinpi(W x) noexcept {
  const W r = ycxx::detail::sf::w::fmod(x, W(2)); // exact
  if (r == W(0) || r == W(1) || r == W(-1)) return W(0);
  return ycxx::detail::sf::w::sin(ycxx::detail::sf::w::pi<W> * r);
}
template <class W>
W cospi(W x) noexcept {
  const W r = ycxx::detail::sf::w::fmod(ycxx::detail::sf::w::abs(x), W(2));
  if (r == W(0.5) || r == W(1.5)) return W(0);
  return ycxx::detail::sf::w::cos(ycxx::detail::sf::w::pi<W> * r);
}
} // namespace w

template <class W, int K>
struct w_table {
  W v[K];
};
template <class W, int K>
consteval w_table<W, K> make_w_table(const ycxx::detail::fpm::mp_const_bits (&src)[K]) {
  w_table<W, K> t{};
  for (int i = 0; i < K; ++i)
    t.v[i] = ycxx::detail::fpm::mp_round<W>(ycxx::detail::fpm::mp_const<ycxx::detail::fpm::mp_limbs<W> + 1>(src[i])).value;
  return t;
}
template <class W>
inline constexpr auto inv_gamma_w = ycxx::detail::sf::make_w_table<W>(ycxx::detail::fpm::inv_gamma_taylor);
template <class W>
inline constexpr auto stirling_w = ycxx::detail::sf::make_w_table<W>(ycxx::detail::fpm::stirling_coefficients);

// ---- polynomials ([sf.cmath.hermite], [sf.cmath.laguerre], [sf.cmath.assoc.laguerre]) ------------
template <class T>
T hermite(unsigned n, T x) noexcept {
  using W = work_t<T>;
  if (__builtin_isnan(x)) return x;
  if (n == 0) return T(1);
  if (__builtin_isinf(x)) return (x < T(0) && n % 2 == 1) ? x : ycxx::detail::fpm::fp_abs(x); // the sign of x^n
  const W wx = x;
  W h0 = 1, h1 = 2 * wx;
  for (unsigned k = 1; k < n; ++k) {
    const W h2 = 2 * wx * h1 - 2 * W(k) * h0;
    h0 = h1;
    h1 = h2;
  }
  return static_cast<T>(h1);
}
template <class T>
T assoc_laguerre(unsigned n, unsigned m, T x) noexcept {
  using W = work_t<T>;
  if (__builtin_isnan(x)) return x;
  if (x < T(0)) return ycxx::detail::sf::domain_error<T>();
  const W wx = x, wm = m;
  W l0 = 1, l1 = 1 + wm - wx;
  if (n == 0) return T(1);
  for (unsigned k = 1; k < n; ++k) {
    const W l2 = ((2 * W(k) + 1 + wm - wx) * l1 - (W(k) + wm) * l0) / W(k + 1);
    l0 = l1;
    l1 = l2;
  }
  return static_cast<T>(l1);
}
template <class T>
T laguerre(unsigned n, T x) noexcept {
  return ycxx::detail::sf::assoc_laguerre<T>(n, 0, x);
}

// ---- Legendre ([sf.cmath.legendre], [sf.cmath.assoc.legendre], [sf.cmath.sph.legendre]) -----------
template <class T>
T assoc_legendre(unsigned l, unsigned m, T x) noexcept {
  using W = work_t<T>;
  if (__builtin_isnan(x)) return x;
  if (x > T(1) || x < T(-1)) return ycxx::detail::sf::domain_error<T>();
  if (m > l) return T(0);
  const W wx = x;
  // P_m^m = (2m - 1)!! (1 - x^2)^(m/2) (no Condon-Shortley phase, Formula 29.23)
  W pmm = 1;
  if (m > 0) {
    const W s = ycxx::detail::sf::w::sqrt((1 - wx) * (1 + wx));
    W f = 1;
    for (unsigned i = 1; i <= m; ++i) {
      pmm *= f * s;
      f += 2;
    }
  }
  if (l == m) return static_cast<T>(pmm);
  W p1 = wx * W(2 * m + 1) * pmm;
  W p0 = pmm;
  for (unsigned k = m + 1; k < l; ++k) {
    const W p2 = (W(2 * k + 1) * wx * p1 - W(k + m) * p0) / W(k + 1 - m);
    p0 = p1;
    p1 = p2;
  }
  return static_cast<T>(p1);
}
template <class T>
T legendre(unsigned l, T x) noexcept {
  return ycxx::detail::sf::assoc_legendre<T>(l, 0, x);
}
template <class T>
T sph_legendre(unsigned l, unsigned m, T theta) noexcept {
  using W = work_t<T>;
  if (__builtin_isnan(theta)) return theta;
  if (m > l) return T(0);
  const W th = theta;
  const W c = ycxx::detail::sf::w::cos(th), s = ycxx::detail::sf::w::sin(th);
  // Y_m^m = (-1)^m sqrt((2m + 1)/(4 pi) (2m - 1)!! / (2m)!!) sin^m(theta), built factor by factor.
  W y = 1 / ycxx::detail::sf::w::sqrt(4 * ycxx::detail::sf::w::pi<W>);
  for (unsigned i = 1; i <= m; ++i) y *= -ycxx::detail::sf::w::sqrt(W(2 * i + 1) / W(2 * i)) * s;
  if (l == m) return static_cast<T>(y);
  W y1 = c * ycxx::detail::sf::w::sqrt(W(2 * m + 3)) * y;
  W y0 = y;
  for (unsigned k = m + 2; k <= l; ++k) {
    const W a = ycxx::detail::sf::w::sqrt(W(4 * k * k - 1) / W((k - m) * (k + m)));
    const W b = ycxx::detail::sf::w::sqrt(W(2 * k + 1) * W(k + m - 1) * W(k - m - 1) / (W(2 * k - 3) * W(k - m) * W(k + m)));
    const W y2 = a * c * y1 - b * y0;
    y0 = y1;
    y1 = y2;
  }
  return static_cast<T>(y1);
}

// ---- beta ([sf.cmath.beta]) -------------------------------------------------------------------------
template <class T>
T beta(T x, T y) noexcept {
  using W = work_t<T>;
  if (__builtin_isnan(x) || __builtin_isnan(y)) return ycxx::detail::sf::nan_result<T>();
  if (!(x > T(0)) || !(y > T(0))) return ycxx::detail::sf::domain_error<T>();
  const W a = x, b = y;
  const W g = ycxx::detail::sf::w::tgamma(a + b);
  if (__builtin_isfinite(g)) {
    const W ga = ycxx::detail::sf::w::tgamma(a), gb = ycxx::detail::sf::w::tgamma(b);
    const W r = a > b ? ga / g * gb : gb / g * ga;
    if (__builtin_isfinite(r) && r != W(0)) return static_cast<T>(r);
  }
  return static_cast<T>(ycxx::detail::sf::w::exp(ycxx::detail::sf::w::lgamma(a) + ycxx::detail::sf::w::lgamma(b) -
                                                  ycxx::detail::sf::w::lgamma(a + b)));
}

// ---- Carlson's symmetric elliptic integrals -------------------------------------------------------
template <class W>
W carlson_tol() noexcept {
  return ycxx::detail::sf::w::pow(4 * ycxx::detail::sf::w::eps<W>, W(1) / W(6));
}
template <class W>
W carlson_rc(W x, W y) noexcept { // y > 0
  const W tol = ycxx::detail::sf::carlson_tol<W>();
  W ave, s;
  for (int i = 0; i < 200; ++i) {
    const W lam = 2 * ycxx::detail::sf::w::sqrt(x) * ycxx::detail::sf::w::sqrt(y) + y;
    x = (x + lam) / 4;
    y = (y + lam) / 4;
    ave = (x + y + y) / 3;
    s = (y - ave) / ave;
    if (ycxx::detail::sf::w::abs(s) < tol) break;
  }
  return (1 + s * s * (W(3) / 10 + s * (W(1) / 7 + s * (W(3) / 8 + s * W(9) / 22)))) / ycxx::detail::sf::w::sqrt(ave);
}
template <class W>
W carlson_rf(W x, W y, W z) noexcept {
  const W tol = ycxx::detail::sf::carlson_tol<W>();
  W ave = 0, dx = 0, dy = 0, dz = 0;
  for (int i = 0; i < 200; ++i) {
    const W sx = ycxx::detail::sf::w::sqrt(x), sy = ycxx::detail::sf::w::sqrt(y), sz = ycxx::detail::sf::w::sqrt(z);
    const W lam = sx * (sy + sz) + sy * sz;
    x = (x + lam) / 4;
    y = (y + lam) / 4;
    z = (z + lam) / 4;
    ave = (x + y + z) / 3;
    dx = (ave - x) / ave;
    dy = (ave - y) / ave;
    dz = (ave - z) / ave;
    if (ycxx::detail::sf::w::abs(dx) < tol && ycxx::detail::sf::w::abs(dy) < tol && ycxx::detail::sf::w::abs(dz) < tol) break;
  }
  const W e2 = dx * dy - dz * dz, e3 = dx * dy * dz;
  return (1 + (e2 / 24 - W(1) / 10 - W(3) * e3 / 44) * e2 + e3 / 14) / ycxx::detail::sf::w::sqrt(ave);
}
template <class W>
W carlson_rd(W x, W y, W z) noexcept {
  const W tol = ycxx::detail::sf::carlson_tol<W>();
  W sum = 0, fac = 1, ave = 0, dx = 0, dy = 0, dz = 0;
  for (int i = 0; i < 200; ++i) {
    const W sx = ycxx::detail::sf::w::sqrt(x), sy = ycxx::detail::sf::w::sqrt(y), sz = ycxx::detail::sf::w::sqrt(z);
    const W lam = sx * (sy + sz) + sy * sz;
    sum += fac / (sz * (z + lam));
    fac /= 4;
    x = (x + lam) / 4;
    y = (y + lam) / 4;
    z = (z + lam) / 4;
    ave = (x + y + 3 * z) / 5;
    dx = (ave - x) / ave;
    dy = (ave - y) / ave;
    dz = (ave - z) / ave;
    if (ycxx::detail::sf::w::abs(dx) < tol && ycxx::detail::sf::w::abs(dy) < tol && ycxx::detail::sf::w::abs(dz) < tol) break;
  }
  const W ea = dx * dy, eb = dz * dz, ec = ea - eb, ed = ea - 6 * eb, ee = ed + ec + ec;
  const W c1 = W(3) / 14, c2 = W(1) / 6, c3 = W(9) / 22, c4 = W(3) / 26, c5 = c3 / 4, c6 = W(3) * c4 / 2;
  return 3 * sum + fac * (1 + ed * (-c1 + c5 * ed - c6 * dz * ee) + dz * (c2 * ee + dz * (-c3 * ec + dz * c4 * ea))) /
                       (ave * ycxx::detail::sf::w::sqrt(ave));
}
template <class W>
W carlson_rj(W x, W y, W z, W p) noexcept { // p > 0
  const W tol = ycxx::detail::sf::carlson_tol<W>();
  W sum = 0, fac = 1, ave = 0, dx = 0, dy = 0, dz = 0, dp = 0;
  for (int i = 0; i < 200; ++i) {
    const W sx = ycxx::detail::sf::w::sqrt(x), sy = ycxx::detail::sf::w::sqrt(y), sz = ycxx::detail::sf::w::sqrt(z);
    const W lam = sx * (sy + sz) + sy * sz;
    const W alpha0 = p * (sx + sy + sz) + sx * sy * sz;
    const W alpha = alpha0 * alpha0, beta = p * (p + lam) * (p + lam);
    sum += fac * ycxx::detail::sf::carlson_rc(alpha, beta);
    fac /= 4;
    x = (x + lam) / 4;
    y = (y + lam) / 4;
    z = (z + lam) / 4;
    p = (p + lam) / 4;
    ave = (x + y + z + p + p) / 5;
    dx = (ave - x) / ave;
    dy = (ave - y) / ave;
    dz = (ave - z) / ave;
    dp = (ave - p) / ave;
    if (ycxx::detail::sf::w::abs(dx) < tol && ycxx::detail::sf::w::abs(dy) < tol && ycxx::detail::sf::w::abs(dz) < tol &&
        ycxx::detail::sf::w::abs(dp) < tol)
      break;
  }
  const W ea = dx * (dy + dz) + dy * dz, eb = dx * dy * dz, ec = dp * dp, ed = ea - 3 * ec, ee = eb + 2 * dp * (ea - ec);
  const W c1 = W(3) / 14, c2 = W(1) / 3, c3 = W(3) / 22, c4 = W(3) / 26, c5 = W(3) * c3 / 4, c6 = W(3) * c4 / 2, c7 = c2 / 2,
          c8 = c3 + c3;
  return 3 * sum + fac * (1 + ed * (-c1 + c5 * ed - c6 * ee) + eb * (c7 + dp * (-c8 + dp * c4)) + dp * ea * (c2 - dp * c3) -
                          c2 * dp * ec) /
                       (ave * ycxx::detail::sf::w::sqrt(ave));
}

// ---- elliptic integrals ([sf.cmath.comp.ellint.*], [sf.cmath.ellint.*]) ------------------------------
// which: 1 (F, K), 2 (E), 3 (Pi). Complete integrals of |k| <= 1, nu < 1 (nu >= 1: callers).
template <class W>
W ellint_complete(int which, W k, W nu) noexcept {
  const W kc2 = (1 - k) * (1 + k);
  if (which == 1) return ycxx::detail::sf::carlson_rf(W(0), kc2, W(1));
  if (which == 2) {
    if (kc2 == W(0)) return W(1);
    return ycxx::detail::sf::carlson_rf(W(0), kc2, W(1)) - k * k / 3 * ycxx::detail::sf::carlson_rd(W(0), kc2, W(1));
  }
  return ycxx::detail::sf::carlson_rf(W(0), kc2, W(1)) + nu / 3 * ycxx::detail::sf::carlson_rj(W(0), kc2, W(1), 1 - nu);
}
template <class T>
T ellint(int which, T k, T nu, T phi, bool complete) noexcept {
  using W = work_t<T>;
  if (__builtin_isnan(k) || __builtin_isnan(nu) || __builtin_isnan(phi)) return ycxx::detail::sf::nan_result<T>();
  if (k > T(1) || k < T(-1)) return ycxx::detail::sf::domain_error<T>();
  const W wk = k, wnu = nu;
  const bool k_one = k == T(1) || k == T(-1);
  if (complete) {
    if (which == 1 && k_one) return ycxx::detail::fpm::fp_infinity<T>(false);
    if (which == 3) {
      if (nu == T(1)) return ycxx::detail::fpm::fp_infinity<T>(false);
      if (nu > T(1)) return ycxx::detail::sf::domain_error<T>(); // only a principal value exists
      if (k_one) return ycxx::detail::fpm::fp_infinity<T>(false);
    }
    return static_cast<T>(ycxx::detail::sf::ellint_complete<W>(which, wk, wnu));
  }
  if (__builtin_isinf(phi)) return ycxx::detail::sf::domain_error<T>();
  // phi = n pi + r, |r| <= pi/2; F(k, n pi + r) = 2n K(k) + F(k, r) (likewise E, Pi).
  const W wphi = phi;
  const W n = ycxx::detail::sf::w::round(wphi / ycxx::detail::sf::w::pi<W>);
  const W r = wphi - n * ycxx::detail::sf::w::pi<W>;
  const W s = ycxx::detail::sf::w::sin(r), c = ycxx::detail::sf::w::cos(r);
  const W s2 = s * s, c2 = c * c, d2 = (1 - wk * s) * (1 + wk * s);
  if (which == 3 && !(1 - wnu * s2 > W(0))) return ycxx::detail::sf::domain_error<T>();
  W v;
  if (d2 == W(0) && c2 == W(0)) {
    if (which == 2) {
      v = s;
    } else {
      return ycxx::detail::fpm::fp_infinity<T>(s < W(0));
    }
  } else {
    const W rf = ycxx::detail::sf::carlson_rf(c2, d2, W(1));
    if (which == 1)
      v = s * rf;
    else if (which == 2)
      v = s * rf - wk * wk / 3 * s * s2 * ycxx::detail::sf::carlson_rd(c2, d2, W(1));
    else
      v = s * rf + wnu / 3 * s * s2 * ycxx::detail::sf::carlson_rj(c2, d2, W(1), 1 - wnu * s2);
  }
  if (n != W(0)) {
    if (which == 1 && k_one) return ycxx::detail::fpm::fp_infinity<T>(n < W(0));
    if (which == 3 && (wnu >= W(1) || k_one)) return ycxx::detail::sf::domain_error<T>();
    v += 2 * n * ycxx::detail::sf::ellint_complete<W>(which, wk, wnu);
  }
  return static_cast<T>(v);
}
template <class T>
T comp_ellint_1(T k) noexcept {
  return ycxx::detail::sf::ellint<T>(1, k, T(0), T(0), true);
}
template <class T>
T comp_ellint_2(T k) noexcept {
  return ycxx::detail::sf::ellint<T>(2, k, T(0), T(0), true);
}
template <class T>
T comp_ellint_3(T k, T nu) noexcept {
  return ycxx::detail::sf::ellint<T>(3, k, nu, T(0), true);
}
template <class T>
T ellint_1(T k, T phi) noexcept {
  return ycxx::detail::sf::ellint<T>(1, k, T(0), phi, false);
}
template <class T>
T ellint_2(T k, T phi) noexcept {
  return ycxx::detail::sf::ellint<T>(2, k, T(0), phi, false);
}
template <class T>
T ellint_3(T k, T nu, T phi) noexcept {
  return ycxx::detail::sf::ellint<T>(3, k, nu, phi, false);
}

// ---- Bessel functions -------------------------------------------------------------------------------
// Temme's auxiliary gammas for |mu| <= 1/2: gam1 = (1/G(1-mu) - 1/G(1+mu)) / (2 mu),
// gam2 = (1/G(1-mu) + 1/G(1+mu)) / 2, gplus = 1/G(1+mu), gminus = 1/G(1-mu).
template <class W>
void temme_gammas(W mu, W& gam1, W& gam2, W& gplus, W& gminus) noexcept {
  const auto& a = ycxx::detail::sf::inv_gamma_w<W>.v;
  constexpr int K = sizeof(a) / sizeof(a[0]);
  W even = 0, odd = 0; // sum of a_2j mu^2j, sum of a_(2j+1) mu^2j
  const W mu2 = mu * mu;
  for (int k = (K - 1) & ~1; k >= 0; k -= 2) even = even * mu2 + a[k];
  for (int k = ((K - 2) | 1); k >= 1; k -= 2) odd = odd * mu2 + a[k];
  gam1 = -odd;
  gam2 = even;
  gplus = even + mu * odd;
  gminus = even - mu * odd;
}

template <class W>
W hankel_term(W nu, int k) noexcept { // a_k(nu) / x^k numerator factor helper: (4nu^2 - (2k-1)^2) / (8k)
  return (4 * nu * nu - W(2 * k - 1) * W(2 * k - 1)) / W(8 * k);
}

// J_nu(x), Y_nu(x) for nu >= 0, x > 0.
template <class W>
void bessel_jy(W nu, W x, W& J, W& Y) noexcept {
  const W eps = ycxx::detail::sf::w::eps<W>, fpmin = ycxx::detail::sf::w::tiny<W>;
  const W pi = ycxx::detail::sf::w::pi<W>;
  if (x > W(1000) && x > nu * nu / 2) {
    // Hankel: J = sqrt(2/(pi x)) (P cos w - Q sin w), Y = sqrt(2/(pi x)) (P sin w + Q cos w)
    W P = 1, Q = 0, term = 1;
    for (int k = 1; k < 200; ++k) {
      const W next = term * ycxx::detail::sf::hankel_term(nu, k) / x;
      if (ycxx::detail::sf::w::abs(next) >= ycxx::detail::sf::w::abs(term) && k > 2) break;
      term = next;
      // a_k / x^k alternates between Q (odd k) and P (even k), with signs (-1)^(k/2)
      if (k % 2 == 1)
        Q += ((k / 2) % 2 == 0 ? term : -term);
      else
        P += ((k / 2) % 2 == 0 ? term : -term);
      if (ycxx::detail::sf::w::abs(term) < eps) break;
    }
    // w = x - (nu/2 + 1/4) pi: cos w = cos x cos f + sin x sin f, f = (nu/2 + 1/4) pi
    const W f = ycxx::detail::sf::w::fmod(nu / 2 + W(0.25), W(2));
    const W cf = ycxx::detail::sf::w::cospi(f), sf = ycxx::detail::sf::w::sinpi(f);
    const W cx = ycxx::detail::sf::w::cos(x), sx = ycxx::detail::sf::w::sin(x);
    const W cw = cx * cf + sx * sf, sw = sx * cf - cx * sf;
    const W amp = ycxx::detail::sf::w::sqrt(2 / (pi * x));
    J = amp * (P * cw - Q * sw);
    Y = amp * (P * sw + Q * cw);
    return;
  }
  const int nl = x < W(2) ? static_cast<int>(nu + W(0.5)) : (nu - x + W(1.5) > W(0) ? static_cast<int>(nu - x + W(1.5)) : 0);
  const W mu = nu - nl, mu2 = mu * mu, xi = 1 / x, xi2 = 2 * xi, wr = xi2 / pi;
  // CF1: h = J'_nu / J_nu (modified Lentz); isign tracks the sign of J_nu relative to the start.
  int isign = 1;
  W h = nu * xi;
  if (h < fpmin) h = fpmin;
  W b = xi2 * nu, d = 0, c = h;
  for (long i = 1; i < 2000000; ++i) {
    b += xi2;
    d = b - d;
    if (ycxx::detail::sf::w::abs(d) < fpmin) d = fpmin;
    c = b - 1 / c;
    if (ycxx::detail::sf::w::abs(c) < fpmin) c = fpmin;
    d = 1 / d;
    const W del = c * d;
    h *= del;
    if (d < W(0)) isign = -isign;
    if (ycxx::detail::sf::w::abs(del - 1) < eps) break;
  }
  // Downward recurrence from nu to mu, unnormalised.
  W rjl = isign * fpmin, rjpl = h * rjl;
  const W rjl1 = rjl;
  W fact = nu * xi;
  for (int l = nl; l >= 1; --l) {
    const W t = fact * rjl + rjpl;
    fact -= xi;
    rjpl = fact * t - rjl;
    rjl = t;
  }
  if (rjl == W(0)) rjl = eps;
  const W f = rjpl / rjl;
  W rjmu, rymu, ry1;
  if (x < W(2)) {
    // Temme's series for Y_mu and Y_(mu+1).
    const W x2 = x / 2, pimu = pi * mu;
    const W fct = ycxx::detail::sf::w::abs(pimu) < eps ? W(1) : pimu / ycxx::detail::sf::w::sin(pimu);
    W dd = -ycxx::detail::sf::w::log(x2);
    W e = mu * dd;
    const W fct2 = ycxx::detail::sf::w::abs(e) < eps ? W(1) : ycxx::detail::sf::w::sinh(e) / e;
    W gam1, gam2, gpl, gmi;
    ycxx::detail::sf::temme_gammas(mu, gam1, gam2, gpl, gmi);
    W ff = 2 / pi * fct * (gam1 * ycxx::detail::sf::w::cosh(e) + gam2 * fct2 * dd);
    e = ycxx::detail::sf::w::exp(e);
    W p = e / (gpl * pi);   // (x/2)^-mu Gamma(1 + mu) / pi
    W q = 1 / (e * pi * gmi); // (x/2)^mu Gamma(1 - mu) / pi
    const W pimu2 = pimu / 2;
    const W fct3 = ycxx::detail::sf::w::abs(pimu2) < eps ? W(1) : ycxx::detail::sf::w::sin(pimu2) / pimu2;
    const W r = pi * pimu2 * fct3 * fct3;
    W cc = 1;
    dd = -x2 * x2;
    W sum = ff + r * q, sum1 = p;
    for (int i = 1; i < 100000; ++i) {
      ff = (i * ff + p + q) / (W(i) * W(i) - mu2);
      cc *= dd / i;
      p /= W(i) - mu;
      q /= W(i) + mu;
      const W del = cc * (ff + r * q);
      sum += del;
      sum1 += cc * p - i * del;
      if (ycxx::detail::sf::w::abs(del) < (1 + ycxx::detail::sf::w::abs(sum)) * eps) break;
    }
    rymu = -sum;
    ry1 = -sum1 * xi2;
    const W rymup = mu * xi * rymu - ry1;
    rjmu = wr / (rymup - f * rymu);
  } else {
    // Steed's complex continued fraction p + iq = (J' + iY') / (J + iY).
    W a = W(0.25) - mu2, p = -xi / 2, q = 1;
    const W br = 2 * x;
    W bi = 2;
    W fct = a * xi / (p * p + q * q);
    W cr = br + q * fct, ci = bi + p * fct;
    W den = br * br + bi * bi;
    W dr = br / den, di = -bi / den;
    W dlr = cr * dr - ci * di, dli = cr * di + ci * dr;
    W t = p * dlr - q * dli;
    q = p * dli + q * dlr;
    p = t;
    for (int i = 2; i < 100000; ++i) {
      a += 2 * (i - 1);
      bi += 2;
      dr = a * dr + br;
      di = a * di + bi;
      if (ycxx::detail::sf::w::abs(dr) + ycxx::detail::sf::w::abs(di) < fpmin) dr = fpmin;
      fct = a / (cr * cr + ci * ci);
      cr = br + cr * fct;
      ci = bi - ci * fct;
      if (ycxx::detail::sf::w::abs(cr) + ycxx::detail::sf::w::abs(ci) < fpmin) cr = fpmin;
      den = dr * dr + di * di;
      dr /= den;
      di /= -den;
      dlr = cr * dr - ci * di;
      dli = cr * di + ci * dr;
      t = p * dlr - q * dli;
      q = p * dli + q * dlr;
      p = t;
      if (ycxx::detail::sf::w::abs(dlr - 1) + ycxx::detail::sf::w::abs(dli) < eps) break;
    }
    const W gam = (p - f) / q;
    rjmu = ycxx::detail::sf::w::sqrt(wr / ((p - f) * gam + q));
    if (rjl < W(0)) rjmu = -rjmu;
    rymu = rjmu * gam;
    const W rymup = rymu * (p + q / gam);
    ry1 = mu * xi * rymu - rymup;
  }
  J = rjl1 * (rjmu / rjl);
  for (int i = 1; i <= nl; ++i) {
    const W t = (mu + i) * xi2 * ry1 - rymu;
    rymu = ry1;
    ry1 = t;
  }
  Y = rymu;
}

// I_nu(x), K_nu(x) for nu >= 0, x > 0.
template <class W>
void bessel_ik(W nu, W x, W& I, W& K) noexcept {
  const W eps = ycxx::detail::sf::w::eps<W>, fpmin = ycxx::detail::sf::w::tiny<W>;
  const W pi = ycxx::detail::sf::w::pi<W>;
  if (x > W(1000) && x > nu * nu / 2) {
    // K ~ sqrt(pi/(2x)) e^-x sum a_k / x^k; I ~ e^x / sqrt(2 pi x) sum (-1)^k a_k / x^k
    W sk = 1, si = 1, term = 1;
    for (int k = 1; k < 200; ++k) {
      const W next = term * ycxx::detail::sf::hankel_term(nu, k) / x;
      if (ycxx::detail::sf::w::abs(next) >= ycxx::detail::sf::w::abs(term) && k > 2) break;
      term = next;
      sk += term;
      si += (k % 2 == 0 ? term : -term);
      if (ycxx::detail::sf::w::abs(term) < eps) break;
    }
    K = ycxx::detail::sf::w::sqrt(pi / (2 * x)) * ycxx::detail::sf::w::exp(-x) * sk;
    const W ehalf = ycxx::detail::sf::w::exp(x / 2); // e^x in two halves: finite as long as I is
    I = ehalf / ycxx::detail::sf::w::sqrt(2 * pi * x) * si * ehalf;
    return;
  }
  const int nl = static_cast<int>(nu + W(0.5));
  const W mu = nu - nl, mu2 = mu * mu, xi = 1 / x, xi2 = 2 * xi;
  // CF1: h = I'_nu / I_nu
  W h = nu * xi;
  if (h < fpmin) h = fpmin;
  W b = xi2 * nu, d = 0, c = h;
  for (long i = 1; i < 2000000; ++i) {
    b += xi2;
    d = 1 / (b + d);
    c = b + 1 / c;
    const W del = c * d;
    h *= del;
    if (ycxx::detail::sf::w::abs(del - 1) < eps) break;
  }
  W ril = fpmin, ripl = h * ril;
  const W ril1 = ril;
  W fact = nu * xi;
  for (int l = nl; l >= 1; --l) {
    const W t = fact * ril + ripl;
    fact -= xi;
    ripl = fact * t + ril;
    ril = t;
  }
  const W f = ripl / ril;
  W rkmu, rk1;
  if (x < W(2)) {
    const W x2 = x / 2, pimu = pi * mu;
    const W fct = ycxx::detail::sf::w::abs(pimu) < eps ? W(1) : pimu / ycxx::detail::sf::w::sin(pimu);
    W dd = -ycxx::detail::sf::w::log(x2);
    W e = mu * dd;
    const W fct2 = ycxx::detail::sf::w::abs(e) < eps ? W(1) : ycxx::detail::sf::w::sinh(e) / e;
    W gam1, gam2, gpl, gmi;
    ycxx::detail::sf::temme_gammas(mu, gam1, gam2, gpl, gmi);
    W ff = fct * (gam1 * ycxx::detail::sf::w::cosh(e) + gam2 * fct2 * dd);
    W sum = ff;
    e = ycxx::detail::sf::w::exp(e);
    W p = e / (2 * gpl);   // (x/2)^-mu Gamma(1 + mu) / 2
    W q = 1 / (2 * e * gmi); // (x/2)^mu Gamma(1 - mu) / 2
    W cc = 1;
    dd = x2 * x2;
    W sum1 = p;
    for (int i = 1; i < 100000; ++i) {
      ff = (i * ff + p + q) / (W(i) * W(i) - mu2);
      cc *= dd / i;
      p /= W(i) - mu;
      q /= W(i) + mu;
      const W del = cc * ff;
      sum += del;
      sum1 += cc * (p - i * ff);
      if (ycxx::detail::sf::w::abs(del) < ycxx::detail::sf::w::abs(sum) * eps) break;
    }
    rkmu = sum;
    rk1 = sum1 * xi2;
  } else {
    // Steed's continued fraction for K (Thompson and Barnett).
    W bb = 2 * (1 + x), dd = 1 / bb, hh = dd, delh = dd;
    W q1 = 0, q2 = 1;
    const W a1 = W(0.25) - mu2;
    W q = a1, cc = a1, a = -a1;
    W s = 1 + q * delh;
    for (int i = 2; i < 100000; ++i) {
      a -= 2 * (i - 1);
      cc = -a * cc / i;
      const W qnew = (q1 - bb * q2) / a;
      q1 = q2;
      q2 = qnew;
      q += cc * qnew;
      bb += 2;
      dd = 1 / (bb + a * dd);
      delh = (bb * dd - 1) * delh;
      hh += delh;
      const W dels = q * delh;
      s += dels;
      if (ycxx::detail::sf::w::abs(dels / s) < eps) break;
    }
    hh = a1 * hh;
    rkmu = ycxx::detail::sf::w::sqrt(pi / (2 * x)) * ycxx::detail::sf::w::exp(-x) / s;
    rk1 = rkmu * (mu + x + W(0.5) - hh) * xi;
  }
  const W rkmup = mu * xi * rkmu - rk1;
  const W rimu = xi / (f * rkmu - rkmup);
  I = rimu * ril1 / ril;
  for (int i = 1; i <= nl; ++i) {
    const W t = (mu + i) * xi2 * rk1 + rkmu;
    rkmu = rk1;
    rk1 = t;
  }
  K = rkmu;
}

// which: 0 J, 1 Y (Neumann), 2 I, 3 K
template <class T>
T cyl_bessel(int which, T nu, T x) noexcept {
  using W = work_t<T>;
  if (__builtin_isnan(nu) || __builtin_isnan(x)) return ycxx::detail::sf::nan_result<T>();
  if (x < T(0) || __builtin_isinf(nu)) return ycxx::detail::sf::domain_error<T>();
  const W wnu = ycxx::detail::sf::w::abs(W(nu)), wx = x;
  const bool neg = nu < T(0);
  const bool integral = ycxx::detail::sf::w::floor(wnu) == wnu;
  if (x == T(0)) {
    switch (which) {
    case 0:
    case 2:
      if (nu == T(0)) return T(1);
      if (!neg || integral) return T(0);
      return ycxx::detail::sf::domain_error<T>(); // |J_nu(0)| = inf for negative non-integral nu
    case 1:
      return ycxx::detail::fpm::fp_infinity<T>(true);
    default:
      return ycxx::detail::fpm::fp_infinity<T>(false);
    }
  }
  if (__builtin_isinf(x)) {
    if (which == 2) return ycxx::detail::fpm::fp_infinity<T>(false);
    return T(0);
  }
  W a, b;
  if (which <= 1) {
    ycxx::detail::sf::bessel_jy(wnu, wx, a, b);
    if (neg) {
      // J_-nu = cos(nu pi) J_nu - sin(nu pi) Y_nu, Y_-nu = sin(nu pi) J_nu + cos(nu pi) Y_nu
      const W c = ycxx::detail::sf::w::cospi(wnu), s = ycxx::detail::sf::w::sinpi(wnu);
      const W j = c * a - s * b, y = s * a + c * b;
      a = j;
      b = y;
    }
    return static_cast<T>(which == 0 ? a : b);
  }
  ycxx::detail::sf::bessel_ik(wnu, wx, a, b);
  if (which == 3) return static_cast<T>(b);
  if (neg && !integral) a += 2 / ycxx::detail::sf::w::pi<W> * ycxx::detail::sf::w::sinpi(wnu) * b;
  return static_cast<T>(a);
}
template <class T>
T cyl_bessel_j(T nu, T x) noexcept {
  return ycxx::detail::sf::cyl_bessel<T>(0, nu, x);
}
template <class T>
T cyl_neumann(T nu, T x) noexcept {
  return ycxx::detail::sf::cyl_bessel<T>(1, nu, x);
}
template <class T>
T cyl_bessel_i(T nu, T x) noexcept {
  return ycxx::detail::sf::cyl_bessel<T>(2, nu, x);
}
template <class T>
T cyl_bessel_k(T nu, T x) noexcept {
  return ycxx::detail::sf::cyl_bessel<T>(3, nu, x);
}
template <class T>
T sph_bessel_impl(bool neumann, unsigned n, T x) noexcept {
  using W = work_t<T>;
  if (__builtin_isnan(x)) return x;
  if (x < T(0)) return ycxx::detail::sf::domain_error<T>();
  if (x == T(0)) {
    if (neumann) return ycxx::detail::fpm::fp_infinity<T>(true);
    return n == 0 ? T(1) : T(0);
  }
  if (__builtin_isinf(x)) return T(0);
  const W wx = x;
  W j, y;
  ycxx::detail::sf::bessel_jy(W(n) + W(0.5), wx, j, y);
  const W f = ycxx::detail::sf::w::sqrt(ycxx::detail::sf::w::pi<W> / (2 * wx));
  return static_cast<T>(f * (neumann ? y : j));
}
template <class T>
T sph_bessel(unsigned n, T x) noexcept {
  return ycxx::detail::sf::sph_bessel_impl<T>(false, n, x);
}
template <class T>
T sph_neumann(unsigned n, T x) noexcept {
  return ycxx::detail::sf::sph_bessel_impl<T>(true, n, x);
}

// ---- exponential integral ([sf.cmath.expint]) -------------------------------------------------------
template <class T>
T expint(T x) noexcept {
  using W = work_t<T>;
  if (__builtin_isnan(x)) return x;
  if (x == T(0)) return ycxx::detail::fpm::fp_infinity<T>(true);
  if (__builtin_isinf(x)) return x > T(0) ? x : T(0);
  const W eps = ycxx::detail::sf::w::eps<W>, wx = x;
  if (x > T(0)) {
    const W big = W(ycxx::detail::fp_format<W>.digits) * W(0.75); // asymptotic series accurate beyond
    if (wx > big) {
      W sum = 1, term = 1;
      for (int k = 1; k < 1000; ++k) {
        const W next = term * k / wx;
        if (next >= term) break;
        term = next;
        sum += term;
        if (term < eps * sum) break;
      }
      return static_cast<T>(ycxx::detail::sf::w::exp(wx) / wx * sum);
    }
    W sum = 0, term = 1;
    for (int k = 1; k < 100000; ++k) {
      term *= wx / k;
      const W t = term / k;
      sum += t;
      if (t < eps * sum) break;
    }
    return static_cast<T>(ycxx::detail::sf::w::egamma<W> + ycxx::detail::sf::w::log(wx) + sum);
  }
  // Ei(x) = -E1(-x)
  const W t = -wx;
  if (t <= W(1)) {
    W sum = 0, term = 1;
    for (int k = 1; k < 100000; ++k) {
      term *= -t / k;
      const W s = term / k;
      sum += s;
      if (ycxx::detail::sf::w::abs(s) < eps * ycxx::detail::sf::w::abs(sum)) break;
    }
    const W e1 = -ycxx::detail::sf::w::egamma<W> - ycxx::detail::sf::w::log(t) - sum;
    return static_cast<T>(-e1);
  }
  // E1(t) = e^-t / (t + 1 - 1/(t + 3 - 4/(t + 5 - ...))) (modified Lentz)
  const W fpmin = ycxx::detail::sf::w::tiny<W>;
  W b = t + 1, c = 1 / fpmin, d = 1 / b, h = d;
  for (int i = 1; i < 100000; ++i) {
    const W an = -W(i) * W(i);
    b += 2;
    d = 1 / (an * d + b);
    c = b + an / c;
    const W del = c * d;
    h *= del;
    if (ycxx::detail::sf::w::abs(del - 1) < eps) break;
  }
  return static_cast<T>(-h * ycxx::detail::sf::w::exp(-t));
}

// ---- Riemann zeta ([sf.cmath.riemann.zeta]) ---------------------------------------------------------
template <class W>
W zeta_em(W s) noexcept { // Euler-Maclaurin, s >= 1/2, s != 1
  constexpr int N = 24;
  const auto& c = ycxx::detail::sf::stirling_w<W>.v; // B_2j / (2j (2j-1))
  constexpr int J = sizeof(c) / sizeof(c[0]);
  const W eps = ycxx::detail::sf::w::eps<W>;
  W sum = 0;
  for (int k = N - 1; k >= 1; --k) sum += ycxx::detail::sf::w::pow(W(k), -s);
  const W Np = ycxx::detail::sf::w::pow(W(N), -s);
  sum += Np * W(N) / (s - 1) + Np / 2;
  // sum_j B_2j / (2j)! s (s+1) ... (s + 2j - 2) N^(-s - 2j + 1)
  W poch = s;            // s (s+1) ... (s+2j-2)
  W npow = Np / W(N);    // N^(-s-1)
  W fact = 1;            // (2j - 2)!
  for (int j = 1; j <= J; ++j) {
    const W term = c[j - 1] / fact * poch * npow;
    sum += term;
    if (ycxx::detail::sf::w::abs(term) < eps * ycxx::detail::sf::w::abs(sum)) break;
    poch *= (s + 2 * j - 1) * (s + 2 * j);
    npow /= W(N) * W(N);
    fact *= W(2 * j - 1) * W(2 * j);
  }
  return sum;
}
template <class T>
T riemann_zeta(T x) noexcept {
  using W = work_t<T>;
  if (__builtin_isnan(x)) return x;
  if (x == T(1) || x == -ycxx::detail::fpm::fp_infinity<T>(false)) return ycxx::detail::sf::domain_error<T>();
  if (__builtin_isinf(x)) return T(1);
  if (x == T(0)) return T(-0.5);
  const W s = x;
  if (s >= W(0.5)) return static_cast<T>(ycxx::detail::sf::zeta_em(s));
  // zeta(s) = 2^s pi^(s-1) sin(pi s / 2) Gamma(1 - s) zeta(1 - s)
  const W half = s / 2;
  if (half == ycxx::detail::sf::w::floor(half)) return T(0); // the trivial zeros
  const W sn = ycxx::detail::sf::w::sinpi(half);
  const W z1 = ycxx::detail::sf::zeta_em(1 - s);
  const W pi = ycxx::detail::sf::w::pi<W>;
  if (1 - s < W(150)) {
    return static_cast<T>(ycxx::detail::sf::w::pow(W(2), s) * ycxx::detail::sf::w::pow(pi, s - 1) * sn *
                          ycxx::detail::sf::w::tgamma(1 - s) * z1);
  }
  const W lg = s * ycxx::detail::sf::w::log(2 * pi) - ycxx::detail::sf::w::log(pi) + ycxx::detail::sf::w::log(ycxx::detail::sf::w::abs(sn)) +
               ycxx::detail::sf::w::lgamma(1 - s) + ycxx::detail::sf::w::log(z1);
  const W r = ycxx::detail::sf::w::exp(lg);
  return static_cast<T>(sn < W(0) ? -r : r);
}

}} // namespace ycxx::detail::sf
