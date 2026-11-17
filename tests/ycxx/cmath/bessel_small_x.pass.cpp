// [sf.cmath.cyl.bessel.i], [sf.cmath.cyl.bessel.j], [sf.cmath.sph.bessel] (and .cyl.neumann,
// .cyl.bessel.k, .sph.neumann): the Bessel functions at small x, where the Wronskian normalisation
// of the continued fraction loses about log10(1/x) digits when the fractional part of nu is >= 1/2
// (cyl_bessel_i(0.5, 1e-12) was off by 5e-5, cyl_bessel_i(0.5, 1e-20) was inf or negative) and its
// intermediates overflow for tiny x (cyl_bessel_j(1.5, 1e-100) was 1e-219 instead of 2.7e-151,
// cyl_bessel_i(0.25, 1e-300) NaN, sph_bessel(0, 1e-100) inf instead of 1, sph_bessel(1, 1e-300)
// NaN). The references are the closed forms of the half-integer orders (I_1/2 = sqrt(2/(pi x))
// sinh x, J_1/2 = sqrt(2/(pi x)) sin x, j_0 = sin x / x, j_1 = sin x / x^2 - cos x / x, y_0 =
// -cos x / x, K_1/2 = sqrt(pi/(2x)) e^-x), the leading terms of the ascending series for the
// others ((x/2)^nu / Gamma(nu + 1), exact to double precision below x = 1e-9), and values from
// mpmath at 60 digits; where the function overflows (Y_(3/2)(1e-300) = -8e449) the result must be
// the infinity of the right sign, never a NaN.
#include <cmath>
#include <initializer_list>
#include <limits>
#include <numbers>
#include "check.hpp"

const double pi = std::numbers::pi;
const double inf = std::numeric_limits<double>::infinity();

bool rel(double got, double want, double tol) {
  if (std::isinf(want)) return got == want;
  return std::isfinite(got) && std::fabs(got - want) <= tol * std::fabs(want);
}
#define CHECK_REL(got, want, tol) CHECK_SAY(rel((got), (want), (tol)), "got %.17g want %.17g", (double)(got), (double)(want))

