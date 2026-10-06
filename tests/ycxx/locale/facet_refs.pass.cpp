// [locale.facet]/3: "For refs == 0, the implementation performs delete ... when the last locale
// object containing the facet is destroyed; for refs == 1, the implementation never destroys the
// facet." Other values are not specified; libycxx treats every nonzero refs as 1, so a facet
// constructed with numeric_limits<size_t>::max() is not destroyed when locales holding it come and
// go (libstdc++'s 22_locale/facet/2.cc).
#include <cstddef>
#include <limits>
#include <locale>
#include "check.hpp"

static int live = 0;

struct counted : std::locale::facet {
  static std::locale::id id;
  explicit counted(std::size_t refs) : std::locale::facet(refs) { ++live; }
  ~counted() override { --live; }
};
std::locale::id counted::id;

int main() {
  {
    std::locale l(std::locale::classic(), new counted(0));
    CHECK(live == 1);
  }
  CHECK(live == 0);

  for (std::size_t refs : {std::size_t(1), std::size_t(2), std::numeric_limits<std::size_t>::max()}) {
    counted* f = new counted(refs);
    {
      std::locale a(std::locale::classic(), f);
      {
        std::locale b(std::locale::classic(), f);
        std::locale c = b;
      }
      CHECK(live == 1);
    }
    CHECK(live == 1);
    delete f; // never destroyed by the library: the program's to delete
    CHECK(live == 0);
  }
}
