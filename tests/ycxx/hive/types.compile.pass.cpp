// [hive.syn]: struct hive_limits { size_t min; size_t max; constexpr hive_limits(size_t,
// size_t) noexcept; }, hive<T, Allocator = allocator<T>>, the non-member swap, erase,
// erase_if and pmr::hive. [hive.overview]/6-7: hive meets the container (except == and !=),
// reversible container and allocator-aware container requirements; its iterators meet the
// Cpp17BidirectionalIterator requirements and model three_way_comparable<strong_ordering>.
// Member types per the synopsis; block_capacity_default_limits / hard_limits /
// is_within_hard_limits are static constexpr noexcept.
#include <hive>
#include <compare>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <memory>
#include <memory_resource>
#include <type_traits>

using H = std::hive<int>;
using It = H::iterator;
using CIt = H::const_iterator;

static_assert(std::is_same_v<decltype(std::hive_limits::min), std::size_t> &&
              std::is_same_v<decltype(std::hive_limits::max), std::size_t>);
static_assert(std::is_nothrow_constructible_v<std::hive_limits, std::size_t, std::size_t>);
static_assert(std::hive_limits(2, 9).min == 2 && std::hive_limits(2, 9).max == 9);
static_assert(std::is_same_v<H::value_type, int> && std::is_same_v<H::allocator_type, std::allocator<int>>);
static_assert(std::is_same_v<H::reference, int&> && std::is_same_v<H::const_reference, const int&>);
static_assert(std::is_same_v<H::pointer, int*> && std::is_same_v<H::const_pointer, const int*>);
static_assert(std::is_same_v<H::reverse_iterator, std::reverse_iterator<It>>);
static_assert(std::is_same_v<H::const_reverse_iterator, std::reverse_iterator<CIt>>);
static_assert(std::bidirectional_iterator<It> && std::bidirectional_iterator<CIt>);
static_assert(std::derived_from<std::iterator_traits<It>::iterator_category, std::bidirectional_iterator_tag>);
static_assert(std::three_way_comparable<It, std::strong_ordering> && std::three_way_comparable<CIt, std::strong_ordering>);
static_assert(std::is_same_v<std::iter_reference_t<It>, int&> && std::is_same_v<std::iter_reference_t<CIt>, const int&>);
static_assert(std::is_convertible_v<It, CIt>);
static_assert(std::is_signed_v<H::difference_type> && std::is_unsigned_v<H::size_type>);
static_assert(std::is_same_v<std::pmr::hive<int>, std::hive<int, std::pmr::polymorphic_allocator<int>>>);
static_assert(std::is_same_v<decltype(H::block_capacity_default_limits()), std::hive_limits>);
static_assert(std::is_same_v<decltype(H::block_capacity_hard_limits()), std::hive_limits>);
static_assert(noexcept(H::block_capacity_default_limits()) && noexcept(H::block_capacity_hard_limits()));
static_assert(noexcept(H::is_within_hard_limits(std::hive_limits(1, 1))));
static_assert(H::is_within_hard_limits(H::block_capacity_hard_limits()));
static_assert(H::is_within_hard_limits(H::block_capacity_default_limits()));
static_assert(H::block_capacity_hard_limits().min <= H::block_capacity_hard_limits().max);
static_assert(!H::is_within_hard_limits(std::hive_limits(H::block_capacity_hard_limits().max,
                                                         H::block_capacity_hard_limits().min)) ||
              H::block_capacity_hard_limits().min == H::block_capacity_hard_limits().max);
static_assert(std::is_nothrow_move_constructible_v<H> && std::is_nothrow_default_constructible_v<H>);
static_assert(std::is_nothrow_move_assignable_v<H> && std::is_nothrow_swappable_v<H>);

template <class X>
concept has_eq = requires(const X& a) { a == a; };
template <class X>
concept has_index = requires(X& a) { a[0]; };
template <class X>
concept has_push_back = requires(X& a) { a.push_back(1); };
static_assert(!has_eq<H> && !has_index<H> && !has_push_back<H>);

H& h() noexcept;
const H& ch() noexcept;
static_assert(noexcept(h().begin()) && noexcept(ch().end()) && noexcept(ch().cbegin()) && noexcept(h().rbegin()));
static_assert(noexcept(ch().size()) && noexcept(ch().empty()) && noexcept(ch().max_size()) && noexcept(ch().capacity()));
static_assert(noexcept(h().clear()) && noexcept(h().trim_capacity()) && noexcept(h().trim_capacity(1)));
static_assert(noexcept(ch().block_capacity_limits()) && noexcept(h().get_iterator(static_cast<const int*>(nullptr))));
static_assert(std::is_same_v<decltype(h().get_iterator(static_cast<const int*>(nullptr))), It>);
static_assert(std::is_same_v<decltype(ch().get_iterator(static_cast<const int*>(nullptr))), CIt>);

int main() { return 0; }
