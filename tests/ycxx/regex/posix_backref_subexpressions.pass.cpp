// The POSIX grammars with back-references, and with bounded repetitions too large for the NFA.
// [re.synopt]/1 Table 118: basic, extended, awk, grep and egrep use the grammars and matching
// rules of IEEE Std 1003.1 (XBD 9; awk and grep/egrep are the utilities' forms of ERE and BRE).
// [re.alg.match]/2, [re.alg.search]/2: m holds the match those rules determine, m[n] the n-th
// subexpression ("matched" false when it did not participate).
// XBD 9.1: "Consistent with the whole match being the longest of the leftmost matches, each
//   subpattern, from left to right, shall match the longest possible string. For this purpose,
//   a null string shall be considered to be longer than no match at all."
// XBD 9.3.6: "\n" matches the same string as the n-th subexpression; when a subexpression matched
//   more than once, the back-reference refers to the last matched string; "\(a\)*\1" fails to
//   match "a"; "\(a\(b\)*\)*\2" fails to match "abab"; a repeated subexpression "shall not match
//   a null expression unless this is the only match for the repetition or it is necessary to
//   satisfy the exact or minimum number of occurrences".
// regexec() (XSH): a subexpression inside a repeated one is reported within the last match of
//   the enclosing one; one that did not participate is reported as no match.
#include <regex>
#include <string>
#include "check.hpp"

namespace rc = std::regex_constants;

namespace {

// m[0] .. m[n] as "(first,last)" offsets, "(-1,-1)" for an unmatched one.
std::string spans(const std::smatch& m, const std::string& s) {
  std::string r;
  for (std::size_t i = 0; i < m.size(); ++i) {
    if (!m[i].matched) {
      r += "(-1,-1)";
      continue;
    }
    r += "(" + std::to_string(m[i].first - s.begin()) + "," + std::to_string(m[i].second - s.begin()) + ")";
  }
  return r;
}
std::string search(const std::string& s, const char* pat, rc::syntax_option_type f) {
  std::smatch m;
  if (!std::regex_search(s, m, std::regex(pat, f)))
    return "none";
  return spans(m, s);
}
std::string match(const std::string& s, const char* pat, rc::syntax_option_type f) {
  std::smatch m;
  if (!std::regex_match(s, m, std::regex(pat, f)))
    return "none";
  return spans(m, s);
}

} // namespace

