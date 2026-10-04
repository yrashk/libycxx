// [re.alg.match], [re.alg.search], [re.results]: regex_match requires the whole sequence to
// match, regex_search finds the first (leftmost) match. On success the match_results is
// ready, size() is 1 + mark_count(), [0] is the whole match, [n] the n-th group (matched
// false for a group that did not participate), prefix()/suffix() the text before and after,
// position(n)/length(n)/str(n) describe groups. On failure the results are empty (size 0).
#include <regex>
#include <string>
#include "check.hpp"

int main() {
  std::regex re("(\\d+)-(\\d+)(x)?");
  std::smatch m;
  std::string s = "abc 12-345 def";
  CHECK(!std::regex_match(s, m, re));
  CHECK(m.ready() && m.empty() && m.size() == 0);
  CHECK(std::regex_search(s, m, re));
  CHECK(m.ready() && !m.empty() && m.size() == 4 && re.mark_count() == 3);
  CHECK(m[0] == "12-345" && m.str() == "12-345" && m.str(1) == "12" && m[2].str() == "345");
  CHECK(m.position() == 4 && m.length() == 6 && m.position(2) == 7 && m.length(2) == 3);
  CHECK(m[0].matched && m[1].matched && !m[3].matched && m[3].length() == 0 && m.str(3).empty());
  CHECK(m.prefix() == "abc " && m.prefix().matched && m.suffix() == " def" && m.suffix().matched);
  CHECK(m[0].first == s.begin() + 4 && m[0].second == s.begin() + 10);
  // Out-of-range group index gives an unmatched sub_match.
  CHECK(!m[10].matched && m[10].length() == 0);

  CHECK(std::regex_match("12-34x", re));
  std::cmatch cm;
  CHECK(std::regex_match("12-34x", cm, re) && cm[3].matched && cm[3] == "x");
  CHECK(std::regex_match(s.begin() + 4, s.begin() + 10, re));
  CHECK(!std::regex_match(s.begin() + 4, s.begin() + 11, re));
  CHECK(std::regex_search("no digits here", re) == false);
  CHECK(std::regex_search(std::string("7-8"), re));

  // Leftmost match; ECMAScript alternation takes the first alternative that matches.
  std::regex alt("a|ab");
  CHECK(std::regex_search("xab", cm, alt) && cm.str() == "a" && cm.position() == 1);
  CHECK(std::regex_match("ab", alt));  // regex_match needs the whole sequence: backtracks to "ab"

  // match_results iteration and empty() / begin()/end().
  std::regex_search(s, m, re);
  int n = 0;
  for (const auto& sm : m) n += sm.matched;
  CHECK(n == 3 && m.end() - m.begin() == 4);

  // wide characters
  std::wsmatch wm;
  std::wstring ws = L"key=value";
  CHECK(std::regex_match(ws, wm, std::wregex(L"(\\w+)=(\\w+)")) && wm.str(2) == L"value");
  return 0;
}
