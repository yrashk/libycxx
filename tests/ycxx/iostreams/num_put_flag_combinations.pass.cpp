// [facet.num.put.virtuals] Stage 1 is defined by printf conversions in the "C" locale:
// Table 97 (%o, %x, %X, %d, %u), Table 98 (%f %F %e %E %a %A %g %G), Table 100 ('+' for
// showpos, '#' for showbase / showpoint); "if floatfield != (ios_base::fixed |
// ios_base::scientific), str.precision() is specified as precision in the conversion
// specification. Otherwise, no precision is specified." Table 101 (ordered): left -> pad after,
// right -> pad before, "internal and a sign occurs in the representation" -> pad after the
// sign, "internal and representation after stage 1 began with 0x or 0X" -> pad after x or X,
// otherwise pad before. "str.width(0) is called." /6: bool without boolalpha is put as (int)val.
// The printf rules relied on ([c.files] refers to ISO C 7.23.6.1): '+' only affects signed
// conversions; "#" with o increases the precision so the first digit is 0, with x/X a nonzero
// result gets 0x/0X; with f/e/g "#" always prints the decimal-point character and with g
// trailing zeros are not removed; a precision of 0 for g is taken as 1; %a without a precision
// is exact; F/E/G/A use upper case (INF, NAN, X, P).
#include <sstream>
#include <ios>
#include <limits>
#include <string>
#include "check.hpp"

template<class T>
static std::string put(T v, std::ios_base::fmtflags f, int prec = 6, int width = 0, char fill = '_') {
  std::ostringstream os;
  os.flags(f);
  os.precision(prec);
  os.width(width);
  os.fill(fill);
  os << v;
  CHECK(os.width() == 0);
  return os.str();
}

int main() {
  using B = std::ios_base;
  const B::fmtflags none{};
  const double inf = std::numeric_limits<double>::infinity();

  // '+' does not apply to unsigned conversions (%u, %o, %x).
  CHECK(put(42u, B::dec | B::showpos) == "42");
  CHECK(put(42, B::hex | B::showpos) == "2a");
  CHECK(put(8, B::oct | B::showpos) == "10");
  CHECK(put(42, B::dec | B::showpos) == "+42");
  CHECK(put(42ull, B::dec | B::showpos) == "42");
  CHECK(put(true, B::dec | B::showpos) == "+1");  // (int)val

  // '#' with a zero value: no 0x prefix; octal prints a single 0.
  CHECK(put(0, B::hex | B::showbase) == "0");
  CHECK(put(0, B::oct | B::showbase) == "0");
  CHECK(put(8, B::oct | B::showbase) == "010");
  CHECK(put(255, B::hex | B::showbase | B::uppercase) == "0XFF");
  CHECK(put(0, B::hex | B::showbase | B::internal, 6, 5) == "____0");  // no 0x: pad before
  CHECK(put(255, B::hex | B::showbase | B::internal, 6, 7) == "0x___ff");
  CHECK(put(8, B::oct | B::showbase | B::internal, 6, 5) == "__010");  // leading 0 is not padding

  // %g with showpoint keeps trailing zeros; precision 0 is 1.
  CHECK(put(1.0, B::showpoint) == "1.00000");
  CHECK(put(1.0, B::showpoint, 0) == "1.");
  CHECK(put(15.0, none, 0) == "2e+01");
  CHECK(put(0.0001, B::showpoint, 2) == "0.00010");
  CHECK(put(123456789.0, B::showpoint | B::uppercase, 3) == "1.23E+08");
  CHECK(put(-0.0, none) == "-0");
  CHECK(put(-0.0, B::showpos) == "-0");
  CHECK(put(0.0, B::showpos) == "+0");

  // fixed / scientific with flags.
  CHECK(put(2.5, B::fixed, 0) == "2");   // round-half-even at the binary value 2.5
  CHECK(put(3.5, B::fixed, 0) == "4");
  CHECK(put(2.5, B::fixed | B::showpoint, 0) == "2.");
  CHECK(put(2.5, B::scientific | B::showpos, 0) == "+2e+00");
  CHECK(put(2.5, B::scientific | B::showpoint, 0) == "2.e+00");
  CHECK(put(1e-5, B::scientific | B::uppercase, 1) == "1.0E-05");
  CHECK(put(inf, B::fixed | B::uppercase) == "INF");
  CHECK(put(-inf, B::scientific) == "-inf");
  CHECK(put(inf, B::showpos) == "+inf");
  CHECK(put(std::numeric_limits<double>::quiet_NaN(), B::fixed | B::uppercase).find("NAN") != std::string::npos);

  // fixed | scientific: %a / %A, the precision is ignored.
  const B::fmtflags hexf = B::fixed | B::scientific;
  CHECK(put(1.0, hexf, 2) == "0x1p+0");
  CHECK(put(0.5, hexf, 0) == "0x1p-1");
  CHECK(put(1.0 + 1.0 / 4096, hexf, 1) == "0x1.001p+0");
  CHECK(put(-1.5, hexf | B::uppercase) == "-0X1.8P+0");
  CHECK(put(1.0, hexf | B::showpos) == "+0x1p+0");
  CHECK(put(1.0, hexf | B::showpoint) == "0x1.p+0");
  CHECK(put(inf, hexf) == "inf");
  // internal: the sign line of Table 101 comes first, then the 0x line.
  CHECK(put(1.0, hexf | B::internal, 6, 10) == "0x____1p+0");
  CHECK(put(-1.0, hexf | B::internal, 6, 10) == "-___0x1p+0");
  CHECK(put(1.0, hexf | B::internal | B::uppercase | B::showpos, 6, 10) == "+___0X1P+0");

  // internal with floating values: after the sign, else before.
  CHECK(put(-1.5, B::internal, 6, 8) == "-____1.5");
  CHECK(put(1.5, B::internal, 6, 6) == "___1.5");
  CHECK(put(-inf, B::internal, 6, 6, '0') == "-00inf");
  // adjustfield with several bits set is none of left/right/internal: pad before.
  CHECK(put(-7, B::left | B::internal, 6, 4) == "__-7");
  CHECK(put(-7, B::left | B::right, 6, 4) == "__-7");

  // long double uses L.
  CHECK(put(0.5L, B::fixed, 3) == "0.500");
  CHECK(put(1.0L, hexf).size() > 0);
  return 0;
}
