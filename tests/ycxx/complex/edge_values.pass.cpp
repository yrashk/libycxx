// Values of the complex transcendental functions where a careless formula fails although the
// result is mathematically defined and representable ([complex.numbers.general]/3 leaves only
// the other cases undefined), and the branch cuts.
//   [complex.transcendentals]/15 exp, /16-17 log: "imag(log(x)) lies in the interval [-pi, pi]",
//     branch cut along the negative real axis (Note 1: the semantics of clog in C, i.e. ISO C
//     G.6.3.2: clog(-0 + i0) aside, the side of the cut is selected by the sign of the zero
//     imaginary part: log(-1 + i0) = i pi, log(-1 - i0) = -i pi), /18-19 log10 = log(x)/log(10),
//     /20-21 pow = exp(y * log(x)) with the same cut, /23 sinh, /14 cosh, /27 tanh,
//     /22 sin, /13 cos, /26 tan, /24-25 sqrt: "in the range of the right half-plane" (Note 2:
//     csqrt, G.6.4.2: csqrt(conj(z)) = conj(csqrt(z)), csqrt(+-0 + i0) = +0 + i0)
//   [complex.value.ops]/1 abs: the magnitude, /3 arg: the phase angle (C carg: atan2(imag, real))
//   Pitfalls exercised: |z| or tanh/sinh of large arguments computed through an overflowing
//   intermediate (exp(1000), x*x for x = 1e200), subnormal magnitudes underflowing to 0.
#include <cmath>
#include <complex>
#include <initializer_list>
#include <limits>
#include <numbers>
#include "check.hpp"

template <class T>
bool near(T a, T b, T rel = 64 * std::numeric_limits<T>::epsilon()) {
  if (a == b) return true;
  return std::fabs(a - b) <= rel * std::fmax(std::fabs(a), std::fabs(b));
}
template <class T>
bool nearc(std::complex<T> a, std::complex<T> b, T abs_tol = 0) {
  auto ok = [&](T x, T y) { return near(x, y) || std::fabs(x - y) <= abs_tol; };
  return ok(a.real(), b.real()) && ok(a.imag(), b.imag());
}
template <class T>
bool pzero(T x) { return x == 0 && !std::signbit(x); }
template <class T>
bool nzero(T x) { return x == 0 && std::signbit(x); }

