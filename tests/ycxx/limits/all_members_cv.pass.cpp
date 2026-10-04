// [numeric.limits.general]/1: numeric_limits<T> declares the static data members is_specialized,
// digits, digits10, max_digits10, is_signed, is_integer, is_exact, radix, min_exponent,
// min_exponent10, max_exponent, max_exponent10, has_infinity, has_quiet_NaN, has_signaling_NaN,
// is_iec559, is_bounded, is_modulo, traps, tinyness_before, round_style and the functions min,
// max, lowest, epsilon, round_error, infinity, quiet_NaN, signaling_NaN, denorm_min, all
// "static constexpr" (the data members are inline variables: odr-usable without a definition
// elsewhere), the functions noexcept and returning T.
// /4: "Specializations shall be provided for each arithmetic type, both floating-point and
// integer, including bool." /5: "The value of each member of a specialization of numeric_limits
// on a cv-qualified type cv T shall be equal to the value of the corresponding member of the
// specialization on the unqualified type T."
// Checked for every standard arithmetic type (and the extended floating-point types that exist)
// under const, volatile and const volatile, odr-using every data member at run time.
#include <limits>
#include <stdfloat>
#include <type_traits>
#include "check.hpp"

template <class T> bool same_value(T a, T b) {
  if constexpr (std::numeric_limits<T>::has_quiet_NaN) {
    if (a != a) return b != b;   // NaN compares by NaN-ness
  }
  return a == b;
}

template <class T, class Q> bool check_q() {
  using L = std::numeric_limits<T>;
  using C = std::numeric_limits<Q>;
  static_assert(std::is_same_v<decltype(C::digits), const int> && std::is_same_v<decltype(C::traps), const bool> &&
                std::is_same_v<decltype(C::round_style), const std::float_round_style>);
  static_assert(std::is_same_v<decltype(C::max()), Q> || std::is_same_v<decltype(C::max()), T>);
  static_assert(noexcept(C::signaling_NaN()) && noexcept(C::denorm_min()) && noexcept(C::round_error()));
  static_assert(C::is_specialized == L::is_specialized && C::digits == L::digits && C::digits10 == L::digits10 &&
                C::max_digits10 == L::max_digits10 && C::is_signed == L::is_signed && C::is_integer == L::is_integer &&
                C::is_exact == L::is_exact && C::radix == L::radix && C::min_exponent == L::min_exponent &&
                C::min_exponent10 == L::min_exponent10 && C::max_exponent == L::max_exponent &&
                C::max_exponent10 == L::max_exponent10 && C::has_infinity == L::has_infinity &&
                C::has_quiet_NaN == L::has_quiet_NaN && C::has_signaling_NaN == L::has_signaling_NaN &&
                C::is_iec559 == L::is_iec559 && C::is_bounded == L::is_bounded && C::is_modulo == L::is_modulo &&
                C::traps == L::traps && C::tinyness_before == L::tinyness_before && C::round_style == L::round_style);
  // odr-use every static data member (address taken at run time).
  const void* addrs[] = {&C::is_specialized, &C::digits, &C::digits10, &C::max_digits10, &C::is_signed,
                         &C::is_integer, &C::is_exact, &C::radix, &C::min_exponent, &C::min_exponent10,
                         &C::max_exponent, &C::max_exponent10, &C::has_infinity, &C::has_quiet_NaN,
                         &C::has_signaling_NaN, &C::is_iec559, &C::is_bounded, &C::is_modulo, &C::traps,
                         &C::tinyness_before, &C::round_style};
  for (const void* a : addrs) CHECK(a != nullptr);
  const int& dref = C::digits;
  CHECK(dref == L::digits);
  T vals_c[] = {T(C::min()), T(C::max()), T(C::lowest()), T(C::epsilon()), T(C::round_error()),
                T(C::infinity()), T(C::quiet_NaN()), T(C::signaling_NaN()), T(C::denorm_min())};
  T vals_l[] = {L::min(), L::max(), L::lowest(), L::epsilon(), L::round_error(),
                L::infinity(), L::quiet_NaN(), L::signaling_NaN(), L::denorm_min()};
  for (int i = 0; i < 9; ++i) {
    if (i == 7) continue;   // signaling NaN may quiet when copied
    CHECK(same_value(vals_c[i], vals_l[i]));
  }
  CHECK(C::is_specialized);
  return true;
}

template <class T> bool check() {
  return check_q<T, T>() && check_q<T, const T>() && check_q<T, volatile T>() && check_q<T, const volatile T>();
}

int main() {
  CHECK(check<bool>());
  CHECK(check<char>() && check<signed char>() && check<unsigned char>());
  CHECK(check<wchar_t>() && check<char8_t>() && check<char16_t>() && check<char32_t>());
  CHECK(check<short>() && check<unsigned short>() && check<int>() && check<unsigned>());
  CHECK(check<long>() && check<unsigned long>() && check<long long>() && check<unsigned long long>());
  CHECK(check<float>() && check<double>() && check<long double>());
#if defined(__STDCPP_FLOAT16_T__)
  CHECK(check<std::float16_t>());
#endif
#if defined(__STDCPP_FLOAT32_T__)
  CHECK(check<std::float32_t>());
#endif
#if defined(__STDCPP_FLOAT64_T__)
  CHECK(check<std::float64_t>());
#endif
#if defined(__STDCPP_FLOAT128_T__)
  CHECK(check<std::float128_t>());
#endif
#if defined(__STDCPP_BFLOAT16_T__)
  CHECK(check<std::bfloat16_t>());
#endif
  return 0;
}
