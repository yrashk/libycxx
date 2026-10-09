// [charconv.to.chars]/13: to_chars(first, last, value, fmt, precision): "value is converted
// to a string in the style of printf in the "C" locale with the given precision", i.e. %.Nf,
// %.Ne, %.Ng and %.Na (without "0x"), correctly rounded.
// COUNTERPART: libcxx:utilities/charconv/charconv.msvc/test.pass.cpp
#include <charconv>
#include <string_view>
#include <system_error>
#include "check.hpp"

char buf[512];
using F = std::chars_format;

template <class T>
std::string_view tc(T v, F f, int p) {
  auto r = std::to_chars(buf, buf + sizeof buf, v, f, p);
  CHECK(r.ec == std::errc{});
  return std::string_view(buf, r.ptr);
}

// printf's a conversion requires a signed decimal exponent; from_chars also accepts
// unsigned exponents and partial input, so successful parsing alone is not a syntax oracle.
constexpr bool hex_precision_syntax(std::string_view s, int precision) {
  if (!s.empty() && s.front() == '-') s.remove_prefix(1);
  auto hex_digit = [](char c) { return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f'); };
  if (s.empty() || !hex_digit(s.front())) return false;
  std::size_t exp = 1;
  if (precision > 0) {
    if (s.size() <= 1 || s[1] != '.') return false;
    exp += 1 + static_cast<std::size_t>(precision);
    if (s.size() <= exp) return false;
    for (std::size_t i = 2; i < exp; ++i) if (!hex_digit(s[i])) return false;
  }
  if (s.size() < exp + 3 || s[exp] != 'p' || (s[exp + 1] != '+' && s[exp + 1] != '-')) return false;
  for (std::size_t i = exp + 2; i < s.size(); ++i) if (s[i] < '0' || s[i] > '9') return false;
  return true;
}
static_assert(hex_precision_syntax("1.000p+0", 3));
static_assert(hex_precision_syntax("8.000p-3", 3));  // leading hex digit is implementation-defined
static_assert(hex_precision_syntax("-1p+10", 0));
static_assert(!hex_precision_syntax("1.000p0x", 3));
static_assert(!hex_precision_syntax("1.000p+0x", 3));
static_assert(!hex_precision_syntax("1.000p+", 3));
static_assert(!hex_precision_syntax("1.00gp+0", 3));
static_assert(!hex_precision_syntax("0x1.000p+0", 3));

int main() {
  // fixed: exactly p digits after the point (none and no point for 0)
  CHECK(tc(1.0 / 3.0, F::fixed, 3) == "0.333");
  CHECK(tc(2.0 / 3.0, F::fixed, 3) == "0.667");
  CHECK(tc(1.5, F::fixed, 0) == "2");    // round half to even of the exact value 1.5
  CHECK(tc(2.5, F::fixed, 0) == "2");
  CHECK(tc(0.125, F::fixed, 2) == "0.12");
  CHECK(tc(0.375, F::fixed, 2) == "0.38");
  CHECK(tc(1.0, F::fixed, 5) == "1.00000");
  CHECK(tc(-0.0, F::fixed, 1) == "-0.0");
  CHECK(tc(0.1, F::fixed, 17) == "0.10000000000000001");
  CHECK(tc(0.1, F::fixed, 25) == "0.1000000000000000055511151");
  CHECK(tc(1e22, F::fixed, 1) == "10000000000000000000000.0");
  CHECK(tc(1.25f, F::fixed, 1) == "1.2");
  CHECK(tc(1.0L, F::fixed, 2) == "1.00");
  // scientific: one digit, then p digits, exponent at least two digits
  CHECK(tc(1.0 / 3.0, F::scientific, 2) == "3.33e-01");
  CHECK(tc(12345.0, F::scientific, 0) == "1e+04");
  CHECK(tc(15000.0, F::scientific, 0) == "2e+04");
  CHECK(tc(0.0, F::scientific, 3) == "0.000e+00");
  CHECK(tc(1e100, F::scientific, 1) == "1.0e+100");
  CHECK(tc(-9.96, F::scientific, 1) == "-1.0e+01");
  // general: p significant digits (0 means 1), trailing zeros removed, e if X < -4 or X >= P
  CHECK(tc(1.0 / 3.0, F::general, 4) == "0.3333");
  CHECK(tc(100.0, F::general, 2) == "1e+02");
  CHECK(tc(100.0, F::general, 3) == "100");
  CHECK(tc(1.5, F::general, 0) == "2");
  CHECK(tc(0.0001, F::general, 6) == "0.0001");
  CHECK(tc(0.00001, F::general, 6) == "1e-05");
  CHECK(tc(123456789.0, F::general, 6) == "1.23457e+08");
  CHECK(tc(2.5, F::general, 6) == "2.5");
  // hex: p hex digits after the point
  {
    std::string_view s = tc(1.0, F::hex, 3);
    CHECK(hex_precision_syntax(s, 3));
    CHECK(s.substr(0, 2) != "0x");
    double back = 0;
    auto r = std::from_chars(s.data(), s.data() + s.size(), back, F::hex);
    CHECK(r.ec == std::errc{} && r.ptr == s.data() + s.size() && back == 1.0);
    s = tc(0.0, F::hex, 2);
    CHECK(s == "0.00p+0");
  }
  // Exact values at sufficient precision, including negative exponents and signs.
  for (double value : {1.0, 0.5, -16.0, 0x1.8p-100, 0x1.fp+100}) {
    std::string_view s = tc(value, F::hex, 3);
    CHECK(hex_precision_syntax(s, 3));
    double back = 0;
    auto r = std::from_chars(s.data(), s.data() + s.size(), back, F::hex);
    CHECK(r.ec == std::errc{} && r.ptr == s.data() + s.size() && back == value);
    // A buffer just large enough succeeds; one byte less reports the specified failure.
    char small[64];
    const std::size_t n = s.size();
    auto fit = std::to_chars(small, small + n, value, F::hex, 3);
    CHECK(fit.ec == std::errc{} && fit.ptr == small + n && std::string_view(small, n) == s);
    small[n - 1] = '!';
    auto short_result = std::to_chars(small, small + n - 1, value, F::hex, 3);
    CHECK(short_result.ec == std::errc::value_too_large && short_result.ptr == small + n - 1);
    CHECK(small[n - 1] == '!');  // no write outside the provided range
  }
  // large precisions are honoured
  {
    std::string_view s = tc(1.0, F::fixed, 300);
    CHECK(s.size() == 302 && s.substr(0, 2) == "1." && s.find_first_not_of('0', 2) == std::string_view::npos);
    s = tc(0.5, F::scientific, 100);
    CHECK(s.size() == 106 && s.substr(0, 3) == "5.0" && s.substr(102) == "e-01");
  }
  return 0;
}
