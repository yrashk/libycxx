// [charconv.from.chars]/6: "(6.2) if fmt has chars_format::scientific set but not
// chars_format::fixed, the otherwise optional exponent part shall appear; (6.3) if fmt has
// chars_format::fixed set but not chars_format::scientific, the optional exponent part shall
// not appear; and (6.4) if fmt is chars_format::hex, the prefix "0x" or "0X" is assumed.
// [Example 1: The string 0x123 is parsed to have the value 0 with remaining characters x123.]"
// The hex subject sequence (strtod) is hex digits optionally containing a radix point, then
// an optional binary exponent "p" or "P".
#include <charconv>
#include <string_view>
#include <system_error>
#include "check.hpp"

using F = std::chars_format;

struct Res {
  std::errc ec;
  long consumed;
  double v;
};

Res fc(std::string_view s, F f, double init = -99.0) {
  double v = init;
  auto r = std::from_chars(s.data(), s.data() + s.size(), v, f);
  return {r.ec, r.ptr - s.data(), v};
}

bool ok(Res r, long consumed, double v) { return r.ec == std::errc{} && r.consumed == consumed && r.v == v; }
bool invalid(Res r) { return r.ec == std::errc::invalid_argument && r.consumed == 0 && r.v == -99.0; }

int main() {
  // scientific: the exponent is required
  CHECK(ok(fc("1.5e3", F::scientific), 5, 1500.0));
  CHECK(ok(fc("-2E-2", F::scientific), 5, -0.02));
  CHECK(invalid(fc("1.5", F::scientific)));
  CHECK(invalid(fc("1.5e", F::scientific)));
  CHECK(invalid(fc("15", F::scientific)));
  // fixed: no exponent
  CHECK(ok(fc("1.5e3", F::fixed), 3, 1.5));
  CHECK(ok(fc("123.25", F::fixed), 6, 123.25));
  CHECK(ok(fc("7E1", F::fixed), 1, 7.0));
  // general = fixed | scientific: the exponent is optional
  CHECK(ok(fc("1.5e3", F::general), 5, 1500.0));
  CHECK(ok(fc("1.5", F::general), 3, 1.5));
  CHECK(ok(fc("1.5e3", F::fixed | F::scientific), 5, 1500.0));
  // hex: no prefix, optional binary exponent
  CHECK(ok(fc("1.8p1", F::hex), 5, 3.0));
  CHECK(ok(fc("1p-2", F::hex), 4, 0.25));
  CHECK(ok(fc("A", F::hex), 1, 10.0));
  CHECK(ok(fc("ff.8", F::hex), 4, 255.5));
  CHECK(ok(fc("-1.0P+4", F::hex), 7, -16.0));
  CHECK(ok(fc(".8", F::hex), 2, 0.5));
  CHECK(ok(fc("1.8", F::hex), 3, 1.5));
  CHECK(ok(fc("1p", F::hex), 1, 1.0));
  CHECK(ok(fc("1p+", F::hex), 1, 1.0));
  CHECK(ok(fc("0x123", F::hex), 1, 0.0));  // the draft's example
  CHECK(ok(fc("0X1p0", F::hex), 1, 0.0));
  CHECK(ok(fc("1g", F::hex), 1, 1.0));
  CHECK(invalid(fc("g1", F::hex)));
  CHECK(invalid(fc("p1", F::hex)));
  // a decimal exponent is not part of a hex number; 'e' is a hex digit
  CHECK(ok(fc("1e1", F::hex), 3, 481.0));
  // hex in float and long double
  {
    float f = 0;
    std::string_view s = "1.fffffep127";
    auto r = std::from_chars(s.data(), s.data() + s.size(), f, F::hex);
    CHECK(r.ec == std::errc{} && r.ptr == s.data() + s.size() && f == 0x1.fffffep127f);
    long double ld = 0;
    s = "c.8p-3";
    auto r2 = std::from_chars(s.data(), s.data() + s.size(), ld, F::hex);
    CHECK(r2.ec == std::errc{} && ld == 1.5625L);
  }
  // decimal formats do not accept hex digits
  CHECK(ok(fc("12ab", F::general), 2, 12.0));
  return 0;
}
