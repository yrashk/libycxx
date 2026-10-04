// [charconv.from.chars]/1,5: from_chars(first, last, value, base = 10) parses the strtol
// pattern for the base, except that no "0x"/"0X" prefix is accepted for base 16 (nor
// "0b"/"0B" for base 2), '-' is the only sign and only for signed types, and no leading
// whitespace is skipped. On success value is set, ec == errc{} and ptr points at the first
// character not matching the pattern. If nothing matches, value is unmodified, ptr == first
// and ec == errc::invalid_argument; if the parsed value is out of range, value is
// unmodified, ec == errc::result_out_of_range and ptr is still past the matched characters.
#include <charconv>
#include <climits>
#include <cstdint>
#include <string_view>
#include <system_error>
#include "check.hpp"

template <class T>
constexpr bool parses(std::string_view s, int base, T expect, std::size_t consumed) {
  T v = 99;
  auto r = std::from_chars(s.data(), s.data() + s.size(), v, base);
  return r.ec == std::errc{} && v == expect && r.ptr == s.data() + consumed;
}
template <class T>
constexpr bool invalid(std::string_view s, int base = 10) {
  T v = 99;
  auto r = std::from_chars(s.data(), s.data() + s.size(), v, base);
  return r.ec == std::errc::invalid_argument && r.ptr == s.data() && v == 99;
}
template <class T>
constexpr bool out_of_range(std::string_view s, int base, std::size_t consumed) {
  T v = 99;
  auto r = std::from_chars(s.data(), s.data() + s.size(), v, base);
  return r.ec == std::errc::result_out_of_range && r.ptr == s.data() + consumed && v == 99;
}

constexpr bool test() {
  if (!parses<int>("123", 10, 123, 3) || !parses<int>("-123", 10, -123, 4)) return false;
  if (!parses<int>("0", 10, 0, 1) || !parses<int>("-0", 10, 0, 2)) return false;
  if (!parses<int>("00042", 10, 42, 5)) return false;
  if (!parses<int>("12ab", 10, 12, 2)) return false;           // stops at the first non-digit
  if (!parses<int>("ff", 16, 255, 2) || !parses<int>("FF", 16, 255, 2) || !parses<int>("Ff", 16, 255, 2)) return false;
  if (!parses<int>("0x1f", 16, 0, 1)) return false;            // no 0x prefix: parses "0"
  if (!parses<int>("0b101", 2, 0, 1)) return false;            // no 0b prefix
  if (!parses<int>("1012", 2, 5, 3)) return false;
  if (!parses<int>("zz", 36, 35 * 36 + 35, 2) || !parses<int>("Z", 36, 35, 1)) return false;
  if (!parses<int>("777", 8, 511, 3) || !parses<int>("789", 8, 7, 1)) return false;
  if (!parses<int>("2147483647", 10, INT_MAX, 10)) return false;
  if (!parses<int>("-2147483648", 10, INT_MIN, 11)) return false;
  if (!parses<long long>("-9223372036854775808", 10, LLONG_MIN, 20)) return false;
  if (!parses<unsigned long long>("18446744073709551615", 10, ULLONG_MAX, 20)) return false;
  if (!parses<std::uint8_t>("255", 10, 255, 3) || !parses<std::int8_t>("-128", 10, -128, 4)) return false;
  if (!parses<char>("65", 10, 'A', 2)) return false;
  if (!parses<short>("-7fff", 16, -32767, 5)) return false;

  // nothing matches
  if (!invalid<int>("") || !invalid<int>("x1") || !invalid<int>("-") || !invalid<int>("-x")) return false;
  if (!invalid<int>("+1")) return false;                      // '+' is not allowed
  if (!invalid<int>(" 1")) return false;                      // no whitespace skipping
  if (!invalid<unsigned>("-1")) return false;                 // no sign for unsigned types
  if (!invalid<int>("g", 16) || !invalid<int>("2", 2) || !invalid<int>("9", 8)) return false;
  if (!parses<int>("0x", 10, 0, 1)) return false;            // "0" matches
  // out of range: value unmodified, ptr past all matching digits
  if (!out_of_range<int>("2147483648", 10, 10)) return false;
  if (!out_of_range<int>("-2147483649", 10, 11)) return false;
  if (!out_of_range<int>("99999999999999999999999xyz", 10, 23)) return false;
  if (!out_of_range<std::uint8_t>("256", 10, 3)) return false;
  if (!out_of_range<std::int8_t>("-129", 10, 4)) return false;
  if (!out_of_range<std::int8_t>("80", 16, 2)) return false;
  if (!out_of_range<unsigned long long>("18446744073709551616", 10, 20)) return false;
  if (!out_of_range<std::uint8_t>("100000000", 2, 9)) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  // round trip through to_chars for every base
  for (int base = 2; base <= 36; ++base) {
    for (long long v : {0LL, 1LL, -1LL, 12345678LL, -987654321LL, LLONG_MAX, LLONG_MIN}) {
      char buf[80];
      auto w = std::to_chars(buf, buf + sizeof buf, v, base);
      CHECK(w.ec == std::errc{});
      long long back = 0;
      auto r = std::from_chars(buf, w.ptr, back, base);
      CHECK(r.ec == std::errc{} && r.ptr == w.ptr && back == v);
    }
  }
  return 0;
}
