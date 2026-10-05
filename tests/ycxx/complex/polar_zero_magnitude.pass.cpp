// [complex.value.ops] polar(rho, theta): "Preconditions: rho is non-negative and non-NaN.
// Returns: The complex value corresponding to a complex number whose magnitude is rho and whose
// phase angle is theta." -0.0 is not negative (it compares equal to 0), so polar(-0.0, theta)
// is a valid call whose result has magnitude 0: both parts are zeros, never NaN. Also for
// rho == +0.0 and for the default theta.
// COUNTERPART: libcxx:numerics/complex.number/complex.value.ops/polar.pass.cpp
#include <cmath>
#include <complex>
#include "check.hpp"

template <class T>
void check(T rho, T theta) {
  const std::complex<T> z = std::polar(rho, theta);
  CHECK(!std::isnan(z.real()) && !std::isnan(z.imag()));
  CHECK(z.real() == 0 && z.imag() == 0);
  CHECK(std::abs(z) == 0);
}

template <class T>
void all() {
  const T thetas[] = {T(0), T(1), T(-1), T(3.14159), T(-2.5), T(100)};
  for (T th : thetas) {
    check<T>(T(-0.0), th);
    check<T>(T(0.0), th);
  }
  const std::complex<T> d = std::polar(T(-0.0));
  CHECK(d.real() == 0 && d.imag() == 0 && !std::isnan(d.imag()));
}

int main() {
  all<float>();
  all<double>();
  all<long double>();
}
