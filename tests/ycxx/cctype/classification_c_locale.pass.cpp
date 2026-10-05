// [cctype.syn], [cwctype.syn]: the <ctype.h>/<wctype.h> functions in namespace std, with the
// meanings of ISO/IEC 9899:2024 7.4 and 7.32 in the "C" locale (the locale at program start):
// isdigit 0-9; isxdigit 0-9 A-F a-f; isupper A-Z; islower a-z; isalpha upper or lower;
// isalnum alpha or digit; isspace the six standard white-space characters; isblank space and
// horizontal tab; iscntrl 0-31 and 127; isprint 32-126; isgraph 33-126; ispunct graph and not
// alnum; tolower/toupper map only the letters; EOF and characters outside the basic set are in
// no class. [headers]/[support.c.headers.other]: names that C may define as macros are
// functions in C++, so the parenthesised calls below are function calls. The wide forms agree
// for the same characters and WEOF; wctype/iswctype and wctrans/towctrans.
#include <cctype>
#include <cwctype>
#include <cwchar>
#include <cstdio>
#include <cstring>
#include "check.hpp"

static bool in(const char* set, int c) { return c > 0 && c < 256 && std::strchr(set, c) != nullptr; }

int main() {
  const char* upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const char* lower = "abcdefghijklmnopqrstuvwxyz";
  const char* digit = "0123456789";
  const char* xdigit = "0123456789abcdefABCDEF";
  const char* space = " \t\n\v\f\r";
  for (int c = -1; c < 256; ++c) {  // EOF and every unsigned char value
    const bool up = in(upper, c), lo = in(lower, c), dg = in(digit, c);
    const bool alpha = up || lo, alnum = alpha || dg;
    const bool cntrl = (c >= 0 && c < 32) || c == 127, print = c >= 32 && c < 127, graph = c > 32 && c < 127;
    CHECK(((std::isupper)(c) != 0) == up && ((std::islower)(c) != 0) == lo);
    CHECK(((std::isdigit)(c) != 0) == dg && ((std::isxdigit)(c) != 0) == in(xdigit, c));
    CHECK(((std::isalpha)(c) != 0) == alpha && ((std::isalnum)(c) != 0) == alnum);
    CHECK(((std::isspace)(c) != 0) == in(space, c) && ((std::isblank)(c) != 0) == (c == ' ' || c == '\t'));
    CHECK(((std::iscntrl)(c) != 0) == cntrl && ((std::isprint)(c) != 0) == print);
    CHECK(((std::isgraph)(c) != 0) == graph && ((std::ispunct)(c) != 0) == (graph && !alnum));
    CHECK((std::toupper)(c) == (lo ? c - 'a' + 'A' : c));
    CHECK((std::tolower)(c) == (up ? c - 'A' + 'a' : c));
    if (c >= 0 && c < 128) {
      const std::wint_t w = static_cast<std::wint_t>(c);
      CHECK(((std::iswupper)(w) != 0) == up && ((std::iswlower)(w) != 0) == lo);
      CHECK(((std::iswdigit)(w) != 0) == dg && ((std::iswxdigit)(w) != 0) == in(xdigit, c));
      CHECK(((std::iswalpha)(w) != 0) == alpha && ((std::iswalnum)(w) != 0) == alnum);
      CHECK(((std::iswspace)(w) != 0) == in(space, c) && ((std::iswblank)(w) != 0) == (c == ' ' || c == '\t'));
      CHECK(((std::iswcntrl)(w) != 0) == cntrl && ((std::iswprint)(w) != 0) == print);
      CHECK(((std::iswgraph)(w) != 0) == graph && ((std::iswpunct)(w) != 0) == (graph && !alnum));
      CHECK((std::towupper)(w) == (lo ? w - L'a' + L'A' : w));
      CHECK((std::towlower)(w) == (up ? w - L'A' + L'a' : w));
      CHECK((std::iswctype(w, std::wctype("alpha")) != 0) == alpha);
      CHECK((std::iswctype(w, std::wctype("punct")) != 0) == (graph && !alnum));
      CHECK(std::towctrans(w, std::wctrans("toupper")) == (std::towupper)(w));
      CHECK(std::towctrans(w, std::wctrans("tolower")) == (std::towlower)(w));
    }
  }
  CHECK(EOF < 0);
  CHECK(!(std::iswalpha)(WEOF) && (std::towupper)(WEOF) == WEOF);
  CHECK(std::wctype("no-such-class") == 0 && std::wctrans("no-such-mapping") == 0);
  std::wctype_t t = std::wctype("digit");
  std::wctrans_t m = std::wctrans("tolower");
  CHECK(std::iswctype(L'7', t) && !std::iswctype(L'x', t) && std::towctrans(L'Q', m) == L'q');
  return 0;
}
