// [basic.string.general]/2: basic_string is a contiguous container ([container.reqmts]);
// with [range.access] / [range.req] it models contiguous_range, sized_range and
// common_range; it is not a view (it owns its characters and copying is linear, so
// enable_view is false) and not a borrowed_range (enable_borrowed_range is not specialized
// for it). ranges::range_value_t is charT and ranges::range_reference_t is charT&.
#include <string>
#include <ranges>
#include <type_traits>

template <class S, class C>
constexpr bool check() {
  static_assert(std::ranges::contiguous_range<S>);
  static_assert(std::ranges::contiguous_range<const S>);
  static_assert(std::ranges::sized_range<S>);
  static_assert(std::ranges::common_range<S>);
  static_assert(!std::ranges::view<S>);
  static_assert(!std::ranges::borrowed_range<S>);
  static_assert(!std::ranges::borrowed_range<S&&>);
  static_assert(std::ranges::borrowed_range<S&>);
  static_assert(std::ranges::viewable_range<S&>);
  static_assert(std::ranges::viewable_range<S>);
  static_assert(std::is_same_v<std::ranges::range_value_t<S>, C>);
  static_assert(std::is_same_v<std::ranges::range_reference_t<S>, C&>);
  static_assert(std::is_same_v<std::ranges::range_reference_t<const S>, const C&>);
  static_assert(std::is_same_v<std::ranges::range_size_t<S>, typename S::size_type>);
  static_assert(std::is_same_v<std::ranges::iterator_t<S>, typename S::iterator>);
  static_assert(std::is_same_v<std::ranges::iterator_t<const S>, typename S::const_iterator>);
  static_assert(std::is_same_v<decltype(std::views::all(std::declval<S&>())), std::ranges::ref_view<S>>);
  static_assert(std::is_same_v<decltype(std::views::all(std::declval<S>())), std::ranges::owning_view<S>>);
  return true;
}
static_assert(check<std::string, char>());
static_assert(check<std::wstring, wchar_t>());
static_assert(check<std::u8string, char8_t>());
static_assert(check<std::u16string, char16_t>());
static_assert(check<std::u32string, char32_t>());

constexpr bool data_and_size() {
  std::string s = "hello";
  return std::ranges::data(s) == s.data() && std::ranges::size(s) == 5 && std::ranges::cdata(s) == s.data() &&
         std::ranges::begin(s) == s.begin() && std::ranges::end(s) == s.end();
}
static_assert(data_and_size());

int main() {}
