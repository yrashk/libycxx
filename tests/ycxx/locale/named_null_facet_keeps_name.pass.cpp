// [locale.cons]/11-12: locale(other, f) with a null f is a copy of other and has other's name;
// with a non-null f the result has no name. [locale.operators]/1: a copy compares equal to its
// original, and two locales with identical names compare equal.
#include <locale>
#include <string>
#include "check.hpp"
#include "named_locale.hpp"

int main() {
  const char* de = require_locale("de_DE.UTF-8");
  const std::locale named(de);

  std::locale a(named, static_cast<std::numpunct<char>*>(nullptr));
  CHECK(a.name() == de);
  CHECK(a == named);
  CHECK(&std::use_facet<std::numpunct<char>>(a) == &std::use_facet<std::numpunct<char>>(named));
  CHECK(std::use_facet<std::numpunct<char>>(a).decimal_point() == ',');

  std::locale b(std::locale::classic(), static_cast<std::ctype<wchar_t>*>(nullptr));
  CHECK(b.name() == "C");
  CHECK(b == std::locale::classic());

  // a non-null facet: no name, and not equal to the original
  std::locale c(named, new std::numpunct<char>);
  CHECK(c.name() == "*");
  CHECK(c != named);
  CHECK(std::use_facet<std::numpunct<char>>(c).decimal_point() == '.');
  // the other facets are named's
  CHECK(&std::use_facet<std::moneypunct<char>>(c) == &std::use_facet<std::moneypunct<char>>(named));
  // a copy of an unnamed locale equals it ([locale.operators]/1), and a null facet keeps that
  std::locale d(c, static_cast<std::collate<char>*>(nullptr));
  CHECK(d == c);
  CHECK(d.name() == "*");
}
