// [char.traits.specializations.char], [char.traits.specializations.char8.t],
// [char.traits.specializations.char16.t], [char.traits.specializations.char32.t],
// [char.traits.specializations.wchar.t]: the member types of the five specializations.
// char: int_type = int; char8_t: unsigned int; char16_t: uint_least16_t; char32_t:
// uint_least32_t; wchar_t: wint_t. state_type = mbstate_t and comparison_category =
// strong_ordering for all five. char_traits is reached through <string_view>
// ([string.view.synop]: basic_string_view<charT, traits = char_traits<charT>>).
#include <string_view>
#include <compare>
#include <cstdint>
#include <cwchar>
#include <type_traits>

template <class C, class Int>
constexpr bool check() {
  using T = std::char_traits<C>;
  static_assert(std::is_same_v<typename T::char_type, C>);
  static_assert(std::is_same_v<typename T::int_type, Int>);
  static_assert(std::is_same_v<typename T::state_type, std::mbstate_t>);
  static_assert(std::is_same_v<typename T::comparison_category, std::strong_ordering>);
  static_assert(std::is_same_v<typename std::basic_string_view<C>::traits_type, T>);
  // [char.traits.typedefs]/2: state_type is Cpp17Destructible, CopyAssignable,
  // CopyConstructible and DefaultConstructible.
  static_assert(std::is_default_constructible_v<typename T::state_type>);
  static_assert(std::is_copy_constructible_v<typename T::state_type>);
  static_assert(std::is_copy_assignable_v<typename T::state_type>);
  static_assert(std::is_destructible_v<typename T::state_type>);
  return true;
}

static_assert(check<char, int>());
static_assert(check<char8_t, unsigned int>());
static_assert(check<char16_t, std::uint_least16_t>());
static_assert(check<char32_t, std::uint_least32_t>());
static_assert(check<wchar_t, std::wint_t>());

// The class template itself is declared and usable as a name ([char.traits.general]/2).
template <template <class> class TT>
struct takes_template {};
using U = takes_template<std::char_traits>;
