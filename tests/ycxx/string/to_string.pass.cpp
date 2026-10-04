// [string.conversions]/7: to_string(val) returns format("{}", val) for int, unsigned, long,
// unsigned long, long long, unsigned long long, float, double, long double. For the
// floating-point types that is the shortest representation that round-trips
// ([format.string.std]: no type -> to_chars(first, last, value)), e.g. "0.1" and "1e+20",
// not printf("%f") output.
#include <string>
#include <climits>
#include <limits>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::to_string(1)), std::string>);
static_assert(std::is_same_v<decltype(std::to_string(1.0)), std::string>);

int main() {
  CHECK(std::to_string(0) == "0");
  CHECK(std::to_string(-1) == "-1");
  CHECK(std::to_string(INT_MAX) == "2147483647");
  CHECK(std::to_string(INT_MIN) == "-2147483648");
  CHECK(std::to_string(4294967295u) == "4294967295");
  CHECK(std::to_string(-123456789L) == "-123456789");
  CHECK(std::to_string(123456789UL) == "123456789");
  CHECK(std::to_string(LLONG_MIN) == "-9223372036854775808");
  CHECK(std::to_string(ULLONG_MAX) == "18446744073709551615");

  CHECK(std::to_string(1.0) == "1");
  CHECK(std::to_string(1.5) == "1.5");
  CHECK(std::to_string(-0.0) == "-0");
  CHECK(std::to_string(0.1) == "0.1");
  CHECK(std::to_string(1e20) == "1e+20");
  CHECK(std::to_string(1e-7) == "1e-07");
  CHECK(std::to_string(123456.0) == "123456");
  CHECK(std::to_string(0.1f) == "0.1");
  CHECK(std::to_string(3.25f) == "3.25");
  CHECK(std::to_string(2.5L) == "2.5");
  CHECK(std::to_string(std::numeric_limits<double>::infinity()) == "inf");
  CHECK(std::to_string(-std::numeric_limits<double>::infinity()) == "-inf");
  CHECK(std::to_string(std::numeric_limits<double>::quiet_NaN()) == "nan");
  CHECK(std::to_string(std::numeric_limits<double>::max()) == "1.7976931348623157e+308");
  CHECK(std::to_string(std::numeric_limits<double>::denorm_min()) == "5e-324");
  return 0;
}
