// [string.cmp]: operator== and operator<=> between basic_strings and between a
// basic_string and a const charT*, each equivalent to the basic_string_view comparison; so
// the <=> result type is that of basic_string_view: traits::comparison_category if present,
// otherwise weak_ordering ([string.view.comparison]). != , <, >, <=, >= are synthesized and
// work in both argument orders. operator==(basic_string, basic_string) is noexcept.
// COUNTERPART: libcxx:strings/basic.string/string.nonmembers/string.cmp/comparison.pass.cpp
#include <string>
#include <compare>
#include <string_view>
#include <type_traits>
#include <utility>
#include "check.hpp"

// Character traits without a comparison_category member.
struct PlainTraits {
  using char_type = char;
  using int_type = int;
  using off_type = std::char_traits<char>::off_type;
  using pos_type = std::char_traits<char>::pos_type;
  using state_type = std::char_traits<char>::state_type;
  static constexpr void assign(char& a, const char& b) noexcept { a = b; }
  static constexpr bool eq(char a, char b) noexcept { return a == b; }
  static constexpr bool lt(char a, char b) noexcept { return (unsigned char)a < (unsigned char)b; }
  static constexpr int compare(const char* a, const char* b, std::size_t n) {
    return std::char_traits<char>::compare(a, b, n);
  }
  static constexpr std::size_t length(const char* s) { return std::char_traits<char>::length(s); }
  static constexpr const char* find(const char* s, std::size_t n, const char& c) {
    return std::char_traits<char>::find(s, n, c);
  }
  static constexpr char* move(char* d, const char* s, std::size_t n) {
    return std::char_traits<char>::move(d, s, n);
  }
  static constexpr char* copy(char* d, const char* s, std::size_t n) {
    return std::char_traits<char>::copy(d, s, n);
  }
  static constexpr char* assign(char* s, std::size_t n, char c) {
    return std::char_traits<char>::assign(s, n, c);
  }
  static constexpr int not_eof(int c) noexcept { return c == -1 ? 0 : c; }
  static constexpr char to_char_type(int c) noexcept { return static_cast<char>(c); }
  static constexpr int to_int_type(char c) noexcept { return static_cast<unsigned char>(c); }
  static constexpr bool eq_int_type(int a, int b) noexcept { return a == b; }
  static constexpr int eof() noexcept { return -1; }
};
using PS = std::basic_string<char, PlainTraits>;

static_assert(std::is_same_v<decltype(std::string() <=> std::string()), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::string() <=> ""), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::wstring() <=> std::wstring()), std::strong_ordering>);
static_assert(std::is_same_v<decltype(PS() <=> PS()), std::weak_ordering>);
static_assert(std::is_same_v<decltype(PS() <=> ""), std::weak_ordering>);
static_assert(std::is_same_v<decltype(std::string() == std::string()), bool>);
static_assert(noexcept(std::declval<const std::string&>() == std::declval<const std::string&>()));
static_assert(noexcept(std::declval<const std::string&>() <=> std::declval<const std::string&>()));

constexpr bool test() {
  const std::string a = "abc", b = "abd", c = "abc", p = "ab";
  if (!(a == c) || a == b || !(a != b) || a != c) return false;
  if (!(a < b) || !(b > a) || !(a <= c) || !(a >= c) || !(p < a)) return false;
  if ((a <=> b) != std::strong_ordering::less || (a <=> c) != 0 || (b <=> p) <= 0) return false;
  if (!(a == "abc") || !("abc" == a) || a != "abc" || "abd" == a) return false;
  if (!(a < "abd") || !("abd" > a) || !("ab" < a) || ("abc" <=> a) != 0) return false;
  // a string_view compares through the basic_string_view operators
  if (!(a == std::string_view("abc")) || !(std::string_view("abd") > a)) return false;
  // embedded nulls
  const std::string z1("a\0b", 3), z2("a\0c", 3);
  if (z1 == z2 || !(z1 < z2) || z1 == "a") return false;
  const std::string hi("\x80");
  if (!(std::string("a") < hi)) return false;  // unsigned comparison of char
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  PS x = "abc", y = "abd";
  CHECK(x < y && x == "abc" && (x <=> y) == std::weak_ordering::less);
  return 0;
}
