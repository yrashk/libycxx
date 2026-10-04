// [re.matchflag]: match_not_bol (the first character is not at the beginning of a line, so ^
// does not match there), match_not_eol ($ does not match at the end), match_not_bow /
// match_not_eow (\b does not match at the start / end), match_any, match_not_null (an empty
// sequence does not match), match_continuous (the match must begin at first),
// match_prev_avail (--first is valid; match_not_bol and match_not_bow are then ignored and
// the previous character is consulted).
#include <regex>
#include <string>
#include "check.hpp"

namespace rc = std::regex_constants;

int main() {
  std::string s = "abc";
  CHECK(std::regex_search(s, std::regex("^a")));
  CHECK(!std::regex_search(s, std::regex("^a"), rc::match_not_bol));
  CHECK(std::regex_search(s, std::regex("c$")));
  CHECK(!std::regex_search(s, std::regex("c$"), rc::match_not_eol));
  CHECK(!std::regex_search(s, std::regex("\\ba"), rc::match_not_bow));
  CHECK(!std::regex_search(s, std::regex("c\\b"), rc::match_not_eow));
  CHECK(std::regex_match("", std::regex("a*")));
  CHECK(!std::regex_match("", std::regex("a*"), rc::match_not_null));
  std::smatch m;
  CHECK(std::regex_search(s, m, std::regex("a*"), rc::match_not_null) && m.str() == "a");
  CHECK(std::regex_search(s, std::regex("b")));
  CHECK(!std::regex_search(s, std::regex("b"), rc::match_continuous));
  CHECK(std::regex_search(s, std::regex("ab"), rc::match_continuous));
  // match_prev_avail: the character before first is consulted.
  std::string t = "xab";
  CHECK(!std::regex_search(t.begin() + 1, t.end(), std::regex("\\bab"), rc::match_prev_avail));
  CHECK(std::regex_search(t.begin() + 1, t.end(), std::regex("\\bab")));
  CHECK(std::regex_search("aaa", std::regex("a+"), rc::match_any));
  return 0;
}
