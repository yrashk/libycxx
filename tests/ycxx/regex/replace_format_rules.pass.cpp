// [re.matchflag] format_default: "the new string shall be constructed using the rules used by
// the ECMAScript replace function in ECMA-262, part 15.5.4.11 String.prototype.replace": $$ is
// "$", $& the matched substring, $` the part before it, $' the part after it, $n (n a single digit
// 1-9 not followed by a decimal digit) the nth capture, $nn (01-99) the nnth capture; "If n<=m
// and the nth capture is undefined, use the empty String instead" (likewise for nn); the
// implementation-defined cases ($0, $00, n > m) are not checked. A '$' that does not start one of
// these sequences is not a replacement and is copied. format_sed: "the rules used by the sed
// utility in POSIX": & is the whole match, \n (1-9) the nth subexpression, \& a literal &, \\ a
// literal backslash; '$' has no meaning. [re.results.form]/1-3: match_results::format copies fmt
// with these replacements; the string-returning overloads agree with the iterator one.
#include <regex>
#include <string>
#include <iterator>
#include "check.hpp"

namespace rc = std::regex_constants;

int main() {
  std::smatch m;
  const std::string s = "<abcdefghijkl>";
  // Twelve groups.
  const std::regex twelve("(a)(b)(c)(d)(e)(f)(g)(h)(i)(j)(k)(l)");
  CHECK(std::regex_search(s, m, twelve));
  CHECK(m.format("$1") == "a");
  CHECK(m.format("$01") == "a");
  CHECK(m.format("$09") == "i");
  CHECK(m.format("$10") == "j");
  CHECK(m.format("$12") == "l");
  CHECK(m.format("$1x") == "ax");
  CHECK(m.format("$012") == "a2");  // $01 then the literal 2
  CHECK(m.format("$`|$'") == "<|>");
  CHECK(m.format("$$1") == "$1");
  CHECK(m.format("$$$&") == "$abcdefghijkl");
  CHECK(m.format("a$") == "a$");
  CHECK(m.format("$x$") == "$x$");
  CHECK(m.format("\\1&") == "\\1&");  // no sed meaning in the default format

  // A group that did not participate gives the empty string.
  const std::string t = "ac";
  const std::regex opt("a(b)?(c)");
  CHECK(std::regex_search(t, m, opt));
  CHECK(m.format("[$1][$01][$2]") == "[][][c]");
  CHECK(m.format("[\\1][\\2]", rc::format_sed) == "[][c]");

  // format_sed.
  CHECK(std::regex_search(s, m, twelve));
  CHECK(m.format("&", rc::format_sed) == "abcdefghijkl");
  CHECK(m.format("\\&", rc::format_sed) == "&");
  CHECK(m.format("\\\\", rc::format_sed) == "\\");
  CHECK(m.format("\\9\\1", rc::format_sed) == "ia");
  CHECK(m.format("\\10", rc::format_sed) == "a0");  // a single digit
  CHECK(m.format("$1 $$", rc::format_sed) == "$1 $$");
  CHECK(m.format("$&", rc::format_sed) == "$abcdefghijkl");  // & is the match even after '$'

  // The overloads agree.
  std::string out;
  m.format(std::back_inserter(out), std::string("$02-$11"));
  CHECK(out == "b-k");
  const char* f = "$02-$11";
  out.clear();
  m.format(std::back_inserter(out), f, f + 7);
  CHECK(out == "b-k");
  CHECK(m.format(f) == "b-k");

  // regex_replace applies the same rules for every match.
  CHECK(std::regex_replace(std::string("a-b c-d"), std::regex("(\\w)-(\\w)"), "$2$$$1") == "b$a d$c");
  CHECK(std::regex_replace(std::string("a-b c-d"), std::regex("(\\w)-(\\w)"), "\\2&\\1", rc::format_sed) ==
        "ba-ba dc-dc");
  CHECK(std::regex_replace(std::string("xay"), std::regex("a"), "[$`$']") == "x[xy]y");
  return 0;
}
