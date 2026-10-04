// [array.members]/4: swap is noexcept(is_nothrow_swappable_v<T>);
// [array.special]: non-member swap noexcept(noexcept(x.swap(y))), constrained on
// N == 0 || is_swappable_v<T>.
#include <array>
#include <type_traits>
#include <utility>

namespace ns {
struct ThrowingSwap { friend void swap(ThrowingSwap&, ThrowingSwap&) noexcept(false) {} };
struct NotSwappable { NotSwappable& operator=(const NotSwappable&) = delete; };
}

static_assert(noexcept(std::declval<std::array<int, 2>&>().swap(std::declval<std::array<int, 2>&>())));
static_assert(!noexcept(std::declval<std::array<ns::ThrowingSwap, 2>&>().swap(
    std::declval<std::array<ns::ThrowingSwap, 2>&>())));
static_assert(std::is_nothrow_swappable_v<std::array<int, 2>>);
static_assert(std::is_swappable_v<std::array<ns::ThrowingSwap, 2>>);
static_assert(!std::is_nothrow_swappable_v<std::array<ns::ThrowingSwap, 2>>);
static_assert(!std::is_swappable_v<std::array<ns::NotSwappable, 2>>);
