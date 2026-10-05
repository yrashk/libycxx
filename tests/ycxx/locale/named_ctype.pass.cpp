// [locale.ctype.byname], [facet.ctype.special]: the ctype facets of a named locale classify and
// map characters as that locale does. The C library's LC_CTYPE of the same name is the
// reference: is*/toupper/tolower for char, isw*/towupper/towlower for wchar_t, btowc/wctob for
// widen/narrow ([locale.ctype.virtuals]/9-12: the simplest reasonable transformation).
#include <ctype.h>
#include <locale>
#include <wchar.h>
#include <wctype.h>
#include "check.hpp"
#include "named_locale.hpp"

using B = std::ctype_base;

static void check_char(const char* name) {
  const std::locale l(name);
  const auto& ct = std::use_facet<std::ctype<char>>(l);
  in_c_locale(name, [&] {
    for (int i = 0; i < 256; ++i) {
      const char c = static_cast<char>(i);
      CHECK(ct.is(B::alpha, c) == (isalpha(i) != 0));
      CHECK(ct.is(B::upper, c) == (isupper(i) != 0));
      CHECK(ct.is(B::lower, c) == (islower(i) != 0));
      CHECK(ct.is(B::digit, c) == (isdigit(i) != 0));
      CHECK(ct.is(B::space, c) == (isspace(i) != 0));
      CHECK(ct.is(B::punct, c) == (ispunct(i) != 0));
      CHECK(ct.is(B::print, c) == (isprint(i) != 0));
      CHECK(ct.is(B::cntrl, c) == (iscntrl(i) != 0));
      CHECK(ct.is(B::xdigit, c) == (isxdigit(i) != 0));
      CHECK(ct.is(B::blank, c) == (isblank(i) != 0));
      CHECK(static_cast<unsigned char>(ct.toupper(c)) == toupper(i));
      CHECK(static_cast<unsigned char>(ct.tolower(c)) == tolower(i));
      CHECK(ct.table()[i] == ct.table()[static_cast<unsigned char>(c)]);
    }
    return 0;
  });
  // the array forms agree with the single ones
  char s[] = "aB\xe9\xc9z";
  char u[sizeof s];
  __builtin_memcpy(u, s, sizeof s);
  ct.toupper(u, u + sizeof s - 1);
  for (unsigned i = 0; i + 1 < sizeof s; ++i)
    CHECK(u[i] == ct.toupper(s[i]));
}

static void check_wchar(const char* name, const wchar_t* sample) {
  const std::locale l(name);
  const auto& ct = std::use_facet<std::ctype<wchar_t>>(l);
  in_c_locale(name, [&] {
    for (const wchar_t* p = sample; *p; ++p) {
      const wint_t w = static_cast<wint_t>(*p);
      CHECK(ct.is(B::alpha, *p) == (iswalpha(w) != 0));
      CHECK(ct.is(B::upper, *p) == (iswupper(w) != 0));
      CHECK(ct.is(B::lower, *p) == (iswlower(w) != 0));
      CHECK(ct.is(B::space, *p) == (iswspace(w) != 0));
      CHECK(ct.is(B::punct, *p) == (iswpunct(w) != 0));
      CHECK(ct.is(B::print, *p) == (iswprint(w) != 0));
      CHECK(ct.toupper(*p) == static_cast<wchar_t>(towupper(w)));
      CHECK(ct.tolower(*p) == static_cast<wchar_t>(towlower(w)));
      B::mask m;
      ct.is(p, p + 1, &m);
      CHECK(((m & B::alpha) != 0) == (iswalpha(w) != 0));
      CHECK(((m & B::digit) != 0) == (iswdigit(w) != 0));
      const int b = wctob(w);
      CHECK(ct.narrow(*p, '?') == (b == EOF ? '?' : static_cast<char>(b)));
    }
    for (int i = 0; i < 256; ++i) {
      const wint_t w = btowc(i);
      CHECK(ct.widen(static_cast<char>(i)) == static_cast<wchar_t>(w));
      if (w != WEOF)
        CHECK(ct.narrow(static_cast<wchar_t>(w), '?') == static_cast<char>(i));
    }
    return 0;
  });
}

int main() {
  const wchar_t sample[] = L"aZ09 \t.,éÉßжЖ中€  　!";
  check_char(require_locale("de_DE.ISO8859-1"));
  check_char(require_locale("de_DE.UTF-8"));
  check_wchar("de_DE.ISO8859-1", sample);
  check_wchar("de_DE.UTF-8", sample);
  check_wchar(require_locale("fr_FR.ISO8859-15"), sample);

  // the facet differs from the classic one where the locale does: e acute in Latin-1
  const std::locale l1("de_DE.ISO8859-1");
  const auto& latin1 = std::use_facet<std::ctype<char>>(l1);
  CHECK(latin1.is(B::alpha, '\xe9') && latin1.toupper('\xe9') == '\xc9');
  CHECK(!std::use_facet<std::ctype<char>>(std::locale::classic()).is(B::alpha, '\xe9'));
}
