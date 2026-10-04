// [complex.syn], [complex.transcendentals], [complex.value.ops]: the functions are constexpr
// (P1383R2), so the values of complex/edge_values must also come out of constant evaluation
// (same paragraphs: right half-plane of sqrt, the branch cut of log/pow selected by the sign of
// zero, no overflowing intermediates where the result is representable).
#include <cmath>
#include <complex>
#include <limits>
#include <numbers>
#include "check.hpp"

template <class T>
constexpr bool near(T a, T b, T tol) {
  T d = a - b;
  if (d < 0) d = -d;
  T m = a < 0 ? -a : a;
  return d <= tol * (m > 1 ? m : 1);
}

template <class T>
constexpr bool values() {
  using C = std::complex<T>;
  const T pi = std::numbers::pi_v<T>;
  const T eps = 64 * std::numeric_limits<T>::epsilon();
  const T big = std::numeric_limits<T>::max() / 4;
  C e0 = std::exp(C(0, 0));
  if (e0.real() != 1 || e0.imag() != 0 || std::signbit(e0.imag())) return false;
  C lp = std::log(C(-1, T(0))), lm = std::log(C(-1, -T(0)));
  if (!near(lp.imag(), pi, eps) || !near(lm.imag(), -pi, eps) || lp.real() != 0) return false;
  C sp = std::sqrt(C(-4, T(0))), sm = std::sqrt(C(-4, -T(0)));
  if (sp != C(0, 2) || sm != C(0, -2) || std::signbit(sm.real())) return false;
  C s = std::sqrt(C(-3, -4));
  if (!near(s.real(), T(1), eps) || !near(s.imag(), T(-2), eps)) return false;
  C h = std::sqrt(C(big, big));
  if (!(h.real() > 0 && h.real() < std::numeric_limits<T>::infinity())) return false;
  if (!near(std::abs(C(big, big)), big * std::numbers::sqrt2_v<T>, eps)) return false;
  if (std::tanh(C(1000, 0)) != C(1, 0)) return false;
  C t = std::tan(C(0, 1000));
  if (!near(t.imag(), T(1), eps) || !near(t.real(), T(0), eps)) return false;
  C pw = std::pow(C(-1, -T(0)), T(0.5));
  if (!near(pw.imag(), T(-1), eps) || !near(pw.real(), T(0), eps)) return false;
  if (!near(std::arg(C(-1, -T(0))), -pi, eps)) return false;
  return true;
}

static_assert(values<float>());
static_assert(values<double>());
static_assert(values<long double>());

int main() {
  CHECK(values<float>() && values<double>() && values<long double>());
  return 0;
}
