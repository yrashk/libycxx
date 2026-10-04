// [string.erasure]: erase(c, value) removes every element equal to value and erase_if(c,
// pred) every element satisfying pred, keeping the relative order of the rest; both return
// the number of elements removed. U defaults to charT, so a braced-init-list works.
#include <string>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<decltype(std::erase(std::declval<std::string&>(), 'a')), std::string::size_type>);
static_assert(std::is_same_v<decltype(std::erase_if(std::declval<std::string&>(), [](char) { return true; })),
                             std::string::size_type>);

constexpr bool test() {
  std::string s = "banana bandana";
  if (std::erase(s, 'a') != 6 || s != "bnn bndn") return false;
  if (std::erase(s, 'z') != 0 || s != "bnn bndn") return false;
  if (std::erase(s, {'n'}) != 4 || s != "b bd") return false;  // U = charT by default
  if (std::erase(s, 32) != 1 || s != "bbd") return false;      // compared as int
  std::string t = "a1b2c3";
  if (std::erase_if(t, [](char c) { return c >= '0' && c <= '9'; }) != 3 || t != "abc") return false;
  if (std::erase_if(t, [](char) { return true; }) != 3 || !t.empty()) return false;
  if (std::erase_if(t, [](char) { return true; }) != 0) return false;
  std::string n("x\0y\0", 4);
  if (std::erase(n, '\0') != 2 || n != "xy") return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  std::wstring w = L"w-i-d-e";
  CHECK(std::erase(w, L'-') == 3 && w == L"wide");
  return 0;
}