template <class T>
void run() {
  using C = std::complex<T>;
  const T pi = std::numbers::pi_v<T>;
  const T big = std::numeric_limits<T>::max() / 4;

  // exp
  C e0 = std::exp(C(0, 0));
  CHECK(e0.real() == 1 && pzero(e0.imag()));
  C e1 = std::exp(C(-0.0, -0.0));
  CHECK(e1.real() == 1 && nzero(e1.imag()));
  CHECK(nearc(std::exp(C(0, pi)), C(-1, 0), T(1e-6)));
  CHECK(std::exp(C(1, 0)).imag() == 0 && near(std::exp(C(1, 0)).real(), std::numbers::e_v<T>));
  C tiny = std::exp(C(-1000, 1));  // e^-1000 underflows to 0 for every type here except long double
  CHECK(sizeof(T) > sizeof(double) || (tiny.real() == 0 && tiny.imag() == 0));

  // log: branch cut and range of the imaginary part
  C lp = std::log(C(-1, T(0)));
  C lm = std::log(C(-1, -T(0)));
  CHECK(pzero(lp.real()) && near(lp.imag(), pi));
  CHECK(pzero(lm.real()) && near(lm.imag(), -pi));
  CHECK(std::log(C(1, 0)) == C(0, 0) && pzero(std::log(C(1, 0)).real()));
  CHECK(nearc(std::log(C(0, 1)), C(0, pi / 2)));
  CHECK(nearc(std::log(C(-2, -0.0)), C(std::log(T(2)), -pi)));
  for (T y : {T(1e-30), T(1), T(1e30)})
    for (T x : {T(-1e30), T(-1), T(-1e-30)}) {
      C l = std::log(C(x, -y));
      CHECK(l.imag() >= -pi && l.imag() < 0);
      CHECK(std::log(C(x, y)).imag() <= pi);
    }
  // no overflow in |z| for huge arguments: log(big(1+i)) = log(big) + log(sqrt 2) + i pi/4
  CHECK(nearc(std::log(C(big, big)), C(std::log(big) + std::log(T(2)) / 2, pi / 4)));
  CHECK(nearc(std::log10(C(-10, 0)), C(1, pi / std::log(T(10)))));
  CHECK(near(std::log10(C(1000, 0)).real(), T(3)) && std::log10(C(1000, 0)).imag() == 0);

  // sqrt: right half-plane, conj symmetry, cut
  CHECK(std::sqrt(C(4, 0)) == C(2, 0));
  C s0 = std::sqrt(C(-0.0, 0));
  CHECK(pzero(s0.real()) && pzero(s0.imag()));
  C s1 = std::sqrt(C(-0.0, -0.0));
  CHECK(pzero(s1.real()) && nzero(s1.imag()));
  C sp = std::sqrt(C(-4, T(0))), sm = std::sqrt(C(-4, -T(0)));
  CHECK(pzero(sp.real()) && sp.imag() == 2);
  CHECK(pzero(sm.real()) && sm.imag() == -2);
  CHECK(nearc(std::sqrt(C(-3, -4)), C(1, -2)));
  CHECK(nearc(std::sqrt(C(0, 2)), C(1, 1)));
  for (T x : {T(-5), T(-1e-20), T(0), T(3), T(1e20)})
    for (T y : {T(-7), T(-1e-20), T(1e-20), T(2)}) {
      C r = std::sqrt(C(x, y));
      CHECK(r.real() >= 0);
      CHECK(std::signbit(r.imag()) == std::signbit(y));
      CHECK(nearc(r * r, C(x, y), 8 * std::numeric_limits<T>::epsilon() * (std::fabs(x) + std::fabs(y))));
    }
  {
    C r = std::sqrt(C(big, big));  // |z| = big * sqrt 2 does not overflow for big = max/4
    T m = std::sqrt(big) * std::sqrt((std::sqrt(T(2)) + 1) / 2);
    CHECK(near(r.real(), m) && near(r.imag(), big / (2 * m)));
    C h = std::sqrt(C(std::numeric_limits<T>::max(), std::numeric_limits<T>::max()));
    CHECK(std::isfinite(h.real()) && std::isfinite(h.imag()) && h.real() > 0);
  }
  {
    const T d = std::numeric_limits<T>::denorm_min();
    C r = std::sqrt(C(0, 8 * d));  // sqrt(8d i) = 2 sqrt(d) (1 + i): not 0
    CHECK(r.real() > 0 && near(r.real(), 2 * std::sqrt(d), T(1e-6)) && near(r.real(), r.imag(), T(1e-6)));
  }

  // abs / arg
  CHECK(near(std::abs(C(big, big)), big * std::sqrt(T(2))));
  CHECK(std::abs(C(3, 4)) == 5 && std::norm(C(3, 4)) == 25);
  {
    const T d = std::numeric_limits<T>::denorm_min();
    CHECK(std::abs(C(3 * d, 4 * d)) == 5 * d);
  }
  CHECK(near(std::arg(C(-1, -0.0)), -pi) && near(std::arg(C(-1, 0)), pi));
  CHECK(pzero(std::arg(C(1, 0))) && nzero(std::arg(C(1, -0.0))));

  // pow = exp(y log x), same cut
  CHECK(nearc(std::pow(C(-1, T(0)), C(T(0.5), 0)), C(0, 1), T(1e-7)));
  CHECK(nearc(std::pow(C(-1, -T(0)), C(T(0.5), 0)), C(0, -1), T(1e-7)));
  CHECK(nearc(std::pow(C(-1, -T(0)), T(0.5)), C(0, -1), T(1e-7)));
  CHECK(nearc(std::pow(C(0, 1), C(2, 0)), C(-1, 0), T(1e-7)));
  CHECK(nearc(std::pow(C(4, 0), T(0.5)), C(2, 0), T(1e-7)));
  CHECK(nearc(std::pow(T(2), C(3, 0)), C(8, 0), T(1e-6)));
  CHECK(nearc(std::pow(C(0, 1), C(0, 1)), C(std::exp(-pi / 2), 0), T(1e-7)));  // i^i

  // hyperbolic and trigonometric: no inf/inf for large arguments
  CHECK(std::tanh(C(1000, 0)) == C(1, 0));
  CHECK(nearc(std::tanh(C(-1000, 1)), C(-1, 0), T(1e-300)));
  CHECK(nearc(std::tanh(C(800, -2)), C(1, -0.0), T(1e-300)));
  CHECK(nearc(std::tan(C(0, 1000)), C(0, 1), T(1e-300)));
  CHECK(nearc(std::tan(C(1, -1000)), C(0, -1), T(1e-300)));
  if constexpr (sizeof(T) == sizeof(double)) {
    // sinh(710.4) = 8.98e307 is representable although exp(710.4) is not
    C sh = std::sinh(C(710.4, 0)), ch = std::cosh(C(-710.4, 0));
    CHECK(std::isfinite(sh.real()) && near(sh.real(), std::sinh(T(710.4))) && sh.imag() == 0);
    CHECK(std::isfinite(ch.real()) && near(ch.real(), std::cosh(T(710.4))) && ch.imag() == 0);
  }
  C z0 = std::sinh(C(0, 0));
  CHECK(pzero(z0.real()) && pzero(z0.imag()));
  CHECK(nearc(std::cosh(C(0, pi)), C(-1, 0), T(1e-6)));
  CHECK(nearc(std::sin(C(0, 1)), C(0, std::sinh(T(1)))));
  CHECK(nearc(std::cos(C(0, 1)), C(std::cosh(T(1)), 0), T(1e-300)));
  CHECK(nearc(std::sin(C(1, 2)), C(std::sin(T(1)) * std::cosh(T(2)), std::cos(T(1)) * std::sinh(T(2)))));
  CHECK(nearc(std::cos(C(1, 2)), C(std::cos(T(1)) * std::cosh(T(2)), -std::sin(T(1)) * std::sinh(T(2)))));
  CHECK(nearc(std::tan(C(1, 2)), std::sin(C(1, 2)) / std::cos(C(1, 2))));
  CHECK(nearc(std::tanh(C(1, 2)), std::sinh(C(1, 2)) / std::cosh(C(1, 2))));
}

int main() {
  run<float>();
  run<double>();
  run<long double>();
  return 0;
}
