// Exception specifications declared in [basic.string.general]: default constructor
// noexcept(noexcept(Allocator())); allocator constructor, move constructor, iterator
// accessors, size/length/max_size/capacity/clear/empty, c_str/data, get_allocator, conversion
// to basic_string_view are noexcept; move assignment and assign(basic_string&&) are
// noexcept(POCMA || is_always_equal); swap is noexcept(POCS || is_always_equal);
// [string.special] non-member swap is noexcept(noexcept(lhs.swap(rhs))).
// REQUIRES: exceptions
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include "test_allocators.hpp"

using S = std::string;
static_assert(std::is_nothrow_default_constructible_v<S>);
static_assert(std::is_nothrow_constructible_v<S, const std::allocator<char>&>);
static_assert(std::is_nothrow_move_constructible_v<S>);
static_assert(std::is_nothrow_move_assignable_v<S>);
static_assert(std::is_nothrow_swappable_v<S>);
static_assert(noexcept(std::declval<S&>().assign(std::declval<S&&>())));

extern S& m;
extern const S& c;
static_assert(noexcept(m.begin()) && noexcept(c.begin()) && noexcept(m.end()) && noexcept(c.end()));
static_assert(noexcept(m.rbegin()) && noexcept(c.rbegin()) && noexcept(m.rend()) && noexcept(c.rend()));
static_assert(noexcept(c.cbegin()) && noexcept(c.cend()) && noexcept(c.crbegin()) && noexcept(c.crend()));
static_assert(noexcept(c.size()) && noexcept(c.length()) && noexcept(c.max_size()));
static_assert(noexcept(c.capacity()) && noexcept(m.clear()) && noexcept(c.empty()));
static_assert(noexcept(c.c_str()) && noexcept(c.data()) && noexcept(m.data()));
static_assert(noexcept(c.get_allocator()));
static_assert(noexcept(static_cast<std::string_view>(c)));
static_assert(std::is_nothrow_convertible_v<const S&, std::string_view>);

// Allocators that neither propagate nor are always equal keep move-assignment possibly
// throwing only by the draft's formula; the propagating ones must be noexcept.
using PM = std::basic_string<char, std::char_traits<char>, IdAlloc<char, false, true, false>>;
using PSw = std::basic_string<char, std::char_traits<char>, IdAlloc<char, false, false, true>>;
static_assert(std::is_nothrow_move_assignable_v<PM>);
static_assert(noexcept(std::declval<PSw&>().swap(std::declval<PSw&>())));
static_assert(noexcept(swap(std::declval<PSw&>(), std::declval<PSw&>())));
static_assert(std::is_nothrow_move_constructible_v<PM>);
