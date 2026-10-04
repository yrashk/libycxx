// [re.regiter]: regex_iterator enumerates successive non-overlapping matches; after an empty
// match the next search uses match_not_null | match_continuous at the same position and, if
// that fails, advances one character (so "a*" over "baaa" yields "", "aaa", ""); the iterator
// compares equal to the end-of-sequence iterator after the last match; (*it).prefix() is the
// text since the previous match. [re.tokiter]: regex_token_iterator with submatch -1 yields
// the text between matches (field splitting), with n >= 0 the n-th sub-expression of each
// match, and with a list of indices each in turn.
#include <regex>
#include <iterator>
#include <type_traits>
#include <string>
#include <vector>
#include "check.hpp"

int main() {
  std::string s = "a1b22c333";
  std::regex num("\\d+");
  std::vector<std::string> found;
  std::vector<long> pos;
  for (std::sregex_iterator it(s.begin(), s.end(), num), end; it != end; ++it) {
    found.push_back(it->str());
    pos.push_back(it->position());
  }
  CHECK((found == std::vector<std::string>{"1", "22", "333"}));
  CHECK((pos == std::vector<long>{1, 3, 6}));  // position() is relative to the start of the sequence

  std::sregex_iterator it(s.begin(), s.end(), num);
  CHECK((*it).prefix() == "a");
  ++it;
  CHECK(it->prefix() == "b" && (*it)[0] == "22");
  std::sregex_iterator copy = it;
  CHECK(copy == it && copy != std::sregex_iterator());
  CHECK(std::distance(std::sregex_iterator(s.begin(), s.end(), num), std::sregex_iterator()) == 3);
  std::regex z("z");
  CHECK(std::sregex_iterator(s.begin(), s.end(), z) == std::sregex_iterator());

  // Empty matches.
  std::string b = "baaa";
  std::vector<std::string> em;
  std::vector<long> epos;
  std::regex astar("a*");
  for (std::sregex_iterator i(b.begin(), b.end(), astar), e; i != e; ++i) {
    em.push_back(i->str());
    epos.push_back(i->position());
  }
  CHECK((em == std::vector<std::string>{"", "aaa", ""}));
  CHECK((epos == std::vector<long>{0, 1, 4}));

  // Token iterator: split on commas with optional spaces.
  std::string csv = "x, y,z,,w";
  std::regex sep(",\\s*");
  std::vector<std::string> fields(std::sregex_token_iterator(csv.begin(), csv.end(), sep, -1), std::sregex_token_iterator());
  CHECK((fields == std::vector<std::string>{"x", "y", "z", "", "w"}));
  // Selecting sub-expressions.
  std::string kv = "a=1;b=2";
  std::regex pair("(\\w)=(\\d)");
  std::vector<std::string> keys(std::sregex_token_iterator(kv.begin(), kv.end(), pair, 1), std::sregex_token_iterator());
  CHECK((keys == std::vector<std::string>{"a", "b"}));
  std::vector<std::string> both(std::sregex_token_iterator(kv.begin(), kv.end(), pair, {2, 1}), std::sregex_token_iterator());
  CHECK((both == std::vector<std::string>{"1", "a", "2", "b"}));
  std::vector<std::string> whole(std::sregex_token_iterator(kv.begin(), kv.end(), pair), std::sregex_token_iterator());
  CHECK((whole == std::vector<std::string>{"a=1", "b=2"}));
  // -1 with no match at all yields the whole sequence.
  std::string none = "abc";
  std::sregex_token_iterator t(none.begin(), none.end(), sep, -1);
  CHECK(t != std::sregex_token_iterator() && t->str() == "abc" && ++t == std::sregex_token_iterator());
  // -1 with a trailing separator does not yield a final empty field.
  std::string trail = "p,q,";
  std::vector<std::string> tf(std::sregex_token_iterator(trail.begin(), trail.end(), sep, -1), std::sregex_token_iterator());
  CHECK((tf == std::vector<std::string>{"p", "q"}));

  // The iterators may not be constructed from a temporary regex.
  static_assert(!std::is_constructible_v<std::sregex_iterator, std::string::const_iterator, std::string::const_iterator, std::regex&&>);
  static_assert(!std::is_constructible_v<std::sregex_token_iterator, std::string::const_iterator, std::string::const_iterator, std::regex&&, int>);
  return 0;
}
