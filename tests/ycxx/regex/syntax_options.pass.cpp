// [re.synopt], [re.regex]: icase matches case-insensitively ([re.grammar]/14.1; ranges:
// 14.2); nosubs suppresses marked sub-expressions (mark_count() is 0 and only [0] is stored);
// multiline makes ^ and $ match at line boundaries (ECMAScript); flags() returns the options;
// basic, extended, awk, grep and egrep select the POSIX grammars (e.g. basic uses \( \) and
// \{ \}, grep and egrep treat a newline as alternation). The default is ECMAScript. assign()
// and operator= replace the expression.
// COUNTERPART: libstdcxx:28_regex/algorithms/regex_match/multiline.cc
#include <regex>
#include <string>
#include "check.hpp"

namespace rc = std::regex_constants;

int main() {
  std::regex ic("hello", rc::icase);
  CHECK(std::regex_match("HeLLo", ic) && !std::regex_match("HeLLo", std::regex("hello")));
  // [re.grammar]/14.2: a range c1-c2 is compared case-insensitively only through the collate
  // algorithm (translate_nocase(c1/c2/c), then transform); without collate the rule is plain
  // "c1 <= c && c <= c2", so icase alone does not fold range endpoints. With icase | collate,
  // regex_traits<char>::translate_nocase is ctype::tolower ([re.traits]/5) and the "C" locale's
  // transform preserves the character order ([locale.collate.virtuals]: transform orders as
  // do_compare, a lexicographical comparison), so "ABC" is in [a-c].
  CHECK(std::regex_match("ABC", std::regex("[a-c]+", rc::icase | rc::collate)));
  CHECK(std::regex_match("abc", std::regex("[A-C]+", rc::icase | rc::collate)));
  CHECK((ic.flags() & rc::icase) == rc::icase);
  CHECK((std::regex("x").flags() & rc::ECMAScript) == rc::ECMAScript);

  std::regex ns("(a)(b)", rc::nosubs);
  CHECK(ns.mark_count() == 0);
  std::cmatch m;
  CHECK(std::regex_match("ab", m, ns) && m.size() == 1 && m.str() == "ab");

  std::regex ml("^b$", rc::ECMAScript | rc::multiline);
  CHECK(std::regex_search("a\nb\nc", m, ml) && m.position() == 2);
  CHECK(!std::regex_search("a\nb\nc", std::regex("^b$")));

  CHECK(std::regex_match("aa", std::regex("\\(a\\)\\1", rc::basic)));
  CHECK(std::regex_match("aaa", std::regex("a\\{3\\}", rc::basic)));
  CHECK(std::regex_match("a{3}", std::regex("a{3}", rc::basic)));  // literal braces in BRE
  CHECK(std::regex_match("aaa", std::regex("a{3}", rc::extended)));
  CHECK(std::regex_match("ab", std::regex("(a|b)+", rc::extended)));
  CHECK(std::regex_match("b", std::regex("a+|b", rc::awk)));
  CHECK(std::regex_match("dog", std::regex("cat\ndog", rc::grep)));
  CHECK(std::regex_match("dog", std::regex("cat\ndog", rc::egrep)));
  CHECK(std::regex_match("cat", std::regex("cat\ndo+g", rc::egrep)));
  // POSIX: leftmost-longest.
  CHECK(std::regex_search("xab", m, std::regex("a|ab", rc::extended)) && m.str() == "ab");

  std::regex r;
  CHECK(r.mark_count() == 0);
  r.assign("(x)(y)");
  CHECK(r.mark_count() == 2 && std::regex_match("xy", r));
  r = "z+";
  CHECK(std::regex_match("zzz", r));
  r.assign("Q", rc::icase);
  CHECK(std::regex_match("q", r));
  std::regex copy = r;
  CHECK(std::regex_match("q", copy));
  return 0;
}
