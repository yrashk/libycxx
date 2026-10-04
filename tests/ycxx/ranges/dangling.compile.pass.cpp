// [range.dangling]: dangling is constructible from any arguments (noexcept), and
// borrowed_iterator_t/borrowed_subrange_t denote it for non-borrowed rvalue ranges (/3).
#include <array>
#include <ranges>
#include <span>
#include <string_view>
#include <type_traits>

namespace rg = std::ranges;
using A = std::array<int, 3>;

static_assert(std::is_nothrow_default_constructible_v<rg::dangling>);
static_assert(std::is_nothrow_constructible_v<rg::dangling, int, double, A&>);
static_assert(std::is_convertible_v<int, rg::dangling>); // the variadic constructor is not explicit
static_assert(std::is_trivially_copyable_v<rg::dangling> && std::is_empty_v<rg::dangling>);

static_assert(std::is_same_v<rg::borrowed_iterator_t<A>, rg::dangling>);
static_assert(std::is_same_v<rg::borrowed_iterator_t<A&>, A::iterator>);
static_assert(std::is_same_v<rg::borrowed_iterator_t<const A&>, A::const_iterator>);
static_assert(std::is_same_v<rg::borrowed_subrange_t<A>, rg::dangling>);
static_assert(std::is_same_v<rg::borrowed_subrange_t<A&>, rg::subrange<A::iterator>>);
static_assert(std::is_same_v<rg::borrowed_iterator_t<std::span<int>>, std::span<int>::iterator>);
static_assert(std::is_same_v<rg::borrowed_subrange_t<std::string_view>, rg::subrange<std::string_view::const_iterator>>);
static_assert(std::is_same_v<rg::borrowed_iterator_t<rg::iota_view<int, int>>, rg::iterator_t<rg::iota_view<int, int>>>);
static_assert(std::is_same_v<rg::borrowed_iterator_t<rg::single_view<int>>, rg::dangling>);
static_assert(std::is_same_v<rg::borrowed_iterator_t<rg::owning_view<A>>, rg::dangling>);
static_assert(std::is_same_v<rg::borrowed_iterator_t<rg::ref_view<A>>, A::iterator>);
static_assert(std::is_same_v<rg::borrowed_iterator_t<int (&)[2]>, int*>);

[[maybe_unused]] constexpr rg::dangling d1{};
[[maybe_unused]] constexpr rg::dangling d2(1, 'c', nullptr);
