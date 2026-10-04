// [string.syn], [string.conversions]/7: the integral overloads of to_string and to_wstring
// are constexpr ("constexpr string to_string(int val);" ...), returning format("{}", val).
#include <string>
#include <climits>
#include "check.hpp"

static_assert(std::to_string(0) == "0");
static_assert(std::to_string(-42) == "-42");
static_assert(std::to_string(INT_MIN) == "-2147483648");
static_assert(std::to_string(4000000000u) == "4000000000");
static_assert(std::to_string(-7L) == "-7");
static_assert(std::to_string(7UL) == "7");
static_assert(std::to_string(LLONG_MIN) == "-9223372036854775808");
static_assert(std::to_string(ULLONG_MAX) == "18446744073709551615");
static_assert(std::to_wstring(-12) == L"-12");
static_assert(std::to_wstring(ULLONG_MAX) == L"18446744073709551615");

int main() {
  CHECK(std::to_string(5) == "5");
  return 0;
}
