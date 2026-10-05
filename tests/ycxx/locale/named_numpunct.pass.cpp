// [locale.numpunct.byname], [facet.numpunct.virtuals]: numpunct of a named locale has that
// locale's radix character, digit group separator and grouping; the C library's localeconv() in
// the same locale is the reference. A separator that is not one char in the locale's encoding
// (fr_FR.UTF-8's U+202F) cannot be a char: the narrow facet uses ' ' for a space character
// (DECISIONS §7), the wide facet has the character itself. truename()/falsename() are "true" and
// "false" ([facet.numpunct.virtuals]/5: no C library equivalent). num_put / num_get use them
// ([facet.num.put.virtuals] stage 2, [facet.num.get.virtuals] stage 2).
#include <locale.h>
#include <locale>
#include <sstream>
#include <string>
#include <wchar.h>
#include "check.hpp"
#include "named_locale.hpp"

struct conv {
  std::string point, sep, grouping;
};

static conv c_numeric(const char* name) {
  return in_c_locale(name, [] {
    const lconv* lc = localeconv();
    return conv{lc->decimal_point, lc->thousands_sep, lc->grouping};
  });
}

static wchar_t wide_first(const char* name, const std::string& s) {
  return in_c_locale(name, [&] {
    wchar_t w = 0;
    mbstate_t st{};
    mbrtowc(&w, s.data(), s.size(), &st);
    return w;
  });
}

static void check(const char* name) {
  const std::locale l(name);
  const conv c = c_numeric(name);
  const auto& np = std::use_facet<std::numpunct<char>>(l);
  const auto& wnp = std::use_facet<std::numpunct<wchar_t>>(l);
  CHECK(c.point.size() == 1 && np.decimal_point() == c.point[0]);
  CHECK(wnp.decimal_point() == wide_first(name, c.point));
  if (c.sep.empty()) {
    CHECK(np.grouping().empty() && wnp.grouping().empty());
  } else {
    CHECK(np.grouping() == c.grouping && wnp.grouping() == c.grouping);
    CHECK(wnp.thousands_sep() == wide_first(name, c.sep));
    if (c.sep.size() == 1)
      CHECK(np.thousands_sep() == c.sep[0]);
    else
      CHECK(np.thousands_sep() == ' '); // the separators the tested locales use are spaces
  }
  CHECK(np.truename() == "true" && np.falsename() == "false" && wnp.truename() == L"true");

  // num_put and num_get with the locale's punctuation
  std::ostringstream os;
  os.imbue(l);
  os << 1234567 << '\n' << std::fixed << 2.5; // not ' ': a separator in fr_FR
  std::string expect = "1234567";
  if (!c.sep.empty() && c.grouping.size() >= 1 && c.grouping[0] == 3)
    expect = std::string("1") + np.thousands_sep() + "234" + np.thousands_sep() + "567";
  CHECK(os.str() == expect + "\n2" + np.decimal_point() + "500000");
  std::istringstream is(os.str());
  is.imbue(l);
  long v = 0;
  double d = 0;
  is >> v >> d;
  CHECK(!is.fail() && v == 1234567 && d == 2.5);
  std::wostringstream wos;
  wos.imbue(l);
  wos << 1234567;
  if (!c.sep.empty() && c.grouping.size() >= 1 && c.grouping[0] == 3)
    CHECK(wos.str() == std::wstring(L"1") + wnp.thousands_sep() + L"234" + wnp.thousands_sep() + L"567");
}

int main() {
  check(require_locale("de_DE.UTF-8"));
  check(require_locale("en_US.UTF-8"));
  check(require_locale("fr_FR.UTF-8"));
  check(require_locale("de_DE.ISO8859-1"));
  // the byname facet built from the name equals the locale's
  std::locale h(std::locale::classic(), new std::numpunct_byname<char>("de_DE.UTF-8"));
  CHECK(std::use_facet<std::numpunct<char>>(h).decimal_point() ==
        std::use_facet<std::numpunct<char>>(std::locale("de_DE.UTF-8")).decimal_point());
}
