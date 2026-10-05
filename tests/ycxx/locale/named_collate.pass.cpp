// [locale.collate.byname], [locale.collate.virtuals]: collate of a named locale orders strings
// as the locale does (strcoll / wcscoll in that locale are the reference); transform() gives
// keys whose lexicographic order is compare()'s (/3: "compare equal ... if and only if"), and
// equal keys hash equal (/4: hash of equal strings ... compare() returns 0). Strings may hold
// null characters ([locale.collate.members] takes ranges). locale::operator() uses the facet
// ([locale.operators]/3).
#include <string.h>
#include <wchar.h>
#include <algorithm>
#include <locale>
#include <string>
#include <vector>
#include "check.hpp"
#include "named_locale.hpp"

static int sign(int v) { return v < 0 ? -1 : v > 0 ? 1 : 0; }

static void check(const char* name) {
  const std::locale l(name);
  const auto& co = std::use_facet<std::collate<char>>(l);
  const auto& wco = std::use_facet<std::collate<wchar_t>>(l);
  const std::vector<std::string> words = {"a", "B", "b", "A", "\xc3\xa4", "z", "Zebra", "apple", "\xc3\xa9t\xc3\xa9",
                                          "ete", "", "10", "9", "co-op", "coop"};
  for (const auto& a : words)
    for (const auto& b : words) {
      const int want = in_c_locale(name, [&] { return sign(strcoll(a.c_str(), b.c_str())); });
      CHECK(co.compare(a.data(), a.data() + a.size(), b.data(), b.data() + b.size()) == want);
      const std::string ka = co.transform(a.data(), a.data() + a.size()),
                        kb = co.transform(b.data(), b.data() + b.size());
      CHECK(sign(ka.compare(kb)) == want);
      if (want == 0)
        CHECK(co.hash(a.data(), a.data() + a.size()) == co.hash(b.data(), b.data() + b.size()));
      CHECK(l(a, b) == (want < 0));
      // wide, through the C library's conversion in the same locale
      const std::wstring wa = in_c_locale(name, [&] {
        wchar_t buf[64];
        const char* p = a.c_str();
        mbstate_t st{};
        return std::wstring(buf, mbsrtowcs(buf, &p, 64, &st));
      });
      const std::wstring wb = in_c_locale(name, [&] {
        wchar_t buf[64];
        const char* p = b.c_str();
        mbstate_t st{};
        return std::wstring(buf, mbsrtowcs(buf, &p, 64, &st));
      });
      const int wwant = in_c_locale(name, [&] { return sign(wcscoll(wa.c_str(), wb.c_str())); });
      CHECK(wco.compare(wa.data(), wa.data() + wa.size(), wb.data(), wb.data() + wb.size()) == wwant);
      CHECK(sign(wco.transform(wa.data(), wa.data() + wa.size()).compare(wco.transform(wb.data(), wb.data() + wb.size()))) ==
            wwant);
    }
  // embedded null characters: the runs compare in turn, a longer string after its prefix
  const std::string x("ab\0c", 4), y("ab\0d", 4), z("ab", 2);
  CHECK(co.compare(x.data(), x.data() + 4, y.data(), y.data() + 4) < 0);
  CHECK(co.compare(z.data(), z.data() + 2, x.data(), x.data() + 4) < 0);
  CHECK(co.compare(x.data(), x.data() + 4, x.data(), x.data() + 4) == 0);
  CHECK(co.transform(x.data(), x.data() + 4) < co.transform(y.data(), y.data() + 4));
}

int main() {
  check(require_locale("de_DE.UTF-8"));
  check(require_locale("en_US.UTF-8"));
}
