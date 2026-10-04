// [facet.num.get.virtuals] Stage 2: "bool discard = ct == thousands_sep() && grouping().length()
// != 0; ... If discard is true, then if '.' has not yet been accumulated, then the position of
// the character is remembered, but the character is otherwise ignored. Otherwise, if '.' has
// already been accumulated, the character is discarded and Stage 2 terminates."
// Stage 3: "The resultant numeric value is stored in val."
// /4: "Digit grouping is checked. That is, the positions of discarded separators are examined
// for consistency with use_facet<numpunct<charT>>(loc).grouping(). If they are not consistent
// then ios_base::failbit is assigned to err." (the value stored in Stage 3 is kept).
// [locale.numpunct]/2: "units: digits | digits thousands-sep units ... where the number of
// digits between thousands-seps is as specified by do_grouping(). For parsing, if the digits
// portion contains no thousands-separators, no grouping constraint is applied."
// [facet.numpunct.virtuals]/3: vec[i] is the size of group i, rightmost first; "If vec.size()
// <= i, the number is the same as group (i - 1)".
#include <locale>
#include <sstream>
#include <iterator>
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
static T get(const std::locale& loc, const char* s, std::ios_base::iostate& err, std::string* rest = nullptr,
             T init = T(-77)) {
  std::istringstream is(s);
  is.imbue(loc);
  using It = std::istreambuf_iterator<char>;
  T v = init;
  err = std::ios_base::goodbit;
  It it = std::use_facet<std::num_get<char>>(loc).get(It(is), It(), is, err, v);
  if (rest) {
    rest->clear();
    for (; it != It(); ++it) rest->push_back(*it);
  }
  return v;
}

int main() {
  using B = std::ios_base;
  B::iostate err;
  std::string rest;
  const std::locale c = std::locale::classic();
  const std::locale g32(c, new Punct("\3\2"));
  const std::locale g3(c, new Punct("\3"));

  // Consistent groupings.
  CHECK(get<long>(g32, "12,34,567", err) == 1234567 && err == B::eofbit);
  CHECK(get<long>(g32, "1,23,456", err) == 123456 && err == B::eofbit);  // leading group shorter
  CHECK(get<long>(g32, "-12,345 ", err, &rest) == -12345 && err == B::goodbit && rest == " ");
  CHECK(get<long>(g3, "1,234,567", err) == 1234567 && err == B::eofbit);
  CHECK(get<unsigned long>(g3, "+999,999", err) == 999999ul && err == B::eofbit);
  // No separators at all: no grouping constraint.
  CHECK(get<long>(g32, "1234567", err) == 1234567 && err == B::eofbit);

  // Inconsistent groupings: the value is still stored, failbit is set.
  CHECK(get<long>(g32, "1,234,567", err) == 1234567 && err == (B::failbit | B::eofbit));
  CHECK(get<long>(g32, "123,45,678", err) == 12345678 && err == (B::failbit | B::eofbit));
  CHECK(get<long>(g3, "12,34", err) == 1234 && err == (B::failbit | B::eofbit));
  CHECK(get<long>(g3, "1234,567", err) == 1234567 && err == (B::failbit | B::eofbit));
  CHECK(get<long>(g3, "1,234,56", err) == 123456 && err == (B::failbit | B::eofbit));
  CHECK(get<long>(g3, "1,,234", err) == 1234 && err == (B::failbit | B::eofbit));  // empty group
  CHECK(get<long>(g3, "1,234, ", err, &rest) == 1234 && (err & B::failbit) && rest == " ");  // empty last group
  CHECK(get<unsigned>(g3, "12,34", err) == 1234u && (err & B::failbit));

  // Without grouping the separator is not discarded and ends the field.
  CHECK(get<long>(c, "1,234", err, &rest) == 1 && err == B::goodbit && rest == ",234");

  // Floating point: separators before the decimal point are grouped ...
  CHECK(get<double>(g3, "1,234,567.25", err) == 1234567.25 && err == B::eofbit);
  CHECK(get<double>(g3, "12,34.5", err) == 1234.5 && err == (B::failbit | B::eofbit));
  // ... and one after it ends Stage 2 (the part before it is converted).
  CHECK(get<double>(g3, "1,234.5,6", err) == 1234.5 && !(err & B::failbit));

  // Separator and decimal point swapped (a common European style).
  const std::locale eu(c, new Punct("\3", '.', ','));
  CHECK(get<double>(eu, "1.234.567,5", err) == 1234567.5 && err == B::eofbit);
  CHECK(get<long>(eu, "7.654.321", err) == 7654321 && err == B::eofbit);
  // A '.' is the separator here, not a decimal point: "1.5" is the group "5" of size 1.
  CHECK(get<double>(eu, "1.5", err) == 15.0 && err == (B::failbit | B::eofbit));

  // Through operator>> (the istream sets failbit, the long value is stored).
  std::istringstream is("1,23,45,678 9");
  is.imbue(g32);
  long v = 0;
  is >> v;
  CHECK(v == 12345678 && !is.fail());
  is >> v;
  CHECK(v == 9 && !is.fail());
  return 0;
}
