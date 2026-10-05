// [format.string.std] Table 110: e/E, f/F, g/G default to precision 6; a/A without
// precision are the shortest hex form; precision with none uses general; # keeps the
// decimal point and, for g/G, trailing zeros (/7); upper-case types give INF/NAN (/24).
// COUNTERPART: libcxx:utilities/format/format.formatter/format.formatter.spec/formatter.floating_point.pass.cpp
#include <format>
#include <limits>
#include <string>
#include "check.hpp"

int main() {
  const double v = 1234.5678;
  CHECK(std::format("{:e}", v) == "1.234568e+03");
  CHECK(std::format("{:E}", v) == "1.234568E+03");
  CHECK(std::format("{:.2e}", v) == "1.23e+03");
  CHECK(std::format("{:f}", v) == "1234.567800");
  CHECK(std::format("{:F}", v) == "1234.567800");
  CHECK(std::format("{:.1f}", v) == "1234.6");
  CHECK(std::format("{:.0f}", v) == "1235");
  CHECK(std::format("{:#.0f}", v) == "1235.");
  CHECK(std::format("{:g}", v) == "1234.57");
  CHECK(std::format("{:G}", 1e-10) == "1E-10");
  CHECK(std::format("{:g}", 100000.0) == "100000");
  CHECK(std::format("{:g}", 1000000.0) == "1e+06");
  CHECK(std::format("{:#g}", 1.0) == "1.00000");
  CHECK(std::format("{:g}", 1.0) == "1");
  CHECK(std::format("{:.3}", v) == "1.23e+03"); // none with precision: general
  CHECK(std::format("{:.10}", v) == "1234.5678");
  CHECK(std::format("{:.0}", 2.5) == "2");
  CHECK(std::format("{:a}", 1.0) == "1p+0");
  CHECK(std::format("{:a}", 0.5) == "1p-1");
  CHECK(std::format("{:A}", 255.0) == "1.FEP+7");
  CHECK(std::format("{:.3a}", 1.0) == "1.000p+0");
  CHECK(std::format("{:#a}", 1.0) == "1.p+0");
  CHECK(std::format("{:a}", -3.0) == "-1.8p+1");
  CHECK(std::format("{:e}", 0.0) == "0.000000e+00");
  CHECK(std::format("{:.3f}", -0.0) == "-0.000");
  const double inf = std::numeric_limits<double>::infinity();
  const double nan = std::numeric_limits<double>::quiet_NaN();
  CHECK(std::format("{:F} {:E} {:G} {:A}", inf, inf, -inf, nan) == "INF INF -INF NAN");
  CHECK(std::format("{:f} {:e} {:a}", nan, -inf, inf) == "nan -inf inf");
  CHECK(std::format("{:#f}", inf) == "inf");
  CHECK(std::format("{:+.2f}", 3.14159) == "+3.14");
  CHECK(std::format("{:010.2f}", -3.14159) == "-000003.14");
  CHECK(std::format("{:^10.1f}", 2.25) == "   2.2    ");
  CHECK(std::format("{:.2f}", 2.675f) == "2.67"); // float: 2.67499995...
  CHECK(std::format("{:.30f}", 0.1) == "0.100000000000000005551115123126");
  CHECK(std::format("{:f}", 1e20) == "100000000000000000000.000000");
}
