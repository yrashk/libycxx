// [string.view.hash]: hash<string_view>, hash<u8string_view>, hash<u16string_view>,
// hash<u32string_view>, hash<wstring_view> are enabled ([unord.hash]). Equal views hash
// equal regardless of where the characters live.
#include <string_view>
#include <cstddef>
#include <functional>
#include <type_traits>
#include "check.hpp"

template <class T>
constexpr bool enabled = std::is_default_constructible_v<std::hash<T>> && std::is_copy_constructible_v<std::hash<T>> &&
                         std::is_move_assignable_v<std::hash<T>> &&
                         std::is_same_v<decltype(std::hash<T>{}(std::declval<const T&>())), std::size_t>;

static_assert(enabled<std::string_view>);
static_assert(enabled<std::u8string_view>);
static_assert(enabled<std::u16string_view>);
static_assert(enabled<std::u32string_view>);
static_assert(enabled<std::wstring_view>);

int main() {
  const char buf1[] = "hello world";
  const char buf2[] = "say hello";
  std::string_view a(buf1, 5), b(buf2 + 4, 5);
  CHECK(a.data() != b.data());
  CHECK(std::hash<std::string_view>{}(a) == std::hash<std::string_view>{}(b));
  std::u16string_view c(u"abc"), d(u"xabc" + 1);
  CHECK(std::hash<std::u16string_view>{}(c) == std::hash<std::u16string_view>{}(d));
  std::hash<std::string_view>{}(std::string_view());
  return 0;
}
