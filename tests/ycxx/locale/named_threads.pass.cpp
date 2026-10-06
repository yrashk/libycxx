// [res.on.data.races], [locale.general] (a locale is immutable): named locales and their facets are
// constructed, copied, used and destroyed concurrently from several threads, each thread with
// its own C locale view untouched ([locale.cons] does not change the C locale). Run under
// ThreadSanitizer as well.
#include <locale.h>
#include <string.h>
#include <locale>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include "check.hpp"
#include "named_locale.hpp"

int main() {
  const char* names[] = {require_locale("de_DE.UTF-8"), require_locale("fr_FR.ISO8859-15"),
                         require_locale("en_US.UTF-8")};
  std::vector<std::thread> ts;
  for (int k = 0; k < 4; ++k)
    ts.emplace_back([&, k] {
      static const char ab[] = "ab";
      for (int i = 0; i < 40; ++i) {
        const char* n = names[(i + k) % 3];
        std::locale l(n);
        std::ostringstream os;
        os.imbue(l);
        os << 1234.5;
        const char point = std::use_facet<std::numpunct<char>>(l).decimal_point();
        CHECK(os.str().find(point) != std::string::npos);
        std::locale copy(std::locale::classic(), l, std::locale::time | std::locale::collate);
        CHECK(std::use_facet<std::collate<char>>(copy).compare(&ab[0], &ab[1], &ab[1], &ab[2]) < 0);
        std::locale h(std::locale::classic(), new std::ctype_byname<wchar_t>(n));
        CHECK(std::use_facet<std::ctype<wchar_t>>(h).is(std::ctype_base::alpha, L'é'));
        CHECK(strcmp(setlocale(LC_ALL, nullptr), "C") == 0);
      }
    });
  for (auto& t : ts)
    t.join();
}
