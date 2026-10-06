// [locale.members]/1-4: combine<Facet>(other) takes every facet of *this except Facet, which it
// takes from other; it throws runtime_error if has_facet<Facet>(other) is false, and the result
// has no name, also when both locales are named.
// REQUIRES: exceptions
#include <locale>
#include <stdexcept>
#include <string>
#include "check.hpp"
#include "named_locale.hpp"

struct mine : std::locale::facet {
  static std::locale::id id;
  int v;
  explicit mine(int x) : v(x) {}
};
std::locale::id mine::id;

int main() {
  const char* de = require_locale("de_DE.UTF-8");
  const std::locale named(de);
  const std::locale& c = std::locale::classic();

  std::locale x = c.combine<std::numpunct<char>>(named);
  CHECK(x.name() == "*");
  CHECK(x != c && x != named);
  CHECK(&std::use_facet<std::numpunct<char>>(x) == &std::use_facet<std::numpunct<char>>(named));
  CHECK(std::use_facet<std::numpunct<char>>(x).decimal_point() == ',');
  CHECK(&std::use_facet<std::numpunct<wchar_t>>(x) == &std::use_facet<std::numpunct<wchar_t>>(c));
  CHECK(&std::use_facet<std::moneypunct<char>>(x) == &std::use_facet<std::moneypunct<char>>(c));
  CHECK(&std::use_facet<std::ctype<char>>(x) == &std::use_facet<std::ctype<char>>(c));

  // combining a facet *this already has still gives an unnamed locale
  std::locale y = named.combine<std::ctype<char>>(named);
  CHECK(y.name() == "*");

  // other lacks the facet
  try {
    (void)named.combine<mine>(c);
    CHECK(false);
  } catch (const std::runtime_error&) {
  }
  // a user facet moves across
  std::locale with(c, new mine(7));
  std::locale z = named.combine<mine>(with);
  CHECK(std::has_facet<mine>(z));
  CHECK(std::use_facet<mine>(z).v == 7);
  CHECK(!std::has_facet<mine>(named)); // *this is unchanged
  CHECK(std::use_facet<std::numpunct<char>>(z).decimal_point() == ',');
}
