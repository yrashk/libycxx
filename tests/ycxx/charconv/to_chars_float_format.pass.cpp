// [charconv.to.chars]/2-3, /10: to_chars(first, last, value, fmt) converts "in the style of
// printf in the "C" locale" with specifier f (fixed), e (scientific), g (general) or a
// "without leading "0x" in the result" (hex), using "the smallest number of characters"
// that round-trips through from_chars; "If there are several such representations, the
// representation with the smallest difference from the floating-point argument value is
// chosen" -- so a fixed-format large value prints its exact decimal digits.
#include <charconv>
#include <cfloat>
#include <string_view>
#include <system_error>
#include "check.hpp"

char buf[512];
using F = std::chars_format;

template <class T>
std::string_view tc(T v, F f) {
  auto r = std::to_chars(buf, buf + sizeof buf, v, f);
  CHECK(r.ec == std::errc{});
  return std::string_view(buf, r.ptr);
}

template <class T>
void hex_ok(T v) {
  std::string_view s = tc(v, F::hex);
  CHECK(s.substr(0, 2) != "0x" && s.substr(0, 3) != "-0x");
  auto p = s.find('p');
  CHECK(p != std::string_view::npos);
  CHECK(s[p + 1] == '+' || s[p + 1] == '-');
  // shortest: no trailing zero hex digit and no bare radix point
  CHECK(s[p - 1] != '.' && (s.find('.') == std::string_view::npos || s[p - 1] != '0'));
  T back{};
  auto r = std::from_chars(s.data(), s.data() + s.size(), back, F::hex);
  CHECK(r.ec == std::errc{} && r.ptr == s.data() + s.size() && back == v);
}

int main() {
  // fixed
  CHECK(tc(1.5, F::fixed) == "1.5");
  CHECK(tc(0.1, F::fixed) == "0.1");
  CHECK(tc(1e-5, F::fixed) == "0.00001");
  CHECK(tc(0.0, F::fixed) == "0");
  CHECK(tc(-0.0, F::fixed) == "-0");
  CHECK(tc(1e22, F::fixed) == "10000000000000000000000");
  CHECK(tc(1e23, F::fixed) == "99999999999999991611392");  // the exact value of double(1e23)
  CHECK(tc(123456789.0, F::fixed) == "123456789");
  CHECK(tc(-2.5f, F::fixed) == "-2.5");
  {
    std::string_view s = tc(DBL_TRUE_MIN, F::fixed);
    CHECK(s.size() == 326 && s.substr(0, 2) == "0." && s.back() == '5');
    CHECK(s.find_first_not_of('0', 2) == 325);
  }
  // scientific
  CHECK(tc(0.1, F::scientific) == "1e-01");
  CHECK(tc(123.456, F::scientific) == "1.23456e+02");
  CHECK(tc(0.0, F::scientific) == "0e+00");
  CHECK(tc(-1e100, F::scientific) == "-1e+100");
  CHECK(tc(1e-300, F::scientific) == "1e-300");
  CHECK(tc(DBL_MAX, F::scientific) == "1.7976931348623157e+308");
  CHECK(tc(FLT_MAX, F::scientific) == "3.4028235e+38");
  CHECK(tc(1e15, F::scientific) == "1e+15");
  // general (g): the shortest over the precisions %g allows
  CHECK(tc(100.0, F::general) == "100");
  CHECK(tc(0.5, F::general) == "0.5");
  CHECK(tc(1e20, F::general) == "1e+20");
  CHECK(tc(1e-5, F::general) == "1e-05");
  CHECK(tc(1e-4, F::general) == "0.0001");
  CHECK(tc(123456.0, F::general) == "123456");
  CHECK(tc(1e6, F::general) == "1e+06");
  CHECK(tc(0.0, F::general) == "0");
  CHECK(tc(0.30000000000000004, F::general) == "0.30000000000000004");
  CHECK(tc(1.0f / 3.0f, F::general) == "0.33333334");
  // hex
  for (double v : {1.0, 0.5, 3.0, -0.1, 1e300, DBL_MIN, DBL_TRUE_MIN, DBL_MAX}) hex_ok(v);
  for (float v : {1.0f, 0.1f, -3.5f, FLT_TRUE_MIN, FLT_MAX}) hex_ok(v);
  for (long double v : {1.0L, 0.1L, -3.5L}) hex_ok(v);
  CHECK(tc(0.0, F::hex) == "0p+0");
  CHECK(tc(-0.0, F::hex) == "-0p+0");
  return 0;
}
