// [charconv.to.chars]/12-13: to_chars(first, last, value, fmt, precision): "value is converted
// to a string in the style of printf in the "C" locale with the given precision" (C 7.23.6.2:
// %.Pf has exactly P digits after the point; %.Pe one digit before the point, P after, and an
// exponent of at least two digits; %.Pa likewise in hex), with large precisions. The C
// standard requires the result to be correctly rounded when the number of significant digits
// is at most DECIMAL_DIG (17 for double), and otherwise to lie between the two adjacent
// DECIMAL_DIG-digit decimal strings around the value; so only the first 16 significant digits
// are compared, and the remaining digits are only checked to be decimal digits. Hex conversions of a value with fewer hex digits than
// the precision are exact (trailing zeros). [charconv.to.chars]/1: if the range is too small,
// ec is value_too_large, ptr is last, and the range's contents are unspecified.
#include <charconv>
#include <cfloat>
#include <cstring>
#include <string>
#include <string_view>
#include <system_error>
#include "check.hpp"

static char buf[4096];
using F = std::chars_format;

std::string_view tc(double v, F f, int p) {
  auto r = std::to_chars(buf, buf + sizeof buf, v, f, p);
  CHECK(r.ec == std::errc{});
  // the same conversion with one character less room fails
  std::size_t n = static_cast<std::size_t>(r.ptr - buf);
  static char other[4096];
  auto r2 = std::to_chars(other, other + n - 1, v, f, p);
  CHECK(r2.ec == std::errc::value_too_large && r2.ptr == other + n - 1);
  auto r3 = std::to_chars(other, other + n, v, f, p);
  CHECK(r3.ec == std::errc{} && r3.ptr == other + n && std::memcmp(buf, other, n) == 0);
  return std::string_view(buf, n);
}

bool all_digits(std::string_view s) {
  for (char c : s)
    if (c < '0' || c > '9') return false;
  return true;
}

int main() {
  // DBL_TRUE_MIN = 2^-1074 = 4.9406564584124654...e-324
  {
    std::string_view s = tc(DBL_TRUE_MIN, F::fixed, 1074);
    CHECK(s.size() == 2 + 1074);
    CHECK(s.substr(0, 2) == "0.");
    CHECK(s.substr(2, 323) == std::string(323, '0'));
    CHECK(s.substr(325, 16) == "4940656458412465");
    CHECK(all_digits(s.substr(2)));
    s = tc(DBL_TRUE_MIN, F::fixed, 1200);
    CHECK(s.size() == 1202 && all_digits(s.substr(2)));
    s = tc(-DBL_TRUE_MIN, F::fixed, 330);  // rounds to -0.000...0049
    CHECK(s.size() == 3 + 330 && s.substr(0, 3) == "-0.");
    CHECK(s.substr(3 + 323) == "4940656");  // ...4940656|4584...: rounded down
  }
  {
    std::string_view s = tc(DBL_TRUE_MIN, F::scientific, 700);
    CHECK(s.size() == 1 + 1 + 700 + 5);  // d . 700 digits e-324
    CHECK(s.substr(0, 17) == "4.940656458412465");
    CHECK(s.substr(s.size() - 5) == "e-324");
    CHECK(all_digits(s.substr(2, 700)));
  }
  // DBL_MAX = 1.7976931348623157e308, an integer with 309 digits
  {
    std::string_view s = tc(DBL_MAX, F::fixed, 0);
    CHECK(s.size() == 309);
    CHECK(s.substr(0, 16) == "1797693134862315");
    CHECK(all_digits(s));
    s = tc(-DBL_MAX, F::fixed, 500);
    CHECK(s.size() == 1 + 309 + 1 + 500);
    CHECK(s.substr(0, 17) == "-1797693134862315");
    CHECK(s.substr(310, 1) == "." && s.substr(311) == std::string(500, '0'));  // an integer
    s = tc(DBL_MAX, F::scientific, 400);
    CHECK(s.size() == 1 + 1 + 400 + 5 && s.substr(s.size() - 5) == "e+308");
    CHECK(s.substr(0, 17) == "1.797693134862315");
  }
  // 0.1 = 0.1000000000000000055511151231257827...
  {
    std::string_view s = tc(0.1, F::fixed, 600);
    CHECK(s.size() == 602 && s.substr(0, 18) == "0.1000000000000000");
    s = tc(0.1, F::scientific, 600);
    CHECK(s.size() == 606 && s.substr(0, 17) == "1.000000000000000" && s.substr(602) == "e-01");
  }
  // integers and dyadic fractions are exactly representable with few digits: the rest is zeros
  {
    std::string_view s = tc(1.0, F::fixed, 1000);
    CHECK(s == "1." + std::string(1000, '0'));
    s = tc(0.5, F::scientific, 900);
    CHECK(s == "5." + std::string(900, '0') + "e-01");
    s = tc(1.0, F::hex, 300);
    CHECK(s == "1." + std::string(300, '0') + "p+0");
    s = tc(-1.5, F::hex, 20);
    CHECK(s == "-1.8" + std::string(19, '0') + "p+0");
    s = tc(1e22, F::fixed, 3);  // 10^22 is exactly representable
    CHECK(s == "10000000000000000000000.000");
    s = tc(0x1p-20, F::fixed, 40);  // 2^-20 = 0.00000095367431640625 exactly (20 digits)
    CHECK(s == "0.0000009536743164062500000000000000000000");
  }
  // general with a large precision: P significant digits, trailing zeros removed
  {
    std::string_view s = tc(0.5, F::general, 800);
    CHECK(s == "0.5");
    s = tc(1e22, F::general, 500);
    CHECK(s == "10000000000000000000000");
    s = tc(0x1p-20, F::general, 300);
    CHECK(s == "9.5367431640625e-07");
  }
  return 0;
}
