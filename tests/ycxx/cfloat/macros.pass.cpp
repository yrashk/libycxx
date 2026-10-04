// [cfloat.syn]: "#define __STDC_VERSION_FLOAT_H__ 202311L" and FLT_ROUNDS, FLT_EVAL_METHOD,
// FLT_RADIX, INFINITY, NAN, FLT_SNAN, DBL_SNAN, LDBL_SNAN, {FLT,DBL,LDBL}_{MANT_DIG,
// DECIMAL_DIG, DIG, MIN_EXP, MIN_10_EXP, MAX_EXP, MAX_10_EXP, MAX, EPSILON, MIN, TRUE_MIN}.
// /1: "defines all macros the same as the C standard library header <float.h>."
// C 5.3.5.3.3 / 7.12: values describe float, double, long double (matching numeric_limits);
// INFINITY: "a constant expression of type float representing positive or unsigned
// infinity, if available"; NAN: "defined if and only if the implementation supports quiet
// NaNs for the float type. It expands to a constant expression of type float representing a
// quiet NaN." FLT_SNAN etc. are defined iff signaling NaNs are supported.
#include <cfloat>
#include <limits>
#include <type_traits>
#include "check.hpp"

#if !defined(__STDC_VERSION_FLOAT_H__) || __STDC_VERSION_FLOAT_H__ != 202311L
#  error "__STDC_VERSION_FLOAT_H__ must be 202311L"
#endif
#if !defined(FLT_ROUNDS) || !defined(FLT_EVAL_METHOD) || !defined(FLT_RADIX)
#  error "FLT_ROUNDS, FLT_EVAL_METHOD, FLT_RADIX"
#endif
#if !defined(INFINITY) || !defined(NAN)
#  error "INFINITY and NAN (IEC 60559 float has both)"
#endif
#if FLT_RADIX != 2 || FLT_MANT_DIG < 1 || DBL_DIG < 10 || FLT_EVAL_METHOD < -1
#  error "usable in #if"
#endif

template <class T>
using L = std::numeric_limits<T>;
#define FAMILY(P, T)                                                         \
  static_assert(P##_MANT_DIG == L<T>::digits);                               \
  static_assert(P##_DIG == L<T>::digits10);                                  \
  static_assert(P##_DECIMAL_DIG == L<T>::max_digits10);                      \
  static_assert(P##_MIN_EXP == L<T>::min_exponent);                          \
  static_assert(P##_MIN_10_EXP == L<T>::min_exponent10);                     \
  static_assert(P##_MAX_EXP == L<T>::max_exponent);                          \
  static_assert(P##_MAX_10_EXP == L<T>::max_exponent10);                     \
  static_assert(P##_MAX == L<T>::max() && std::is_same_v<decltype(P##_MAX), T>);           \
  static_assert(P##_MIN == L<T>::min() && std::is_same_v<decltype(P##_MIN), T>);           \
  static_assert(P##_EPSILON == L<T>::epsilon() && std::is_same_v<decltype(P##_EPSILON), T>); \
  static_assert(P##_TRUE_MIN == L<T>::denorm_min() && std::is_same_v<decltype(P##_TRUE_MIN), T>)

FAMILY(FLT, float);
FAMILY(DBL, double);
FAMILY(LDBL, long double);
static_assert(FLT_RADIX == L<float>::radix);

static_assert(std::is_same_v<std::remove_cv_t<decltype(INFINITY)>, float>);
static_assert(std::is_same_v<std::remove_cv_t<decltype(NAN)>, float>);
constexpr float inf = INFINITY;
static_assert(inf == L<float>::infinity() && inf > 0);
constexpr float qnan = NAN;
static_assert(qnan != qnan);
#ifdef FLT_SNAN
static_assert(std::is_same_v<std::remove_cv_t<decltype(FLT_SNAN)>, float>);
#endif
#ifdef DBL_SNAN
static_assert(std::is_same_v<std::remove_cv_t<decltype(DBL_SNAN)>, double>);
#endif
#ifdef LDBL_SNAN
static_assert(std::is_same_v<std::remove_cv_t<decltype(LDBL_SNAN)>, long double>);
#endif

int main() {
  // FLT_ROUNDS need not be a constant expression: evaluate at run time.
  // C: -1 indeterminable, 0 toward zero, 1 to nearest, 2 toward +inf, 3 toward -inf.
  int r = FLT_ROUNDS;
  CHECK(r >= -1);
  // [numeric.limits.members] footnote: round_style is "Equivalent to FLT_ROUNDS"; nothing in
  // this program changes the rounding mode.
  CHECK(r == static_cast<int>(L<float>::round_style));
#ifdef FLT_SNAN
  volatile float s = FLT_SNAN;
  CHECK(s != s);
#endif
  return 0;
}
