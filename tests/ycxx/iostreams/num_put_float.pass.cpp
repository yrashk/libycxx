// [facet.num.put.virtuals] Table 98 (C locale): fixed -> %f / %F, scientific -> %e / %E,
// fixed|scientific -> %a / %A without a precision, otherwise %g / %G; precision() is the
// printf precision; showpos '+', showpoint '#'; long double uses L. Padding per Table 101.
#include <sstream>
#include <ios>
#include <limits>
#include <string>
#include "check.hpp"

static std::string fmt(double v, std::ios_base::fmtflags f, int prec = 6, int width = 0, char fill = ' ') {
  std::ostringstream os;
  os.flags(f);
  os.precision(prec);
  os.width(width);
  os.fill(fill);
  os << v;
  return os.str();
}

int main() {
  using B = std::ios_base;
  const B::fmtflags none{};
  CHECK(fmt(1.5, none) == "1.5");
  CHECK(fmt(1.0, none) == "1");
  CHECK(fmt(1e6, none) == "1e+06");
  CHECK(fmt(123456.0, none) == "123456");
  CHECK(fmt(0.0001, none) == "0.0001");
  CHECK(fmt(0.00001, none) == "1e-05");
  CHECK(fmt(1e6, B::uppercase) == "1E+06");
  CHECK(fmt(3.14159265, none, 3) == "3.14");
  CHECK(fmt(3.14159265, none, 0) == "3");  // %.0g behaves as precision 1
  CHECK(fmt(1.0, B::showpoint) == "1.00000");
  CHECK(fmt(2.5, B::showpos) == "+2.5");
  CHECK(fmt(-0.0, none) == "-0");

  CHECK(fmt(1.5, B::fixed) == "1.500000");
  CHECK(fmt(1.5, B::fixed, 2) == "1.50");
  CHECK(fmt(2.5, B::fixed, 0) == "2");      // round half to even, as printf
  CHECK(fmt(2.5, B::fixed | B::showpoint, 0) == "2.");
  CHECK(fmt(1e20, B::fixed, 0) == "100000000000000000000");
  CHECK(fmt(1234.5, B::scientific, 2) == "1.23e+03");
  CHECK(fmt(1234.5, B::scientific | B::uppercase, 2) == "1.23E+03");
  CHECK(fmt(0.0, B::scientific, 1) == "0.0e+00");
  CHECK(fmt(1.0, B::fixed | B::scientific) == "0x1p+0");
  CHECK(fmt(1.0, B::fixed | B::scientific, 2) == "0x1p+0");  // no precision for %a
  CHECK(fmt(-0.5, B::fixed | B::scientific | B::uppercase) == "-0X1P-1");

  const double inf = std::numeric_limits<double>::infinity();
  CHECK(fmt(inf, none) == "inf");
  CHECK(fmt(-inf, none) == "-inf");
  CHECK(fmt(inf, B::uppercase) == "INF");
  CHECK(fmt(inf, B::fixed | B::uppercase) == "INF");
  CHECK(fmt(std::numeric_limits<double>::quiet_NaN(), B::fixed) == "nan");

  CHECK(fmt(-1.5, B::internal, 6, 8, '0') == "-00001.5");
  CHECK(fmt(1.5, B::left, 6, 6, '*') == "1.5***");
  CHECK(fmt(1.5, B::fixed | B::showpos | B::internal, 1, 7, ' ') == "+   1.5");

  std::ostringstream ld;
  ld.precision(3);
  ld << std::fixed << 2.25L << ' ' << 0.5f;
  CHECK(ld.str() == "2.250 0.500");
  return 0;
}
