// regex_traits and basic_regex with named locales.
// [re.traits]/5: translate_nocase(c) is use_facet<ctype<charT>>(getloc()).tolower(c); /6:
//   transform is the locale's collate::transform; /12: isctype(c, f) asks the locale's ctype
//   (and '_' is in the class of "w"); /15-17: imbue(loc) makes getloc() == loc and returns the
//   global locale at construction (no earlier imbue) or the previous argument; /18: getloc();
//   /7: transform_primary is the primary key for a collate_byname whose key form is known (else
//   an empty string), and
//   [re.grammar]/14.3 matches [[=a=]] by it.
// [re.regex.locale]/1: basic_regex::imbue returns the traits' imbue result, and afterwards the
//   regex does not match any character sequence (until it is assigned a new pattern).
// [re.grammar]/14.1: with icase, characters match if translate_nocase gives the same result;
//   /14.2: with collate, a range c1-c2 matches c if transform(c1) <= transform(c) <=
//   transform(c2) (the C library's wcscoll in the same locale is the reference for the order).
// The locales: de_DE.UTF-8 (wide) and de_DE.ISO8859-1 (char, where 0xE4 is 'ä').
// REQUIRES: exceptions
#include <wchar.h>
#include <locale>
#include <regex>
#include <string>
#include "check.hpp"
#include "named_locale.hpp"

#if defined(__APPLE__)
constexpr bool kDarwin = true; // Darwin's key form is not documented: whole keys
#else
constexpr bool kDarwin = false;
#endif

int main() {
  const char* utf8_name = require_locale("de_DE.UTF-8");
  const std::locale utf8(utf8_name), latin1(require_locale("de_DE.ISO8859-1"));

  // traits, wide
  {
    std::regex_traits<wchar_t> t;
    const std::locale prev = t.imbue(utf8);
    CHECK(prev == std::locale::classic()); // the global locale at construction
    CHECK(t.getloc() == utf8);
    CHECK(t.imbue(utf8) == utf8);
    CHECK(t.translate_nocase(L'\u00c4') == L'\u00e4');
    const std::wstring alpha = L"alpha", w = L"w", upper = L"upper";
    const auto a = t.lookup_classname(alpha.begin(), alpha.end());
    const auto wc = t.lookup_classname(w.begin(), w.end());
    const auto up = t.lookup_classname(upper.begin(), upper.end());
    CHECK(t.isctype(L'\u00e4', a));
    CHECK(t.isctype(L'\u00df', a));
    CHECK(!t.isctype(L'\u20ac', a));
    CHECK(t.isctype(L'_', wc));
    CHECK(t.isctype(L'\u00e4', wc));
    CHECK(t.isctype(L'\u00c4', up) && !t.isctype(L'\u00e4', up));
    const std::wstring ae = L"\u00e4", b = L"b";
    CHECK(t.transform(ae.begin(), ae.end()) < t.transform(b.begin(), b.end()));
  }
  // traits, char, single-byte locale
  {
    std::regex_traits<char> t;
    t.imbue(latin1);
    const char AE = static_cast<char>(0xC4), ae = static_cast<char>(0xE4);
    CHECK(t.translate_nocase(AE) == ae);
    const std::string alpha = "alpha";
    CHECK(t.isctype(ae, t.lookup_classname(alpha.begin(), alpha.end())));
  }
  // basic_regex::imbue
  {
    std::wregex r(L"a");
    CHECK(std::regex_match(L"a", r));
    const std::locale prev = r.imbue(utf8);
    CHECK(prev == std::locale::classic());
    CHECK(r.getloc() == utf8);
    CHECK(!std::regex_match(L"a", r)); // matches nothing now
    CHECK(!std::regex_search(L"", r));
    r.assign(L"[[:alpha:]]+", std::regex_constants::icase);
    CHECK(r.getloc() == utf8); // assign keeps the imbued traits (the imbue-then-assign idiom)
    CHECK(std::regex_match(L"\u00c4\u00d6\u00dc\u00e4\u00f6\u00fc\u00df", r));
    CHECK(!std::regex_match(L"\u00e4\u20ac", r));
    r.assign(L"\u00e4+", std::regex_constants::icase);
    CHECK(std::regex_match(L"\u00c4\u00e4\u00c4", r));
    r.assign(L"\\w+");
    CHECK(std::regex_match(L"gr\u00fc\u00dfe_1", r));
  }
  // a collating range
  {
    const bool in_range = in_c_locale(utf8_name, [] { return wcscoll(L"a", L"\u00e4") <= 0 && wcscoll(L"\u00e4", L"c") <= 0; });
    std::wregex r;
    r.imbue(utf8);
    r.assign(L"[a-c]", std::regex_constants::collate);
    CHECK(std::regex_match(L"\u00e4", r) == in_range);
    CHECK(std::regex_match(L"b", r));
    CHECK(!std::regex_match(L"d", r));
    std::wregex plain(L"[a-c]"); // without collate: code point order, U+00E4 > 'c'
    CHECK(!std::regex_match(L"\u00e4", plain));
  }
  // equivalence classes: [re.traits]/7 gives the primary key when the collate_byname's key form
  // is known, which libycxx knows for the C library's multi-level keys (glibc: the levels each
  // end with the value 1) and for keys that copy the string (every character its own class);
  // otherwise an empty string, and [re.grammar]/10 makes [[=a=]] invalid. The C library's own
  // wcsxfrm keys tell which form it uses (Darwin's is not documented: unknown).
  {
    const bool levels = !kDarwin && in_c_locale(utf8_name, [] {
      wchar_t k[64];
      const size_t n = wcsxfrm(k, L"a", 64);
      return n < 64 && wmemchr(k, 1, n) != nullptr;
    });
    const bool copies = !kDarwin && in_c_locale(utf8_name, [] {
      wchar_t k[64];
      return wcsxfrm(k, L"aB", 64) == 2 && k[0] == L'a' && k[1] == L'B';
    });
    std::regex_traits<wchar_t> t;
    t.imbue(utf8);
    const std::wstring a = L"a", A = L"A", ae = L"ä", b = L"b";
    const auto pa = t.transform_primary(a.begin(), a.end());
    CHECK(pa.empty() == !(levels || copies));
    if (levels || copies) {
      CHECK((pa == t.transform_primary(ae.begin(), ae.end())) == levels);
      CHECK((pa == t.transform_primary(A.begin(), A.end())) == levels);
      CHECK(pa != t.transform_primary(b.begin(), b.end()));
      std::wregex r;
      r.imbue(utf8);
      r.assign(L"[[=a=]]+");
      CHECK(std::regex_match(L"a", r));
      CHECK(std::regex_match(L"aAä", r) == levels);
      CHECK(!std::regex_match(L"b", r));
    } else {
      std::wregex r;
      r.imbue(utf8);
      try {
        r.assign(L"[[=a=]]+");
        CHECK(false);
      } catch (const std::regex_error& e) {
        CHECK(e.code() == std::regex_constants::error_collate);
      }
    }
    // the classic locale's collate facet is not a collate_byname: an empty key
    std::regex_traits<wchar_t> c;
    CHECK(c.transform_primary(a.begin(), a.end()).empty());
  }
  // the global locale at construction
  {
    std::locale::global(utf8);
    std::wregex r(L"[[:upper:]]");
    CHECK(r.getloc() == utf8);
    CHECK(std::regex_match(L"\u00c4", r));
    std::locale::global(std::locale::classic());
    CHECK(r.getloc() == utf8); // unaffected by a later global change
  }
}
