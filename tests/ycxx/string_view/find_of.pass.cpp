// [string.view.find]/9-20: find_first_of, find_last_of, find_first_not_of, find_last_not_of
// with basic_string_view, charT, (const charT*, pos, n) and (const charT*, pos) arguments.
#include <string_view>
#include <utility>
#include "check.hpp"

using SV = std::string_view;
constexpr auto npos = SV::npos;
static_assert(noexcept(std::declval<const SV&>().find_first_of(SV())));
static_assert(noexcept(std::declval<const SV&>().find_last_of('a')));
static_assert(noexcept(std::declval<const SV&>().find_first_not_of(SV())));
static_assert(noexcept(std::declval<const SV&>().find_last_not_of('a')));

constexpr bool test() {
  SV s("  hello, world  ");
  //    0123456789012345
  if (s.find_first_of(SV("lo")) != 4 || s.find_first_of(SV("lo"), 5) != 5) return false;
  if (s.find_first_of(SV("xyz")) != npos || s.find_first_of(SV()) != npos) return false;
  if (s.find_first_of(',') != 7 || s.find_first_of("wd") != 9 || s.find_first_of("wdx", 10, 2) != 13) return false;
  if (s.find_last_of(SV("lo")) != 12 || s.find_last_of(SV("lo"), 11) != 10) return false;
  if (s.find_last_of('h') != 2 || s.find_last_of('h', 1) != npos || s.find_last_of("") != npos) return false;
  if (s.find_last_of("he", 3, 1) != 2) return false;
  if (s.find_first_not_of(' ') != 2 || s.find_first_not_of(SV(" h")) != 3) return false;
  if (s.find_first_not_of(" ", 14) != npos) return false;
  if (s.find_first_not_of(SV()) != 0 || s.find_first_not_of(SV(), 16) != npos) return false;
  if (s.find_first_not_of(" he", 0, 1) != 2) return false;
  if (s.find_last_not_of(' ') != 13 || s.find_last_not_of(" d") != 12) return false;
  if (s.find_last_not_of(" ", 1) != npos || s.find_last_not_of(SV()) != 15) return false;
  if (s.find_last_not_of(" dl", npos, 2) != 12) return false;
  // pos beyond size
  if (s.find_first_of('h', 100) != npos || s.find_last_of('h', 100) != 2) return false;
  SV e;
  if (e.find_first_of("a") != npos || e.find_last_of("a") != npos) return false;
  if (e.find_first_not_of("a") != npos || e.find_last_not_of("a") != npos) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
