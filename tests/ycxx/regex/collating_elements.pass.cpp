// Collating elements and primary equivalence classes.
// [re.traits]/7: transform_primary returns the primary sort key if the collate facet is exactly a
//   collate_byname whose key form is known, "otherwise returns an empty string". libycxx's
//   deliberate divergence (DECISIONS §3, STATUS): the classic locale's collate<charT> also gives
//   its whole key (code point order, each character its own class), so [[=a=]] is valid in the
//   default locale, as with libc++ and libstdc++.
// [re.traits]/8: lookup_collatename returns the characters of the collating element named by
//   [first, last), or an empty string if that is not a valid collating element.
// [re.grammar]/8: [.name.] is invalid if lookup_collatename returns an empty string; /10: [=name=]
//   is invalid if lookup_collatename or transform_primary of its result returns an empty
//   string; /11: an invalid class name throws regex_error (error_collate, [re.err]).
// [re.grammar]/14.2: with collate, a range compares sort keys; /14.3: [=x=] compares primary keys.
// XBD 9.3.5: a bracket expression matches a collating element, which may be a multi-character
//   one written as a collating symbol ("[[.ch.]]" where the locale defines "ch").
// The named locale: cs_CZ.ISO8859-2 (glibc defines the collating elements "ch", "Ch" and "CH";
// "ch" collates after "h", before "i"); the wide part uses the same locale's wide facets.
// REQUIRES: exceptions
// COUNTERPART: libstdcxx:28_regex/traits/(char|wchar_t)/transform_primary\.cc
#include <locale>
#include <regex>
#include <string>
#include "check.hpp"
#include "named_locale.hpp"

namespace rc = std::regex_constants;

namespace {

template <class R, class S>
bool throws_code(const S& pat, rc::syntax_option_type f, rc::error_type code) {
  try {
    R r(pat, f);
  } catch (const std::regex_error& e) {
    return e.code() == code;
  }
  return false;
}

template <class R, class S>
R imbued(const std::locale& loc, const S& pat, rc::syntax_option_type f) {
  R r;
  r.imbue(loc);
  r.assign(pat, f);
  return r;
}

} // namespace

