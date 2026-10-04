// [re.results.const]: a default-constructed match_results is not ready(), size() 0, empty().
// [re.results.state]: ready() is true after a call to regex_match/regex_search.
// [re.alg.match]/[re.alg.search]: on failure m.ready() is true, m.size() == 0 and m.empty();
// on success size() == 1 + mark_count(), prefix() and suffix() describe the text before and
// after the match (matched iff non-empty), m[0].matched is true.
// [re.results.acc]/8: operator[](n) with n >= size() returns an unmatched sub_match;
// /13-14 begin()/end() enumerate size() sub-expressions; position(n), length(n), str(n).
// [re.results.form]: format with $n for a group that did not participate gives the empty
// string; $nn uses two digits when that names a group; format_sed: \0? & and \n; format_default
// leaves sed escapes alone.
// [re.results.swap], [re.results.eq]: swap exchanges, == compares (empty == empty; equal
// prefix, suffix and sub-expressions).
#include <iterator>
#include <regex>
#include <string>
#include "check.hpp"

namespace rc = std::regex_constants;

int main() {
  std::cmatch m;
  CHECK(!m.ready() && m.size() == 0 && m.empty() && m.begin() == m.end());

  CHECK(!std::regex_search("abc", m, std::regex("z")));
  CHECK(m.ready() && m.size() == 0 && m.empty());
  CHECK(!std::regex_match("abc", m, std::regex("ab")));  // regex_match needs the whole sequence
  CHECK(m.ready() && m.empty());

  const char* text = "xx12-34yy";
  std::regex re("(\\d+)-(\\d+)(z)?");
  CHECK(std::regex_search(text, m, re));
  CHECK(m.ready() && !m.empty() && m.size() == 4 && re.mark_count() == 3);
  CHECK(m[0].matched && m[0].first == text + 2 && m[0].second == text + 7);
  CHECK(m.prefix().first == text && m.prefix().second == text + 2 && m.prefix().matched);
  CHECK(m.suffix().first == text + 7 && m.suffix().str() == "yy" && m.suffix().matched);
  CHECK(!m[3].matched && m.length(3) == 0 && m.str(3) == "");
  CHECK(m.position(1) == 2 && m.length(1) == 2 && m.position(2) == 5 && m.str(2) == "34");
  CHECK(!m[4].matched && !m[100].matched && m[100].length() == 0);
  CHECK(std::distance(m.begin(), m.end()) == 4 && m.cbegin() == m.begin());
  CHECK(m.begin()->str() == "12-34");

  // A match at the very start / end: empty prefix / suffix are not matched.
  CHECK(std::regex_search("12-3", m, re));
  CHECK(!m.prefix().matched && m.prefix().length() == 0 && !m.suffix().matched);
  CHECK(std::regex_match("12-3", m, re) && !m.prefix().matched && !m.suffix().matched);

  // format
  CHECK(std::regex_search(text, m, re));
  CHECK(m.format("<$3>") == "<>");
  CHECK(m.format("$1$2") == "1234");
  CHECK(m.format("$01|$02") == "12|34");
  CHECK(m.format("[$&]", rc::format_default) == "[12-34]");
  CHECK(m.format("\\1-&", rc::format_sed) == "12-12-34");
  CHECK(m.format("\\2\\&", rc::format_sed) == "34&");
  CHECK(m.format("\\1&", rc::format_default) == "\\1&");  // sed escapes are not special by default
  CHECK(m.format("$1&", rc::format_sed) == "$112-34");    // $ is not special with format_sed
  std::string fmt = "$2";
  std::string out;
  m.format(std::back_inserter(out), fmt.data(), fmt.data() + fmt.size());
  CHECK(out == "34");

  // swap / ==
  std::cmatch a, b;
  CHECK(a == b);
  std::regex_search(text, a, re);
  CHECK(a != b);
  a.swap(b);
  CHECK(a.empty() && b.str(1) == "12");
  swap(a, b);
  CHECK(a.str(2) == "34" && b.empty());
  std::cmatch c;
  std::regex_search(text, c, re);
  CHECK(a == c);
  std::regex_search(text, c, std::regex("(\\d+)-(\\d+)(y)?"));
  CHECK(a != c);

  // Copy keeps the results.
  std::cmatch d = a;
  CHECK(d == a && d.str(1) == "12" && d.ready());
  return 0;
}
