// EXPECT-ERROR-GCC: error: use of deleted function [^\n]*std::regex_match\(
// EXPECT-ERROR-CLANG: error: call to deleted function 'regex_match'
// [re.alg.match]: template<class ST, class SA, class Allocator, class charT, class traits>
//   bool regex_match(const basic_string<charT, ST, SA>&&, match_results<...>&,
//                    const basic_regex<charT, traits>&, match_flag_type = match_default) = delete;
// The overload without match_results is not deleted (the control).
#include <regex>
#include <string>

int main() {
  std::regex re("a+");
  (void)std::regex_match(std::string("aa"), re);  // control
  std::smatch m;
  std::string s("aa");
  (void)std::regex_match(s, m, re);               // control
#ifndef YCXX_CONTROL
  (void)std::regex_match(std::string("aa"), m, re);
#endif
}
