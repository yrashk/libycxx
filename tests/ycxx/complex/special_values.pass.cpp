// [complex.value.ops]/8 and [complex.transcendentals]/2,4,6,8,10,12: proj, acos, asin, atan,
// acosh, asinh and atanh "Behave the same as the C functions" cproj, cacos, casin, catan,
// cacosh, casinh and catanh, whose special values are those of ISO/IEC 9899:2024 Annex G
// (G.6.1.1, G.6.2.1-3): e.g. cacos(+-0 + i0) = pi/2 - i0, cacosh(+-0 + i0) = +0 + i pi/2,
// casinh(+0 + i0) = +0 + i0, casinh(+inf + iy) = +inf + i0 for positive finite y,
// catanh(+0 + i0) = +0 + i0, catanh(+inf + iy) = +0 + i pi/2, cacos(x + i inf) = pi/2 - i inf,
// cacosh(-inf + iy) = +inf + i pi, cacos(conj(z)) = conj(cacos(z)).
#include <complex>
#include <cmath>
#include <limits>
#include <numbers>
#include "check.hpp"

using C = std::complex<double>;
const double kPi = std::numbers::pi;
const double kInf = std::numeric_limits<double>::infinity();
const double kNan = std::numeric_limits<double>::quiet_NaN();

bool near(double a, double b) { return std::fabs(a - b) <= 1e-15 * std::fabs(b); }
bool pz(double x) { return x == 0 && !std::signbit(x); }
bool nz(double x) { return x == 0 && std::signbit(x); }

int main() {
  C r = std::acos(C(0, 0));
  CHECK(near(r.real(), kPi / 2) && nz(r.imag()));
  r = std::acos(C(-0.0, 0));
  CHECK(near(r.real(), kPi / 2) && nz(r.imag()));
  r = std::acos(C(0, -0.0));  // conj symmetry
  CHECK(near(r.real(), kPi / 2) && pz(r.imag()));
  r = std::acos(C(3, kInf));
  CHECK(near(r.real(), kPi / 2) && r.imag() == -kInf);
  r = std::acos(C(-kInf, 2));
  CHECK(near(r.real(), kPi) && r.imag() == -kInf);
  r = std::acos(C(kInf, 2));
  CHECK(pz(r.real()) && r.imag() == -kInf);
  r = std::acos(C(-kInf, kInf));
  CHECK(near(r.real(), 3 * kPi / 4) && r.imag() == -kInf);
  r = std::acos(C(kInf, kInf));
  CHECK(near(r.real(), kPi / 4) && r.imag() == -kInf);
  r = std::acos(C(kNan, kInf));
  CHECK(std::isnan(r.real()) && r.imag() == -kInf);
  r = std::acos(C(0, kNan));
  CHECK(near(r.real(), kPi / 2) && std::isnan(r.imag()));

  r = std::acosh(C(0, 0));
  CHECK(pz(r.real()) && near(r.imag(), kPi / 2));
  r = std::acosh(C(-0.0, 0));
  CHECK(pz(r.real()) && near(r.imag(), kPi / 2));
  r = std::acosh(C(1.5, kInf));
  CHECK(r.real() == kInf && near(r.imag(), kPi / 2));
  r = std::acosh(C(-kInf, 1));
  CHECK(r.real() == kInf && near(r.imag(), kPi));
  r = std::acosh(C(kInf, 1));
  CHECK(r.real() == kInf && pz(r.imag()));
  r = std::acosh(C(kInf, -1));
  CHECK(r.real() == kInf && nz(r.imag()));

  r = std::asinh(C(0, 0));
  CHECK(pz(r.real()) && pz(r.imag()));
  r = std::asinh(C(-0.0, -0.0));  // odd
  CHECK(nz(r.real()) && nz(r.imag()));
  r = std::asinh(C(2, kInf));
  CHECK(r.real() == kInf && near(r.imag(), kPi / 2));
  r = std::asinh(C(kInf, 2));
  CHECK(r.real() == kInf && pz(r.imag()));
  r = std::asinh(C(kInf, kInf));
  CHECK(r.real() == kInf && near(r.imag(), kPi / 4));
  r = std::asinh(C(kNan, 0));
  CHECK(std::isnan(r.real()) && pz(r.imag()));

  r = std::atanh(C(0, 0));
  CHECK(pz(r.real()) && pz(r.imag()));
  r = std::atanh(C(0, kNan));
  CHECK(pz(r.real()) && std::isnan(r.imag()));
  r = std::atanh(C(1, 0));
  CHECK(r.real() == kInf && pz(r.imag()));
  r = std::atanh(C(2, kInf));
  CHECK(pz(r.real()) && near(r.imag(), kPi / 2));
  r = std::atanh(C(kInf, 2));
  CHECK(pz(r.real()) && near(r.imag(), kPi / 2));
  r = std::atanh(C(kInf, kInf));
  CHECK(pz(r.real()) && near(r.imag(), kPi / 2));
  r = std::atanh(C(-kInf, -2));  // odd
  CHECK(nz(r.real()) && near(r.imag(), -kPi / 2));

  // casin(z) = -i casinh(iz), catan(z) = -i catanh(iz) (G.6.1.2-3 via G.6).
  r = std::asin(C(0, 0));
  CHECK(pz(r.real()) && pz(r.imag()));
  r = std::asin(C(-0.0, -0.0));
  CHECK(nz(r.real()) && nz(r.imag()));
  r = std::atan(C(0, 0));
  CHECK(pz(r.real()) && pz(r.imag()));
  r = std::atan(C(-0.0, 0));
  CHECK(nz(r.real()) && pz(r.imag()));
  r = std::atan(C(2, kInf));  // catanh(-kInf + 2i) = -0 + i kPi/2, times -i
  CHECK(near(r.real(), kPi / 2) && pz(r.imag()));
  return 0;
}
