// [complex.syn] (P1383R2): abs, arg, norm, conj, proj, polar and the transcendental functions
// are constexpr for every complex<T> with T a floating-point type ([complex.numbers.general]/2),
// so they are usable in constant expressions for float and long double as well as double.
// [complex.value.ops]/3: abs returns the magnitude of x - for a representable magnitude no
// floating-point exception other than inexact is raised (as for hypot, ISO/IEC 9899:2024
// 7.12.7.4 "without undue overflow or underflow"), which a constant expression requires
// ([library.c]/3 applies to the C functions; abs is specified by its result, so computing it with
// an overflowing intermediate would give inf instead of the magnitude).
// [complex.transcendentals]/24: sqrt returns the complex square root "in the range of the right
// half-plane"; Note 2 and ISO/IEC 9899:2024 G.6.4.2: csqrt(conj(z)) = conj(csqrt(z)), so the
// sign of a zero imaginary part selects the side of the branch cut on the negative real axis.
// [complex.value.ops]/5: norm is the squared magnitude; /9: polar(rho, theta).
#include <complex>
#include <cmath>
#include <limits>
#include <numbers>
#include "check.hpp"

template <class T>
constexpr bool close(T a, T b, T rel = 8) {
  T d = a > b ? a - b : b - a;
  T m = (a < 0 ? -a : a) > (b < 0 ? -b : b) ? (a < 0 ? -a : a) : (b < 0 ? -b : b);
  return d <= m * std::numeric_limits<T>::epsilon() * rel || d <= std::numeric_limits<T>::epsilon() * rel;
}

template <class T>
constexpr bool test() {
  using C = std::complex<T>;
  using L = std::numeric_limits<T>;
  const T pi = std::numbers::pi_v<T>;

  if (std::norm(C(3, 4)) != T(25) || std::abs(C(3, -4)) != T(5)) return false;
  if (std::conj(C(1, 2)) != C(1, -2)) return false;
  if (std::polar(T(2)) != C(2, 0) || std::polar(T(2), T(0)) != C(2, 0)) return false;
  if (!close(std::arg(C(-1, 0)), pi) || !close(std::arg(C(0, 1)), pi / 2)) return false;

  // magnitude near the overflow and underflow thresholds
  T big = std::abs(C(L::max() / 2, L::max() / 2));
  if (!close(big, L::max() / 2 * std::numbers::sqrt2_v<T>)) return false;
  constexpr int s = L::min_exponent < -10000 ? -9000 : (L::min_exponent < -1000 ? -600 : -100);
  if (!close(std::abs(C(std::ldexp(T(3), s), std::ldexp(T(-4), s))), std::ldexp(T(5), s))) return false;

  // sqrt: right half-plane, both sides of the branch cut
  C r1 = std::sqrt(C(-4, T(0)));
  C r2 = std::sqrt(C(-4, -T(0)));
  if (!(r1.real() >= 0) || !(r2.real() >= 0)) return false;
  if (!close(r1.imag(), T(2)) || !close(r2.imag(), T(-2)) || !close(r1.real() + 1, T(1))) return false;
  C r3 = std::sqrt(C(3, 4));  // = 2 + i
  if (!close(r3.real(), T(2)) || !close(r3.imag(), T(1))) return false;
  C r4 = std::sqrt(C(0, -2));  // = 1 - i
  if (!close(r4.real(), T(1)) || !close(r4.imag(), T(-1))) return false;

  // exp/log on the unit circle
  C e = std::exp(C(0, pi));
  if (!close(e.real(), T(-1)) || !close(e.imag() + 1, T(1))) return false;
  C lg = std::log(C(-1, T(0)));
  if (!close(lg.real() + 1, T(1)) || !close(lg.imag(), pi)) return false;
  C lg2 = std::log(C(-1, -T(0)));
  if (!close(lg2.imag(), -pi)) return false;
  C p = std::pow(C(0, 1), C(2, 0));  // i^2 = -1
  if (!close(p.real(), T(-1)) || !close(p.imag() + 1, T(1))) return false;
  return true;
}

static_assert(test<float>());
static_assert(test<double>());
static_assert(test<long double>());

int main() {
  CHECK(test<float>());
  CHECK(test<double>());
  CHECK(test<long double>());
  return 0;
}