int main() {
  // Half-integer orders: closed forms, across the whole small-x range (the x < 2 algorithm) and a
  // little beyond it.
  for (double x : {1e-300, 1e-200, 1e-100, 1e-30, 1e-20, 1e-15, 1e-12, 1e-10, 1e-8, 1e-6, 1e-4, 1e-3, 1e-2, 0.1, 0.5, 1.0, 1.9, 1.99, 2.0, 2.01, 2.5}) {
    const double a = std::sqrt(2 / (pi * x));
    CHECK_REL(std::cyl_bessel_i(0.5, x), a * std::sinh(x), 1e-13);
    CHECK_REL(std::cyl_bessel_j(0.5, x), a * std::sin(x), 1e-13);
    CHECK_REL(std::cyl_bessel_j(-0.5, x), a * std::cos(x), 1e-13);
    CHECK_REL(std::cyl_neumann(0.5, x), -a * std::cos(x), 1e-13);
    CHECK_REL(std::cyl_bessel_k(0.5, x), std::sqrt(pi / (2 * x)) * std::exp(-x), 1e-13);
    CHECK_REL(std::sph_bessel(0, x), std::sin(x) / x, 1e-13);
    CHECK_REL(std::sph_neumann(0, x), -std::cos(x) / x, 1e-13);
    if (x >= 0.1) { // these closed forms cancel below (sin x / x^2 - cos x / x = x/3 + ..., to about 3 eps / x^2)
      CHECK_REL(std::sph_bessel(1, x), std::sin(x) / (x * x) - std::cos(x) / x, 1e-12);
      CHECK_REL(std::cyl_bessel_i(1.5, x), a * (std::cosh(x) - std::sinh(x) / x), 1e-12);
    }
  }
  // Leading term of the series below x = 1e-9 (the next term is x^2/4 / (nu + 1) relative), for
  // orders whose fractional part is below and above 1/2 (mu >= 0 and mu < 0 in the algorithm).
  for (double x : {1e-300, 1e-250, 1e-100, 1e-40, 1e-12, 1e-9}) {
    for (double nu : {0.1, 0.25, 0.49, 0.51, 0.75, 0.9, 1.0, 1.5, 2.5, 3.3, 7.0, 12.7}) {
      const double lead = std::pow(x / 2, nu) / std::tgamma(nu + 1);
      if (lead < 1e-290) continue; // underflows (legitimately) for the larger orders
      CHECK_REL(std::cyl_bessel_j(nu, x), lead, 1e-13);
      CHECK_REL(std::cyl_bessel_i(nu, x), lead, 1e-13);
    }
    CHECK_REL(std::cyl_bessel_j(0.0, x), 1.0, 1e-15);
    CHECK_REL(std::cyl_bessel_i(0.0, x), 1.0, 1e-15);
    CHECK_REL(std::sph_bessel(0, x), 1.0, 1e-15);
    CHECK_REL(std::sph_bessel(1, x), x / 3, 1e-14); // j_n ~ x^n / (2n + 1)!!: representable where J_(n+1/2) is not
    if (x >= 1e-150) CHECK_REL(std::sph_bessel(2, x), x * x / 15, 1e-14);
    if (x >= 1e-100) CHECK_REL(std::sph_bessel(3, x), x * x * x / 105, 1e-14);
  }
  // The reported cases, and mpmath values.
  CHECK_REL(std::cyl_bessel_i(0.5, 1e-12), 7.9788456080286536e-07, 1e-14);
  CHECK_REL(std::cyl_bessel_i(0.5, 1e-20), 7.9788456080286536e-11, 1e-14);
  CHECK_REL(std::cyl_bessel_j(1.5, 1e-100), 2.6596152026762179e-151, 1e-14);
  CHECK_REL(std::cyl_bessel_i(0.25, 1e-300), 9.2772960857900084e-76, 1e-14);
  CHECK_REL(std::sph_bessel(0, 1e-100), 1.0, 1e-15);
  CHECK_REL(std::sph_bessel(1, 1e-300), 3.3333333333333333e-301, 1e-14);
  CHECK_REL(std::cyl_bessel_j(0.1, 1e-300), 9.8074550532751547e-31, 1e-13);
  CHECK_REL(std::cyl_bessel_i(0.9, 1e-80), 5.5719044437809621e-73, 1e-13);
  CHECK_REL(std::cyl_bessel_k(0.51, 1e-300), 1.2377951591374312e+153, 1e-13);
  CHECK_REL(std::cyl_neumann(0.0, 1e-300), -439.83516362276533, 1e-14); // (2/pi) (ln(x/2) + gamma)
  CHECK_REL(std::cyl_bessel_k(0.0, 1e-300), 690.89145941387212, 1e-14);
  CHECK_REL(std::cyl_bessel_j(3.3, 1e-3), 1.4434295225618453e-12, 1e-13);
  CHECK_REL(std::cyl_bessel_i(3.3, 1e-3), 1.4434296904024973e-12, 1e-13);
  CHECK_REL(std::cyl_bessel_j(-1.5, 1e-10), -797884560802865.36, 1e-13); // reflection: dominated by Y
  CHECK_REL(std::cyl_bessel_i(-1.5, 1e-10), -797884560802865.36, 1e-13);
  CHECK_REL(std::cyl_bessel_j(20.0, 1.5), 1.268997218933256e-21, 1e-13);
  CHECK_REL(std::cyl_bessel_i(20.0, 1.5), 1.338833183968348e-21, 1e-13);
  CHECK_REL(std::sph_bessel(60, 1.0), 1.1804018719355415e-101, 1e-13);
  // Overflow: Y_nu, K_nu, y_n ~ x^-nu; the right infinity, not NaN (the recurrence's inf - inf).
  CHECK(std::cyl_neumann(1.5, 1e-300) == -inf);
  CHECK(std::cyl_neumann(3.3, 1e-300) == -inf);
  CHECK(std::cyl_bessel_k(1.5, 1e-300) == inf);
  CHECK(std::cyl_bessel_i(-1.5, 1e-300) == -inf); // I_-nu = I_nu + (2/pi) sin(nu pi) K_nu
  CHECK(std::sph_neumann(1, 1e-300) == -inf);
  CHECK(std::sph_neumann(3, 1e-300) == -inf);
  CHECK(std::cyl_neumann(200.0, 2.0) == -inf);
  CHECK(std::sph_neumann(200, 2.0) == -inf);
  // Subnormal x: 1/x and x/2 are not usable (inf, lost bits); the values stay right.
  for (double x : {5e-324, 1e-310, 2.2250738585072014e-308}) {
    const double a = std::sqrt(2 / pi) / std::sqrt(x); // not sqrt(2 / (pi x)): 1/x overflows
    CHECK_REL(std::cyl_bessel_j(0.0, x), 1.0, 1e-15);
    CHECK_REL(std::cyl_bessel_j(0.5, x), a * x, 1e-12);
    CHECK_REL(std::cyl_neumann(0.5, x), -a, 1e-12);
    CHECK_REL(std::cyl_bessel_k(0.5, x), std::sqrt(pi / 2) / std::sqrt(x), 1e-12);
    const double lx2 = std::log(x) - std::numbers::ln2; // ln(x/2), with x/2 of the smallest subnormal being 0
    CHECK_REL(std::cyl_neumann(0.0, x), 2 / pi * (lx2 + std::numbers::egamma), 1e-12);
    CHECK_REL(std::cyl_bessel_k(0.0, x), -(lx2 + std::numbers::egamma), 1e-12);
    CHECK_REL(std::sph_bessel(0, x), 1.0, 1e-15);
    CHECK_REL(std::sph_neumann(0, x), -1 / x, 1e-12); // -inf for the subnormals, -4.5e307 at DBL_MIN
  }
  // The two sides of the x = 2 switch agree to a few ulps.
  for (double nu : {0.0, 0.5, 3.3, 20.0}) {
    const double below = std::nextafter(2.0, 0.0);
    CHECK_REL(std::cyl_bessel_j(nu, below), std::cyl_bessel_j(nu, 2.0), 1e-14);
    CHECK_REL(std::cyl_bessel_i(nu, below), std::cyl_bessel_i(nu, 2.0), 1e-14);
    CHECK_REL(std::cyl_neumann(nu, below), std::cyl_neumann(nu, 2.0), 1e-13);
    CHECK_REL(std::cyl_bessel_k(nu, below), std::cyl_bessel_k(nu, 2.0), 1e-13);
  }
  CHECK_REL(std::sph_bessel(5, std::nextafter(2.0, 0.0)), std::sph_bessel(5, 2.0), 1e-14);
  // The f and l forms.
  CHECK(std::fabs(std::cyl_bessel_if(0.5f, 1e-18f) - 7.9788458e-10f) <= 1e-6f * 7.9788458e-10f);
  CHECK(std::fabs(std::cyl_bessel_jf(0.75f, 1e-30f) - 2.0458908e-23f) <= 1e-6f * 2.0458908e-23f); // (x/2)^0.75 / Gamma(1.75)
  CHECK(std::sph_besself(0, 1e-38f) == 1.0f);
  CHECK(std::fabs(std::sph_besself(1, 1e-38f) - 1e-38f / 3) <= 1e-6f * (1e-38f / 3));
  CHECK(std::fabs(std::cyl_bessel_il(0.5L, 1e-20L) - 7.9788456080286536e-11L) <= 1e-14L * 7.9788456080286536e-11L);
  CHECK(std::fabs(std::cyl_bessel_jl(1.5L, 1e-100L) - 2.6596152026762179e-151L) <= 1e-14L * 2.6596152026762179e-151L);
  CHECK(std::sph_bessell(0, 1e-100L) == 1.0L);
  return 0;
}
