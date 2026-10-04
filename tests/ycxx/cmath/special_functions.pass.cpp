// [sf.cmath]: values of the mathematical special functions at points where they have closed
// forms (Formulas 29.22-29.42 of the draft): note that assoc_legendre has no Condon-Shortley
// phase (29.23: P_l^m(x) = (1 - x^2)^(m/2) d^m/dx^m P_l(x)), while sph_legendre includes
// (-1)^m (29.41); hermite is the physicists' H_n (29.36); assoc_laguerre is
// (-1)^m d^m/dx^m L_(n+m) (29.22). [sf.cmath.general]/1: a NaN argument gives a NaN.
#include <cmath>
#include <initializer_list>
#include <limits>
#include <numbers>
#include "check.hpp"

bool close(double a, double b, double tol = 1e-12) {
  return std::fabs(a - b) <= tol * (std::fabs(b) > 1 ? std::fabs(b) : 1.0);
}

int main() {
  const double pi = std::numbers::pi;
  for (double x : {-0.9, -0.3, 0.0, 0.25, 0.6, 1.0}) {
    CHECK(close(std::legendre(0, x), 1));
    CHECK(close(std::legendre(1, x), x));
    CHECK(close(std::legendre(2, x), (3 * x * x - 1) / 2));
    CHECK(close(std::legendre(3, x), (5 * x * x * x - 3 * x) / 2));
    CHECK(close(std::assoc_legendre(1, 0, x), x));
    CHECK(close(std::assoc_legendre(1, 1, x), std::sqrt(1 - x * x)));
    CHECK(close(std::assoc_legendre(2, 1, x), 3 * x * std::sqrt(1 - x * x)));
    CHECK(close(std::assoc_legendre(2, 2, x), 3 * (1 - x * x)));
  }
  for (double x : {-2.0, -0.5, 0.0, 0.5, 1.5, 3.0}) {
    CHECK(close(std::hermite(0, x), 1));
    CHECK(close(std::hermite(1, x), 2 * x));
    CHECK(close(std::hermite(2, x), 4 * x * x - 2));
    CHECK(close(std::hermite(3, x), 8 * x * x * x - 12 * x));
  }
  for (double x : {0.0, 0.5, 1.0, 2.5, 7.0}) {
    CHECK(close(std::laguerre(0, x), 1));
    CHECK(close(std::laguerre(1, x), 1 - x));
    CHECK(close(std::laguerre(2, x), (x * x - 4 * x + 2) / 2));
    CHECK(close(std::laguerre(3, x), (-x * x * x + 9 * x * x - 18 * x + 6) / 6));
    CHECK(close(std::assoc_laguerre(0, 3, x), 1));
    CHECK(close(std::assoc_laguerre(1, 1, x), 2 - x));
    CHECK(close(std::assoc_laguerre(2, 0, x), std::laguerre(2, x)));
  }
  CHECK(close(std::beta(1.0, 1.0), 1));
  CHECK(close(std::beta(2.0, 3.0), 1.0 / 12));
  CHECK(close(std::beta(0.5, 0.5), pi));
  CHECK(close(std::beta(3.0, 2.0), std::beta(2.0, 3.0)));

  CHECK(close(std::comp_ellint_1(0.0), pi / 2));
  CHECK(close(std::comp_ellint_2(0.0), pi / 2));
  CHECK(close(std::comp_ellint_2(1.0), 1));
  CHECK(close(std::comp_ellint_1(std::sqrt(0.5)), 1.8540746773013719, 1e-10));
  CHECK(close(std::comp_ellint_2(std::sqrt(0.5)), 1.3506438810476755, 1e-10));
  CHECK(close(std::comp_ellint_3(0.0, 0.0), pi / 2));
  CHECK(close(std::comp_ellint_3(0.0, 0.75), pi, 1e-10));  // pi / (2 sqrt(1 - nu))
  CHECK(close(std::ellint_1(0.0, 0.7), 0.7));
  CHECK(close(std::ellint_2(0.0, 0.7), 0.7));
  CHECK(close(std::ellint_3(0.0, 0.0, 0.7), 0.7));
  CHECK(close(std::ellint_1(0.5, pi / 2), std::comp_ellint_1(0.5), 1e-10));
  CHECK(close(std::ellint_2(0.5, pi / 2), std::comp_ellint_2(0.5), 1e-10));
  CHECK(close(std::ellint_1(1.0, 0.5), std::atanh(std::sin(0.5)), 1e-10));  // F(1, phi) = artanh(sin phi)

  CHECK(close(std::expint(1.0), 1.8951178163559368, 1e-10));
  CHECK(close(std::expint(-1.0), -0.21938393439552029, 1e-10));  // Ei(-1) = -E1(1)

  CHECK(close(std::riemann_zeta(2.0), pi * pi / 6, 1e-10));
  CHECK(close(std::riemann_zeta(4.0), pi * pi * pi * pi / 90, 1e-10));
  CHECK(close(std::riemann_zeta(0.0), -0.5, 1e-10));
  CHECK(close(std::riemann_zeta(-1.0), -1.0 / 12, 1e-10));

  CHECK(close(std::cyl_bessel_j(0.0, 0.0), 1));
  CHECK(close(std::cyl_bessel_j(1.0, 0.0), 0));
  CHECK(close(std::cyl_bessel_j(0.0, 1.0), 0.76519768655796655, 1e-10));
  CHECK(close(std::cyl_bessel_i(0.0, 0.0), 1));
  CHECK(close(std::cyl_bessel_i(0.0, 1.0), 1.2660658777520082, 1e-10));
  CHECK(close(std::cyl_bessel_k(0.0, 1.0), 0.42102443824070834, 1e-10));
  CHECK(close(std::cyl_neumann(0.0, 1.0), 0.088256964215676957, 1e-9));
  for (double x : {0.5, 1.0, 2.0, 5.0}) {
    CHECK(close(std::cyl_bessel_j(0.5, x), std::sqrt(2 / (pi * x)) * std::sin(x), 1e-10));
    CHECK(close(std::cyl_neumann(0.5, x), -std::sqrt(2 / (pi * x)) * std::cos(x), 1e-10));
    CHECK(close(std::cyl_bessel_i(0.5, x), std::sqrt(2 / (pi * x)) * std::sinh(x), 1e-10));
    CHECK(close(std::cyl_bessel_k(0.5, x), std::sqrt(pi / (2 * x)) * std::exp(-x), 1e-10));
    CHECK(close(std::sph_bessel(0, x), std::sin(x) / x, 1e-10));
    CHECK(close(std::sph_bessel(1, x), std::sin(x) / (x * x) - std::cos(x) / x, 1e-10));
    CHECK(close(std::sph_neumann(0, x), -std::cos(x) / x, 1e-10));
  }
  for (double t : {0.0, 0.4, 1.2, 2.5}) {
    CHECK(close(std::sph_legendre(0, 0, t), 1 / std::sqrt(4 * pi)));
    CHECK(close(std::sph_legendre(1, 0, t), std::sqrt(3 / (4 * pi)) * std::cos(t)));
    CHECK(close(std::sph_legendre(1, 1, t), -std::sqrt(3 / (8 * pi)) * std::sin(t)));
  }

  // [sf.cmath.general]/1: NaN in, NaN out.
  const double nan = std::numeric_limits<double>::quiet_NaN();
  CHECK(std::isnan(std::legendre(2, nan)) && std::isnan(std::hermite(3, nan)));
  CHECK(std::isnan(std::beta(nan, 1.0)) && std::isnan(std::riemann_zeta(nan)));
  CHECK(std::isnan(std::cyl_bessel_j(nan, 1.0)) && std::isnan(std::cyl_bessel_j(1.0, nan)));
  CHECK(std::isnan(std::ellint_1(0.5, nan)) && std::isnan(std::expint(nan)));
  CHECK(std::isnan(std::sph_legendre(1, 1, nan)) && std::isnan(std::assoc_laguerre(1, 1, nan)));

  // The f and l forms.
  CHECK(std::fabs(std::legendref(2, 0.5f) - (-0.125f)) < 1e-6f);
  CHECK(std::fabs(std::hermitel(2, 1.0L) - 2.0L) < 1e-15L);
  CHECK(std::fabs(std::betaf(2.0f, 3.0f) - 1.0f / 12) < 1e-6f);
  CHECK(std::fabs(std::comp_ellint_1l(0.0L) - std::numbers::pi_v<long double> / 2) < 1e-15L);
  CHECK(std::fabs(std::sph_besself(0, 1.0f) - std::sin(1.0f)) < 1e-6f);
  return 0;
}
