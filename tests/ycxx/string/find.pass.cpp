// [string.find]: find, rfind, find_first_of, find_last_of, find_first_not_of,
// find_last_not_of for basic_string, const charT* (with and without n), charT and
// string-view-like T; each is equivalent to the basic_string_view operation (npos mapped to
// basic_string::npos). Default pos is 0 for the forward searches and npos for the
// backward ones.
#include <string>
#include <string_view>
#include "check.hpp"

constexpr auto npos = std::string::npos;

constexpr bool test() {
  const std::string s = "abcabcXYZ";
  // find
  if (s.find(std::string("bc")) != 1) return false;
  if (s.find(std::string("bc"), 2) != 4) return false;
  if (s.find("bc", 5) != npos) return false;
  if (s.find("bcX", 0, 2) != 1) return false;
  if (s.find('X') != 6) return false;
  if (s.find('X', 7) != npos) return false;
  if (s.find(std::string_view("YZ")) != 7) return false;
  if (s.find("") != 0 || s.find("", 9) != 9 || s.find("", 10) != npos) return false;
  // rfind
  if (s.rfind(std::string("abc")) != 3) return false;
  if (s.rfind("abc", 2) != 0) return false;
  if (s.rfind("abcQ", npos, 3) != 3) return false;
  if (s.rfind('a') != 3 || s.rfind('a', 2) != 0) return false;
  if (s.rfind(std::string_view("Q")) != npos) return false;
  if (s.rfind("") != 9) return false;
  // find_first_of
  if (s.find_first_of(std::string("ZYc")) != 2) return false;
  if (s.find_first_of("ZYc", 3) != 5) return false;
  if (s.find_first_of("ZYc", 0, 2) != 7) return false;
  if (s.find_first_of('c') != 2) return false;
  if (s.find_first_of(std::string_view("qrs")) != npos) return false;
  if (s.find_first_of("") != npos) return false;
  // find_last_of
  if (s.find_last_of(std::string("ab")) != 4) return false;
  if (s.find_last_of("ab", 3) != 3) return false;
  if (s.find_last_of("abX", 8, 2) != 4) return false;
  if (s.find_last_of('a', 2) != 0) return false;
  if (s.find_last_of(std::string_view("Z")) != 8) return false;
  // find_first_not_of
  if (s.find_first_not_of(std::string("abc")) != 6) return false;
  if (s.find_first_not_of("abc", 7) != 7) return false;
  if (s.find_first_not_of("abcX", 0, 3) != 6) return false;
  if (s.find_first_not_of('a') != 1) return false;
  if (s.find_first_not_of(std::string_view("abcXYZ")) != npos) return false;
  if (s.find_first_not_of("") != 0) return false;
  // find_last_not_of
  if (s.find_last_not_of(std::string("XYZ")) != 5) return false;
  if (s.find_last_not_of("XYZc", 5) != 4) return false;
  if (s.find_last_not_of("ZYXc", npos, 2) != 6) return false;
  if (s.find_last_not_of('Z') != 7) return false;
  if (s.find_last_not_of(std::string_view("abcXYZ")) != npos) return false;
  // empty string
  const std::string e;
  if (e.find('a') != npos || e.rfind('a') != npos || e.find("") != 0 || e.rfind("") != 0) return false;
  if (e.find_first_of("a") != npos || e.find_last_not_of("a") != npos) return false;
  // embedded null characters are ordinary characters
  const std::string z("a\0b\0c", 5);
  if (z.find('\0') != 1 || z.rfind('\0') != 3) return false;
  if (z.find(std::string("\0c", 2)) != 3) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  const std::wstring w = L"wide wide";
  CHECK(w.find(L"de") == 2);
  CHECK(w.rfind(L'w') == 5);
  CHECK(w.find_first_of(L" ") == 4);
  return 0;
}
