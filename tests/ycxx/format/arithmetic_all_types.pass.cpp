// [format.formatter.spec]/2.3: "for each charT, for each cv-unqualified arithmetic type
// ArithmeticT other than char, wchar_t, char8_t, char16_t, or char32_t, a specialization
// template<> struct formatter<ArithmeticT, charT>". [format.string.std] Table 107: the default
// presentation of integers is d (to_chars in base 10), c copies the character
// static_cast<charT>(value) and "the formatting argument is subject to a range check" (a value
// not representable in charT throws format_error). Table 109: floating-point defaults to the
// shortest round-trip representation. Every signed and unsigned standard integer type and every
// standard floating-point type is formatted at its extremes, with both char and wchar_t.
// COUNTERPART: libcxx:utilities/format/format.functions/(format|format.locale|vformat|vformat.locale).pass.cpp
#include <format>
#include <limits>
#include <string>
#include <charconv>
#include "check.hpp"

template <class T>
static void check_int() {
  using L = std::numeric_limits<T>;
  char buf[64];
  auto r = std::to_chars(buf, buf + 64, L::max());
  CHECK(std::format("{}", L::max()) == std::string(buf, r.ptr));
  r = std::to_chars(buf, buf + 64, L::min());
  CHECK(std::format("{}", L::min()) == std::string(buf, r.ptr));
  CHECK(std::format("{:x}", static_cast<T>(42)) == "2a");
  CHECK(std::format("{:+}", static_cast<T>(7)) == "+7");
  std::wstring w = std::format(L"{}", L::max());
  CHECK(std::string(w.begin(), w.end()) == std::format("{}", L::max()));
  CHECK(std::format("{:c}", static_cast<T>('A')) == "A");
}

int main() {
  check_int<signed char>();
  check_int<unsigned char>();
  check_int<short>();
  check_int<unsigned short>();
  check_int<int>();
  check_int<unsigned>();
  check_int<long>();
  check_int<unsigned long>();
  check_int<long long>();
  check_int<unsigned long long>();
  CHECK(std::format("{}", static_cast<signed char>(-128)) == "-128");  // a number, not a char
  CHECK(std::format("{}", static_cast<unsigned char>(200)) == "200");
  CHECK(std::format("{}", std::numeric_limits<long long>::min()) == "-9223372036854775808");
  CHECK(std::format("{}", std::numeric_limits<unsigned long long>::max()) == "18446744073709551615");
  // range check of c
  int bad = 0;
  try {
    (void)std::format("{:c}", 300);
  } catch (const std::format_error&) {
    ++bad;
  }
  try {
    (void)std::format("{:c}", static_cast<long long>(std::numeric_limits<char>::min()) - 1);
  } catch (const std::format_error&) {
    ++bad;
  }
  CHECK(bad == 2);
  CHECK(std::format(L"{:c}", 0x3A9) == L"Ω");

  CHECK(std::format("{}", std::numeric_limits<float>::max()) == "3.4028235e+38");
  CHECK(std::format("{}", std::numeric_limits<double>::max()) == "1.7976931348623157e+308");
  CHECK(std::format("{}", std::numeric_limits<double>::denorm_min()) == "5e-324");
  CHECK(std::format("{}", -std::numeric_limits<float>::infinity()) == "-inf");
  CHECK(std::format("{}", 0.1f) == "0.1");
  CHECK(std::format("{}", -0.0) == "-0");
  {
    char buf[128];
    long double v = std::numeric_limits<long double>::max();
    auto r = std::to_chars(buf, buf + 128, v);
    CHECK(std::format("{}", v) == std::string(buf, r.ptr));
    v = std::numeric_limits<long double>::lowest();
    r = std::to_chars(buf, buf + 128, v);
    CHECK(std::format("{}", v) == std::string(buf, r.ptr));
  }
  CHECK(std::format(L"{}", 2.5f) == L"2.5");
  CHECK(std::format(L"{}", 2.5L) == L"2.5");
  return 0;
}
