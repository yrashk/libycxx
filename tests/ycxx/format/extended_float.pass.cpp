// [format.formatter.spec]/2.4: "For each charT, for each FloatingT that is a cv-unqualified
// floating-point type, a specialization template<> struct formatter<FloatingT, charT>" --
// including the extended floating-point types of <stdfloat> ([basic.extended.fp]) the
// implementation provides. [format.arg]/6.7 stores "a standard floating-point type" as is;
// an extended one is stored as a handle (/6.11) but still formats through its formatter.
// [format.string.std] Table 110: none without precision is to_chars(first, last, value), the
// shortest representation that round-trips in that type ([charconv.to.chars]/2, with the
// <charconv> overloads for every floating-point type); e, f, g, a with precision as for
// double. The plain to_chars picks f or e by [charconv.to.chars]/7: f only for |v| in [l, u),
// u being radix^(digits + 1) rounded down to a power of 10: 1e7 for float (2^25), 1e3 for
// float16 (2^12), 1e2 for bfloat16 (2^9); e otherwise, still with the fewest digits (/2).
// (Clang defines none of the __STDCPP_*_T__ macros, so it checks only float/double/
// long double here.)
#include <format>
#include <stdfloat>
#include <string>
#include "check.hpp"

int main() {
  static_assert(std::formattable<float, char> && std::formattable<long double, wchar_t>);
  CHECK(std::format("{} {} {}", 0.1f, 0.1, 0.1L) == "0.1 0.1 0.1");
  CHECK(std::format("{}", 16777217.0f) == "1.6777216e+07");  // float 16777216 >= u = 1e7
  CHECK(std::format("{}", 9999999.0f) == "9999999");
#if defined(__STDCPP_FLOAT16_T__)
  static_assert(std::formattable<std::float16_t, char> && std::formattable<std::float16_t, wchar_t>);
  CHECK(std::format("{}", static_cast<std::float16_t>(0.1)) == "0.1");     // nearest float16: 0.0999755859375
  CHECK(std::format("{}", static_cast<std::float16_t>(65504)) == "6.55e+04");  // largest finite, >= 1e3
  CHECK(std::format("{}", static_cast<std::float16_t>(999)) == "999");
  CHECK(std::format("{:.3e}", static_cast<std::float16_t>(1.5)) == "1.500e+00");
  CHECK(std::format("{:a}", static_cast<std::float16_t>(1.0)) == "1p+0");
  CHECK(std::format("{:+08.2f}", static_cast<std::float16_t>(-2.5)) == "-0002.50");
  CHECK(std::format(L"{}", static_cast<std::float16_t>(0.5)) == L"0.5");
#endif
#if defined(__STDCPP_BFLOAT16_T__)
  static_assert(std::formattable<std::bfloat16_t, char>);
  CHECK(std::format("{}", static_cast<std::bfloat16_t>(0.1)) == "0.1");
  CHECK(std::format("{}", static_cast<std::bfloat16_t>(257)) == "2.56e+02");  // 256 (8 bits), >= 1e2
  CHECK(std::format("{}", static_cast<std::bfloat16_t>(99)) == "99");
  CHECK(std::format("{:g}", static_cast<std::bfloat16_t>(3.0)) == "3");
#endif
#if defined(__STDCPP_FLOAT32_T__)
  static_assert(std::formattable<std::float32_t, char>);
  CHECK(std::format("{}", static_cast<std::float32_t>(0.1)) == "0.1");
  CHECK(std::format("{:e}", static_cast<std::float32_t>(1e10)) == "1.000000e+10");
#endif
#if defined(__STDCPP_FLOAT64_T__)
  static_assert(std::formattable<std::float64_t, char>);
  CHECK(std::format("{}", static_cast<std::float64_t>(0.1)) == "0.1");
  CHECK(std::format("{:.17g}", static_cast<std::float64_t>(0.1)) == "0.10000000000000001");
#endif
#if defined(__STDCPP_FLOAT128_T__)
  static_assert(std::formattable<std::float128_t, char>);
  CHECK(std::format("{}", static_cast<std::float128_t>(0.1)) == "0.1000000000000000055511151231257827");  // the double 0.1
  CHECK(std::format("{}", static_cast<std::float128_t>(1) / 3) == "0.3333333333333333333333333333333333");
  CHECK(std::format("{:.3f}", static_cast<std::float128_t>(2.0)) == "2.000");
#endif
  return 0;
}
