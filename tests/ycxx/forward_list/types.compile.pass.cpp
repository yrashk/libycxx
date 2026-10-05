// [forward.list.overview]/1-2: forward_list supports forward iterators and "meets all of the
// requirements of a container ([container.reqmts]), except that the size() member function
// is not provided"; it is not a reversible container (no reverse_iterator members, no
// rbegin). Member types per the synopsis and [container.reqmts]/2-9; iterator copy/move do
// not throw ([container.reqmts]/66.4); before_begin / cbefore_begin ([forward.list.iter])
// return iterator / const_iterator and are noexcept.
// REQUIRES: exceptions
#include <forward_list>
#include <concepts>
#include <iterator>
#include <limits>
#include <memory>
#include <ranges>
#include <type_traits>
#include "test_allocators.hpp"

using L = std::forward_list<int>;
using It = L::iterator;
using CIt = L::const_iterator;

static_assert(std::is_same_v<L::value_type, int>);
static_assert(std::is_same_v<L::allocator_type, std::allocator<int>>);
static_assert(std::is_same_v<L::reference, int&> && std::is_same_v<L::const_reference, const int&>);
static_assert(std::is_same_v<L::pointer, int*> && std::is_same_v<L::const_pointer, const int*>);
static_assert(std::is_same_v<std::forward_list<int, MinimalAlloc<int>>::pointer, int*>);
static_assert(std::forward_iterator<It> && std::forward_iterator<CIt>);
static_assert(std::derived_from<std::iterator_traits<It>::iterator_category, std::forward_iterator_tag>);
static_assert(std::is_same_v<std::iter_reference_t<It>, int&> && std::is_same_v<std::iter_reference_t<CIt>, const int&>);
static_assert(std::is_same_v<std::iter_value_t<CIt>, int>);
static_assert(std::is_convertible_v<It, CIt> && !std::is_convertible_v<CIt, It>);
static_assert(std::is_signed_v<L::difference_type> && std::is_same_v<L::difference_type, std::iter_difference_t<It>>);
static_assert(std::is_unsigned_v<L::size_type>);
static_assert(std::numeric_limits<L::size_type>::max() >=
              static_cast<std::make_unsigned_t<L::difference_type>>(std::numeric_limits<L::difference_type>::max()));
static_assert(std::is_nothrow_copy_constructible_v<It> && std::is_nothrow_copy_assignable_v<It>);
static_assert(std::is_nothrow_copy_constructible_v<CIt> && std::is_nothrow_move_assignable_v<CIt>);

template <class X>
concept has_size = requires(const X& x) { x.size(); };
template <class X>
concept has_rbegin = requires(X& x) { x.rbegin(); };
template <class X>
concept has_reverse_iterator_type = requires { typename X::reverse_iterator; };
template <class X>
concept has_back = requires(X& x) { x.back(); };
static_assert(!has_size<L> && !has_rbegin<L> && !has_reverse_iterator_type<L> && !has_back<L>);
static_assert(std::ranges::forward_range<L> && std::ranges::common_range<L>);
static_assert(!std::ranges::sized_range<L>);

L& l() noexcept;
const L& cl() noexcept;
static_assert(std::is_same_v<decltype(l().before_begin()), It>);
static_assert(std::is_same_v<decltype(cl().before_begin()), CIt>);
static_assert(std::is_same_v<decltype(l().cbefore_begin()), CIt>);
static_assert(noexcept(l().before_begin()) && noexcept(cl().before_begin()) && noexcept(l().cbefore_begin()));
static_assert(noexcept(l().begin()) && noexcept(l().end()) && noexcept(l().cbegin()) && noexcept(l().cend()));
static_assert(noexcept(cl().empty()) && noexcept(cl().max_size()) && noexcept(l().clear()));
static_assert(noexcept(l().reverse()) && noexcept(l().get_allocator()));
static_assert(std::is_nothrow_move_assignable_v<L> && noexcept(l().swap(l())) && noexcept(swap(l(), l())));
static_assert(std::is_same_v<std::pmr::forward_list<int>, std::forward_list<int, std::pmr::polymorphic_allocator<int>>>);

int main() { return 0; }
