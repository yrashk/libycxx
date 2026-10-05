// [re.req]: basic_regex<charT, traits> works with any class meeting the regular expression traits
// requirements; [re.grammar]/2: basic_regex stores a default-constructed traits_inst and calls
// its members instead of locale-dependent APIs:
//  - /7, /14.4: [:name:] (and \d \s \w) use lookup_classname and isctype, so a traits class can
//    add class names; an unknown name (lookup_classname returns 0) makes the constructor throw
//    regex_error (/11);
//  - /14.1.1: with icase, characters compare equal iff their translate_nocase values are equal;
//  - /14.1.2: otherwise, with collate, iff their translate values are equal; without either,
//    iff they are equal;
//  - /13: integral values (e.g. in {n,m} and \xhh) are obtained with traits_inst.value.
// [re.regex.locale]: imbue() calls traits_inst.imbue and getloc() returns traits_inst.getloc();
// after imbue the basic_regex object does not match any character sequence.
// COUNTERPART: libstdcxx:28_regex/traits/char/user_defined.cc
// REQUIRES: exceptions
#include <locale>
#include <regex>
#include <string>
#include "check.hpp"

int constructed = 0, nocase_calls = 0, translate_calls = 0, value_calls = 0, classname_calls = 0, imbue_calls = 0;

struct Traits {
  using char_type = char;
  using string_type = std::string;
  using locale_type = std::locale;
  using char_class_type = unsigned;  // an integer type is a bitmask type ([bitmask.types])
  enum : unsigned { digit = 1, alpha = 2, space = 4, under = 8, vowel = 16, lower = 32, upper = 64 };

  std::locale loc;
  Traits() { ++constructed; }

  static std::size_t length(const char* p) { return std::char_traits<char>::length(p); }
  char translate(char c) const {
    ++translate_calls;
    return c == '-' ? '_' : c;  // '-' and '_' are equivalent under collate
  }
  char translate_nocase(char c) const {
    ++nocase_calls;
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return c == '0' ? 'o' : c;  // '0' and 'o' are equivalent under icase
  }
  template <class It>
  std::string transform(It f, It l) const { return std::string(f, l); }
  template <class It>
  std::string transform_primary(It f, It l) const {
    std::string s(f, l);
    for (char& c : s) c = translate_nocase(c);
    return s;
  }
  template <class It>
  std::string lookup_collatename(It f, It l) const {
    std::string s(f, l);
    return s.size() == 1 ? s : std::string();
  }
  template <class It>
  unsigned lookup_classname(It f, It l, bool icase = false) const {
    ++classname_calls;
    std::string s;
    for (; f != l; ++f) s += static_cast<char>(*f >= 'A' && *f <= 'Z' ? *f - 'A' + 'a' : *f);
    if (s == "d" || s == "digit") return digit;
    if (s == "alpha") return alpha;
    if (s == "alnum") return alpha | digit;
    if (s == "s" || s == "space") return space;
    if (s == "w") return alpha | digit | under;
    if (s == "vowel") return vowel;
    if (s == "lower") return icase ? alpha : lower;
    if (s == "upper") return icase ? alpha : upper;
    return 0;
  }
  bool isctype(char c, unsigned cl) const {
    bool lo = c >= 'a' && c <= 'z', up = c >= 'A' && c <= 'Z';
    return ((cl & digit) && c >= '0' && c <= '9') || ((cl & alpha) && (lo || up)) ||
           ((cl & space) && (c == ' ' || c == '\t' || c == '\n')) || ((cl & under) && c == '_') ||
           ((cl & vowel) && std::string("aeiouAEIOU").find(c) != std::string::npos) || ((cl & lower) && lo) ||
           ((cl & upper) && up);
  }
  int value(char c, int radix) const {
    ++value_calls;
    int v = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
    return v < radix ? v : -1;
  }
  std::locale imbue(std::locale l) {
    ++imbue_calls;
    std::locale old = loc;
    loc = l;
    return old;
  }
  std::locale getloc() const { return loc; }
};

using R = std::basic_regex<char, Traits>;

bool full(const std::string& s, const R& r) { return std::regex_match(s, r); }

int main() {
  constructed = 0;
  R vow("[[:vowel:]]+");
  CHECK(constructed >= 1);
  CHECK(classname_calls >= 1);
  CHECK(full("aEiou", vow) && !full("ab", vow));
  CHECK(full("bc", R("[^[:VOWEL:]]+")));  // class names are case-independent
  CHECK(full("a1_", R("\\w+")) && !full("-", R("\\w")) && full("-", R("\\W")));
  CHECK(full("42", R("\\d\\d")) && full(" ", R("\\s")) && full("x", R("\\D")));
  CHECK(full("A", R("[[:lower:]]", std::regex_constants::icase)));
  CHECK(!full("A", R("[[:lower:]]")));

  bool threw = false;
  try {
    R bad("[[:nonsense:]]");
  } catch (const std::regex_error&) {
    threw = true;
  }
  CHECK(threw);

  // Literal comparison: icase uses translate_nocase.
  nocase_calls = 0;
  R icase("foo", std::regex_constants::icase);
  CHECK(full("F00", icase) && full("fOo", icase) && !full("fpo", icase));
  CHECK(nocase_calls > 0);
  CHECK(!full("f00", R("foo")));
  // collate uses translate.
  translate_calls = 0;
  R coll("a-b", std::regex_constants::collate);
  CHECK(full("a_b", coll) && full("a-b", coll));
  CHECK(translate_calls > 0);
  CHECK(!full("a_b", R("a-b")));

  // Integral values come from value().
  value_calls = 0;
  CHECK(full("aaaaaaaaaaaa", R("a{12}")) && !full("aaaaaaaaaaa", R("a{12}")));
  CHECK(value_calls > 0);
  CHECK(full("J", R("\\x4a")));

  // Locale.
  R r("x");
  imbue_calls = 0;
  std::locale prev = r.imbue(std::locale::classic());
  CHECK(imbue_calls >= 1);
  CHECK(r.getloc() == std::locale::classic());
  (void)prev;
  CHECK(!full("x", r));  // after imbue the regex matches nothing
  return 0;
}