int main() {
  // Leftmost-longest with a back-reference: from 0, \1 cannot follow; from 1, "aa" b "aa".
  CHECK(search("aaabaa", "\\(a*\\)b\\1", rc::basic) == "(1,6)(1,3)");
  CHECK(search("aaabaa", "(a*)b\\1", rc::extended) == "(1,6)(1,3)");
  CHECK(search("abcabc", "^\\(.*\\)\\1$", rc::basic) == "(0,6)(0,3)");
  CHECK(match("abcabd", "\\(.*\\)\\1", rc::basic) == "none");

  // Each subpattern, left to right, the longest: (x*)(a|ab)(c|bcd)(d*)\1 on "abcd" matches all
  // of it as a+bcd+"" and as ab+c+d; the second subexpression takes "ab".
  CHECK(match("abcd", "(x*)(a|ab)(c|bcd)(d*)\\1", rc::extended) == "(0,4)(0,0)(0,2)(2,3)(3,4)");
  // A subpattern before another that refers to it: the first takes all it can.
  CHECK(match("aaaa", "\\(a*\\)\\(a*\\)\\2", rc::basic) == "(0,4)(0,4)(4,4)");
  CHECK(match("aaaa", "\\(a*\\)\\1\\(a*\\)", rc::basic) == "(0,4)(0,2)(4,4)");

  // Back-references to a subexpression that did not participate fail (XBD 9.3.6 examples).
  CHECK(search("a", "\\(a\\)*\\1", rc::basic) == "none");
  CHECK(search("b", "\\(a\\)*\\1", rc::basic) == "none");
  CHECK(search("aa", "\\(a\\)*\\1", rc::basic) == "(0,2)(0,1)");
  CHECK(search("abab", "\\(a\\(b\\)*\\)*\\2", rc::basic) == "none");
  CHECK(search("ababb", "\\(a\\(b\\)*\\)*\\2", rc::basic) == "(0,5)(2,4)(3,4)");

  // The only match of a repetition may be empty, and is then a null string (\1 defined).
  CHECK(search("b", "\\(a*\\)*b\\1", rc::basic) == "(0,1)(0,0)");
  CHECK(search("bc", "\\(a*\\)*\\1", rc::basic) == "(0,0)(0,0)");
  // No empty iteration after a non-empty one: \(a\{0,1\}\)*b\1 on "ab" cannot end the loop with
  // an empty iteration to make \1 empty; it matches from 1, where the only iteration is empty.
  CHECK(search("ab", "\\(a\\{0,1\\}\\)*b\\1", rc::basic) == "(1,2)(1,1)");
  // An empty iteration needed for the minimum count.
  CHECK(match("aa", "\\(a*\\)\\{2\\}\\1", rc::basic) == "(0,2)(2,2)");
  CHECK(match("aa", "(a*){2,3}\\1", rc::extended) == "(0,2)(2,2)");

  // Back-references inside repetitions: each iteration's own subexpressions.
  CHECK(match("aabaab", "((a*)b\\2)*", rc::extended) == "(0,6)(5,6)(5,5)");
  CHECK(match("aaba", "((a*)b\\2)*", rc::extended) == "none");
  CHECK(search("aabaabab", "((a*)b\\2)*", rc::extended) == "(0,6)(5,6)(5,5)");
  CHECK(match("abb", "(a|b)*\\1", rc::extended) == "(0,3)(1,2)");
  CHECK(match("abba", "(a|b)*\\1", rc::extended) == "none");
  // Back-references inside alternations.
  CHECK(match("abcbc", "(a|ab)(bc|c)\\2", rc::extended) == "(0,5)(0,1)(1,3)");
  CHECK(match("abcc", "(a|ab)(bc|c)\\2", rc::extended) == "(0,4)(0,2)(2,3)");
  CHECK(match("aab", "(a)(\\1b|b)", rc::extended) == "(0,3)(0,1)(1,3)");
  CHECK(match("ab", "(a)(\\1b|b)", rc::extended) == "(0,2)(0,1)(1,2)");
  CHECK(match("xyxyx", "(x|xy)(\\1|y)*", rc::extended) == "(0,5)(0,1)(4,5)");

  // grep and egrep: the lines are alternatives; the leftmost-longest match over all of them.
  CHECK(search("aaabaa", "zz\n\\(a*\\)b\\1", rc::grep) == "(1,6)(1,3)");
  CHECK(search("abcd", "zz\n(x*)(a|ab)(c|bcd)(d*)\\1", rc::egrep) == "(0,4)(0,0)(0,2)(2,3)(3,4)");
  CHECK(search("xaba", "b\n(a)b\\1", rc::egrep) == "(1,4)(1,2)");

  // Bounded repetitions too large for the NFA (more than 256 copies): the same rules.
  CHECK(match("abcd", "(a|ab)(c|bcd)(d*)(x){0,300}", rc::extended) == "(0,4)(0,2)(2,3)(3,4)(-1,-1)");
  CHECK(match("abcd", "(a|ab)(c|bcd)(d*)(x){0,300}", rc::awk) == "(0,4)(0,2)(2,3)(3,4)(-1,-1)");
  CHECK(match("abcd", "(a|ab)(c|bcd)(d*)(x){0,300}", rc::egrep) == "(0,4)(0,2)(2,3)(3,4)(-1,-1)");
  CHECK(match("abcd", "\\(a*\\)\\(a*b\\)\\{1,300\\}\\(.*\\)", rc::basic) == "(0,4)(0,1)(1,2)(2,4)");

  // icase compares the back-reference's characters case-insensitively ([re.grammar]/14.1).
  CHECK(match("aA", "\\(a\\)\\1", rc::basic | rc::icase) == "(0,2)(0,1)");
  CHECK(match("aA", "\\(a\\)\\1", rc::basic) == "none");

  // nosubs: the match is the same; no subexpressions are reported ([re.synopt]/1).
  {
    std::smatch m;
    const std::string s = "aaabaa";
    CHECK(std::regex_search(s, m, std::regex("\\(a*\\)b\\1", rc::basic | rc::nosubs)));
    CHECK(m.size() == 1 && m.position(0) == 1 && m.length(0) == 5);
  }
  // Flags: match_not_null and match_continuous select among the same matches.
  {
    std::smatch m;
    const std::string s = "baab";
    CHECK(std::regex_search(s, m, std::regex("\\(a*\\)\\1", rc::basic), rc::match_not_null));
    CHECK(spans(m, s) == "(1,3)(1,2)");
    CHECK(std::regex_search(s, m, std::regex("\\(a*\\)\\1", rc::basic), rc::match_continuous));
    CHECK(spans(m, s) == "(0,0)(0,0)");
  }
  // regex_iterator over the matches.
  {
    const std::string s = "xaaxbbcddc";
    const std::regex r("\\(.\\)\\(.\\)\\2\\1", rc::basic);
    std::string all;
    for (std::sregex_iterator i(s.begin(), s.end(), r), e; i != e; ++i)
      all += (*i)[0].str() + ":" + (*i)[2].str() + " ";
    CHECK(all == "xaax:a cddc:d ");
  }
  return 0;
}
