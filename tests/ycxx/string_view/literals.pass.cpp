// [string.view.literals]: operator""sv for char, char8_t, char16_t, char32_t and wchar_t,
// constexpr and noexcept, returning e.g. string_view{str, len} (so embedded nulls are kept).
// The operators are in inline namespaces std::literals::string_view_literals.
#include <string_view>
#include <type_traits>
#include "check.hpp"

constexpr bool test() {
  using namespace std::literals::string_view_literals;
  auto a = "ab\0c"sv;
  static_assert(std::is_same_v<decltype(a), std::string_view>);
  if (a.size() != 4 || a[3] != 'c') return false;
  auto b = u8"xy"sv;
  static_assert(std::is_same_v<decltype(b), std::u8string_view>);
  auto c = u"xyz"sv;
  static_assert(std::is_same_v<decltype(c), std::u16string_view>);
  auto d = U"x"sv;
  static_assert(std::is_same_v<decltype(d), std::u32string_view>);
  auto e = L"wide"sv;
  static_assert(std::is_same_v<decltype(e), std::wstring_view>);
  static_assert(noexcept("x"sv));
  if (b.size() != 2 || c.size() != 3 || d.size() != 1 || e.size() != 4) return false;
  if (""sv.size() != 0) return false;
  return true;
}
static_assert(test());

namespace inline_ns {
using namespace std::literals;
static_assert(std::is_same_v<decltype("a"sv), std::string_view>);
}
namespace inline_ns2 {
using namespace std::string_view_literals;
static_assert("abc"sv.size() == 3);
}
namespace inline_ns3 {
using namespace std;
static_assert("abcd"sv.size() == 4);
}

int main() {
  CHECK(test());
  return 0;
}
