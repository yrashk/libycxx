// [re.synopt] Table 118: basic, extended, awk, grep, egrep select the POSIX grammars (IEEE 1003.1
// "Regular Expressions" and the awk/grep utilities); [re.alg.match]/[re.alg.search]: for the
// POSIX grammars the match found is the leftmost-longest one.
//  - BRE (basic): + ? | ( ) { } are ordinary characters; \( \) group, \{ \} interval, \n back
//    reference.
//  - ERE (extended): + ? | ( ) { } are special; ^ and $ are anchors anywhere.
//  - awk: ERE plus the escapes \" \/ \ddd (octal) and \n \t etc.
//  - grep / egrep: BRE / ERE with newline separating alternatives.
#include <regex>
#include <string>
#include "check.hpp"

namespace rc = std::regex_constants;

bool full(const std::string& s, const char* re, rc::syntax_option_type f) {
  try {
    return std::regex_match(s, std::regex(re, f));
  } catch (const std::regex_error&) {
    return false;  // reported by the CHECK
  }
}
std::string first(const std::string& s, const char* re, rc::syntax_option_type f, int g = 0) {
  std::smatch m;
  return std::regex_search(s, m, std::regex(re, f)) ? m.str(g) : std::string("<none>");
}

int main() {
  // BRE
  CHECK(full("a+", "a+", rc::basic) && !full("aa", "a+", rc::basic));
  CHECK(full("a?", "a?", rc::basic) && full("a|b", "a|b", rc::basic) && full("(a)", "(a)", rc::basic));
  CHECK(full("abab", "\\(ab\\)*", rc::basic) && full("abab", "\\(ab\\)\\1", rc::basic));
  CHECK(full("aa", "a\\{2\\}", rc::basic) && full("aaa", "a\\{2,\\}", rc::basic) && !full("a", "a\\{2,3\\}", rc::basic));
  CHECK(full("ab", "a.", rc::basic) && full("a.", "a\\.", rc::basic) && !full("ab", "a\\.", rc::basic));
  CHECK(full("a]", "a[]]", rc::basic) && full("b", "[^]a]", rc::basic) && !full("]", "[^]a]", rc::basic));
  CHECK(full("3", "[[:digit:]]", rc::basic) && full("-", "[a-]", rc::basic));
  // ERE
  CHECK(full("aaa", "a+", rc::extended) && full("", "a?", rc::extended) && full("b", "a|b", rc::extended));
  CHECK(full("ababc", "(ab)*c", rc::extended) && full("aab", "a{2}b", rc::extended));
  CHECK(full("a+", "a\\+", rc::extended) && full("(", "\\(", rc::extended));
  // leftmost-longest
  CHECK(first("xyz abcd", "a|ab|abc", rc::extended) == "abc");
  CHECK(first("xabcd", "(a|ab)(c|bcd)", rc::extended) == "abcd");
  CHECK(first("abcd", "(a|ab)(c|bcd)(d*)", rc::extended) == "abcd");
  CHECK(first("xyxyz", "(xy|xyx)*z", rc::basic) == "<none>");  // BRE: ( | ) are literal
  CHECK(first("xyxyz", "(xy|x)*z", rc::extended) == "xyxyz");
  // awk escapes
  CHECK(full("A", "\\101", rc::awk) && full("/", "\\/", rc::awk) && full("\"", "\\\"", rc::awk));
  CHECK(full("\t", "\\t", rc::awk) && full("ab", "a|ab", rc::awk) && first("ab", "a|ab", rc::awk) == "ab");
  // grep / egrep
  CHECK(full("a+", "x\na+", rc::grep) && !full("aa", "x\na+", rc::grep));
  CHECK(full("aa", "x\na+", rc::egrep) && full("x", "x\na+", rc::egrep));
  CHECK(first("zzabc", "ab\nabc", rc::egrep) == "abc");
  // icase and nosubs with a POSIX grammar
  CHECK(full("AB", "ab", rc::extended | rc::icase));
  std::smatch m;
  std::string s = "ab";
  CHECK(std::regex_match(s, m, std::regex("(a)(b)", rc::extended | rc::nosubs)) && m.size() == 1);
  CHECK((std::regex("x", rc::grep).flags() & rc::grep) == rc::grep);
  return 0;
}
