// [charconv.from.chars]/6: from_chars(first, last, value, fmt = general) matches "the
// expected form of the subject sequence in the "C" locale, as described for strtod, except
// that the sign '+' may only appear in the exponent part"; ptr "points to the first
// character not matching the pattern, or has the value last if all characters match"; the
// value is "set to the parsed value, after rounding according to round_to_nearest", and ec
// is value-initialized. The subject sequence has no leading white space.
#include <charconv>
#include <limits>
#include <string_view>
#include <system_error>
#include "check.hpp"

template <class T>
std::from_chars_result fc(std::string_view s, T& v, std::chars_format f = std::chars_format::general) {
  return std::from_chars(s.data(), s.data() + s.size(), v, f);
}

template <class T>
void check_ok(std::string_view s, T want, std::size_t consumed) {
  T v{};
  auto r = fc(s, v);
  CHECK(r.ec == std::errc{});
  CHECK(r.ptr == s.data() + consumed);
  CHECK(v == want);
}

int main() {
  static_assert(std::is_same_v<decltype(std::from_chars((const char*)0, (const char*)0, *(double*)0)), std::from_chars_result>);
  check_ok<double>("1.5", 1.5, 3);
  check_ok<double>("-2.25e3", -2250.0, 7);
  check_ok<double>("2.5E-1", 0.25, 6);
  check_ok<double>("1e+2", 100.0, 4);
  check_ok<double>(".5", 0.5, 2);
  check_ok<double>("5.", 5.0, 2);
  check_ok<double>("0", 0.0, 1);
  check_ok<double>("007.25", 7.25, 6);
  check_ok<double>("123abc", 123.0, 3);
  check_ok<double>("1.5.5", 1.5, 3);
  // an exponent without digits is not part of the match
  check_ok<double>("1.5e", 1.5, 3);
  check_ok<double>("1.5e+", 1.5, 3);
  check_ok<double>("2e-x", 2.0, 1);
  // general accepts a hex-free decimal only: "0x10" is 0 followed by "x10"
  check_ok<double>("0x10", 0.0, 1);
  check_ok<float>("0.1", 0.1f, 3);
  check_ok<float>("3.4028235e38", 3.4028235e38f, 12);
  check_ok<long double>("0.1", 0.1L, 3);
  // A magnitude beyond double's range is parsed in long double itself where long double has the
  // range (x87, binary128: numeric_limits). Where long double is double (Apple arm64), 1e-4000
  // is below its smallest subnormal (not a value the test can expect), so a small double instead.
  if constexpr (std::numeric_limits<long double>::min_exponent10 < -4000)
    check_ok<long double>("-1e-4000", -1e-4000L, 8);
  else
    check_ok<long double>("-1e-300", -1e-300L, 7);
  // negative zero
  {
    double v = 1;
    auto r = fc("-0.0", v);
    CHECK(r.ec == std::errc{} && v == 0.0 && 1 / v < 0);
  }
  // a long digit sequence is consumed entirely
  {
    std::string_view s = "3.14159265358979323846264338327950288419716939937510582097494459230781640628620899";
    double v = 0;
    auto r = fc(s, v);
    CHECK(r.ec == std::errc{} && r.ptr == s.data() + s.size() && v == 3.141592653589793);
  }
  return 0;
}
