// [string.accessors]/3: operator basic_string_view<charT, traits>() const noexcept is
// basic_string_view(data(), size()) — an implicit conversion that views the string's own
// storage, embedded nulls included. get_allocator() returns the stored allocator (/4).
#include <string>
#include <string_view>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_convertible_v<const std::string&, std::string_view>);
static_assert(std::is_convertible_v<const std::wstring&, std::wstring_view>);
static_assert(!std::is_convertible_v<const std::string&, std::wstring_view>);
static_assert(!std::is_convertible_v<const std::string&, const char*>);

constexpr bool takes_view(std::string_view v, std::size_t n) { return v.size() == n; }

constexpr bool test() {
  std::string s("ab\0cd", 5);
  std::string_view v = s;
  if (v.data() != s.data() || v.size() != 5) return false;
  if (!takes_view(s, 5)) return false;
  std::u8string u = u8"x";
  std::u8string_view uv = u;
  if (uv.size() != 1 || uv.data() != u.data()) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
