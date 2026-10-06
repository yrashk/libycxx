// Numeric I/O on wide streams with a named locale's numpunct<wchar_t>
// ([locale.numpunct.byname]: the de_DE.UTF-8 conventions, decimal_point ',' and thousands_sep
// '.' with groups of 3, which localeconv() in that locale confirms).
// [facet.num.put.virtuals] stage 2: the decimal point is numpunct's decimal_point() and the
// integer part is grouped with thousands_sep() as grouping() says.
// [facet.num.get.virtuals] stage 2: a thousands_sep before the decimal point is remembered and
// skipped, the decimal_point() is read as '.', a thousands_sep after it ends the field; /4: the
// positions of the separators are checked against grouping(), failbit when inconsistent; /5:
// eofbit when stage 2 ended at the end of the input.
#include <locale.h>
#include <string.h>
#include <locale>
#include <sstream>
#include <string>
#include "check.hpp"
#include "named_locale.hpp"

template <class T>
static std::ios_base::iostate read(const wchar_t* text, T& v, const std::locale& loc) {
  std::wistringstream in(text);
  in.imbue(loc);
  in >> v;
  return in.rdstate();
}

int main() {
  const char* de = require_locale("de_DE.UTF-8");
  struct conv {
    std::string dp, ts, grouping;
  };
  const conv c = in_c_locale(de, [] {
    const lconv* l = localeconv();
    return conv{l->decimal_point, l->thousands_sep, l->grouping};
  });
  CHECK(c.dp == "," && c.ts == ".");
  CHECK(!c.grouping.empty() && c.grouping[0] == 3);

  const std::locale loc(de);
  const auto& np = std::use_facet<std::numpunct<wchar_t>>(loc);
  CHECK(np.decimal_point() == L',');
  CHECK(np.thousands_sep() == L'.');
  CHECK(np.grouping() == c.grouping);

  std::wostringstream out;
  out.imbue(loc);
  out << 1234567L << L' ' << -1234 << L' ' << 999 << L' ' << std::fixed;
  out.precision(2);
  out << 12345.5;
  CHECK(out.str() == L"1.234.567 -1.234 999 12.345,50");

  long v = 0;
  CHECK(read(L"1.234.567", v, loc) == std::ios_base::eofbit);
  CHECK(v == 1234567);
  CHECK(read(L"1234567", v, loc) == std::ios_base::eofbit); // no separators: consistent
  CHECK(v == 1234567);
  CHECK(read(L"-12.345 ", v, loc) == std::ios_base::goodbit);
  CHECK(v == -12345);
  // inconsistent groups
  CHECK(read(L"12.34", v, loc) & std::ios_base::failbit);
  CHECK(read(L"1.2345.678", v, loc) & std::ios_base::failbit);

  double d = 0;
  CHECK(read(L"1.234,25", d, loc) == std::ios_base::eofbit);
  CHECK(d == 1234.25);
  // a separator after the decimal point ends the field
  std::wistringstream in(L"3,5.7");
  in.imbue(loc);
  in >> d;
  CHECK(in.good() && d == 3.5);
  CHECK(in.peek() == L'.');
}
