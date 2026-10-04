// [facet.num.put.virtuals] Stage 2: "For arithmetic types, punct.thousands_sep() characters are
// inserted into the sequence as determined by the value returned by punct.do_grouping() using
// the method described in [facet.numpunct.virtuals]. Decimal point characters(.) are replaced
// by punct.decimal_point()." Stage 3: padding is added "if str.width() is nonzero and the
// number of charT's in the sequence after stage 2 is less than str.width()".
// [facet.numpunct.virtuals]/3: vec[i] is the number of digits in group i, "starting with
// position 0 as the rightmost group. If vec.size() <= i, the number is the same as group
// (i - 1); if (i < 0 || vec[i] <= 0 || vec[i] == CHAR_MAX), the size of the digit group is
// unlimited." [locale.numpunct]/2: the separators are in the digits before the decimal point
// ("floatval: signopt units fractionalopt exponentopt").
#include <locale>
#include <sstream>
#include <ios>
#include <climits>
#include <limits>
#include <string>
#include "check.hpp"

struct Punct : std::numpunct<char> {
  std::string g;
  char sep, dp;
  Punct(std::string grp, char s = ',', char d = '.') : g(grp), sep(s), dp(d) {}
  char do_thousands_sep() const override { return sep; }
  char do_decimal_point() const override { return dp; }
  std::string do_grouping() const override { return g; }
};

template<class T>
static std::string put(const std::locale& loc, T v, std::ios_base::fmtflags f = std::ios_base::dec,
                       int width = 0, int prec = 6, char fill = '*') {
  std::ostringstream os;
  os.imbue(loc);
  os.flags(f);
  os.width(width);
  os.precision(prec);
  os.fill(fill);
  os << v;
  CHECK(os.width() == 0);
  return os.str();
}

int main() {
  using B = std::ios_base;
  const std::locale c = std::locale::classic();
  const std::locale g3(c, new Punct("\3"));
  const std::locale g32(c, new Punct("\3\2"));
  const std::locale g1(c, new Punct("\1"));
  const std::string capped_s{'\1', static_cast<char>(CHAR_MAX)};
  const std::locale capped(c, new Punct(capped_s));
  const std::locale zero(c, new Punct(std::string(1, '\0')));
  const std::locale eu(c, new Punct("\3", '.', ','));

  CHECK(put(g3, 1234567) == "1,234,567");
  CHECK(put(g3, -1234567) == "-1,234,567");
  CHECK(put(g3, 123) == "123");
  CHECK(put(g3, 1234) == "1,234");
  CHECK(put(g3, 0) == "0");
  CHECK(put(g3, 1234567, B::dec | B::showpos) == "+1,234,567");
  CHECK(put(g3, 18446744073709551615ull) == "18,446,744,073,709,551,615");
  CHECK(put(g32, 1234567) == "12,34,567");
  CHECK(put(g32, 123456789L) == "12,34,56,789");
  CHECK(put(g1, 4321) == "4,3,2,1");
  CHECK(put(capped, 1234567) == "123456,7");  // CHAR_MAX: the next group is unlimited
  CHECK(put(zero, 1234567) == "1234567");     // vec[0] <= 0: unlimited

  // Hexadecimal and octal digits are grouped as well.
  CHECK(put(g32, 0x1234567, B::hex) == "12,34,567");
  CHECK(put(g3, 01234567, B::oct) == "1,234,567");

  // Width counts the separators; internal pads after the sign.
  CHECK(put(g3, 1234567, B::dec, 12) == "***1,234,567");
  CHECK(put(g3, -1234567, B::dec | B::internal, 12) == "-**1,234,567");
  CHECK(put(g3, 1234567, B::dec | B::left, 10) == "1,234,567*");
  CHECK(put(g3, 1234567, B::dec, 9) == "1,234,567");

  // Floating point: only the digits before the decimal point are grouped.
  CHECK(put(g3, 1234567.25, B::fixed, 0, 2) == "1,234,567.25");
  CHECK(put(g3, 1234.5678, B::fixed, 0, 4) == "1,234.5678");
  CHECK(put(g3, -1234567.0, B::fmtflags{}, 0, 10) == "-1,234,567");
  CHECK(put(g3, 1234.5, B::scientific, 0, 2) == "1.23e+03");
  CHECK(put(eu, 1234567.5, B::fixed, 0, 1) == "1.234.567,5");
  CHECK(put(eu, 0.5, B::fmtflags{}) == "0,5");
  CHECK(put(g3, std::numeric_limits<double>::infinity()) == "inf");
  CHECK(put(g3, 123.0, B::fixed | B::showpoint, 0, 0) == "123.");
  return 0;
}
