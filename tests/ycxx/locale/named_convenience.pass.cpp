// [classification]/1: isF(c, loc) returns use_facet<ctype<charT>>(loc).is(ctype_base::F, c);
// [conversions.character]/1-2: toupper(c, loc) / tolower(c, loc) return the facet's toupper /
// tolower. With named locales: char in an ISO-8859-1 locale (a single-byte encoding where 0xE4
// is a lowercase letter), wchar_t in a UTF-8 one; the C library in the same locale
// (isalpha_l, towupper_l, ...) is the reference for what the facet answers.
#include <ctype.h>
#include <wctype.h>
#include <locale>
#include "check.hpp"
#include "named_locale.hpp"

template <class C>
void agrees(C c, const std::locale& loc) {
  using B = std::ctype_base;
  const auto& f = std::use_facet<std::ctype<C>>(loc);
  CHECK(std::isspace(c, loc) == f.is(B::space, c));
  CHECK(std::isprint(c, loc) == f.is(B::print, c));
  CHECK(std::iscntrl(c, loc) == f.is(B::cntrl, c));
  CHECK(std::isupper(c, loc) == f.is(B::upper, c));
  CHECK(std::islower(c, loc) == f.is(B::lower, c));
  CHECK(std::isalpha(c, loc) == f.is(B::alpha, c));
  CHECK(std::isdigit(c, loc) == f.is(B::digit, c));
  CHECK(std::ispunct(c, loc) == f.is(B::punct, c));
  CHECK(std::isxdigit(c, loc) == f.is(B::xdigit, c));
  CHECK(std::isalnum(c, loc) == f.is(B::alnum, c));
  CHECK(std::isgraph(c, loc) == f.is(B::graph, c));
  CHECK(std::isblank(c, loc) == f.is(B::blank, c));
  CHECK(std::toupper(c, loc) == f.toupper(c));
  CHECK(std::tolower(c, loc) == f.tolower(c));
}

int main() {
  const char* latin1 = require_locale("de_DE.ISO8859-1");
  const char* utf8 = require_locale("de_DE.UTF-8");
  const std::locale l1(latin1), u8(utf8);

  for (int i = 0; i < 256; ++i)
    agrees(static_cast<char>(i), l1);
  for (wchar_t w : {L'a', L'Z', L' ', L'\t', L'7', L'\u00e4', L'\u00c4', L'\u00df', L'\u03a9', L'\u20ac', L'\u3000'})
    agrees(w, u8);

  // the C library's view of the same characters
  const char ae = static_cast<char>(0xE4), AE = static_cast<char>(0xC4);
  CHECK(std::isalpha(ae, l1));
  CHECK(std::islower(ae, l1) && !std::isupper(ae, l1));
  CHECK(std::toupper(ae, l1) == AE);
  CHECK(std::tolower(AE, l1) == ae);
  const bool c_alpha = in_c_locale(latin1, [] { return isalpha(0xE4) != 0; });
  CHECK(c_alpha);

  CHECK(std::isalpha(L'\u00e4', u8));
  CHECK(std::toupper(L'\u00e4', u8) == L'\u00c4');
  CHECK(std::tolower(L'\u03a9', u8) == L'\u03c9');
  CHECK(!std::isalpha(L'\u20ac', u8));
  const bool c_space = in_c_locale(utf8, [] { return iswspace(L'\u3000') != 0; });
  CHECK(std::isspace(L'\u3000', u8) == c_space);
  // the classic locale's char facet: an unsigned char beyond ASCII is not a letter there
  CHECK(!std::isalpha(ae, std::locale::classic()));
  CHECK(std::toupper(ae, std::locale::classic()) == ae);
}
