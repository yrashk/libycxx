// [string.require]/3: basic_string uses its traits parameter for all character operations
// (compare, find, length, eq, ...). A case-insensitive traits class changes searching and
// comparison accordingly ([string.find], [string.compare], [string.cmp] are all defined via
// basic_string_view<charT, traits>).
#include <string>
#include <cstddef>
#include <string_view>
#include "check.hpp"

constexpr char up(char c) { return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c; }

struct CiTraits : std::char_traits<char> {
  static constexpr bool eq(char a, char b) noexcept { return up(a) == up(b); }
  static constexpr bool lt(char a, char b) noexcept {
    return static_cast<unsigned char>(up(a)) < static_cast<unsigned char>(up(b));
  }
  static constexpr int compare(const char* a, const char* b, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
      if (lt(a[i], b[i])) return -1;
      if (lt(b[i], a[i])) return 1;
    }
    return 0;
  }
  static constexpr const char* find(const char* s, std::size_t n, const char& c) {
    for (std::size_t i = 0; i < n; ++i)
      if (eq(s[i], c)) return s + i;
    return nullptr;
  }
};
using CiString = std::basic_string<char, CiTraits>;
using CiView = std::basic_string_view<char, CiTraits>;

constexpr bool test() {
  CiString s = "Hello World";
  if (s != "hello world" || !(s == "HELLO WORLD")) return false;
  if (s.find("WORLD") != 6 || s.find('o') != 4 || s.rfind('O') != 7) return false;
  if (s.find_first_of("LW") != 2 || s.find_first_not_of("heL") != 4) return false;
  if (!s.starts_with("hElLo") || !s.ends_with('D') || !s.contains("O W")) return false;
  if (s.compare("HELLO WORLD") != 0 || s.compare(0, 5, "hello") != 0) return false;
  if (!(CiString("abc") < CiString("ABD"))) return false;
  CiView v = s;
  if (v != CiView("HELLO world")) return false;
  if (std::erase(s, 'l') != 3 || s != "heo word") return false;  // erase uses ==, not traits
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
