// [charconv.to.chars]/1,7: to_chars(first, last, value, base = 10) for char and every
// signed and unsigned integer type writes the digits of value in the given base (2..36),
// with no redundant leading zeroes, digits 10..35 as lowercase a..z, a leading '-' for
// negative values; on success ec == errc{} and ptr is one past the last character written
// (nothing else is written, in particular no terminator). If the range is too small, ec ==
// errc::value_too_large and ptr == last. The integer overloads are constexpr.
#include <charconv>
#include <climits>
#include <cstdint>
#include <limits>
#include <string_view>
#include <system_error>
#include "check.hpp"

template <class T>
constexpr bool writes(T v, int base, std::string_view expect) {
  char buf[80];
  for (char& c : buf) c = '#';
  auto r = std::to_chars(buf, buf + sizeof buf, v, base);
  if (r.ec != std::errc{} || r.ptr != buf + expect.size()) return false;
  if (std::string_view(buf, r.ptr) != expect) return false;
  return buf[expect.size()] == '#';  // nothing written past ptr
}

template <class T>
constexpr bool too_large(T v, int base, int room) {
  char buf[80];
  auto r = std::to_chars(buf, buf + room, v, base);
  return r.ec == std::errc::value_too_large && r.ptr == buf + room;
}

constexpr bool test() {
  if (!writes(0, 10, "0") || !writes(7, 10, "7") || !writes(-7, 10, "-7")) return false;
  if (!writes(1234567890, 10, "1234567890")) return false;
  if (!writes(255, 16, "ff") || !writes(-255, 16, "-ff") || !writes(255, 2, "11111111")) return false;
  if (!writes(35, 36, "z") || !writes(36, 36, "10") || !writes(8, 8, "10")) return false;
  if (!writes(0, 2, "0") || !writes(0, 36, "0")) return false;
  if (!writes(INT_MIN, 10, "-2147483648")) return false;
  if (!writes(INT_MAX, 16, "7fffffff")) return false;
  if (!writes(std::numeric_limits<long long>::min(), 10, "-9223372036854775808")) return false;
  if (!writes(std::numeric_limits<unsigned long long>::max(), 10, "18446744073709551615")) return false;
  if (!writes(std::numeric_limits<unsigned long long>::max(), 2, std::string_view(
                  "1111111111111111111111111111111111111111111111111111111111111111")))
    return false;
  if (!writes(std::numeric_limits<long long>::min(), 2, std::string_view(
                  "-1000000000000000000000000000000000000000000000000000000000000000")))
    return false;
  if (!writes(std::numeric_limits<std::int8_t>::min(), 10, "-128")) return false;
  if (!writes(std::numeric_limits<std::uint8_t>::max(), 16, "ff")) return false;
  if (!writes(static_cast<short>(-32768), 16, "-8000")) return false;
  if (!writes(static_cast<unsigned short>(65535), 8, "177777")) return false;
  if (!writes('A', 10, "65")) return false;  // char is an integer-type here
  if (!writes(static_cast<signed char>(-1), 3, "-1")) return false;
  if (!writes(100L, 7, "202")) return false;
  if (!writes(123456789UL, 36, "21i3v9")) return false;

  // insufficient room
  if (!too_large(12345, 10, 4) || !too_large(-1, 10, 1) || !too_large(0, 10, 0)) return false;
  if (!too_large(255, 2, 7)) return false;
  // exact room is enough
  char b4[4];
  auto r = std::to_chars(b4, b4 + 4, -123);
  if (r.ec != std::errc{} || r.ptr != b4 + 4) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
