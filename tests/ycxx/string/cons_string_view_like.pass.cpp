// [string.cons]/11-12: template<class T> explicit basic_string(const T& t, const Allocator& =
// Allocator()). Constraints: is_convertible_v<const T&, basic_string_view<charT, traits>> is
// true and is_convertible_v<const T&, const charT*> is false. Effects: as if
// basic_string_view sv = t; basic_string(sv.data(), sv.size(), a).
#include <string>
#include <string_view>
#include <type_traits>
#include "test_allocators.hpp"
#include "check.hpp"

// Converts only to string_view.
struct ViewLike {
  constexpr operator std::string_view() const { return std::string_view("view\0like", 9); }
};
// Converts to both string_view and const char*: the template is not viable, so the
// const charT* constructor is used (traits::length stops at the embedded null).
struct BothLike {
  constexpr operator std::string_view() const { return std::string_view("from-view"); }
  constexpr operator const char*() const { return "ptr\0tail"; }
};

static_assert(std::is_constructible_v<std::string, std::string_view>);
static_assert(!std::is_convertible_v<std::string_view, std::string>);  // explicit
static_assert(std::is_constructible_v<std::string, ViewLike>);
static_assert(!std::is_convertible_v<ViewLike, std::string>);
static_assert(std::is_constructible_v<std::string, std::string_view, std::allocator<char>>);
// A view of another character type does not convert.
static_assert(!std::is_constructible_v<std::string, std::wstring_view>);
static_assert(!std::is_constructible_v<std::string, std::u8string_view>);

constexpr bool test() {
  std::string_view sv("abc\0def", 7);
  std::string s(sv);
  if (s.size() != 7 || s[4] != 'd') return false;
  std::string e(std::string_view{});
  if (!e.empty()) return false;
  std::string v{ViewLike{}};
  if (v.size() != 9 || v[4] != '\0') return false;
  std::string b{BothLike{}};
  if (b != "ptr") return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  using S = std::basic_string<char, std::char_traits<char>, IdAlloc<char>>;
  S s(std::string_view("hi"), IdAlloc<char>(5));
  CHECK(s == "hi");
  CHECK(s.get_allocator().id == 5);
  return 0;
}
