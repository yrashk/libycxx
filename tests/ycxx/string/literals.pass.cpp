// [basic.string.literals]: operator""s(const charT* str, size_t len) returns
// basic_string{str, len} for char, char8_t, char16_t, char32_t and wchar_t; declared in
// std::literals::string_literals (both inline namespaces) and constexpr.
#include <string>
#include <type_traits>
#include "check.hpp"

void using_literals() {
  using namespace std::literals;
  static_assert(std::is_same_v<decltype("a"s), std::string>);
}
void using_string_literals() {
  using namespace std::string_literals;
  static_assert(std::is_same_v<decltype(u8"a"s), std::u8string>);
}
void using_both() {
  using namespace std::literals::string_literals;
  static_assert(std::is_same_v<decltype(u"a"s), std::u16string>);
}

using namespace std::string_literals;
static_assert(std::is_same_v<decltype(U"a"s), std::u32string>);
static_assert(std::is_same_v<decltype(L"a"s), std::wstring>);
static_assert("abc"s.size() == 3);
static_assert("a\0b"s.size() == 3);  // the length includes embedded nulls
static_assert(""s.empty());
static_assert(u8"é"s.size() == 2);
static_assert(std::operator""s("xyz", 2) == "xy");

int main() {
  auto s = "lit\0eral"s;
  CHECK(s.size() == 8 && s[3] == '\0');
  CHECK(L"w"s == L"w");
  return 0;
}
