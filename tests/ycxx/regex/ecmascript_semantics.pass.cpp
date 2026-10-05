// [re.grammar]/14: matching behaves as described in ECMA-262 (RegExp pattern semantics,
// ECMA-262 22.2.2): alternatives are tried left to right (the first that leads to a match wins,
// not the longest); captures inside a quantified group are reset at each iteration; a
// backreference to a group that did not participate matches the empty string; captures made in
// a positive lookahead are kept, those in a negative lookahead are not; an iteration of * that
// matches the empty string ends the repetition; . does not match a LineTerminator (\n, \r).
// The checks with exact capture results are the examples of ECMA-262 22.2.2.3.1 (RepeatMatcher),
// 22.2.2.4 (Assertion: lookahead) and 22.2.2.7.3 (BackreferenceMatcher).
// [re.grammar]/3-7: [:name:], [.name.] in bracket expressions; the class names alnum ... xdigit,
// d, s, w are recognised; \d, \s, \w are [[:digit:]], [[:space:]], [_[:alnum:]].
// [re.grammar]/14.1.1: with icase, characters (also those matched by a backreference) compare
// equal after translate_nocase.
// [re.synopt] multiline: ^ and $ also match after/before a LineTerminator.
// COUNTERPART: libcxx:re/re.alg/re.alg.search/ecma.pass.cpp
#include <regex>
#include <string>
#include "check.hpp"

namespace rc = std::regex_constants;

bool full(const std::string& s, const char* re, rc::syntax_option_type f = rc::ECMAScript) {
  return std::regex_match(s, std::regex(re, f));
}
bool search(const std::string& s, const char* re, rc::syntax_option_type f = rc::ECMAScript) {
  return std::regex_search(s, std::regex(re, f));
}

