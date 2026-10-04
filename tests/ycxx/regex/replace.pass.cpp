// [re.alg.replace], [re.results.form]: regex_replace copies non-matching text and replaces each
// match by the format string: $& (whole match), $n / $nn (groups), $` (prefix), $' (suffix),
// $$ (a $) in the ECMAScript format; format_sed uses & and \n; format_first_only replaces only
// the first match; format_no_copy drops the unmatched text. match_results::format applies the
// same rules to one match.
#include <regex>
#include <iterator>
#include <string>
#include "check.hpp"

namespace rc = std::regex_constants;

int main() {
  std::string s = "John Smith, Jane Doe";
  std::regex name("(\\w+) (\\w+)");
  CHECK(std::regex_replace(s, name, "$2 $1") == "Smith John, Doe Jane");
  CHECK(std::regex_replace(s, name, "[$&]") == "[John Smith], [Jane Doe]");
  CHECK(std::regex_replace(s, name, "$2", rc::format_first_only) == "Smith, Jane Doe");
  CHECK(std::regex_replace(s, name, "$1;", rc::format_no_copy) == "John;Jane;");
  CHECK(std::regex_replace(s, name, "\\2 & \\1", rc::format_sed) == "Smith John Smith John, Doe Jane Doe Jane");
  CHECK(std::regex_replace("cost: 5", std::regex("\\d"), "$$$&") == "cost: $5");
  CHECK(std::regex_replace("a-b", std::regex("-"), "[$`|$']") == "a[a|b]b");
  CHECK(std::regex_replace("abc", std::regex("x"), "y") == "abc");
  CHECK(std::regex_replace(std::string("aaa"), std::regex("a"), std::string("bb")) == "bbbbbb");
  CHECK(std::regex_replace("abc", std::regex(""), "-") == "-a-b-c-");

  std::string out;
  std::regex_replace(std::back_inserter(out), s.begin(), s.end(), name, "<$1>");
  CHECK(out == "<John>, <Jane>");

  std::smatch m;
  CHECK(std::regex_search(s, m, name));
  CHECK(m.format("$2/$1/$&/$$") == "Smith/John/John Smith/$");
  CHECK(m.format("$`|$'") == "|, Jane Doe");
  CHECK(m.format("\\1 &", rc::format_sed) == "John John Smith");
  std::string f;
  m.format(std::back_inserter(f), std::string("$1!"));
  CHECK(f == "John!");
  return 0;
}
