// [format.string.std] Tables 108 and 109: charT formats as a character by default (left
// aligned); b, B, d, o, x, X format the value converted to the unsigned version of the
// underlying type; bool formats as true/false (left aligned) or, with an integer type, as
// static_cast<unsigned char>(value) (right aligned).
#include <format>
#include <string>
#include "check.hpp"

int main() {
  // char
  CHECK(std::format("{}", 'x') == "x");
  CHECK(std::format("{:6}", 'x') == "x     ");
  CHECK(std::format("{:*<6}", 'x') == "x*****");
  CHECK(std::format("{:*>6}", 'x') == "*****x");
  CHECK(std::format("{:*^6}", 'x') == "**x***");
  CHECK(std::format("{:c}", 'q') == "q");
  char c = 120;
  CHECK(std::format("{:6d}", c) == "   120"); // integer presentation: right aligned
  CHECK(std::format("{:+06d}", c) == "+00120");
  CHECK(std::format("{:x} {:#o} {:b}", 'A', 'A', 'A') == "41 0101 1000001");
  CHECK(std::format("{:d}", static_cast<char>(-1)) == "255"); // unsigned version
  CHECK(std::format("{:x}", static_cast<char>(-128)) == "80");
  // bool
  CHECK(std::format("{}", true) == "true" && std::format("{:s}", false) == "false");
  CHECK(std::format("{:6}", true) == "true  ");
  CHECK(std::format("{:>6}", false) == " false");
  CHECK(std::format("{:d} {:d}", true, false) == "1 0");
  CHECK(std::format("{:#x} {:#b} {:o}", true, true, true) == "0x1 0b1 1");
  CHECK(std::format("{:4d}", true) == "   1");
  CHECK(std::format("{:+d}", true) == "+1");
  // wchar_t
  CHECK(std::format(L"{}", L'w') == L"w");
  CHECK(std::format(L"{}", 'n') == L"n"); // formatter<char, wchar_t> is enabled
  CHECK(std::format(L"{:d}", L'A') == L"65");
  CHECK(std::format(L"{}", true) == L"true");
}
