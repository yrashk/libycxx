// [numeric.limits.general]/4: "Specializations shall be provided for each arithmetic type, both
// floating-point and integer" (the extended floating-point types of [basic.extended.fp] are
// floating-point types), and /5: cv-qualified versions give the same values.
// [basic.extended.fp]: float16_t is ISO/IEC 60559 binary16, float32_t binary32, float64_t
// binary64, float128_t binary128; bfloat16_t has "8 bits of precision" and the exponent range
// of binary32. [numeric.limits.members]: digits ("number of radix digits"), digits10 ("number
// of base 10 digits that can be represented without change", floor((p-1) log10 2)),
// max_digits10 (ceil(1 + p log10 2)), min/max/lowest/epsilon/denorm_min, min_exponent ("one
// more than the minimum negative integer such that radix raised to the power of one less than
// that integer is a normalized floating-point number"), min_exponent10, max_exponent,
// max_exponent10, round_error ("measure of the maximum rounding error": 0.5 for
// round_to_nearest), is_iec559 "true if and only if the type adheres to ISO/IEC 60559".
// The values below follow from the formats (binary16: p = 11, emin = -14, emax = 15; bfloat16:
// p = 8, emin = -126, emax = 127; binary128: p = 113, emin = -16382, emax = 16383).
#include <limits>
#include <stdfloat>
#include <type_traits>

template <class T> constexpr T pow2(int e) {
  T r = 1;
  if (e >= 0) { for (int i = 0; i < e; ++i) r = r * T(2); }
  else { for (int i = 0; i < -e; ++i) r = r / T(2); }
  return r;
}

template <class T, int P, int Emin, int Emax, int D10, int MaxD10, int Min10, int Max10, bool Iec>
constexpr bool check_one() {
  using L = std::numeric_limits<T>;
  static_assert(L::is_specialized && L::is_signed && !L::is_integer && !L::is_exact && L::is_bounded);
  static_assert(L::radix == 2 && !L::is_modulo);
  static_assert(L::digits == P && L::digits10 == D10 && L::max_digits10 == MaxD10);
  static_assert(L::min_exponent == Emin + 1 && L::max_exponent == Emax + 1);
  static_assert(L::min_exponent10 == Min10 && L::max_exponent10 == Max10);
  static_assert(L::min() == pow2<T>(Emin));
  static_assert(L::max() == (T(2) - pow2<T>(1 - P)) * pow2<T>(Emax));
  static_assert(L::lowest() == -L::max());
  static_assert(L::epsilon() == pow2<T>(1 - P));
  static_assert(L::denorm_min() == pow2<T>(Emin + 1 - P));
  static_assert(L::round_style == std::round_to_nearest && L::round_error() == T(0.5));
  static_assert(L::has_infinity && L::has_quiet_NaN);
  static_assert(L::infinity() > L::max() && -L::infinity() < L::lowest());
  static_assert(L::quiet_NaN() != L::quiet_NaN());
  if constexpr (Iec) static_assert(L::is_iec559 && L::has_signaling_NaN);
  static_assert(std::is_same_v<decltype(L::max()), T> && std::is_same_v<decltype(L::denorm_min()), T>);
  static_assert(noexcept(L::max()) && noexcept(L::infinity()));
  return true;
}
template <class T, int P, int Emin, int Emax, int D10, int MaxD10, int Min10, int Max10, bool Iec>
constexpr bool check() {
  using L = std::numeric_limits<T>;
  using CL = std::numeric_limits<const volatile T>;
  static_assert(CL::is_specialized && CL::digits == L::digits && CL::max() == L::max() &&
                CL::min_exponent10 == L::min_exponent10 && CL::is_iec559 == L::is_iec559);
  return check_one<T, P, Emin, Emax, D10, MaxD10, Min10, Max10, Iec>();
}

#if defined(__STDCPP_FLOAT16_T__)
static_assert(check<std::float16_t, 11, -14, 15, 3, 5, -4, 4, true>());
static_assert(std::numeric_limits<std::float16_t>::max() == std::float16_t(65504));
#endif
#if defined(__STDCPP_BFLOAT16_T__)
static_assert(check<std::bfloat16_t, 8, -126, 127, 2, 4, -37, 38, false>());
#endif
#if defined(__STDCPP_FLOAT32_T__)
static_assert(check<std::float32_t, 24, -126, 127, 6, 9, -37, 38, true>());
#endif
#if defined(__STDCPP_FLOAT64_T__)
static_assert(check<std::float64_t, 53, -1022, 1023, 15, 17, -307, 308, true>());
#endif
#if defined(__STDCPP_FLOAT128_T__)
static_assert(check<std::float128_t, 113, -16382, 16383, 33, 36, -4931, 4932, true>());
#endif
// The standard types on this platform (binary32, binary64) with the same formulas.
static_assert(check<float, 24, -126, 127, 6, 9, -37, 38, true>());
static_assert(check<double, 53, -1022, 1023, 15, 17, -307, 308, true>());

int main() {}
