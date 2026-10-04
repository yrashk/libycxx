// [facet.num.put.virtuals] (C locale): integers are formatted as printf with %d / %u, %o for
// oct, %x / %X for hex (uppercase), with '+' for showpos and '#' for showbase (Tables 97,
// 100); padding (Table 101): left pads after, right before, internal after the sign or after a
// leading 0x / 0X, otherwise before; width is reset to 0 after each output.
#include <sstream>
#include <ios>
#include <string>
#include <climits>
#include "check.hpp"

template<class T>
static std::string fmt(T v, std::ios_base::fmtflags f, int width = 0, char fill = ' ') {
  std::ostringstream os;
  os.flags(f);
  os.width(width);
  os.fill(fill);
  os << v;
  CHECK(os.width() == 0);
  return os.str();
}

int main() {
  using B = std::ios_base;
  CHECK(fmt(42, B::dec) == "42");
  CHECK(fmt(-42, B::dec) == "-42");
  CHECK(fmt(42, B::dec | B::showpos) == "+42");
  CHECK(fmt(0, B::dec | B::showpos) == "+0");
  CHECK(fmt(255, B::hex) == "ff");
  CHECK(fmt(255, B::hex | B::uppercase) == "FF");
  CHECK(fmt(255, B::hex | B::showbase) == "0xff");
  CHECK(fmt(255, B::hex | B::showbase | B::uppercase) == "0XFF");
  CHECK(fmt(0, B::hex | B::showbase) == "0");  // %#x of 0
  CHECK(fmt(8, B::oct) == "10");
  CHECK(fmt(8, B::oct | B::showbase) == "010");
  CHECK(fmt(0, B::oct | B::showbase) == "0");
  CHECK(fmt(42u, B::dec | B::showpos) == "42");  // %+u: no sign for unsigned conversions
  CHECK(fmt(UINT_MAX, B::dec) == std::to_string(UINT_MAX));
  CHECK(fmt(LLONG_MIN, B::dec) == std::to_string(LLONG_MIN));
  CHECK(fmt(ULLONG_MAX, B::hex) == "ffffffffffffffff");
  CHECK(fmt(static_cast<short>(-5), B::dec) == "-5");
  CHECK(fmt(42, B::fmtflags{}) == "42");  // basefield 0: %d

  // padding
  CHECK(fmt(42, B::dec, 6) == "    42");
  CHECK(fmt(-42, B::dec | B::left, 6, '*') == "-42***");
  CHECK(fmt(-42, B::dec | B::right, 6, '*') == "***-42");
  CHECK(fmt(-42, B::dec | B::internal, 6, '*') == "-***42");
  CHECK(fmt(42, B::dec | B::showpos | B::internal, 6, '0') == "+00042");
  CHECK(fmt(255, B::hex | B::showbase | B::internal, 8, '0') == "0x0000ff");
  CHECK(fmt(8, B::oct | B::showbase | B::internal, 5, '.') == "..010");  // no sign / 0x: before
  CHECK(fmt(42, B::dec | B::internal, 5, '_') == "___42");
  CHECK(fmt(123456, B::dec, 3) == "123456");  // no truncation

  // char types are not numbers; signed/unsigned char print as characters
  std::ostringstream os;
  os << static_cast<signed char>('A') << static_cast<unsigned char>('B') << 'C';
  CHECK(os.str() == "ABC");
  std::ostringstream w;
  w << std::hex << 0x1234 << ' ' << std::dec << 10 << ' ' << std::oct << 64;
  CHECK(w.str() == "1234 10 100");
  return 0;
}
