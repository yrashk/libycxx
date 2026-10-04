// [type.index]: type_index has ==, <, >, <=, >= and <=> returning strong_ordering, all const
// noexcept, an implicit converting constructor from const type_info& and implicitly declared
// copy/move operations (the exposition-only member is a pointer "so that the default copy/move
// constructor and assignment operators will be provided"); there is no default constructor.
// So type_index models totally_ordered and three_way_comparable<strong_ordering> and is
// copyable; and the comparison function objects ([comparisons], [range.cmp]) accept it.
#include <typeindex>
#include <compare>
#include <concepts>
#include <functional>
#include <type_traits>

using std::type_index;
static_assert(std::equality_comparable<type_index>);
static_assert(std::totally_ordered<type_index>);
static_assert(std::three_way_comparable<type_index, std::strong_ordering>);
static_assert(std::is_same_v<std::compare_three_way_result_t<type_index>, std::strong_ordering>);
static_assert(std::copyable<type_index>);
static_assert(!std::default_initializable<type_index>);
static_assert(!std::regular<type_index>);
static_assert(std::is_nothrow_move_constructible_v<type_index>);
static_assert(std::is_nothrow_destructible_v<type_index>);
static_assert(std::is_convertible_v<const std::type_info&, type_index>);
static_assert(!std::is_convertible_v<const char*, type_index>);
static_assert(!std::is_constructible_v<type_index, const std::type_info*>);

static_assert(noexcept(std::declval<const type_index&>() > std::declval<const type_index&>()));
static_assert(noexcept(std::declval<const type_index&>() <= std::declval<const type_index&>()));
static_assert(noexcept(std::declval<const type_index&>() >= std::declval<const type_index&>()));
static_assert(noexcept(std::declval<const type_index&>() != std::declval<const type_index&>()));
static_assert(std::is_same_v<decltype(std::declval<const type_index&>() > std::declval<const type_index&>()), bool>);
static_assert(std::is_same_v<decltype(std::declval<const type_index&>() <= std::declval<const type_index&>()), bool>);
static_assert(std::is_same_v<decltype(std::declval<const type_index&>() >= std::declval<const type_index&>()), bool>);

static_assert(std::is_invocable_r_v<bool, std::less<type_index>, type_index, type_index>);
static_assert(std::is_invocable_r_v<bool, std::less<>, type_index, type_index>);
static_assert(std::is_invocable_r_v<bool, std::equal_to<type_index>, type_index, type_index>);
static_assert(std::is_invocable_r_v<bool, std::ranges::less, type_index, type_index>);
static_assert(std::is_invocable_r_v<std::strong_ordering, std::compare_three_way, type_index, type_index>);
