// [format.string.std] Table 110, type none without precision: "to_chars(first, last,
// value)", the shortest round-trip representation ([charconv.to.chars]/2; the choice
// between f and e style, /7, is tested in float_shortest_plain_style); negative zero keeps
// its sign; infinity and NaN are inf and nan (/24); sign and 0 options.
#include <format>
#include <limits>
#include <string>
#include "check.hpp"

int main() {
  CHECK(std::format("{}", 0.0) == "0");
  CHECK(std::format("{}", -0.0) == "-0");
  CHECK(std::format("{}", 1.0) == "1");
  CHECK(std::format("{}", 0.1) == "0.1");
  CHECK(std::format("{}", 0.1f) == "0.1");
  CHECK(std::format("{}", 0.3) == "0.3");
  CHECK(std::format("{}", 0.1 + 0.2) == "0.30000000000000004");
  CHECK(std::format("{}", 1.5) == "1.5");
  CHECK(std::format("{}", 123456789.0) == "123456789");
  CHECK(std::format("{}", 1e-5) == "1e-05");
  CHECK(std::format("{}", 1e16) == "1e+16");
  CHECK(std::format("{}", 1.25e100) == "1.25e+100");
  CHECK(std::format("{}", 5e-324) == "5e-324");
  CHECK(std::format("{}", std::numeric_limits<double>::max()) == "1.7976931348623157e+308");
  CHECK(std::format("{}", 2.5L) == "2.5");
  const double inf = std::numeric_limits<double>::infinity();
  const double nan = std::numeric_limits<double>::quiet_NaN();
  CHECK(std::format("{0:},{0:+},{0:-},{0: }", inf) == "inf,+inf,inf, inf");
  CHECK(std::format("{0:},{0:+},{0:-},{0: }", nan) == "nan,+nan,nan, nan");
  CHECK(std::format("{}", -inf) == "-inf");
  CHECK(std::format("{0:},{0:+},{0: }", -0.0) == "-0,-0,-0");
  CHECK(std::format("{0:+},{0: }", 0.0) == "+0, 0");
  CHECK(std::format("{:06}", inf) == "   inf"); // 0 has no effect for infinity
  CHECK(std::format("{:06}", -1.5) == "-001.5");
  CHECK(std::format("{:8}", 1.5) == "     1.5"); // right aligned by default
  CHECK(std::format("{:<8}|", 1.5) == "1.5     |");
}
