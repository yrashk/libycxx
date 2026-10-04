// [stdfloat.syn], [basic.extended.fp]: <stdfloat> declares float16_t, float32_t, float64_t,
// float128_t and bfloat16_t exactly when __STDCPP_FLOAT16_T__ etc. are defined; each names an
// extended floating-point type with the ISO/IEC 60559 parameters of Table 15 (binary16: p = 11,
// emax = 15; binary32: p = 24, emax = 127; binary64: p = 53, emax = 1023; binary128: p = 113,
// emax = 16383; bfloat16: p = 8, emax = 127), and the literal suffixes match the types.
// [numeric.limits.general]: numeric_limits is specialized for every arithmetic type.
// Extended types are distinct from the standard ones ([basic.fundamental]/12).
#include <stdfloat>
#include <limits>
#include <type_traits>
#include "check.hpp"

template <class T, int Digits, int MaxExp, bool Iec>
constexpr bool props() {
  using L = std::numeric_limits<T>;
  static_assert(std::is_floating_point_v<T>);
  static_assert(L::is_specialized && L::radix == 2 && L::digits == Digits && L::max_exponent == MaxExp + 1);
  static_assert(L::min_exponent == 2 - MaxExp);
  static_assert(!Iec || L::is_iec559);
  static_assert(L::has_infinity && L::has_quiet_NaN);
  static_assert(!std::is_same_v<T, float> && !std::is_same_v<T, double> && !std::is_same_v<T, long double>);
  return true;
}

#ifdef __STDCPP_FLOAT16_T__
static_assert(props<std::float16_t, 11, 15, true>());
static_assert(std::is_same_v<decltype(1.5f16), std::float16_t> && std::is_same_v<decltype(1.5F16), std::float16_t>);
static_assert(sizeof(std::float16_t) == 2);
#endif
#ifdef __STDCPP_FLOAT32_T__
static_assert(props<std::float32_t, 24, 127, true>());
static_assert(std::is_same_v<decltype(1.5f32), std::float32_t> && std::is_same_v<decltype(1.5F32), std::float32_t>);
static_assert(sizeof(std::float32_t) == 4);
#endif
#ifdef __STDCPP_FLOAT64_T__
static_assert(props<std::float64_t, 53, 1023, true>());
static_assert(std::is_same_v<decltype(1.5f64), std::float64_t> && std::is_same_v<decltype(1.5F64), std::float64_t>);
#endif
#ifdef __STDCPP_FLOAT128_T__
static_assert(props<std::float128_t, 113, 16383, true>());
static_assert(std::is_same_v<decltype(1.5f128), std::float128_t> && std::is_same_v<decltype(1.5F128), std::float128_t>);
#endif
#ifdef __STDCPP_BFLOAT16_T__
static_assert(props<std::bfloat16_t, 8, 127, false>());
static_assert(std::is_same_v<decltype(1.5bf16), std::bfloat16_t> && std::is_same_v<decltype(1.5BF16), std::bfloat16_t>);
static_assert(sizeof(std::bfloat16_t) == 2);
#endif

int main() {
#ifdef __STDCPP_FLOAT32_T__
  std::float32_t a = 1.5f32;
  a += 2.25f32;
  CHECK(a == 3.75f32);
  CHECK(std::numeric_limits<std::float32_t>::epsilon() == 0x1p-23f32);
#endif
#ifdef __STDCPP_FLOAT16_T__
  CHECK(std::numeric_limits<std::float16_t>::max() == 65504.0f16);
  CHECK(std::numeric_limits<std::float16_t>::epsilon() == 0x1p-10f16);
#endif
#ifdef __STDCPP_BFLOAT16_T__
  CHECK(std::numeric_limits<std::bfloat16_t>::epsilon() == 0x1p-7bf16);
#endif
#ifdef __STDCPP_FLOAT128_T__
  CHECK(std::numeric_limits<std::float128_t>::epsilon() == 0x1p-112f128);
#endif
  return 0;
}
