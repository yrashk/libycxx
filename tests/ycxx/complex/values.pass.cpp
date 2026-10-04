// [complex.value.ops]: real, imag, abs (magnitude), arg (phase angle, atan2(imag, real)), norm
// (squared magnitude), conj, proj ("Behaves the same as the C function cproj": unchanged
// unless a part is infinite, else (inf, copysign(0, imag))), polar(rho, theta = 0). All are
// constexpr.
#include <complex>
#include <cmath>
#include <limits>
#include <numbers>
#include "check.hpp"

using C = std::complex<double>;

static_assert(std::real(C(1, 2)) == 1 && std::imag(C(1, 2)) == 2);
static_assert(std::norm(C(3, 4)) == 25);
static_assert(std::conj(C(3, 4)) == C(3, -4));
static_assert(std::abs(C(3, 4)) == 5);
static_assert(std::arg(C(1, 0)) == 0);
static_assert(std::proj(C(1, 2)) == C(1, 2));
static_assert(std::polar(2.0) == C(2, 0));

bool close(double a, double b) { return std::fabs(a - b) <= 1e-12 * (std::fabs(b) > 1 ? std::fabs(b) : 1); }

int main() {
  const double pi = std::numbers::pi, inf = std::numeric_limits<double>::infinity();
  const double nan = std::numeric_limits<double>::quiet_NaN();
  CHECK(std::abs(C(-5, 12)) == 13);
  CHECK(std::abs(std::complex<float>(3, -4)) == 5.0f);
  CHECK(std::abs(std::complex<long double>(-8, -6)) == 10.0L);
  CHECK(close(std::arg(C(0, 1)), pi / 2));
  CHECK(close(std::arg(C(-1, 0)), pi));
  CHECK(close(std::arg(C(-1, -0.0)), -pi));
  CHECK(close(std::arg(C(1, -1)), -pi / 4));
  CHECK(std::arg(C(2, 3)) == std::atan2(3.0, 2.0));
  CHECK(std::norm(C(-2, 3)) == 13);
  CHECK(std::conj(C(1, -0.0)).imag() == 0 && !std::signbit(std::conj(C(1, -0.0)).imag()));
  CHECK(std::signbit(std::conj(C(1, 0)).imag()));

  // proj: values with an infinite part go to (inf, +-0).
  C p = std::proj(C(inf, -2));
  CHECK(p.real() == inf && p.imag() == 0 && std::signbit(p.imag()));
  p = std::proj(C(-inf, 3));
  CHECK(p.real() == inf && p.imag() == 0 && !std::signbit(p.imag()));
  p = std::proj(C(nan, -inf));
  CHECK(p.real() == inf && p.imag() == 0 && std::signbit(p.imag()));
  p = std::proj(C(1, inf));
  CHECK(p.real() == inf && !std::signbit(p.imag()));
  p = std::proj(C(nan, 2));
  CHECK(std::isnan(p.real()) && p.imag() == 2);

  // polar
  C z = std::polar(2.0, pi / 2);
  CHECK(std::fabs(z.real()) < 1e-15 && close(z.imag(), 2));
  z = std::polar(3.0, pi);
  CHECK(close(z.real(), -3) && std::fabs(z.imag()) < 1e-15);
  z = std::polar(0.0, 1.0);
  CHECK(z.real() == 0 && z.imag() == 0);
  std::complex<float> zf = std::polar(1.0f, 0.0f);
  CHECK(zf == std::complex<float>(1, 0));
  for (double t = -3; t < 3; t += 0.37) {
    C w = std::polar(1.5, t);
    CHECK(close(std::abs(w), 1.5) && close(std::arg(w), t));
  }
  return 0;
}
