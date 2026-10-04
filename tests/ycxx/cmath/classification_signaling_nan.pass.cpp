// [cmath.syn]: fpclassify, isfinite, isinf, isnan, isnormal and signbit are constexpr
// (P0533R9). [library.c]/3: as constant expressions they have the semantics of ISO/IEC
// 9899:2024 Annex F and are non-constant only if they raise a floating-point exception other
// than FE_INEXACT. C23 7.12.3 / F.3: the classification macros (and signbit) are IEC 60559
// non-computational operations that raise no floating-point exception, even for a signaling NaN.
// So classifying numeric_limits<T>::signaling_NaN() (or its negation, which only flips the
// sign bit, F.3: "-x" is negate) is a constant expression with the usual results.
#include <cmath>
#include <limits>
#include "check.hpp"

template <class T>
constexpr bool check() {
  using L = std::numeric_limits<T>;
  static_assert(L::has_signaling_NaN);
  constexpr T s = L::signaling_NaN();
  constexpr T ns = -L::signaling_NaN();
  static_assert(std::fpclassify(s) == FP_NAN);
  static_assert(std::fpclassify(ns) == FP_NAN);
  static_assert(std::isnan(s) && std::isnan(ns));
  static_assert(!std::isinf(s) && !std::isfinite(s) && !std::isnormal(s));
  static_assert(!std::isinf(ns) && !std::isfinite(ns) && !std::isnormal(ns));
  static_assert(!std::signbit(s));
  static_assert(std::signbit(ns));
  return true;
}

static_assert(check<float>());
static_assert(check<double>());
static_assert(check<long double>());

int main() {
  CHECK(check<float>() && check<double>() && check<long double>());
  volatile double s = std::numeric_limits<double>::signaling_NaN();
  CHECK(std::isnan(s) && std::fpclassify(s) == FP_NAN && !std::signbit(s));
  return 0;
}
