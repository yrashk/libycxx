// [charconv.to.chars]/2: to_chars without a precision produces "the smallest number of
// characters such that there is at least one digit before the radix point (if present) and
// parsing the representation using the corresponding from_chars function recovers value
// exactly"; ties go to the representation closest to the value. /7: the plain overload uses
// printf's f conversion "if the absolute value of value is in the range [l, u), where l is
// the smallest value larger than or equal to 10^-4 that is representable by T and u is the
// largest value smaller than or equal to radix^(digits + 1) rounded down to the nearest
// power of 10 representable by T, otherwise e" -- u is 1e16 for double and 1e7 for float.
// (The values here have the same output whether the style is chosen by that range or by
// the shorter of f and e; to_chars_float_plain_style tests where the two differ.)
// COUNTERPART: libcxx:utilities/charconv/charconv.msvc/test.pass.cpp
#include <charconv>
#include <cfloat>
#include <string_view>
#include <system_error>
#include "check.hpp"

char buf[256];

template <class T>
std::string_view tc(T v) {
  auto r = std::to_chars(buf, buf + sizeof buf, v);
  CHECK(r.ec == std::errc{});
  return std::string_view(buf, r.ptr);
}

int main() {
  static_assert(std::is_same_v<decltype(std::to_chars(buf, buf, 1.0)), std::to_chars_result>);
  static_assert(std::is_same_v<decltype(std::to_chars(buf, buf, 1.0f)), std::to_chars_result>);
  static_assert(std::is_same_v<decltype(std::to_chars(buf, buf, 1.0L)), std::to_chars_result>);

  // double: shortest digits
  CHECK(tc(0.0) == "0");
  CHECK(tc(-0.0) == "-0");
  CHECK(tc(1.0) == "1");
  CHECK(tc(-1.5) == "-1.5");
  CHECK(tc(0.1) == "0.1");
  CHECK(tc(0.3) == "0.3");
  CHECK(tc(0.1 + 0.2) == "0.30000000000000004");
  CHECK(tc(2.0 / 3.0) == "0.6666666666666666");
  CHECK(tc(123.456) == "123.456");
  CHECK(tc(DBL_MAX) == "1.7976931348623157e+308");
  CHECK(tc(-DBL_MAX) == "-1.7976931348623157e+308");
  CHECK(tc(DBL_MIN) == "2.2250738585072014e-308");
  CHECK(tc(DBL_TRUE_MIN) == "5e-324");
  CHECK(tc(DBL_EPSILON) == "2.220446049250313e-16");
  // the f/e boundaries for double: [1e-4, 1e16)
  CHECK(tc(1.5e-4) == "0.00015");
  CHECK(tc(9.9e-5) == "9.9e-05");
  CHECK(tc(1e-5) == "1e-05");
  CHECK(tc(9007199254740992.0) == "9007199254740992");
  CHECK(tc(9999999999999998.0) == "9999999999999998");
  CHECK(tc(1e16) == "1e+16");
  CHECK(tc(1.5e16) == "1.5e+16");
  CHECK(tc(-1e16) == "-1e+16");
  CHECK(tc(1e22) == "1e+22");
  CHECK(tc(1e100) == "1e+100");
  CHECK(tc(123456789.0) == "123456789");

  // float
  CHECK(tc(0.1f) == "0.1");
  CHECK(tc(1.0f / 3.0f) == "0.33333334");
  CHECK(tc(9999999.0f) == "9999999");
  CHECK(tc(1e7f) == "1e+07");
  CHECK(tc(FLT_MAX) == "3.4028235e+38");
  CHECK(tc(FLT_MIN) == "1.1754944e-38");
  CHECK(tc(FLT_TRUE_MIN) == "1e-45");
  CHECK(tc(-0.0f) == "-0");
  // float(1e-4) is below 1e-4, so l is the next float and 1e-4f is printed in e style
  CHECK(1e-4f < 1e-4);
  CHECK(tc(1e-4f) == "1e-04");

  // long double: round trip of the shortest form and the f/e choice at 1 and 1e-5
  CHECK(tc(1.0L) == "1");
  CHECK(tc(0.5L) == "0.5");
  CHECK(tc(1e-5L) == "1e-05");
  CHECK(tc(-2.25L) == "-2.25");
  return 0;
}
