// [string.view.deduct]: basic_string_view(It, End) -> basic_string_view<iter_value_t<It>>;
// basic_string_view(R&&) -> basic_string_view<ranges::range_value_t<R>>. Also deduction
// from const charT* via the implicit guide of basic_string_view(const charT*).
#include <string_view>
#include <array>
#include <type_traits>
#include "check.hpp"

int main() {
  const char* p = "abc";
  std::basic_string_view a(p);
  static_assert(std::is_same_v<decltype(a), std::string_view>);
  std::basic_string_view b(L"xy", 1);
  static_assert(std::is_same_v<decltype(b), std::wstring_view>);
  std::basic_string_view c(p, p + 2);
  static_assert(std::is_same_v<decltype(c), std::string_view>);
  std::array<char16_t, 2> arr{u'a', u'b'};
  std::basic_string_view d(arr.begin(), arr.end());
  static_assert(std::is_same_v<decltype(d), std::u16string_view>);
  std::basic_string_view e(arr);
  static_assert(std::is_same_v<decltype(e), std::u16string_view>);
  std::basic_string_view f(U"z");
  static_assert(std::is_same_v<decltype(f), std::u32string_view>);
  std::basic_string_view g(a);
  static_assert(std::is_same_v<decltype(g), std::string_view>);
  CHECK(a.size() == 3 && b.size() == 1 && c.size() == 2 && d.size() == 2 && e.size() == 2 && f.size() == 1);
  return 0;
}
