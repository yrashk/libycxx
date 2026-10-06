// [locale.facet]/3: with refs == 0 the implementation deletes a facet when the last locale
// containing it is destroyed; with refs == 1 it never destroys it. /4-5: the _byname facets pass
// refs to their base class, and their string constructor has the same effect as the const char*
// one. Checked with program-defined classes derived from the _byname facets of a named locale
// (their destructors record the deletion); the facets keep the named locale's semantics.
#include <locale>
#include <string>
#include "check.hpp"
#include "named_locale.hpp"

static int destroyed = 0;

template <class F>
struct counted : F {
  template <class N>
  counted(N name, std::size_t refs) : F(name, refs) {}
  ~counted() override { ++destroyed; }
};

using ctype_w = counted<std::ctype_byname<wchar_t>>;
using numpunct_c = counted<std::numpunct_byname<char>>;
using collate_c = counted<std::collate_byname<char>>;
using money_c = counted<std::moneypunct_byname<char, false>>;

int main() {
  const char* de = require_locale("de_DE.UTF-8");

  // refs == 0: deleted with the last locale holding it
  {
    std::locale a(std::locale::classic(), new ctype_w(de, 0));
    std::locale b = a;
    CHECK(std::use_facet<std::ctype<wchar_t>>(b).toupper(L'\u00e4') == L'\u00c4');
    {
      std::locale c(b, new numpunct_c(std::string(de), 0));
      CHECK(std::use_facet<std::numpunct<char>>(c).decimal_point() == ',');
    }
    CHECK(destroyed == 1); // c was the only holder of the numpunct
    a = std::locale::classic();
    CHECK(destroyed == 1); // b still holds the ctype
  }
  CHECK(destroyed == 2);

  // a facet replaced in a locale's copy stays alive while the original holds it
  {
    std::locale a(std::locale::classic(), new collate_c(de, 0));
    std::locale b(a, new collate_c(std::string(de), 0));
    CHECK(destroyed == 2);
    a = b;
    CHECK(destroyed == 3);
    const std::string x = "\xc3\xa4pfel", y = "birne"; // "äpfel" sorts before "birne" in German
    CHECK(b(x, y));
  }
  CHECK(destroyed == 4);

  // refs == 1: never destroyed by the implementation
  {
    static money_c keep(de, 1);
    {
      std::locale a(std::locale::classic(), &keep);
      std::locale b(a);
      CHECK(std::use_facet<std::moneypunct<char>>(b).decimal_point() == ',');
    }
    CHECK(destroyed == 4);
    std::locale again(std::locale::classic(), &keep); // still usable
    CHECK(&std::use_facet<std::moneypunct<char>>(again) == &keep);
  }
  CHECK(destroyed == 4);
}
