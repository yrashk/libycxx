// [charconv.from.chars]/1: "If no characters match the pattern, value is unmodified, the
// member ptr of the return value is first and the member ec is equal to
// errc::invalid_argument. [Note 1: If the pattern allows for an optional sign, but the
// string has no digit characters following the sign, no characters match the pattern.]"
// "If the parsed value is not in the range representable by the type of value, value is
// unmodified and the member ec of the return value is equal to errc::result_out_of_range"
// (ptr still points past the matched characters). /6.1: "the sign '+' may only appear in
// the exponent part".
// COUNTERPART: libcxx:utilities/charconv/charconv.from.chars/floating_point.pass.cpp
#include <charconv>
#include <string>
#include <string_view>
#include <system_error>
#include "check.hpp"

template <class T>
void invalid(std::string_view s, std::chars_format f = std::chars_format::general) {
  T v = T(42);
  auto r = std::from_chars(s.data(), s.data() + s.size(), v, f);
  CHECK(r.ec == std::errc::invalid_argument);
  CHECK(r.ptr == s.data());
  CHECK(v == T(42));
}

template <class T>
void out_of_range(std::string_view s, std::size_t consumed, std::chars_format f = std::chars_format::general) {
  T v = T(42);
  auto r = std::from_chars(s.data(), s.data() + s.size(), v, f);
  CHECK(r.ec == std::errc::result_out_of_range);
  CHECK(r.ptr == s.data() + consumed);
  CHECK(v == T(42));
}

int main() {
  for (auto f : {std::chars_format::general, std::chars_format::fixed, std::chars_format::scientific,
                 std::chars_format::hex}) {
    invalid<double>("", f);
    invalid<double>("-", f);
    invalid<double>("+1", f);
    invalid<double>("+", f);
    invalid<double>(" 1", f);
    invalid<double>("\t1", f);
    invalid<double>(".", f);
    invalid<double>("-.", f);
    invalid<double>("--1", f);
    invalid<float>("-x", f);
    invalid<long double>("+inf", f);
  }
  invalid<double>("e5");
  invalid<double>("x1", std::chars_format::hex);
  // empty range: ptr is first (== last)
  {
    const char* p = "1";
    double v = 3;
    auto r = std::from_chars(p, p, v);
    CHECK(r.ec == std::errc::invalid_argument && r.ptr == p && v == 3);
  }
  // overflow
  out_of_range<double>("1e400", 5);
  out_of_range<double>("-1e400", 6);
  out_of_range<double>("1e400xyz", 5);
  out_of_range<double>("2e308", 5, std::chars_format::scientific);
  out_of_range<float>("3.5e38", 6);
  out_of_range<float>("1e39", 4);
  out_of_range<double>("1p1024", 6, std::chars_format::hex);
  out_of_range<float>("1p128", 5, std::chars_format::hex);
  {
    // a long run of integer digits beyond the range
    std::string s(400, '9');
    out_of_range<double>(s, 400, std::chars_format::fixed);
  }
  // the largest finite values are in range
  {
    double d = 0;
    std::string_view s = "1.7976931348623157e308";
    auto r = std::from_chars(s.data(), s.data() + s.size(), d);
    CHECK(r.ec == std::errc{} && d == 1.7976931348623157e308);
    float f = 0;
    s = "3.4028234e38";
    auto r2 = std::from_chars(s.data(), s.data() + s.size(), f);
    CHECK(r2.ec == std::errc{} && f == 3.4028234e38f);
  }
  return 0;
}
