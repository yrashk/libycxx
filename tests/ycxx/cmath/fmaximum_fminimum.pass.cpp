// [cmath.syn]: C++26 adds the constexpr functions fmaximum, fmaximum_num, fminimum and
// fminimum_num (ISO/IEC 9899:2024 7.12.12.4-7). fmaximum/fminimum return NaN if either
// argument is a NaN and treat -0 as less than +0; fmaximum_num/fminimum_num return the
// numeric argument when exactly one argument is a NaN, and also order -0 below +0.
#include <cmath>
#include <limits>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::fmaximum(1.0f, 2.0f)), float>);
static_assert(std::is_same_v<decltype(std::fminimum_num(1.0L, 2.0L)), long double>);
static_assert(std::is_same_v<decltype(std::fmaximum_num(1, 2.0f)), double>);

template <class T>
constexpr bool check() {
  const T nan = std::numeric_limits<T>::quiet_NaN(), inf = std::numeric_limits<T>::infinity();
  const T pz = T(0), nz = -T(0);
  if (std::fmaximum(T(1), T(2)) != 2 || std::fminimum(T(1), T(2)) != 1) return false;
  if (!std::isnan(std::fmaximum(nan, T(1))) || !std::isnan(std::fmaximum(T(1), nan))) return false;
  if (!std::isnan(std::fminimum(nan, T(1))) || !std::isnan(std::fminimum(T(1), nan))) return false;
  if (std::signbit(std::fmaximum(nz, pz)) || std::signbit(std::fmaximum(pz, nz))) return false;
  if (!std::signbit(std::fminimum(nz, pz)) || !std::signbit(std::fminimum(pz, nz))) return false;
  if (std::fmaximum_num(nan, T(1)) != 1 || std::fmaximum_num(T(1), nan) != 1) return false;
  if (std::fminimum_num(nan, T(-1)) != -1 || std::fminimum_num(T(-1), nan) != -1) return false;
  if (!std::isnan(std::fmaximum_num(nan, nan)) || !std::isnan(std::fminimum_num(nan, nan))) return false;
  if (std::signbit(std::fmaximum_num(nz, pz)) || !std::signbit(std::fminimum_num(pz, nz))) return false;
  if (std::fmaximum(-inf, inf) != inf || std::fminimum_num(-inf, nan) != -inf) return false;
  return true;
}
static_assert(check<float>() && check<double>() && check<long double>());

int main() {
  CHECK(check<float>() && check<double>() && check<long double>());
  return 0;
}
