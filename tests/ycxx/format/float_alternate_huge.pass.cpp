// The alternate form with fixed notation and precision 0 for the largest finite values.
// [format.string.std]/8 (# option): "For floating-point types, the alternate form causes the
// result of the conversion of finite values to always contain a decimal-point character, even
// if no digits follow it." Table 105 (f/F): to_chars(first, last, value, chars_format::fixed,
// precision) with the given precision; [charconv.to.chars]/13: printf %.0f in the "C"
// locale, i.e. the exact decimal value of DBL_MAX (309 digits), which is an integer.
// Width and zero padding count the decimal point; formatted_size and format_to_n agree
// ([format.functions]/19-24).
#include <cfloat>
#include <charconv>
#include <format>
#include <string>
#include "check.hpp"

int main() {
  const std::string dmax =
      "17976931348623157081452742373170435679807056752584499659891747680315726078002853876058955863276687"
      "81715404589535143824642343213268894641827684675467035375169860499105765512820762454900903893289440"
      "75868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404"
      "026184124858368";
  CHECK(dmax.size() == 309);
  CHECK(std::format("{:.0f}", DBL_MAX) == dmax);
  CHECK(std::format("{:#.0f}", DBL_MAX) == dmax + ".");
  CHECK(std::format("{:#.0F}", -DBL_MAX) == "-" + dmax + ".");
  CHECK(std::format("{:#.2f}", DBL_MAX) == dmax + ".00");
  CHECK(std::format("{:+#315.0f}", DBL_MAX) == "    +" + dmax + ".");
  CHECK(std::format("{:#0315.0f}", DBL_MAX) == "00000" + dmax + ".");
  CHECK(std::formatted_size("{:#.0f}", DBL_MAX) == 310);
  char buf[400];
  const auto r = std::format_to_n(buf, 400, "{:#.0f}", DBL_MAX);
  CHECK(r.size == 310 && std::string(buf, r.out) == dmax + ".");

  // long double: the same as to_chars with fixed and precision 0, plus the decimal point.
  char lbuf[5000];
  const auto lr = std::to_chars(lbuf, lbuf + sizeof lbuf, LDBL_MAX, std::chars_format::fixed, 0);
  CHECK(lr.ec == std::errc());
  const std::string lmax(lbuf, lr.ptr);
  CHECK(std::format("{:#.0f}", LDBL_MAX) == lmax + ".");
  CHECK(std::format("{:.0f}", LDBL_MAX) == lmax);
  CHECK(std::format("{:#.0f}", 1e300) == std::format("{:.0f}", 1e300) + ".");
  CHECK(std::format("{:#.0e}", DBL_MAX) == "2.e+308");
  CHECK(std::format("{:#.0g}", DBL_MAX) == "2.e+308");
  return 0;
}
