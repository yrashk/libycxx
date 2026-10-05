// [string.view.find]/1-8: find and rfind for basic_string_view, charT, (const charT*, pos, n)
// and (const charT*, pos). find: lowest xpos with pos <= xpos and xpos + str.size() <= size();
// rfind: highest xpos with xpos <= pos and xpos + str.size() <= size(); npos otherwise.
// COUNTERPART: libcxx:strings/string.view/string.view.find/.*.pass.cpp
#include <string_view>
#include <utility>
#include "check.hpp"

using SV = std::string_view;
constexpr auto npos = SV::npos;
static_assert(noexcept(std::declval<const SV&>().find(SV())));
static_assert(noexcept(std::declval<const SV&>().find('a')));
static_assert(noexcept(std::declval<const SV&>().rfind(SV())));
static_assert(noexcept(std::declval<const SV&>().rfind('a')));

constexpr bool test() {
  SV s("abcabcab");
  if (s.find(SV("bc")) != 1 || s.find(SV("bc"), 2) != 4 || s.find(SV("bc"), 5) != npos) return false;
  if (s.find(SV("abcabcab")) != 0 || s.find(SV("abcabcabc")) != npos) return false;
  if (s.find('c') != 2 || s.find('c', 3) != 5 || s.find('z') != npos) return false;
  if (s.find("cab") != 2 || s.find("cab", 3) != 5) return false;
  if (s.find("abx", 1, 2) != 3) return false;  // only the first n characters of the needle
  // empty needle: found at pos if pos <= size()
  if (s.find(SV()) != 0 || s.find(SV(), 8) != 8 || s.find(SV(), 9) != npos) return false;
  if (s.find('a', 100) != npos) return false;

  if (s.rfind(SV("ab")) != 6 || s.rfind(SV("ab"), 5) != 3 || s.rfind(SV("ab"), 0) != 0) return false;
  if (s.rfind(SV("ca"), 1) != npos) return false;
  if (s.rfind('a') != 6 || s.rfind('a', 5) != 3 || s.rfind('z') != npos) return false;
  if (s.rfind("bc") != 4 || s.rfind("bcz", npos, 2) != 4) return false;
  // empty needle: highest xpos <= min(pos, size())
  if (s.rfind(SV()) != 8 || s.rfind(SV(), 3) != 3 || s.rfind(SV(), 100) != 8) return false;
  SV e;
  if (e.find(SV()) != 0 || e.rfind(SV()) != 0 || e.find('a') != npos || e.rfind('a') != npos) return false;
  // embedded nulls are ordinary characters
  constexpr char raw[] = {'x', '\0', 'y', '\0'};
  SV n(raw, 4);
  if (n.find('\0') != 1 || n.rfind('\0') != 3 || n.find(SV(raw + 1, 2)) != 1) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
