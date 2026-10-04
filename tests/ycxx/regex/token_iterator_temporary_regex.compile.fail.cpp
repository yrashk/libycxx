// [re.tokiter]: regex_token_iterator(BidirectionalIterator, BidirectionalIterator,
//   const regex_type&&, const vector<int>& submatches, ...) = delete; (likewise for every
// constructor taking a regex rvalue).
#include <regex>
#include <string>
#include <vector>

int main() {
  std::string s = "a,b";
  std::regex re(",");
  std::vector<int> subs{-1, 0};
  std::sregex_token_iterator ok(s.begin(), s.end(), re, subs);  // control
  (void)ok;
#ifndef YCXX_CONTROL
  std::sregex_token_iterator bad(s.begin(), s.end(), std::regex(","), subs);
  (void)bad;
#endif
}
