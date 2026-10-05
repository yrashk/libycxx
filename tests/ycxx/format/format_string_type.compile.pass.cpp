// [format.fmt.string], [format.syn]: format_string<Args...> and wformat_string<Args...> are
// basic_format_string<charT, type_identity_t<Args>...>; get() returns the string_view and is
// noexcept; the consteval constructor requires a type convertible to basic_string_view.
// REQUIRES: exceptions
#include <format>
#include <string_view>
#include <type_traits>

static_assert(std::is_same_v<std::format_string<int, char>, std::basic_format_string<char, int, char>>);
static_assert(std::is_same_v<std::wformat_string<int>, std::basic_format_string<wchar_t, int>>);
static_assert(std::is_same_v<decltype(std::declval<const std::format_string<int>&>().get()), std::string_view>);
static_assert(noexcept(std::declval<const std::format_string<int>&>().get()));
static_assert(!std::is_constructible_v<std::format_string<int>, int>);
static_assert(!std::is_constructible_v<std::format_string<int>, const wchar_t*>);

consteval bool check() {
  std::format_string<int> f = "{:>4}";
  return f.get() == "{:>4}" && f.get().size() == 5;
}
static_assert(check());

// A user formatter with its own format-spec syntax is checked at compile time through its
// parse member.
struct Dummy {};
template <>
struct std::formatter<Dummy, char> {
  constexpr auto parse(std::format_parse_context& ctx) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it == 'q') ++it;
    if (it != ctx.end() && *it != '}') throw std::format_error("bad");
    return it;
  }
  auto format(Dummy, std::format_context& ctx) const { return ctx.out(); }
};
consteval bool user() {
  std::format_string<Dummy> a = "{:q}";
  std::format_string<Dummy> b = "{}";
  return a.get().size() == 4 && b.get().size() == 2;
}
static_assert(user());
