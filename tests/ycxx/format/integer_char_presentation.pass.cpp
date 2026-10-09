// [format.string.std] Table 107, type c: "Copies the character static_cast<charT>(value) to
// the output. Throws format_error if value is not in the range of representable values for
// charT." With c, an integer is left-aligned by default (Table 104: < is the default for
// charT ... unless an integer presentation type is specified; c is not an integer
// presentation type for an integer).
// REQUIRES: exceptions
// COUNTERPART: libcxx:utilities/format/format.formatter/format.formatter.spec/formatter.char(.fsigned-char|.funsigned-char)?.pass.cpp
#include <format>
#include <limits>
#include <string>
#include "check.hpp"

int main() {
  CHECK(std::format("{:c}", 65) == "A");
  CHECK(std::format("{:c}", 'z' + 0L) == "z");
  CHECK(std::format("{:3c}|", 66) == "B  |");
  CHECK(std::format("{:>3c}|", 66) == "  B|");
  CHECK(std::format("{:c}", 127) == "\x7f");
  constexpr int first = std::numeric_limits<char>::min(), last = std::numeric_limits<char>::max();
  CHECK(std::format("{:c}", first) == std::string(1, static_cast<char>(first)));
  CHECK(std::format("{:c}", last) == std::string(1, static_cast<char>(last)));
  for (const int outside : {first - 1, last + 1}) {
    bool rejected = false;
    try {
      (void)std::format("{:c}", outside);
    } catch (const std::format_error&) {
      rejected = true;
    }
    CHECK(rejected);
  }
  bool threw = false;
  try {
    (void)std::format("{:c}", 256);
  } catch (const std::format_error&) {
    threw = true;
  }
  CHECK(threw);
  threw = false;
  try {
    (void)std::format("{:c}", 100000LL);
  } catch (const std::format_error&) {
    threw = true;
  }
  CHECK(threw);
  threw = false;
  try {
    (void)std::format(L"{:c}", 0x10FFFF + 0LL); // representable in wchar_t (32-bit)
  } catch (const std::format_error&) {
    threw = true;
  }
  CHECK(!threw);
}
