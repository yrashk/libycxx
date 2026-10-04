// [locale]: locale::classic() is the "C" locale (name() "C"); the default-constructed locale is
// a copy of the global locale, initially classic(); locale::global(loc) sets it and returns the
// previous one; locales compare equal when they are copies or have the same non-"*" name;
// has_facet/use_facet find the required facets in every locale; combine<Facet>(other) takes
// that facet from other; locale(other, f) replaces the facet; a locale is a callable comparing
// strings with collate<charT>.
#include <locale>
#include <string>
#include <type_traits>
#include "check.hpp"

struct MyNum : std::numpunct<char> {
  char do_decimal_point() const override { return ','; }
};

int main() {
  const std::locale& c = std::locale::classic();
  CHECK(c.name() == "C");
  std::locale def;
  CHECK(def == c && def.name() == "C");
  CHECK(std::locale("C") == c);
  CHECK(&std::locale::classic() == &std::locale::classic());

  CHECK(std::has_facet<std::ctype<char>>(c) && std::has_facet<std::numpunct<wchar_t>>(c));
  CHECK(std::has_facet<std::collate<char>>(c) && std::has_facet<std::num_get<char>>(c));
  CHECK(std::has_facet<std::codecvt<char, char, std::mbstate_t>>(c));
  const std::numpunct<char>& np = std::use_facet<std::numpunct<char>>(c);
  CHECK(np.decimal_point() == '.');

  // Replacing a facet: the result has no name ("*").
  std::locale mine(c, new MyNum);
  CHECK(std::use_facet<std::numpunct<char>>(mine).decimal_point() == ',');
  CHECK(mine.name() == "*" && mine != c && mine == mine);
  std::locale copy = mine;
  CHECK(copy == mine);
  std::locale back = c.combine<std::numpunct<char>>(mine);
  CHECK(std::use_facet<std::numpunct<char>>(back).decimal_point() == ',');
  std::locale cat(mine, c, std::locale::numeric);  // numeric category from c
  CHECK(std::use_facet<std::numpunct<char>>(cat).decimal_point() == '.');

  std::locale prev = std::locale::global(mine);
  CHECK(prev == c && std::locale() == mine);
  std::locale::global(c);
  CHECK(std::locale() == c);

  // operator() compares with collate.
  CHECK(c(std::string("apple"), std::string("banana")) && !c(std::string("b"), std::string("a")));
  static_assert(std::is_same_v<decltype(c(std::string(), std::string())), bool>);

  // Unknown names throw runtime_error.
  bool threw = false;
  try {
    std::locale bad("no-such-locale-name-xyz");
  } catch (const std::runtime_error&) {
    threw = true;
  }
  CHECK(threw);
  return 0;
}
