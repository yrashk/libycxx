// [string.view.template.general]: member types; "iterator = const_iterator";
// "reverse_iterator = const_reverse_iterator"; npos = size_type(-1); "basic_string_view<charT,
// traits> is a trivially copyable type". [string.view.synop]: typedef-names and
// enable_view / enable_borrowed_range. [string.view.iterators]/1: const_iterator models
// contiguous_iterator with value_type charT.
#include <string_view>
#include <cstddef>
#include <iterator>
#include <type_traits>

template <class C>
constexpr bool check() {
  using SV = std::basic_string_view<C>;
  static_assert(std::is_same_v<typename SV::value_type, C>);
  static_assert(std::is_same_v<typename SV::pointer, C*>);
  static_assert(std::is_same_v<typename SV::const_pointer, const C*>);
  static_assert(std::is_same_v<typename SV::reference, C&>);
  static_assert(std::is_same_v<typename SV::const_reference, const C&>);
  static_assert(std::is_same_v<typename SV::iterator, typename SV::const_iterator>);
  static_assert(std::is_same_v<typename SV::const_reverse_iterator, std::reverse_iterator<typename SV::const_iterator>>);
  static_assert(std::is_same_v<typename SV::reverse_iterator, typename SV::const_reverse_iterator>);
  static_assert(std::is_same_v<typename SV::size_type, std::size_t>);
  static_assert(std::is_same_v<typename SV::difference_type, std::ptrdiff_t>);
  static_assert(std::is_same_v<decltype(SV::npos), const std::size_t>);
  static_assert(SV::npos == std::size_t(-1));
  static_assert(std::contiguous_iterator<typename SV::const_iterator>);
  static_assert(std::is_same_v<std::iter_value_t<typename SV::const_iterator>, C>);
  static_assert(std::is_same_v<std::iter_reference_t<typename SV::const_iterator>, const C&>);
  static_assert(std::is_trivially_copyable_v<SV>);
  static_assert(std::is_nothrow_default_constructible_v<SV>);
  static_assert(std::is_nothrow_copy_constructible_v<SV>);
  static_assert(std::is_nothrow_copy_assignable_v<SV>);
  static_assert(std::ranges::enable_view<SV>);
  static_assert(std::ranges::enable_borrowed_range<SV>);
  return true;
}
static_assert(check<char>());
static_assert(check<wchar_t>());
static_assert(check<char8_t>());
static_assert(check<char16_t>());
static_assert(check<char32_t>());

static_assert(std::is_same_v<std::string_view, std::basic_string_view<char>>);
static_assert(std::is_same_v<std::wstring_view, std::basic_string_view<wchar_t>>);
static_assert(std::is_same_v<std::u8string_view, std::basic_string_view<char8_t>>);
static_assert(std::is_same_v<std::u16string_view, std::basic_string_view<char16_t>>);
static_assert(std::is_same_v<std::u32string_view, std::basic_string_view<char32_t>>);
