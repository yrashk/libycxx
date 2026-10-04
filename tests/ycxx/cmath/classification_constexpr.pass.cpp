// [cmath.syn]: fpclassify, isfinite, isinf, isnan, isnormal, signbit and the quiet comparison
// functions (isgreater ... isunordered) are constexpr in C++26 (P0533, P1383).
#include <cmath>
#include <limits>

template <class T>
constexpr bool check() {
  using L = std::numeric_limits<T>;
  constexpr T inf = L::infinity(), nan = L::quiet_NaN();
  static_assert(std::fpclassify(T(0)) == FP_ZERO);
  static_assert(std::fpclassify(T(1)) == FP_NORMAL);
  static_assert(std::fpclassify(L::denorm_min()) == FP_SUBNORMAL);
  static_assert(std::fpclassify(inf) == FP_INFINITE);
  static_assert(std::fpclassify(nan) == FP_NAN);
  static_assert(std::isfinite(L::max()) && !std::isfinite(inf) && !std::isfinite(nan));
  static_assert(std::isinf(-inf) && !std::isinf(T(1)));
  static_assert(std::isnan(nan) && !std::isnan(inf));
  static_assert(std::isnormal(L::min()) && !std::isnormal(L::denorm_min()) && !std::isnormal(T(0)));
  static_assert(std::signbit(-T(0)) && !std::signbit(T(0)) && std::signbit(-inf));
  static_assert(std::isgreater(T(2), T(1)) && !std::isgreater(nan, T(1)));
  static_assert(std::isgreaterequal(T(1), T(1)) && !std::isgreaterequal(T(1), nan));
  static_assert(std::isless(T(1), T(2)) && !std::isless(nan, nan));
  static_assert(std::islessequal(-T(0), T(0)) && !std::islessequal(nan, T(0)));
  static_assert(std::islessgreater(T(1), T(2)) && !std::islessgreater(T(0), -T(0)));
  static_assert(std::isunordered(nan, T(0)) && !std::isunordered(T(0), inf));
  return true;
}
static_assert(check<float>());
static_assert(check<double>());
static_assert(check<long double>());
static_assert(std::fpclassify(0) == FP_ZERO && std::isnormal(1) && std::signbit(-1));
static_assert(std::isless(1, 2.0f));

int main() { return 0; }
