// Adversarial uses of <regex>:
// 1. regex_iterator over zero-length matches, worked through [re.regiter.incr]/1-5 by hand:
//    after a zero-length match not at end, a non-null match continuing at the same position is
//    tried first (/3: match_not_null | match_continuous), otherwise the start advances by one and
//    the search continues with match_prev_avail (/4); a zero-length match at end ends the
//    iteration (/2), and a non-empty match ending at end is followed by one more search there
//    (/4), which can find an empty match at end. /5: prefix().first is the previous match's end.
// 2. match_flag_type combinations ([re.matchflag] Table 119) and the format flags in regex_replace
//    ([re.alg.replace]/1-2: format_no_copy, format_first_only; format_default is ECMA-262
//    String.prototype.replace ($&, $`, $', $n, $$); format_sed uses sed's & and \n).
// 3. Long inputs (10^5 .. 10^6 characters). [re.err]: the library may report error_complexity or
//    error_stack ("insufficient memory to determine whether the regular expression could match");
//    either the correct result or one of those regex_error codes is accepted, nothing else.
#include <regex>
#include <string>
#include <utility>
#include <vector>
#include "check.hpp"

namespace rc = std::regex_constants;
using PL = std::vector<std::pair<long, long>>;

PL iterate(const std::string& s, const std::regex& re, rc::match_flag_type f = rc::match_default) {
  PL out;
  auto prev_end = s.begin();
  for (std::sregex_iterator it(s.begin(), s.end(), re, f), e; it != e; ++it) {
    CHECK(it->prefix().first == prev_end);  // /5
    CHECK(it->prefix().second == (*it)[0].first);
    prev_end = (*it)[0].second;
    out.emplace_back(it->position(), it->length());
  }
  return out;
}

template <class F>
void within_limits(F f) {
  try {
    f();
  } catch (const std::regex_error& e) {
    CHECK(e.code() == rc::error_complexity || e.code() == rc::error_stack);
  }
}

