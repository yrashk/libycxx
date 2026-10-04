// [numeric.limits.members]: for floating-point types digits is "the number of radix digits in
// the significand" (FLT_MANT_DIG...), digits10 (FLT_DIG...), max_digits10, min() is the
// minimum positive normalized value (FLT_MIN...), max() (FLT_MAX...), lowest(), epsilon()
// (FLT_EPSILON...), min_exponent (FLT_MIN_EXP...), min_exponent10, max_exponent,
// max_exponent10, denorm_min() "Minimum positive subnormal value, if available. Otherwise,
// minimum positive normalized value.", radix (FLT_RADIX). has_infinity, has_quiet_NaN and
// has_signaling_NaN "Shall be true for all specializations in which is_iec559 != false".
#include <limits>
#include <cfloat>

template <class T>
constexpr bool common() {
  using L = std::numeric_limits<T>;
  static_assert(L::is_specialized && L::is_signed && !L::is_integer && !L::is_exact && L::is_bounded);
  static_assert(!L::is_modulo);
  static_assert(L::radix == FLT_RADIX);
  static_assert(L::lowest() == -L::max());
  static_assert(L::min() > 0 && L::denorm_min() > 0 && L::denorm_min() <= L::min());
  static_assert(T(1) + L::epsilon() > T(1));
  static_assert(T(1) + L::epsilon() / 2 == T(1) || L::round_style != std::round_to_nearest);
  if constexpr (L::is_iec559) {
    static_assert(L::has_infinity && L::has_quiet_NaN && L::has_signaling_NaN);
    static_assert(L::infinity() > L::max() && -L::infinity() < L::lowest());
    static_assert(L::quiet_NaN() != L::quiet_NaN());
  }
  // max_digits10 = ceil(1 + digits * log10(radix)) for radix 2
  static_assert(L::max_digits10 == 2 + (L::digits * 30103) / 100000);
  return true;
}
static_assert(common<float>());
static_assert(common<double>());
static_assert(common<long double>());

using F = std::numeric_limits<float>;
static_assert(F::digits == FLT_MANT_DIG && F::digits10 == FLT_DIG && F::max_digits10 == FLT_DECIMAL_DIG);
static_assert(F::min() == FLT_MIN && F::max() == FLT_MAX && F::epsilon() == FLT_EPSILON && F::denorm_min() == FLT_TRUE_MIN);
static_assert(F::min_exponent == FLT_MIN_EXP && F::min_exponent10 == FLT_MIN_10_EXP);
static_assert(F::max_exponent == FLT_MAX_EXP && F::max_exponent10 == FLT_MAX_10_EXP);
using D = std::numeric_limits<double>;
static_assert(D::digits == DBL_MANT_DIG && D::digits10 == DBL_DIG && D::max_digits10 == DBL_DECIMAL_DIG);
static_assert(D::min() == DBL_MIN && D::max() == DBL_MAX && D::epsilon() == DBL_EPSILON && D::denorm_min() == DBL_TRUE_MIN);
static_assert(D::min_exponent == DBL_MIN_EXP && D::min_exponent10 == DBL_MIN_10_EXP);
static_assert(D::max_exponent == DBL_MAX_EXP && D::max_exponent10 == DBL_MAX_10_EXP);
using LD = std::numeric_limits<long double>;
static_assert(LD::digits == LDBL_MANT_DIG && LD::digits10 == LDBL_DIG && LD::max_digits10 == LDBL_DECIMAL_DIG);
static_assert(LD::min() == LDBL_MIN && LD::max() == LDBL_MAX && LD::epsilon() == LDBL_EPSILON);
static_assert(LD::denorm_min() == LDBL_TRUE_MIN);
static_assert(LD::min_exponent == LDBL_MIN_EXP && LD::max_exponent == LDBL_MAX_EXP);
static_assert(LD::min_exponent10 == LDBL_MIN_10_EXP && LD::max_exponent10 == LDBL_MAX_10_EXP);

// IEC 60559 binary32/binary64 on this platform (x86-64)
static_assert(F::is_iec559 && D::is_iec559);
static_assert(F::digits == 24 && D::digits == 53);
