// [string.conversions]/1-3: stoul / stoull call strtoul / strtoull, "Each function returns
// the converted result"; out_of_range only for ERANGE or a value not representable in the
// return type. C's strtoul / strtoull ([c.strings] / ISO C 7.24.1.7) accept an optional
// minus sign and negate the value in the return type, so stoul("-1") is ULONG_MAX without
// an error. Bases 2..36 use letters in either case; base 0 auto-detects "0x"/"0X" (hex) and
// a leading "0" (octal); for base 16 an "0x" prefix is permitted; when the prefix is not
// followed by a hex digit only the "0" is converted. idx receives the index of the first
// unconverted character ([string.conversions]/1).
#include <string>
#include <climits>
#include <stdexcept>
#include "check.hpp"

template <class F>
int which_throw(F f) {
  try {
    f();
  } catch (const std::invalid_argument&) {
    return 1;
  } catch (const std::out_of_range&) {
    return 2;
  } catch (...) {
    return 3;
  }
  return 0;
}

int main() {
  std::size_t idx = 0;
  CHECK(std::stoul("-1") == ULONG_MAX);
  CHECK(std::stoull("-1", &idx) == ULLONG_MAX && idx == 2);
  CHECK(std::stoull("-0") == 0);
  CHECK(std::stoul("  -2", &idx) == ULONG_MAX - 1 && idx == 4);

  CHECK(std::stoi("ZZ", &idx, 36) == 35 * 36 + 35 && idx == 2);
  CHECK(std::stoi("zz", nullptr, 36) == 35 * 36 + 35);
  CHECK(std::stoi("FF", nullptr, 16) == 255 && std::stoi("fF", nullptr, 16) == 255);
  CHECK(std::stoi("0X1f", &idx, 0) == 31 && idx == 4);
  CHECK(std::stoi("0x1f", &idx, 16) == 31 && idx == 4);
  CHECK(std::stoi("0x", &idx, 16) == 0 && idx == 1);
  CHECK(std::stoi("0xg", &idx, 0) == 0 && idx == 1);
  CHECK(std::stoi("010", &idx, 0) == 8 && idx == 3);
  CHECK(std::stoi("08", &idx, 0) == 0 && idx == 1);
  CHECK(std::stoi("0", &idx, 0) == 0 && idx == 1);
  CHECK(std::stoi("12", &idx, 3) == 5 && idx == 2);
  CHECK(std::stoi("123", &idx, 3) == 5 && idx == 2);
  CHECK(std::stoi("-0x10", &idx, 16) == -16 && idx == 5);
  CHECK(std::stol("\t\n 7", &idx) == 7 && idx == 4);
  CHECK(which_throw([] { (void)std::stoll("+-1"); }) == 1);
  CHECK(which_throw([] { (void)std::stoi("-", nullptr, 16); }) == 1);
  CHECK(which_throw([] { (void)std::stoi("z", nullptr, 35); }) == 1);
  CHECK(which_throw([] { (void)std::stoi("2", nullptr, 2); }) == 1);

  // INT_MAX / INT_MIN boundaries through stoi
  CHECK(std::stoi("2147483647") == INT_MAX);
  CHECK(std::stoi("-2147483648") == INT_MIN);
  CHECK(std::stoi("7fffffff", nullptr, 16) == INT_MAX);
  // leading zeros do not count against the range
  CHECK(std::stoi("000000000000000000000000042") == 42);
  return 0;
}
