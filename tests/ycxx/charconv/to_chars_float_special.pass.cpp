// [charconv.to.chars]: floating-point values are converted "in the style of printf in the
// "C" locale": shortest forms use "inf" or "-inf"; precision forms also permit
// "infinity" or "-infinity" (ISO C 7.23.6.2). A NaN is "nan" or "-nan", optionally followed by an implementation-defined
// "(n-char-sequence)" (ISO C 7.23.6.1), and negative zero keeps its sign. The from_chars
// pattern accepts what is produced.
#include <charconv>
#include <cmath>
#include <limits>
#include <string_view>
#include <system_error>
#include "check.hpp"

char buf[64];
using F = std::chars_format;

template <class T>
void check_type() {
  const T inf = std::numeric_limits<T>::infinity();
  const T nan = std::numeric_limits<T>::quiet_NaN();
  auto str = [](std::to_chars_result r) {
    CHECK(r.ec == std::errc{});
    return std::string_view(buf, r.ptr);
  };
  auto is_nan = [](std::string_view s, bool neg) {
    std::string_view p = neg ? "-nan" : "nan";
    return s.substr(0, p.size()) == p && (s.size() == p.size() || (s[p.size()] == '(' && s.back() == ')'));
  };
  auto is_inf = [](std::string_view s, bool neg) {
    return neg ? (s == "-inf" || s == "-infinity") : (s == "inf" || s == "infinity");
  };
  CHECK(str(std::to_chars(buf, buf + 64, inf)) == "inf");
  CHECK(str(std::to_chars(buf, buf + 64, -inf)) == "-inf");
  CHECK(is_nan(str(std::to_chars(buf, buf + 64, nan)), false));
  CHECK(is_nan(str(std::to_chars(buf, buf + 64, -nan)), true));
  for (F f : {F::fixed, F::scientific, F::general, F::hex}) {
    CHECK(str(std::to_chars(buf, buf + 64, inf, f)) == "inf");
    CHECK(str(std::to_chars(buf, buf + 64, -inf, f)) == "-inf");
    CHECK(is_inf(str(std::to_chars(buf, buf + 64, inf, f, 5)), false));
    CHECK(is_inf(str(std::to_chars(buf, buf + 64, -inf, f, 0)), true));
    CHECK(is_nan(str(std::to_chars(buf, buf + 64, nan, f)), false));
    CHECK(is_nan(str(std::to_chars(buf, buf + 64, -nan, f, 3)), true));
  }
  // Empty and too-short buffers must report value_too_large, with ptr == last.
  auto empty = std::to_chars(buf, buf, inf, F::fixed, 5);
  CHECK(empty.ec == std::errc::value_too_large && empty.ptr == buf);
  auto short_buffer = std::to_chars(buf, buf + 2, -inf, F::general, 0);
  CHECK(short_buffer.ec == std::errc::value_too_large && short_buffer.ptr == buf + 2);
  // round trip of the specials
  std::string_view s = str(std::to_chars(buf, buf + 64, -inf));
  T v{};
  auto r = std::from_chars(s.data(), s.data() + s.size(), v);
  CHECK(r.ec == std::errc{} && v == -inf);
  s = str(std::to_chars(buf, buf + 64, nan));
  r = std::from_chars(s.data(), s.data() + s.size(), v);
  CHECK(r.ec == std::errc{} && r.ptr == s.data() + s.size() && std::isnan(v));
  // signed zero
  CHECK(str(std::to_chars(buf, buf + 64, T(-0.0))) == "-0");
  CHECK(str(std::to_chars(buf, buf + 64, T(-0.0), F::scientific, 1)) == "-0.0e+00");
  s = str(std::to_chars(buf, buf + 64, T(-0.0)));
  v = 1;
  r = std::from_chars(s.data(), s.data() + s.size(), v);
  CHECK(r.ec == std::errc{} && v == 0 && std::signbit(v));
}

int main() {
  check_type<float>();
  check_type<double>();
  check_type<long double>();
  return 0;
}
