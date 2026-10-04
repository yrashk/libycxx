// [re.regiter.incr]/4: after a non-empty match the next search uses match_prev_avail, so ^ and \b
// look at the character before the new start (^ does not match in the middle of a line, \b only
// at a real word boundary). /3: after an empty match, a non-null match continuing at the same
// position is tried first, then the start advances. /5: match.prefix().first is the end of the
// previous match, and position(i) is relative to the start of the target sequence.
// [re.regiter.comp]/1: iterators are equal iff both are end-of-sequence or they have the same
// begin, end, regex, flags and match[0].
// [re.regiter.cnstr]: if the first regex_search fails, the iterator is end-of-sequence.
// [re.tokiter.incr]: with submatch -1 a non-empty suffix after the last match is a final token.
#include <regex>
#include <string>
#include <vector>
#include "check.hpp"

namespace rc = std::regex_constants;

std::vector<long> positions(const std::string& s, const std::regex& re, rc::match_flag_type f = rc::match_default) {
  std::vector<long> out;
  for (std::sregex_iterator it(s.begin(), s.end(), re, f), e; it != e; ++it) out.push_back(it->position());
  return out;
}

int main() {
  std::string aaa = "aaa";
  CHECK((positions(aaa, std::regex("^a")) == std::vector<long>{0}));
  CHECK((positions(aaa, std::regex("a")) == std::vector<long>{0, 1, 2}));
  std::string words = "aa a";
  CHECK((positions(words, std::regex("\\ba")) == std::vector<long>{0, 3}));
  CHECK((positions(words, std::regex("a\\b")) == std::vector<long>{1, 3}));
  std::string ml = "ab\nab";
  CHECK((positions(ml, std::regex("^a", rc::ECMAScript | rc::multiline)) == std::vector<long>{0, 3}));
  // Flags given to the constructor are used by every search: match_not_bol stops ^ at the start.
  CHECK(positions(aaa, std::regex("^a"), rc::match_not_bol).empty());

  // Empty matches and prefixes.
  std::string b = "baaa";
  std::regex astar("a*");
  std::sregex_iterator it(b.begin(), b.end(), astar), end;
  CHECK(it->str() == "" && it->position() == 0 && it->prefix().str() == "" && !it->prefix().matched);
  ++it;
  CHECK(it->str() == "aaa" && it->position() == 1);
  CHECK(it->prefix().first == b.begin() && it->prefix().str() == "b" && it->prefix().matched);
  ++it;
  CHECK(it->str() == "" && it->position() == 4 && it->prefix().str() == "");
  ++it;
  CHECK(it == end);
  // An empty match followed by a non-empty one at the same position.
  std::string xy = "xy";
  std::regex opt("(?:)|x");  // first alternative matches empty, then match_not_null finds "x"
  std::vector<std::string> got;
  for (std::sregex_iterator i(xy.begin(), xy.end(), opt); i != end; ++i) got.push_back(i->str());
  CHECK((got == std::vector<std::string>{"", "x", "", ""}));

  // Equality.
  std::regex d("\\d");
  std::string s = "1a2";
  std::sregex_iterator i1(s.begin(), s.end(), d), i2(s.begin(), s.end(), d);
  CHECK(i1 == i2);
  ++i2;
  CHECK(i1 != i2);
  ++i1;
  CHECK(i1 == i2);
  std::sregex_iterator i3(s.begin(), s.end(), d, rc::match_not_null);
  std::sregex_iterator i4(s.begin(), s.end(), d);
  CHECK(i3 != i4);  // different flags
  std::regex d2("\\d");
  CHECK(std::sregex_iterator(s.begin(), s.end(), d2) != std::sregex_iterator(s.begin(), s.end(), d));
  std::regex z("z");
  CHECK(std::sregex_iterator(s.begin(), s.end(), z) == end);

  // Post-increment returns the previous value.
  std::sregex_iterator p(s.begin(), s.end(), d);
  std::sregex_iterator old = p++;
  CHECK(old->str() == "1" && p->str() == "2");

  // Token iterator: the suffix after the last match is a final token.
  std::string csv = "a,b,c";
  std::regex comma(",");
  std::vector<std::string> toks(std::sregex_token_iterator(csv.begin(), csv.end(), comma, -1), std::sregex_token_iterator());
  CHECK((toks == std::vector<std::string>{"a", "b", "c"}));
  std::string lead = ",a";
  std::vector<std::string> lt(std::sregex_token_iterator(lead.begin(), lead.end(), comma, -1), std::sregex_token_iterator());
  CHECK((lt == std::vector<std::string>{"", "a"}));
  // {-1, 0}: the separators interleaved with the fields.
  std::vector<std::string> both(std::sregex_token_iterator(csv.begin(), csv.end(), comma, {-1, 0}), std::sregex_token_iterator());
  CHECK((both == std::vector<std::string>{"a", ",", "b", ",", "c"}));
  // A sub-expression that did not participate gives an unmatched token.
  std::string opt2 = "ab";
  std::regex og("(x)?(a|b)");
  std::sregex_token_iterator ti(opt2.begin(), opt2.end(), og, 1);
  CHECK(!ti->matched && ti->str() == "");
  return 0;
}
