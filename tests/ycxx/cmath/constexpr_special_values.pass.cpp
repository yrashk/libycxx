// [cmath.syn]: the exponential, logarithmic, power, trigonometric and hyperbolic functions are
// constexpr in C++26 (P1383R2). [library.c]/3: as constant expressions they have the
// semantics of ISO/IEC 9899:2024 Annex F, which fixes these special values exactly (F.10):
// exp(+-0) = 1, exp(-inf) = +0, log(1) = +0, log(+inf) = +inf, log2(1) = log10(1) = +0,
// sin(+-0) = +-0, cos(+-0) = 1, tan(+-0) = +-0, atan2(+-0, +0) = +-0, atan(+-0) = +-0,
// sinh(+-0) = +-0, cosh(+-0) = 1, tanh(+-inf) = +-1, pow(x, +-0) = 1 for any x (even NaN),
// pow(+1, y) = 1 for any y (even NaN), expm1(+-0) = +-0, log1p(+-0) = +-0, exp2(3) is exact.
#include <cmath>
#include <limits>
#include "check.hpp"

template <class T>
constexpr bool special() {
  using L = std::numeric_limits<T>;
  const T inf = L::infinity(), nan = L::quiet_NaN(), nz = -T(0);
  if (std::exp(T(0)) != 1 || std::exp(nz) != 1 || std::exp(-inf) != 0 || std::exp(inf) != inf) return false;
  if (std::exp2(T(3)) != 8 || std::exp2(-inf) != 0) return false;
  if (std::expm1(T(0)) != 0 || !std::signbit(std::expm1(nz)) || std::expm1(-inf) != -1) return false;
  if (std::log(T(1)) != 0 || std::signbit(std::log(T(1))) || std::log(inf) != inf) return false;
  if (std::log2(T(1)) != 0 || std::log10(T(1)) != 0 || std::log2(inf) != inf) return false;
  if (!std::signbit(std::log1p(nz)) || std::log1p(T(0)) != 0) return false;
  if (std::sin(T(0)) != 0 || !std::signbit(std::sin(nz)) || std::cos(nz) != 1 || std::cos(T(0)) != 1) return false;
  if (!std::signbit(std::tan(nz)) || std::tan(T(0)) != 0) return false;
  if (std::asin(T(0)) != 0 || !std::signbit(std::asin(nz)) || std::acos(T(1)) != 0) return false;
  if (!std::signbit(std::atan(nz)) || std::atan2(T(0), T(0)) != 0 || !std::signbit(std::atan2(nz, T(0)))) return false;
  if (std::sinh(T(0)) != 0 || !std::signbit(std::sinh(nz)) || std::cosh(nz) != 1) return false;
  if (std::tanh(inf) != 1 || std::tanh(-inf) != -1 || !std::signbit(std::tanh(nz))) return false;
  if (std::asinh(T(0)) != 0 || std::acosh(T(1)) != 0 || std::atanh(T(0)) != 0) return false;
  if (std::pow(nan, T(0)) != 1 || std::pow(T(1), nan) != 1 || std::pow(T(-1), inf) != 1) return false;
  if (std::pow(T(2), T(-1)) != T(0.5) || std::pow(inf, T(-1)) != 0) return false;
  if (std::isnan(std::exp(nan)) == false || std::isnan(std::sin(nan)) == false) return false;
  if (std::erf(T(0)) != 0 || std::erfc(T(0)) != 1 || std::erf(inf) != 1 || std::erfc(inf) != 0) return false;
  if (std::tgamma(T(1)) != 1 || std::lgamma(T(1)) != 0 || std::lgamma(T(2)) != 0) return false;
  return true;
}

static_assert(special<float>());
static_assert(special<double>());
static_assert(special<long double>());
static_assert(std::expf(0.0f) == 1.0f && std::logl(1.0L) == 0.0L && std::powf(4.0f, 0.5f) == 2.0f);

int main() {
  CHECK(special<float>() && special<double>() && special<long double>());
  return 0;
}
