// [clocale.syn]: std::lconv, std::setlocale, std::localeconv, NULL and the LC_* macros, with the
// meaning of ISO C 7.11: at program startup the "C" locale is selected (7.11.1.1/4);
// setlocale(category, nullptr) queries; in the "C" locale localeconv() gives decimal_point "."
// and every other string member "", every char member CHAR_MAX (7.11.2.1/3).
// [locale.statics]/1: locale::global(loc) "If the argument has a name, does setlocale(LC_ALL,
// loc.name().c_str())"; /2 returns the previous global locale; /3 nothing else changes
// locale(). [locale.cons]: locale(const char*) names a locale the C library knows; std::locale
// itself is not changed by setlocale.
#include <clocale>
#include <climits>
#include <cstring>
#include <locale>
#include <string>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::setlocale(LC_ALL, nullptr)), char*>);
static_assert(std::is_same_v<decltype(std::localeconv()), std::lconv*>);

static bool c_lconv(const std::lconv* l) {
  return std::strcmp(l->decimal_point, ".") == 0 && *l->thousands_sep == 0 && *l->grouping == 0 &&
         *l->int_curr_symbol == 0 && *l->currency_symbol == 0 && *l->mon_decimal_point == 0 &&
         *l->mon_thousands_sep == 0 && *l->mon_grouping == 0 && *l->positive_sign == 0 && *l->negative_sign == 0 &&
         l->int_frac_digits == CHAR_MAX && l->frac_digits == CHAR_MAX && l->p_cs_precedes == CHAR_MAX &&
         l->p_sep_by_space == CHAR_MAX && l->n_cs_precedes == CHAR_MAX && l->n_sep_by_space == CHAR_MAX &&
         l->p_sign_posn == CHAR_MAX && l->n_sign_posn == CHAR_MAX && l->int_p_cs_precedes == CHAR_MAX &&
         l->int_n_cs_precedes == CHAR_MAX && l->int_p_sep_by_space == CHAR_MAX &&
         l->int_n_sep_by_space == CHAR_MAX && l->int_p_sign_posn == CHAR_MAX && l->int_n_sign_posn == CHAR_MAX;
}

int main() {
  const int cats[] = {LC_ALL, LC_COLLATE, LC_CTYPE, LC_MONETARY, LC_NUMERIC, LC_TIME};
  for (int i = 0; i < 6; ++i)
    for (int j = i + 1; j < 6; ++j) CHECK(cats[i] != cats[j]);
  for (int c : cats) CHECK(std::strcmp(std::setlocale(c, nullptr), "C") == 0);
  CHECK(c_lconv(std::localeconv()));
  CHECK(std::setlocale(LC_ALL, "no-such-locale-xyz") == nullptr);
  CHECK(std::strcmp(std::setlocale(LC_ALL, nullptr), "C") == 0);

  const char* utf8 = std::setlocale(LC_CTYPE, "C.UTF-8") ? "C.UTF-8" : std::setlocale(LC_CTYPE, "C.utf8") ? "C.utf8" : nullptr;
  if (utf8) {
    CHECK(std::strcmp(std::setlocale(LC_CTYPE, nullptr), utf8) == 0);
    CHECK(std::strcmp(std::setlocale(LC_NUMERIC, nullptr), "C") == 0);  // only LC_CTYPE changed
    CHECK(std::locale().name() == "C");  // setlocale does not change the C++ global locale
    std::setlocale(LC_ALL, "C");
    // locale::global with a named locale sets the C locale
    std::locale prev = std::locale::global(std::locale(utf8));
    CHECK(prev.name() == "C");
    CHECK(std::locale().name() == utf8);
    CHECK(std::strcmp(std::setlocale(LC_CTYPE, nullptr), utf8) == 0);
    CHECK(std::strcmp(std::setlocale(LC_NUMERIC, nullptr), utf8) == 0);
    CHECK(std::strcmp(std::localeconv()->decimal_point, ".") == 0);
    prev = std::locale::global(std::locale::classic());
    CHECK(prev.name() == utf8);
    CHECK(std::strcmp(std::setlocale(LC_ALL, nullptr), "C") == 0);
    CHECK(c_lconv(std::localeconv()));
  }
}
