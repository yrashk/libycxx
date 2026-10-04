// [re.alg.search]: template<class ST, class SA, class Allocator, class charT, class traits>
//   bool regex_search(const basic_string<charT, ST, SA>&&, match_results<...>&,
//                     const basic_regex<charT, traits>&, match_flag_type = match_default) = delete;
#include <regex>
#include <string>

int main() {
  std::regex re("a+");
  (void)std::regex_search(std::string("baa"), re);  // control
  std::smatch m;
  std::string s("baa");
  (void)std::regex_search(s, m, re);                // control
#ifndef YCXX_CONTROL
  (void)std::regex_search(std::string("baa"), m, re);
#endif
}
