// [cwctype.syn]: std::wint_t, wctrans_t, wctype_t, the isw* classification functions,
// iswctype/wctype, towlower/towupper, towctrans/wctrans and WEOF, with the meaning of ISO C 7.32.
// 7.32.2.1: in the "C" locale the wide classifications of the basic character set agree with
// the <cctype> ones of the corresponding char ("for which the corresponding isxxx returns true"
// in the C locale for single-byte characters); 7.32.2.2.2 wctype("alpha") etc. give the
// descriptors of the named classes, with iswctype(wc, wctype("alpha")) == iswalpha(wc);
// wctype of an unknown name is 0; 7.32.3.2 wctrans("tolower"/"toupper"), towctrans; WEOF is
// in no class and maps to itself. Under "C.UTF-8" the descriptors agree with the named
// functions for characters beyond ASCII too, and iswdigit holds only for '0'-'9' (7.32.2.1.5).
#include <cwctype>
#include <cctype>
#include <clocale>
#include <cwchar>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::towupper(std::wint_t{})), std::wint_t>);
static_assert(std::is_same_v<decltype(std::wctype("alpha")), std::wctype_t>);
static_assert(std::is_same_v<decltype(std::wctrans("toupper")), std::wctrans_t>);

static bool b(int x) { return x != 0; }

int main() {
  const char* names[] = {"alnum", "alpha", "blank", "cntrl", "digit", "graph",
                         "lower", "print", "punct", "space", "upper", "xdigit"};
  int (*wide[])(std::wint_t) = {std::iswalnum, std::iswalpha, std::iswblank, std::iswcntrl,
                                std::iswdigit, std::iswgraph, std::iswlower, std::iswprint,
                                std::iswpunct, std::iswspace, std::iswupper, std::iswxdigit};
  int (*narrow[])(int) = {std::isalnum, std::isalpha, std::isblank, std::iscntrl, std::isdigit, std::isgraph,
                          std::islower, std::isprint, std::ispunct, std::isspace, std::isupper, std::isxdigit};
  for (int k = 0; k < 12; ++k) {
    std::wctype_t d = std::wctype(names[k]);
    CHECK(d != 0);
    for (int c = 0; c < 128; ++c) {
      const auto wc = static_cast<std::wint_t>(c);
      CHECK(b(wide[k](wc)) == b(narrow[k](c)));
      CHECK(b(std::iswctype(wc, d)) == b(wide[k](wc)));
    }
    CHECK(!wide[k](WEOF));
  }
  CHECK(std::wctype("no-such-class") == 0);
  const std::wctrans_t up = std::wctrans("toupper"), lo = std::wctrans("tolower");
  CHECK(up != 0 && lo != 0 && std::wctrans("sideways") == 0);
  for (int c = 0; c < 128; ++c) {
    const auto wc = static_cast<std::wint_t>(c);
    CHECK(std::towupper(wc) == static_cast<std::wint_t>(std::toupper(c)));
    CHECK(std::towlower(wc) == static_cast<std::wint_t>(std::tolower(c)));
    CHECK(std::towctrans(wc, up) == std::towupper(wc) && std::towctrans(wc, lo) == std::towlower(wc));
  }
  CHECK(std::towupper(WEOF) == WEOF && std::towlower(WEOF) == WEOF);
  if (std::setlocale(LC_ALL, "C.UTF-8") || std::setlocale(LC_ALL, "C.utf8")) {
    // the descriptors and the named functions agree for every character
    for (std::wint_t wc = 0; wc < 0x3100; ++wc) {
      for (int k = 0; k < 12; ++k) CHECK(b(std::iswctype(wc, std::wctype(names[k]))) == b(wide[k](wc)));
      CHECK(std::towctrans(wc, std::wctrans("toupper")) == std::towupper(wc));
      CHECK(std::towctrans(wc, std::wctrans("tolower")) == std::towlower(wc));
      CHECK(!std::iswdigit(wc) || (wc >= L'0' && wc <= L'9'));  // 7.32.2.1.5: decimal digits only
    }
  }
}
