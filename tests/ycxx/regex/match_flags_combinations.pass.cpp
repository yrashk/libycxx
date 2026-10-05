// [re.matchflag] Table 119, the effects that are limited to one position or combine:
// match_not_bol: "the character ^ in the regular expression shall not match [first, first)"
// (with multiline, ^ still matches after a line terminator inside the sequence);
// match_not_eol: "$" "shall not match [last, last)" (it still matches before a line
// terminator); match_not_bow / match_not_eow: "\b" shall not match [first, first) /
// [last, last) (only there); match_not_null: "The expression shall not match an empty
// sequence" (a later non-empty match, or a longer one, is found instead); match_continuous:
// "shall only match a sub-sequence that begins at first"; match_prev_avail: "--first is a valid
// iterator position. When this flag is set the flags match_not_bol and match_not_bow shall be
// ignored by the regular expression algorithms ([re.alg]) and iterators ([re.iter])";
// match_any: "any match is an acceptable result".
// [re.synopt] multiline: ^ matches the beginning of a line and $ the end of a line.
// [re.regiter.cnstr]/2, [re.regiter.incr]: the flags given to a regex_iterator are passed to
// every search; [re.alg.replace]: regex_replace uses a regex_iterator with the flags and copies
// unmatched text unless format_no_copy, replacing only the first match with format_first_only.
#include <regex>
#include <string>
#include <vector>
#include "check.hpp"

namespace rc = std::regex_constants;

static std::vector<std::pair<long, std::string>> all(const std::string& s, const std::regex& re,
                                                     rc::match_flag_type f) {
  std::vector<std::pair<long, std::string>> r;
  for (std::sregex_iterator i(s.begin(), s.end(), re, f), e; i != e; ++i)
    r.emplace_back(i->position(), i->str());
  return r;
}

int main() {
  using std::regex;
  std::smatch m;
  const auto ml = regex::ECMAScript | regex::multiline;

  // match_not_bol / match_not_eol only concern [first, first) and [last, last).
  std::string s = "a\nb";
  CHECK(std::regex_search(s, m, regex("^b", ml), rc::match_not_bol) && m.position() == 2);
  CHECK(!std::regex_search(s, m, regex("^a", ml), rc::match_not_bol));
  CHECK(std::regex_search(s, m, regex("a$", ml), rc::match_not_eol) && m.position() == 0);
  CHECK(!std::regex_search(s, m, regex("b$", ml), rc::match_not_eol));
  std::string two = "x\nab";
  // match_prev_avail: the character before first is a line terminator, so ^ matches at first
  // even with match_not_bol (ignored); a word character before first: no line start there.
  CHECK(std::regex_search(two.cbegin() + 2, two.cend(), regex("^ab", ml),
                          rc::match_prev_avail | rc::match_not_bol));
  std::string glued = "xab";
  CHECK(!std::regex_search(glued.cbegin() + 1, glued.cend(), regex("^ab", ml), rc::match_prev_avail));

  // match_not_bow / match_not_eow only concern the ends.
  std::string ab = "a b";
  CHECK(std::regex_search(ab, m, regex("\\bb"), rc::match_not_bow) && m.position() == 2);
  CHECK(std::regex_search(ab, m, regex("a\\b"), rc::match_not_eow) && m.position() == 0);
  CHECK(!std::regex_search(ab, m, regex("^\\b"), rc::match_not_bow));
  CHECK(!std::regex_search(ab, m, regex("\\b$"), rc::match_not_eow));
  // match_prev_avail makes match_not_bow ignored: a space before first is a word boundary.
  std::string sp = " ab";
  CHECK(std::regex_search(sp.cbegin() + 1, sp.cend(), regex("\\bab"),
                          rc::match_prev_avail | rc::match_not_bow | rc::match_continuous));
  CHECK(!std::regex_search(glued.cbegin() + 1, glued.cend(), regex("\\bab"),
                           rc::match_prev_avail | rc::match_not_bow));

  // match_not_null.
  std::string bba = "bba";
  CHECK(std::regex_search(bba, m, regex("a*"), rc::match_not_null) && m.position() == 2 && m.str() == "a");
  std::string aa = "aa";
  CHECK(std::regex_search(aa, m, regex("a*?"), rc::match_not_null) && m.position() == 0 && m.str() == "a");
  CHECK(std::regex_search(aa, m, regex("(?:)|a"), rc::match_not_null) && m.str() == "a");
  CHECK(!std::regex_search(bba, m, regex("x*|y?"), rc::match_not_null));
  // match_not_null | match_continuous.
  std::string baa = "baa", aab = "aab";
  CHECK(!std::regex_search(baa, m, regex("a*"), rc::match_not_null | rc::match_continuous));
  CHECK(std::regex_search(aab, m, regex("a*"), rc::match_not_null | rc::match_continuous) && m.str() == "aa");
  CHECK(std::regex_search(baa, m, regex("a*"), rc::match_continuous) && m.length() == 0 && m.position() == 0);
  CHECK(std::regex_search(ab, m, regex("b|a b"), rc::match_continuous) && m.str() == "a b");

  // match_any: some match of the expression is returned.
  std::string abc = "abc";
  CHECK(std::regex_search(abc, m, regex("a|ab|bc"), rc::match_any));
  CHECK(m.str() == "a" || m.str() == "ab" || m.str() == "bc");
  CHECK(std::regex_match(m.str(), regex("a|ab|bc")));

  // Flags passed to regex_iterator apply to every search.
  using V = std::vector<std::pair<long, std::string>>;
  CHECK((all("baab", regex("a*"), rc::match_not_null) == V{{1, "aa"}}));
  CHECK((all("aab", regex("a"), rc::match_continuous) == V{{0, "a"}, {1, "a"}}));
  CHECK((all("a\nb\nc", regex("^.", ml), rc::match_not_bol) == V{{2, "b"}, {4, "c"}}));
  CHECK((all("a\nb\nc", regex(".$", ml), rc::match_not_eol) == V{{0, "a"}, {2, "b"}}));

  // regex_replace with match and format flags.
  CHECK(std::regex_replace(std::string("abc"), regex("x*"), "-", rc::match_not_null) == "abc");
  CHECK(std::regex_replace(std::string("aab"), regex("a"), "-", rc::match_continuous) == "--b");
  CHECK(std::regex_replace(std::string("bab"), regex("a"), "-", rc::match_continuous) == "bab");
  CHECK(std::regex_replace(std::string("a1b2c3"), regex("\\d"), "<$&>", rc::format_no_copy) == "<1><2><3>");
  CHECK(std::regex_replace(std::string("a1b2c3"), regex("\\d"), "<$&>",
                           rc::format_no_copy | rc::format_first_only) == "<1>");
  CHECK(std::regex_replace(std::string("a1b2c3"), regex("\\d"), "<$&>", rc::format_first_only) == "a<1>b2c3");
  CHECK(std::regex_replace(std::string("x\ny"), regex("^", ml), "> ", rc::match_not_bol) == "x\n> y");
  return 0;
}
