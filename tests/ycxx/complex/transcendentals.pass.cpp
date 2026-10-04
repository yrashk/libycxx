// [complex.transcendentals]: exp, log (imag(log(x)) in [-pi, pi], branch cut along the negative
// real axis), log10 = log(x) / log(10), pow = exp(y * log(x)), sqrt (in the right
// half-plane), sin, cos, tan, sinh, cosh, tanh, and the inverse functions acos, asin, atan,
// acosh, asinh, atanh. Values at points with closed forms, plus identities.
#include <complex>
#include <cmath>
#include <initializer_list>
#include <numbers>
#include "check.hpp"

using C = std::complex<double>;
const double pi = std::numbers::pi;

bool close(C a, C b, double tol = 1e-12) { return std::abs(a - b) <= tol * (std::abs(b) > 1 ? std::abs(b) : 1); }

int main() {
  const C i(0, 1);
  CHECK(close(std::exp(C(0, pi)), C(-1, 0)));
  CHECK(close(std::exp(C(1, 0)), C(std::numbers::e, 0)));
  CHECK(close(std::exp(C(std::log(2.0), pi / 2)), C(0, 2)));
  CHECK(close(std::log(C(-1, 0)), C(0, pi)) || close(std::log(C(-1, 0)), C(0, -pi)));
  CHECK(std::abs(std::log(C(-1, 0)).imag()) <= pi);
  CHECK(close(std::log(C(0, 1)), C(0, pi / 2)));
  CHECK(close(std::log(C(std::numbers::e, 0)), C(1, 0)));
  CHECK(close(std::log(C(-2, -1e-300)), C(std::log(2.0), -pi)));  // just below the cut
  CHECK(close(std::log(C(-2, 1e-300)), C(std::log(2.0), pi)));    // just above the cut
  CHECK(close(std::log10(C(100, 0)), C(2, 0)));
  CHECK(close(std::log10(C(0, 10)), std::log(C(0, 10)) / std::log(10.0)));
  CHECK(close(std::sqrt(C(-4, 0)), C(0, 2)));
  CHECK(close(std::sqrt(C(0, 2)), C(1, 1)));
  CHECK(close(std::sqrt(C(3, -4)), C(2, -1)));
  CHECK(close(std::sqrt(C(-3, -4)), C(1, -2)));
  for (C z : {C(1, 2), C(-3, 0.5), C(0.25, -7), C(-1, -1)}) {
    C r = std::sqrt(z);
    CHECK(r.real() >= 0);  // right half-plane
    CHECK(close(r * r, z));
    CHECK(close(std::exp(std::log(z)), z));
    CHECK(close(std::sin(z) * std::sin(z) + std::cos(z) * std::cos(z), C(1, 0), 1e-9));
    CHECK(close(std::cosh(z) * std::cosh(z) - std::sinh(z) * std::sinh(z), C(1, 0), 1e-9));
    CHECK(close(std::tan(z), std::sin(z) / std::cos(z), 1e-9));
    CHECK(close(std::tanh(z), std::sinh(z) / std::cosh(z), 1e-9));
    CHECK(close(std::sin(std::asin(z)), z, 1e-9));
    CHECK(close(std::cos(std::acos(z)), z, 1e-9));
    CHECK(close(std::tan(std::atan(z)), z, 1e-9));
    CHECK(close(std::sinh(std::asinh(z)), z, 1e-9));
    CHECK(close(std::cosh(std::acosh(z)), z, 1e-9));
    CHECK(close(std::tanh(std::atanh(z)), z, 1e-9));
    CHECK(close(std::pow(z, C(2, 0)), z * z, 1e-9));
    CHECK(close(std::pow(z, 0.5), std::sqrt(z), 1e-9));
    CHECK(close(std::pow(2.0, z), std::exp(z * std::log(2.0)), 1e-9));
    CHECK(close(std::pow(z, C(0.5, 1)), std::exp(C(0.5, 1) * std::log(z)), 1e-9));
  }
  CHECK(close(std::sin(C(0, 1)), C(0, std::sinh(1.0))));
  CHECK(close(std::cos(C(0, 1)), C(std::cosh(1.0), 0)));
  CHECK(close(std::sinh(C(0, pi / 2)), C(0, 1)));
  CHECK(close(std::cosh(C(0, pi)), C(-1, 0)));
  CHECK(close(std::pow(i, C(2, 0)), C(-1, 0)));
  CHECK(close(std::pow(i, i), C(std::exp(-pi / 2), 0)));
  CHECK(close(std::acos(C(0, 0)), C(pi / 2, 0)));
  CHECK(close(std::atan(C(1, 0)), C(pi / 4, 0)));
  CHECK(close(std::acosh(C(1, 0)), C(0, 0)));
  CHECK(close(std::asinh(C(0, 1)), C(0, pi / 2)));
  CHECK(close(std::atanh(C(0, 1)), C(0, pi / 4)));
  // Other element types.
  std::complex<float> f = std::exp(std::complex<float>(0, static_cast<float>(pi)));
  CHECK(std::abs(f - std::complex<float>(-1, 0)) < 1e-6f);
  std::complex<long double> l = std::sqrt(std::complex<long double>(-9, 0));
  CHECK(std::abs(l - std::complex<long double>(0, 3)) < 1e-15L);
  return 0;
}