int main() {
  std::smatch m;
  // Ordered alternation.
  std::string ab = "abc";
  CHECK(std::regex_search(ab, m, std::regex("a|ab")) && m.str() == "a");
  CHECK(std::regex_search(ab, m, std::regex("ab|a")) && m.str() == "ab");
  CHECK(full("ab", "a|ab"));  // regex_match backtracks into the second alternative

  // ECMA-262: /a[a-z]{2,4}/ and /a[a-z]{2,4}?/ on "abcdefghi".
  std::string s1 = "abcdefghi";
  CHECK(std::regex_search(s1, m, std::regex("a[a-z]{2,4}")) && m.str() == "abcde");
  CHECK(std::regex_search(s1, m, std::regex("a[a-z]{2,4}?")) && m.str() == "abc");
  // /(aa|aabaac|ba|b|c)*/ on "aabaac" -> ["aaba", "ba"]
  std::string s2 = "aabaac";
  CHECK(std::regex_search(s2, m, std::regex("(aa|aabaac|ba|b|c)*")) && m.str() == "aaba" && m.str(1) == "ba");
  // /(z)((a+)?(b+)?(c))*/ on "zaacbbbcac" -> ["zaacbbbcac", "z", "ac", "a", undefined, "c"]
  std::string s3 = "zaacbbbcac";
  CHECK(std::regex_search(s3, m, std::regex("(z)((a+)?(b+)?(c))*")));
  CHECK(m.size() == 6 && m.str(0) == "zaacbbbcac" && m.str(1) == "z" && m.str(2) == "ac");
  CHECK(m[3].matched && m.str(3) == "a");
  CHECK(!m[4].matched);  // reset at the start of the last iteration
  CHECK(m.str(5) == "c");
  // /(a*)*/ on "b" -> ["", undefined]; /(a*)b\1+/ on "baaaac" -> ["b", ""]
  std::string s4 = "b";
  CHECK(std::regex_search(s4, m, std::regex("(a*)*")) && m.str() == "" && m.position() == 0 && !m[1].matched);
  std::string s5 = "baaaac";
  CHECK(std::regex_search(s5, m, std::regex("(a*)b\\1+")) && m.str() == "b" && m[1].matched && m.str(1) == "");

  // Lookahead: /(?=(a+))/ on "baaabac" -> ["", "aaa"] at index 1;
  // /(?=(a+))a*b\1/ on "baaabac" -> ["aba", "a"].
  std::string s6 = "baaabac";
  CHECK(std::regex_search(s6, m, std::regex("(?=(a+))")) && m.str() == "" && m.position() == 1 && m.str(1) == "aaa");
  CHECK(std::regex_search(s6, m, std::regex("(?=(a+))a*b\\1")) && m.str() == "aba" && m.position() == 3 &&
        m.str(1) == "a");
  // Negative lookahead: /(.*?)a(?!(a+)b\2c)\2(.*)/ on "baaabaac" -> ["baaabaac", "ba", undefined, "abaac"]
  std::string s7 = "baaabaac";
  CHECK(std::regex_search(s7, m, std::regex("(.*?)a(?!(a+)b\\2c)\\2(.*)")));
  CHECK(m.str() == "baaabaac" && m.str(1) == "ba" && !m[2].matched && m.str(3) == "abaac");

  // A backreference to a non-participating group matches empty.
  CHECK(full("b", "(a)?b\\1"));
  CHECK(full("bc", "(?:(a)|b)\\1c"));

  // . excludes line terminators.
  CHECK(full("x", ".") && !full("\n", ".") && !full("\r", "."));
  CHECK(!search("a\nb", "a.b") && search("a\tb", "a.b"));

  // Escapes.
  CHECK(full("\f\v\r", "\\f\\v\\r"));
  CHECK(full(std::string(1, '\0'), "\\0"));
  CHECK(full("\b", "[\\b]") && !full("b", "[\\b]"));  // \b in a class is backspace
  CHECK(full("/", "\\/") && full("a-z", "a\\-z") && full("$^", "\\$\\^") && full("{}", "\\{\\}"));
  CHECK(full("\x1b", "\\x1B") && full("\x1b", "\\u001b"));

  // Character classes ([re.grammar]/7).
  CHECK(full("a_9", "[[:w:]]+") && !full("-", "[[:w:]]"));
  CHECK(full("09", "[[:d:]]+") && !full("a", "[[:d:]]"));
  CHECK(full(" \t\n", "[[:s:]]+") && !full("x", "[[:s:]]"));
  CHECK(full(" \t", "[[:blank:]]+") && !full("\n", "[[:blank:]]"));
  CHECK(full("\x01\x7f", "[[:cntrl:]]+") && !full("a", "[[:cntrl:]]"));
  CHECK(full("a ", "[[:print:]]+") && !full("\x01", "[[:print:]]"));
  CHECK(full("a!", "[[:graph:]]+") && !full(" ", "[[:graph:]]"));
  CHECK(full("x", "[^[:digit:]]") && !full("5", "[^[:digit:]]"));
  CHECK(full("a1", "[^\\W]+") && !full("-", "[^\\W]"));
  CHECK(full("a", "[\\D]") && !full("1", "[\\D]") && full("1 ", "[\\d\\s]+"));
  CHECK(full("_", "\\w") && full("-", "[^_[:alnum:]]") && full("-", "\\W"));
  // Collating elements.
  CHECK(full("a", "[[.a.]]") && !full("b", "[[.a.]]"));
  CHECK(full("b", "[[.a.]-c]") && !full("d", "[[.a.]-c]"));

  // icase applies to literals, classes ([re.grammar]/14.4 passes icase to lookup_classname) and
  // backreferences.
  CHECK(full("aA", "(a)\\1", rc::icase) && !full("aA", "(a)\\1"));
  CHECK(full("AbC", "abc", rc::icase));
  CHECK(full("A", "[[:lower:]]", rc::icase) && full("a", "[[:upper:]]", rc::icase));

  // multiline: \r is a LineTerminator too.
  std::string cr = "x\rb", nl = "a\nb";
  CHECK(std::regex_search(cr, m, std::regex("^b", rc::ECMAScript | rc::multiline)));
  CHECK(std::regex_search(nl, m, std::regex("a$", rc::ECMAScript | rc::multiline)) &&
        m.position() == 0);
  CHECK(!search("a\nb", "a$") && !search("a\nb", "^b"));
  std::string lines = "one\ntwo\nthree";
  int n = 0;
  std::regex word_line("^\\w+$", rc::ECMAScript | rc::multiline);
  for (std::sregex_iterator it(lines.begin(), lines.end(), word_line), e; it != e; ++it) ++n;
  CHECK(n == 3);

  // Non-greedy with backtracking into the rest of the pattern.
  std::string s8 = "<<a>>";
  CHECK(std::regex_search(s8, m, std::regex("<(.+?)>")) && m.str(1) == "<a");
  CHECK(std::regex_search(s8, m, std::regex("<([^<]+?)>+")) && m.str() == "<a>>");
  return 0;
}
