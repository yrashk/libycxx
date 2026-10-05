// [locale.cons]/2-4: locale(const char*) "constructs a locale using standard C locale names";
// the set of valid names is "C", "", and implementation-defined values, here every name the C
// library has; runtime_error for a name that is not valid, or null. /6-9: locale(other, name,
// cats) takes the facets of cats from locale(name), and has a name iff other has one.
// [locale.members]/5: name() is the locale's name ("*" without one); [locale.operators]/1: two
// locales with the same name (other than "*") are equal.
// REQUIRES: exceptions
#include <locale>
#include <stdexcept>
#include <string>
#include "check.hpp"
#include "named_locale.hpp"

static bool throws(const char* n) {
  try {
    std::locale l(n);
  } catch (const std::runtime_error&) {
    return true;
  }
  return false;
}

int main() {
  const char* de = require_locale("de_DE.UTF-8");
  const char* fr = require_locale("fr_FR.ISO8859-15");

  std::locale l(de);
  CHECK(l.name() == de);
  CHECK(l == std::locale(std::string(de)));
  CHECK(l != std::locale::classic());
  CHECK(std::locale(fr).name() == fr);

  // what the C library rejects is not valid
  CHECK(!c_has_locale("xx_NOWHERE.UTF-8"));
  CHECK(throws("xx_NOWHERE.UTF-8"));
  CHECK(throws(nullptr));

  // one category from a name: a composite name, equal to the locale made from that name
  std::locale mixed(std::locale::classic(), de, std::locale::numeric);
  const std::string n = mixed.name();
  CHECK(n != "*" && n != "C" && n != de);
  CHECK(n.find(de) != std::string::npos);
  CHECK(std::locale(n.c_str()) == mixed);
  CHECK(std::use_facet<std::numpunct<char>>(mixed).decimal_point() ==
        std::use_facet<std::numpunct<char>>(l).decimal_point());
  CHECK(&std::use_facet<std::ctype<char>>(mixed) == &std::use_facet<std::ctype<char>>(std::locale::classic()));
  // all categories from the name: the name itself
  CHECK(std::locale(mixed, de, std::locale::all).name() == de);
  // an invalid name for the categories concerned throws, an unnamed other gives no name
  try {
    std::locale bad(l, "xx_NOWHERE.UTF-8", std::locale::time);
    CHECK(false);
  } catch (const std::runtime_error&) {
  }
  std::locale unnamed(l, new std::numpunct<char>);
  CHECK(unnamed.name() == "*");
  CHECK(std::locale(unnamed, fr, std::locale::ctype).name() == "*");
  // locale(other, one, cats): named iff both are
  CHECK(std::locale(l, std::locale(fr), std::locale::time).name() != "*");
  CHECK(std::locale(l, unnamed, std::locale::time).name() == "*");
  CHECK(std::locale(l, unnamed, std::locale::none).name() == "*"); // also with no category

  // a facet of the composite comes from its own name
  std::locale both(std::locale(de), fr, std::locale::ctype);
  CHECK(std::locale(both.name()) == both);
  CHECK(std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(both).max_length() == 1); // ISO-8859-15
  CHECK(std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(l).max_length() > 1);    // UTF-8
}
