// [format.string.std]/17: "When the L option is used, the form used for the conversion is
// called the locale-specific form." (17.1) integral types: digit group separators as if from
// numpunct<charT>::grouping and thousands_sep; (17.2) floating-point types: group separators
// and the radix character from decimal_point; (17.3) the textual representation of bool:
// numpunct<charT>::truename / falsename. Without L the locale is not used.
// [format.functions]: format(const locale& loc, fmt, args...) uses loc; the overloads without a
// locale use std::locale() ([format.context]/7: "std::locale() otherwise").
// [format.string.std] width: the separators count toward the field width.
#include <format>
#include <locale>
#include <string>
#include <limits>
#include "check.hpp"

template <class C>
struct Punct : std::numpunct<C> {
  C do_thousands_sep() const override { return C('\''); }
  C do_decimal_point() const override { return C(','); }
  std::string do_grouping() const override { return "\3"; }
  std::basic_string<C> do_truename() const override { return std::basic_string<C>(1, C('Y')) + C('e') + C('s'); }
  std::basic_string<C> do_falsename() const override { return std::basic_string<C>(1, C('N')) + C('o'); }
};

struct Punct2 : std::numpunct<char> {  // groups of 2 after a first group of 3, sep '.'
  char do_thousands_sep() const override { return '.'; }
  std::string do_grouping() const override { return "\3\2"; }
};

int main() {
  const std::locale loc(std::locale(std::locale::classic(), new Punct<char>), new Punct<wchar_t>);
  // integers
  CHECK(std::format(loc, "{:L}", 1234567) == "1'234'567");
  CHECK(std::format(loc, "{:L}", -1234567) == "-1'234'567");
  CHECK(std::format(loc, "{:L}", 123) == "123");
  CHECK(std::format(loc, "{:L}", 1000u) == "1'000");
  CHECK(std::format(loc, "{:L}", 0) == "0");
  CHECK(std::format(loc, "{:Ld}", 123456789012LL) == "123'456'789'012");
  CHECK(std::format(loc, "{:+L}", 1000) == "+1'000");
  CHECK(std::format(loc, "{}", 1234567) == "1234567");  // no L: locale not used
  CHECK(std::format(loc, "{:>12L}|", 1234567) == "   1'234'567|");
  CHECK(std::format(loc, "{:*<10L}|", 12345) == "12'345****|");
  // floating point
  CHECK(std::format(loc, "{:L}", 1234.5) == "1'234,5");
  CHECK(std::format(loc, "{:Lf}", 1234567.125) == "1'234'567,125000");
  CHECK(std::format(loc, "{:.2Lf}", -9876.5) == "-9'876,50");
  CHECK(std::format(loc, "{:Le}", 1234.5) == "1,234500e+03");
  CHECK(std::format(loc, "{:L}", 0.5) == "0,5");
  CHECK(std::format(loc, "{}", 1234.5) == "1234.5");
  CHECK(std::format(loc, "{:L}", std::numeric_limits<double>::infinity()) == "inf");
  CHECK(std::format(loc, "{:L}", 1.5f) == "1,5");
  CHECK(std::format(loc, "{:L}", 12345.25L) == "12'345,25");
  // bool
  CHECK(std::format(loc, "{:L}", true) == "Yes");
  CHECK(std::format(loc, "{:L}", false) == "No");
  CHECK(std::format(loc, "{:Ls}", false) == "No");
  CHECK(std::format(loc, "{:>5L}|", true) == "  Yes|");
  CHECK(std::format(loc, "{}", true) == "true");
  // wide
  CHECK(std::format(loc, L"{:L}", 1234567) == L"1'234'567");
  CHECK(std::format(loc, L"{:L}", 1234.5) == L"1'234,5");
  CHECK(std::format(loc, L"{:L}", true) == L"Yes");
  // a non-uniform grouping
  const std::locale loc2(std::locale::classic(), new Punct2);
  CHECK(std::format(loc2, "{:L}", 123456789) == "12.34.56.789");
  // the classic locale has no grouping
  CHECK(std::format(std::locale::classic(), "{:L}", 1234567) == "1234567");
  CHECK(std::format(std::locale::classic(), "{:L}", 1.5) == "1.5");
  // without a locale argument: the global locale
  std::locale old = std::locale::global(loc);
  CHECK(std::format("{:L}", 1234567) == "1'234'567");
  CHECK(std::format("{:L}", true) == "Yes");
  CHECK(std::format("{}", 1234567) == "1234567");
  char buf[32] = {};
  auto r = std::format_to_n(buf, 31, "{:L}", 1234);
  CHECK(std::string(buf, r.out) == "1'234" && r.size == 5);
  CHECK(std::formatted_size("{:L}", 1234567) == 9);
  std::locale::global(old);
  CHECK(std::format("{:L}", 1234567) == "1234567");
  CHECK(std::formatted_size(loc, "{:L}", 1234567) == 9);
  std::string out;
  std::format_to(std::back_inserter(out), loc, "{:L}", 12345);
  CHECK(out == "12'345");
  return 0;
}
