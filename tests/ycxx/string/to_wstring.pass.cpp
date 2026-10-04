// [string.conversions]/14: to_wstring(val) returns format(L"{}", val) for all the integral
// and floating-point overloads (shortest round-trip form for floating point).
#include <string>
#include <climits>
#include <limits>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::to_wstring(1)), std::wstring>);

int main() {
  CHECK(std::to_wstring(0) == L"0");
  CHECK(std::to_wstring(-15) == L"-15");
  CHECK(std::to_wstring(15u) == L"15");
  CHECK(std::to_wstring(LONG_MIN) == std::to_wstring(static_cast<long long>(LONG_MIN)));
  CHECK(std::to_wstring(ULLONG_MAX) == L"18446744073709551615");
  CHECK(std::to_wstring(0.1) == L"0.1");
  CHECK(std::to_wstring(1e20) == L"1e+20");
  CHECK(std::to_wstring(-2.5f) == L"-2.5");
  CHECK(std::to_wstring(0.75L) == L"0.75");
  CHECK(std::to_wstring(std::numeric_limits<double>::infinity()) == L"inf");
  return 0;
}