int main() {
  // 1. zero-length matches
  CHECK((iterate("baaa", std::regex("a*")) == PL{{0, 0}, {1, 3}, {4, 0}}));
  CHECK((iterate("aa", std::regex("a*?")) == PL{{0, 0}, {0, 1}, {1, 0}, {1, 1}, {2, 0}}));
  CHECK((iterate("ab cd", std::regex("\\b")) == PL{{0, 0}, {2, 0}, {3, 0}, {5, 0}}));
  CHECK((iterate("abc", std::regex("$")) == PL{{3, 0}}));
  CHECK((iterate("aaa", std::regex("(?=a)")) == PL{{0, 0}, {1, 0}, {2, 0}}));
  CHECK((iterate("", std::regex("x*")) == PL{{0, 0}}));
  CHECK((iterate("ab", std::regex("")) == PL{{0, 0}, {1, 0}, {2, 0}}));
  CHECK((iterate("abab", std::regex("(?:ab)*")) == PL{{0, 4}, {4, 0}}));
  CHECK((iterate("ab", std::regex("b*")) == PL{{0, 0}, {1, 1}, {2, 0}}));
  // match_not_eow: \b may not match [last, last); match_not_bow: not [first, first)
  CHECK((iterate("ab", std::regex("\\b"), rc::match_not_eow) == PL{{0, 0}}));
  CHECK((iterate("ab", std::regex("\\b"), rc::match_not_bow) == PL{{2, 0}}));
  // match_not_null for the whole iteration: only non-empty matches
  CHECK((iterate("baab", std::regex("a*"), rc::match_not_null) == PL{{1, 2}}));
  // token iterator over zero-length matches: -1 gives the pieces between them
  {
    std::string s = "a,b,,c";
    std::regex sep(",");
    std::vector<std::string> parts(std::sregex_token_iterator(s.begin(), s.end(), sep, -1), {});
    CHECK((parts == std::vector<std::string>{"a", "b", "", "c"}));
  }

  // 2. flag combinations
  {
    std::string e;
    std::smatch m;
    CHECK(std::regex_search(e, m, std::regex("^$")));
    CHECK(!std::regex_search(e, m, std::regex("^$"), rc::match_not_bol));
    CHECK(!std::regex_search(e, m, std::regex("^$"), rc::match_not_eol));
    CHECK(!std::regex_search(e, m, std::regex("^$"), rc::match_not_null));
    std::string s = "xab";
    CHECK(!std::regex_search(s.cbegin() + 1, s.cend(), m, std::regex("\\bab"), rc::match_prev_avail));
    CHECK(std::regex_search(s.cbegin() + 1, s.cend(), m, std::regex("\\bab")));
    CHECK(std::regex_search(s.cbegin() + 1, s.cend(), m, std::regex("^ab"), rc::match_not_bol | rc::match_prev_avail) ==
          false);  // prev_avail: not_bol ignored, but 'x' precedes: ^ does not match (no multiline)
    CHECK(!std::regex_search(s, m, std::regex("ab"), rc::match_continuous));
    CHECK(std::regex_search(s, m, std::regex("x?a"), rc::match_continuous | rc::match_not_null) && m.length() == 2);
    CHECK(!std::regex_match(s, m, std::regex("x*"), rc::match_not_null));
    CHECK(std::regex_search(s, m, std::regex("a|ab"), rc::match_any) && m.position() == 1);
  }
  {
    std::string s = "a1b22c333";
    std::regex d("\\d+");
    CHECK(std::regex_replace(s, d, "<$&>") == "a<1>b<22>c<333>");
    CHECK(std::regex_replace(s, d, "<$&>", rc::format_first_only) == "a<1>b22c333");
    CHECK(std::regex_replace(s, d, "<$&>", rc::format_no_copy) == "<1><22><333>");
    CHECK(std::regex_replace(s, d, "<$&>", rc::format_no_copy | rc::format_first_only) == "<1>");
    CHECK(std::regex_replace(std::string("xay"), std::regex("a"), "[$`|$'|$$]") == "x[x|y|$]y");
    CHECK(std::regex_replace(std::string("ab"), std::regex("(a)(b)"), "\\2\\1&", rc::format_sed) == "baab");
    CHECK(std::regex_replace(std::string("ab"), std::regex("(a)(b)"), "$2$1&", rc::format_sed) == "$2$1ab");
    CHECK(std::regex_replace(std::string("abc"), std::regex("q"), "Z", rc::format_no_copy).empty());
    // zero-length matches in replace: every position, as the iterator finds them
    CHECK(std::regex_replace(std::string("baaa"), std::regex("a*"), "-") == "-b--");
    CHECK(std::regex_replace(std::string("ab"), std::regex(""), "|") == "|a|b|");
  }

  // 3. long inputs
  const std::string many_a(1000000, 'a');
  within_limits([&] { CHECK(std::regex_match(many_a, std::regex("a*"))); });
  within_limits([&] {
    std::smatch m;
    CHECK(std::regex_match(many_a, m, std::regex("(a|b)*")));
    CHECK(m[1].matched && m.length(1) == 1 && m.position(1) == 999999);
  });
  within_limits([&] { CHECK(std::regex_match(many_a, std::regex("[ab]+", rc::extended))); });
  within_limits([&] {
    std::string s = many_a + "x";
    std::smatch m;
    CHECK(std::regex_search(s, m, std::regex("x")) && m.position() == 1000000);
  });
  within_limits([&] {
    std::string s(100000, 'a');
    s += 'b';
    std::smatch m;
    CHECK(std::regex_search(s, m, std::regex("a*b")) && m.position() == 0 && m.length() == 100001);
  });
  within_limits([&] { CHECK(!std::regex_search(std::string(5000, 'a'), std::regex("a*b"))); });
  within_limits([&] {
    std::string s;
    for (int i = 0; i < 100000; ++i) s += "ab";
    std::regex b("b");
    CHECK(std::distance(std::sregex_iterator(s.begin(), s.end(), b), std::sregex_iterator()) == 100000);
    CHECK(std::regex_replace(s, b, "") == std::string(100000, 'a'));
  });
  // (kept short: backtracking implementations take time exponential in the length here)
  within_limits([&] { CHECK(std::regex_match(std::string(24, 'a'), std::regex("(a|aa)*", rc::extended))); });
  return 0;
}