int main() {
  // The classic locale: each character its own primary class, no multi-character elements.
  {
    std::regex_traits<char> t;
    const std::string a = "a", A = "A", ab = "ab", ch = "ch", period = "period";
    CHECK(!t.transform_primary(a.begin(), a.end()).empty());
    CHECK(t.transform_primary(a.begin(), a.end()) == t.transform(a.begin(), a.end()));
    CHECK(t.transform_primary(a.begin(), a.end()) != t.transform_primary(A.begin(), A.end()));
    CHECK(t.transform_primary(ab.begin(), ab.end()) == t.transform(ab.begin(), ab.end()));
    CHECK(t.lookup_collatename(a.begin(), a.end()) == "a");
    CHECK(t.lookup_collatename(ch.begin(), ch.end()).empty());
    CHECK(t.lookup_collatename(period.begin(), period.end()) == ".");
    std::regex_traits<wchar_t> w;
    const std::wstring wa = L"a", wch = L"ch", whyphen = L"hyphen";
    CHECK(w.transform_primary(wa.begin(), wa.end()) == w.transform(wa.begin(), wa.end()));
    CHECK(w.lookup_collatename(wch.begin(), wch.end()).empty());
    CHECK(w.lookup_collatename(whyphen.begin(), whyphen.end()) == L"-");
    // [=a=] is the class of 'a' alone; [.ch.] is invalid ([re.grammar]/8), in every grammar.
    for (auto g : {rc::ECMAScript, rc::basic, rc::extended, rc::awk, rc::grep, rc::egrep}) {
      CHECK(std::regex_match("a", std::regex("[[=a=]]", g)));
      CHECK(!std::regex_match("A", std::regex("[[=a=]]", g)));
      CHECK(std::regex_match(L"m", std::wregex(L"[a[=m=]z]", g)));
      CHECK(!std::regex_match(L"M", std::wregex(L"[a[=m=]z]", g)));
      CHECK(throws_code<std::regex>("[[=ch=]]", g, rc::error_collate));
      CHECK(throws_code<std::regex>("[[.ch.]]", g, rc::error_collate));
    }
    CHECK(std::regex_match("-", std::regex("[[.hyphen.]]", rc::basic)));
  }

  const std::locale cz(require_locale("cs_CZ.ISO8859-2"));
  // A collate_byname: primary keys, the element "ch".
  {
    std::regex_traits<char> t;
    t.imbue(cz);
    const std::string a = "a", A = "A", aacute = "\xE1", b = "b", ch = "ch", CH = "CH", cx = "cx";
    CHECK(!t.transform_primary(a.begin(), a.end()).empty());
    CHECK(t.transform_primary(a.begin(), a.end()) == t.transform_primary(A.begin(), A.end()));
    CHECK(t.transform_primary(a.begin(), a.end()) == t.transform_primary(aacute.begin(), aacute.end()));
    CHECK(t.transform_primary(a.begin(), a.end()) != t.transform_primary(b.begin(), b.end()));
    CHECK(t.lookup_collatename(ch.begin(), ch.end()) == "ch");
    CHECK(t.lookup_collatename(CH.begin(), CH.end()) == "CH");
    CHECK(t.lookup_collatename(cx.begin(), cx.end()).empty());
    std::regex_traits<wchar_t> w;
    w.imbue(cz);
    const std::wstring wch = L"ch", wcx = L"cx";
    CHECK(w.lookup_collatename(wch.begin(), wch.end()) == L"ch");
    CHECK(w.lookup_collatename(wcx.begin(), wcx.end()).empty());
  }
  for (auto g : {rc::ECMAScript, rc::basic, rc::extended, rc::awk, rc::grep, rc::egrep}) {
    // A matching list matches the element as one collating element.
    const std::regex r = imbued<std::regex>(cz, std::string("[[.ch.]a]"), g);
    CHECK(std::regex_match("ch", r));
    CHECK(std::regex_match("a", r));
    CHECK(!std::regex_match("c", r));
    CHECK(!std::regex_match("h", r));
    CHECK(!std::regex_match("cha", r));
    const std::regex rep = imbued<std::regex>(cz, std::string("x[[.ch.]]*y"), g);
    CHECK(std::regex_match("xchchy", rep));
    CHECK(std::regex_match("xy", rep));
    CHECK(!std::regex_match("xchcy", rep));
    // A non-matching list does not match where a listed element begins.
    const std::regex neg = imbued<std::regex>(cz, std::string("[^[.ch.]a]h*"), g);
    CHECK(std::regex_match("c", neg));
    CHECK(std::regex_match("xhh", neg));
    CHECK(!std::regex_match("chh", neg));
    CHECK(!std::regex_match("a", neg));
    // [=a=]: the primary class (a, A, á, ...).
    const std::regex eq = imbued<std::regex>(cz, std::string("[[=a=]]*"), g);
    CHECK(std::regex_match("aA\xE1\xC1", eq));
    CHECK(!std::regex_match("b", eq));
    // [=ch=]: the element itself.
    CHECK(std::regex_match("ch", imbued<std::regex>(cz, std::string("[[=ch=]]"), g)));
    // A multi-character range end needs collate (it has a sort key, no code point).
    try {
      (void)imbued<std::regex>(cz, std::string("[[.ch.]-i]"), g);
      CHECK(false);
    } catch (const std::regex_error& e) {
      CHECK(e.code() == rc::error_range);
    }
    const std::regex range = imbued<std::regex>(cz, std::string("[[.ch.]-i]"), g | rc::collate);
    CHECK(std::regex_match("i", range));
    CHECK(!std::regex_match("h", range));
    CHECK(!std::regex_match("c", range));
  }
  // Leftmost-longest with the element (POSIX): [[.ch.]c] at "ch" takes both characters.
  {
    std::smatch m;
    const std::string s = "xch";
    CHECK(std::regex_search(s, m, imbued<std::regex>(cz, std::string("[[.ch.]c]"), rc::extended)));
    CHECK(m.position(0) == 1 && m.length(0) == 2);
    CHECK(std::regex_search(s, m, imbued<std::regex>(cz, std::string("([^[.ch.]x])h*"), rc::extended)));
    CHECK(m.position(0) == 2 && m.length(0) == 1 && m.str(1) == "h"); // not at "ch"'s 'c'
  }
  // Wide.
  {
    const std::wregex r = imbued<std::wregex>(cz, std::wstring(L"[[.ch.]a]+"), rc::extended);
    CHECK(std::regex_match(L"chach", r));
    CHECK(!std::regex_match(L"chc", r));
  }
  return 0;
}
