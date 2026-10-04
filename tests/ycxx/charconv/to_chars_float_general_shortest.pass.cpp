// [charconv.to.chars]/2-3: to_chars(first, last, value, chars_format::general) without a
// precision produces, in the style of printf's g conversion, "the smallest number of
// characters such that ... parsing the representation using the corresponding from_chars
// function recovers value exactly". %g output is available for every precision P, and with
// P = 7 the value 1234567 is printed as "1234567" (X = 6 < P selects the f style), which is
// shorter than the scientific "1.234567e+06"; likewise 12345678 with P = 8.
#include <charconv>
#include <string_view>
#include <system_error>
#include "check.hpp"

char buf[64];

std::string_view tc(double v) {
  auto r = std::to_chars(buf, buf + sizeof buf, v, std::chars_format::general);
  CHECK(r.ec == std::errc{});
  return std::string_view(buf, r.ptr);
}

int main() {
  CHECK(tc(1234567.0) == "1234567");
  CHECK(tc(12345678.0) == "12345678");
  CHECK(tc(-123456789.0) == "-123456789");
  return 0;
}
