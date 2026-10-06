// The L option with named locales. [format.string.std]/17: the locale-specific form uses the
// context's locale: (17.1) integral types get digit group separators as if from
// numpunct<charT>::grouping and thousands_sep (for every integer presentation type); (17.2)
// floating-point types also the radix character from decimal_point; (17.3) bool's text is
// numpunct's truename / falsename. [format.functions]: format(loc, fmt, args...) uses loc;
// [format.context]/7: without a locale argument the context's locale is std::locale(), so
// after locale::global(named) ([locale.statics]/1) the named locale is used.
// The reference is the C library's localeconv() in the same locale (de_DE: decimal ',' and
// thousands '.'; en_US: '.' and ','; both with groups of three; fr_FR.UTF-8: ',' and U+202F).
// [format.string.std]: separators count toward the field width.
#include <locale.h>
#include <format>
#include <locale>
#include <string>
#include "check.hpp"
#include "named_locale.hpp"

int main() {
  const char* de_name = require_locale("de_DE.UTF-8");
  const char* us_name = require_locale("en_US.UTF-8");
  const std::string de_conv = in_c_locale(de_name, [] {
    const lconv* l = localeconv();
    return std::string(l->decimal_point) + l->thousands_sep;
  });
  CHECK(de_conv == ",.");
  const std::locale de(de_name), us(us_name);

  CHECK(std::format(de, "{:L}", 1234567) == "1.234.567");
  CHECK(std::format(de, "{:L}", -1234567LL) == "-1.234.567");
  CHECK(std::format(de, "{:L}", 999) == "999");
  CHECK(std::format(de, "{}", 1234567) == "1234567"); // no L: no locale
  CHECK(std::format(de, "{:L}", 1234.5) == "1.234,5");
  CHECK(std::format(de, "{:.2Lf}", 1234567.125) == "1.234.567,12");
  CHECK(std::format(de, "{:Le}", 1.5e10) == "1,500000e+10");
  CHECK(std::format(de, "{:>12L}", 1234567) == "   1.234.567");
  CHECK(std::format(de, "{:012L}", 1234567) == "0001.234.567");
  CHECK(std::format(de, "{:Lx}", 0x123456) == "123.456");
  CHECK(std::format(de, "{:#Lx}", 0x123456) == "0x123.456");
  CHECK(std::format(us, "{:.3Lf}", 1234567.891) == "1,234,567.891");
  CHECK(std::format(us, "{:L}", 1234567u) == "1,234,567");
  // bool: numpunct's names (the named locales keep "true" / "false", [locale.numpunct.virtuals])
  const auto& np = std::use_facet<std::numpunct<char>>(de);
  CHECK(std::format(de, "{:L}", true) == np.truename());
  // wide
  CHECK(std::format(de, L"{:L}", 1234567) == L"1.234.567");
  CHECK(std::format(de, L"{:L}", 0.25) == L"0,25");
  // a separator that is not a single char in the locale's encoding: fr_FR.UTF-8's U+202F is the
  // wide numpunct's thousands_sep ([locale.numpunct.byname]), and so the wide L form's
  const std::locale fr(require_locale("fr_FR.UTF-8"));
  const wchar_t sep = std::use_facet<std::numpunct<wchar_t>>(fr).thousands_sep();
  CHECK(sep == L'\u202f');
  CHECK(std::format(fr, L"{:L}", 1234567) == L"1\u202f234\u202f567");
  CHECK(std::format(fr, L"{:.1Lf}", 12345.25) == L"12\u202f345,2");

  // the global locale
  CHECK(std::format("{:L}", 1234567) == "1234567"); // classic: no grouping
  std::locale::global(de);
  CHECK(std::format("{:L}", 1234567) == "1.234.567");
  CHECK(std::format("{:L}", 2.5) == "2,5");
  CHECK(std::format("{}", 2.5) == "2.5");
  std::locale::global(std::locale::classic());
  CHECK(std::format("{:L}", 2.5) == "2.5");
}
