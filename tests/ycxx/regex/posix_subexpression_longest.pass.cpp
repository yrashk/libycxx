// [re.synopt] Table 118: extended selects the POSIX ERE grammar of IEEE 1003.1 (XBD 9), whose
// matching rule (XBD 9.1) is: "Consistent with the whole match being the longest of the leftmost
// matches, each subpattern, from left to right, shall match the longest possible string."
// (a|ab)(c|bcd)(d*) on "abcd": both a+bcd+"" and ab+c+d match all of "abcd"; the first
// subpattern takes the longer "ab".
// regexec() (XSH): a subexpression inside a repeated one reports its match in the last iteration
// (none if it did not take part there), also for counted repetitions.
#include <regex>
#include <string>
#include "check.hpp"

namespace rc = std::regex_constants;

int main() {
  std::string s = "abcd";
  std::smatch m;
  CHECK(std::regex_search(s, m, std::regex("(a|ab)(c|bcd)(d*)", rc::extended)));
  CHECK(m.str(0) == "abcd");
  CHECK(m.str(1) == "ab" && m.str(2) == "c" && m.str(3) == "d");
  CHECK(std::regex_match(s, m, std::regex("(a|ab)(c|bcd)(d*)", rc::extended)));
  CHECK(m.str(1) == "ab" && m.str(2) == "c" && m.str(3) == "d");
  // The same with awk and egrep (ERE-based).
  CHECK(std::regex_match(s, m, std::regex("(a|ab)(c|bcd)(d*)", rc::awk)) && m.str(1) == "ab");
  CHECK(std::regex_match(s, m, std::regex("(a|ab)(c|bcd)(d*)", rc::egrep)) && m.str(1) == "ab");
  // regexec() (XSH): a subexpression inside a repeated one reports its match within the last
  // iteration of the enclosing one; (a) does not take part in the last iteration "b".
  const std::string ab = "ab";
  for (const char* pat : {"((a)|b){2}", "((a)|b){2,3}", "((a)|b){1,}", "((a)|b)+"}) {
    CHECK(std::regex_match(ab, m, std::regex(pat, rc::extended)));
    CHECK(m.size() == 3 && m.str(1) == "b" && !m[2].matched);
  }
  return 0;
}
