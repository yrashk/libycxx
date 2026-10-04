// [basic.string.hash]/1: hash<S> is enabled for the five string types with any allocator,
// and hash<S>()(s) == hash<SV>()(SV(s)) for the corresponding string view type.
// [unord.hash]: enabled specializations are default constructible, callable on const S&,
// return size_t.
#include <string>
#include <cstddef>
#include <functional>
#include <string_view>
#include <type_traits>
#include "test_allocators.hpp"
#include "check.hpp"

template <class S, class SV>
void check(const S& s) {
  static_assert(std::is_default_constructible_v<std::hash<S>>);
  static_assert(std::is_same_v<decltype(std::hash<S>{}(s)), std::size_t>);
  CHECK(std::hash<S>{}(s) == std::hash<SV>{}(SV(s)));
}

int main() {
  check<std::string, std::string_view>("hello");
  check<std::string, std::string_view>(std::string("a\0b", 3));
  check<std::string, std::string_view>("");
  check<std::wstring, std::wstring_view>(L"wide");
  check<std::u8string, std::u8string_view>(u8"utf8");
  check<std::u16string, std::u16string_view>(u"utf16");
  check<std::u32string, std::u32string_view>(U"utf32");
  using MS = std::basic_string<char, std::char_traits<char>, MinimalAlloc<char>>;
  check<MS, std::string_view>(MS("custom allocator"));
  CHECK(std::hash<MS>{}(MS("x")) == std::hash<std::string>{}(std::string("x")));
  return 0;
}
