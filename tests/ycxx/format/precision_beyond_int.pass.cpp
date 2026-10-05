// Width and precision values that do not fit in int. [format.string.std]/10: "If
// { arg-id_opt } is used in a width or precision option, the value of the corresponding
// formatting argument is used as the value of the option. The option is valid only if the
// corresponding formatting argument is of standard signed or unsigned integer type. If its
// value is negative, an exception of type format_error is thrown." No upper bound is given
// (nor is one listed in [implimits]); /15: for strings the precision is the longest prefix
// whose field width is no greater than the value, so any precision at least the string's
// width keeps the whole string, without producing a large output. /16: a nonnegative-integer
// precision is the value of the decimal integer. [format.arg]/9: integer arguments of types
// long long and unsigned long long are stored as such (not narrowed).
// (Only precision on strings is used: a large width or a large floating-point precision
// would need that much output.)
// REQUIRES: exceptions
#include <climits>
#include <format>
#include <string>
#include <string_view>
#include "check.hpp"

template <class... Args>
std::string vf(std::string_view f, Args... args) {
  return std::vformat(f, std::make_format_args(args...));
}

int main() {
  const std::string ab = "ab";
  CHECK(vf("{:.{}}", ab, 2147483647LL) == "ab");
  CHECK(vf("{:.{}}", ab, 2147483648LL) == "ab");
  CHECK(vf("{:.{}}", ab, 4294967296ULL) == "ab");
  CHECK(vf("{:.{}}", ab, 1ULL << 40) == "ab");
  CHECK(vf("{:.{}}", ab, LLONG_MAX) == "ab");
  CHECK(vf("{:.{}}", ab, ULLONG_MAX) == "ab");
  CHECK(vf("{:.{}}", ab, 4294967295UL) == "ab");
  CHECK(vf("{1:*^5.{0}}", 3000000000U, ab) == "*ab**");
  CHECK(vf("{:.{}?}", ab, 5000000000LL) == "\"ab\"");
  CHECK(vf("{:.{}s}", "中文", 2147483648LL) == "中文");
  const wchar_t* xyz = L"xyz";
  long long big = LLONG_MAX;
  CHECK(std::vformat(L"{:.{}}", std::make_wformat_args(xyz, big)) == L"xyz");
  // A literal precision of INT_MAX.
  CHECK(vf("{:.2147483647}", ab) == "ab");
  // Negative values are rejected whatever the type.
  bool threw = false;
  try {
    (void)vf("{:.{}}", ab, LLONG_MIN);
  } catch (const std::format_error&) {
    threw = true;
  }
  CHECK(threw);
  return 0;
}
