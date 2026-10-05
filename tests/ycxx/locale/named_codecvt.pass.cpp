// [locale.codecvt.byname], [locale.codecvt.virtuals]: codecvt<wchar_t, char, mbstate_t> of a named
// locale converts between the locale's multibyte encoding and wchar_t as the C library does in
// that locale (wcrtomb / mbrtowc); encoding() and max_length() follow MB_CUR_MAX ([locale.codecvt.
// virtuals]/7-12); a character that does not fit is partial, an invalid sequence an error, and an
// incomplete sequence at the end of the input partial with from_next before it (/2-4).
#include <cstring>
#include <locale>
#include <stdlib.h>
#include <wchar.h>
#include "check.hpp"
#include "named_locale.hpp"

using CV = std::codecvt<wchar_t, char, std::mbstate_t>;

static void round_trip(const char* name, const wchar_t* text) {
  const std::locale l(name);
  const CV& cv = std::use_facet<CV>(l);
  const std::size_t n = wcslen(text);
  char ext[256];
  // the C library's encoding of text
  char ref[256];
  const std::size_t refn = in_c_locale(name, [&] {
    mbstate_t st{};
    const wchar_t* p = text;
    return wcsrtombs(ref, &p, sizeof ref, &st);
  });
  CHECK(refn != static_cast<std::size_t>(-1));
  std::mbstate_t st{};
  const wchar_t* fn;
  char* tn;
  CHECK(cv.out(st, text, text + n, fn, ext, ext + sizeof ext, tn) == std::codecvt_base::ok);
  CHECK(fn == text + n && static_cast<std::size_t>(tn - ext) == refn && std::memcmp(ext, ref, refn) == 0);
  CHECK(mbsinit(&st));
  wchar_t back[256];
  const char* fe;
  wchar_t* te;
  std::mbstate_t st2{};
  CHECK(cv.in(st2, ext, tn, fe, back, back + 256, te) == std::codecvt_base::ok);
  CHECK(fe == tn && static_cast<std::size_t>(te - back) == n && std::wmemcmp(back, text, n) == 0);
  std::mbstate_t st3{};
  CHECK(cv.length(st3, ext, tn, n) == tn - ext);
  std::mbstate_t st4{};
  CHECK(cv.length(st4, ext, tn, 1) == static_cast<int>(in_c_locale(name, [&] {
          mbstate_t s{};
          return mbrlen(ext, static_cast<std::size_t>(tn - ext), &s);
        })));

  const int mbmax = in_c_locale(name, [] { return static_cast<int>(MB_CUR_MAX); });
  CHECK(cv.max_length() == mbmax);
  CHECK(cv.encoding() == (mbmax == 1 ? 1 : 0));
  CHECK(!cv.always_noconv());
  std::mbstate_t st5{};
  char* un;
  CHECK(cv.unshift(st5, ext, ext + sizeof ext, un) == std::codecvt_base::noconv && un == ext);
}

int main() {
  const char* utf8 = require_locale("de_DE.UTF-8");
  const char* latin9 = require_locale("fr_FR.ISO8859-15");
  round_trip(utf8, L"Grüße € 中\U0001F600");
  round_trip(latin9, L"déjà € œ");

  // ISO-8859-15: the euro sign is the byte A4, and a character outside it is an error
  const CV& l9 = std::use_facet<CV>(std::locale(latin9));
  {
    const wchar_t in[] = L"a€b中c";
    char out[8];
    std::mbstate_t st{};
    const wchar_t* fn;
    char* tn;
    CHECK(l9.out(st, in, in + 5, fn, out, out + 8, tn) == std::codecvt_base::error);
    CHECK(fn == in + 3 && tn == out + 3 && out[1] == '\xa4');
  }
  const CV& u8 = std::use_facet<CV>(std::locale(utf8));
  {
    // no room for a three-byte character: partial, nothing of it written
    const wchar_t in[] = L"a€";
    char out[3];
    std::mbstate_t st{};
    const wchar_t* fn;
    char* tn;
    CHECK(u8.out(st, in, in + 2, fn, out, out + 3, tn) == std::codecvt_base::partial);
    CHECK(fn == in + 1 && tn == out + 1);
  }
  {
    // an incomplete sequence at the end: partial before it; an invalid one: error at it
    const char in[] = "a\xe2\x82";
    wchar_t out[4];
    std::mbstate_t st{};
    const char* fn;
    wchar_t* tn;
    CHECK(u8.in(st, in, in + 3, fn, out, out + 4, tn) == std::codecvt_base::partial);
    CHECK(fn == in + 1 && tn == out + 1 && out[0] == L'a');
    const char bad[] = "ab\xff";
    std::mbstate_t st2{};
    CHECK(u8.in(st2, bad, bad + 3, fn, out, out + 4, tn) == std::codecvt_base::error);
    CHECK(fn == bad + 2 && tn == out + 2);
    // no room in the destination: partial
    std::mbstate_t st3{};
    CHECK(u8.in(st3, bad, bad + 2, fn, out, out + 1, tn) == std::codecvt_base::partial);
    CHECK(fn == bad + 1 && tn == out + 1);
  }
}
