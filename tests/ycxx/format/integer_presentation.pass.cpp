// [format.string.std] Table 107 and /5-/9: integer presentation types b B d o x X (via
// to_chars), base prefixes with # (0b, 0B, 0 for nonzero octal, 0x, 0X) after the sign,
// sign options, the 0 option (zero padding after sign and prefix, ignored with an explicit
// alignment), width, and the default right alignment.
// COUNTERPART: libcxx:utilities/format/format.formatter/format.formatter.spec/formatter.(signed|unsigned)_integral.pass.cpp
#include <climits>
#include <cstdint>
#include <format>
#include <string>
#include "check.hpp"

int main() {
  CHECK(std::format("{}", 42) == "42");
  CHECK(std::format("{0:b} {0:d} {0:o} {0:x}", 42) == "101010 42 52 2a");
  CHECK(std::format("{0:#x} {0:#X}", 42) == "0x2a 0X2A");
  CHECK(std::format("{0:#b} {0:#B} {0:#o} {0:#d}", 5) == "0b101 0B101 05 5");
  CHECK(std::format("{:#o}", 0) == "0"); // the octal prefix is empty for zero
  CHECK(std::format("{:#x}", 0) == "0x0");
  CHECK(std::format("{:X}", 0xABCDEF) == "ABCDEF");
  CHECK(std::format("{:x}", -255) == "-ff");
  CHECK(std::format("{:#x}", -255) == "-0xff"); // prefix after the sign
  CHECK(std::format("{:+#x}", 255) == "+0xff");
  CHECK(std::format("{: #o}", 8) == " 010");
  // Sign options (Table 105).
  CHECK(std::format("{0:},{0:+},{0:-},{0: }", 1) == "1,+1,1, 1");
  CHECK(std::format("{0:},{0:+},{0:-},{0: }", -1) == "-1,-1,-1,-1");
  CHECK(std::format("{0:+},{0: }", 0) == "+0, 0");
  // Width and alignment: arithmetic types are right-aligned by default.
  CHECK(std::format("{:6}", 42) == "    42");
  CHECK(std::format("{:<6}", 42) == "42    ");
  CHECK(std::format("{:^6}", 42) == "  42  ");
  CHECK(std::format("{:^5}", 42) == " 42  "); // floor(n/2) before, ceil(n/2) after
  CHECK(std::format("{:*>6}", -42) == "***-42");
  CHECK(std::format("{:02}", 1234) == "1234");
  // The 0 option.
  CHECK(std::format("{:06}", -42) == "-00042");
  CHECK(std::format("{:+06d}", 120) == "+00120");
  CHECK(std::format("{:#06x}", 0xa) == "0x000a");
  CHECK(std::format("{:#010b}", 5) == "0b00000101");
  CHECK(std::format("{:<06}", -42) == "-42   "); // 0 has no effect with an align option
  CHECK(std::format("{:>06}", 7) == "     7");
  CHECK(std::format("{:x<06}", 7) == "7xxxxx");
  // Extremes of every width.
  CHECK(std::format("{}", INT_MIN) == "-2147483648");
  CHECK(std::format("{}", LLONG_MIN) == "-9223372036854775808");
  CHECK(std::format("{}", ULLONG_MAX) == "18446744073709551615");
  CHECK(std::format("{:x}", ULLONG_MAX) == "ffffffffffffffff");
  CHECK(std::format("{:b}", LLONG_MIN) == "-1000000000000000000000000000000000000000000000000000000000000000");
  CHECK(std::format("{:o}", static_cast<unsigned char>(255)) == "377");
  CHECK(std::format("{}", static_cast<signed char>(-128)) == "-128");
  CHECK(std::format("{}", static_cast<unsigned short>(65535)) == "65535");
  CHECK(std::format("{}", static_cast<short>(-32768)) == "-32768");
  CHECK(std::format("{:d}", std::int64_t{-1}) == "-1");
  CHECK(std::format("{:#X}", std::uint32_t{0xDEADBEEF}) == "0XDEADBEEF");
  CHECK(std::format("{:d}", 0L) == "0" && std::format("{}", 0UL) == "0");
}
