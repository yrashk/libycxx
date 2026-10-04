// [range.cmp]: ranges::equal_to / not_equal_to require equality_comparable_with<T, U>;
// ranges::less / greater / less_equal / greater_equal require totally_ordered_with<T, U>;
// "using is_transparent = unspecified;"; operator() is constexpr, returns bool.
// [comparisons.three.way]: compare_three_way requires three_way_comparable_with<T, U>.
#include <functional>
#include <compare>
#include <type_traits>
#include <utility>
#include "check.hpp"

// Has == and < but not the full set required by totally_ordered_with.
struct OnlyLess {
  int v;
  friend constexpr bool operator<(OnlyLess a, OnlyLess b) { return a.v < b.v; }
  friend constexpr bool operator==(OnlyLess a, OnlyLess b) { return a.v == b.v; }
};
struct Full {
  int v;
  friend constexpr auto operator<=>(Full, Full) = default;
};
struct EqNoBool {
  friend void operator==(EqNoBool, EqNoBool) {}
};

template <class F>
concept transparent = requires { typename F::is_transparent; };

template <class F, class A, class B>
concept callable = requires(F f, A a, B b) { f(a, b); };

static_assert(callable<std::less<>, OnlyLess, OnlyLess>);
static_assert(!callable<std::ranges::less, OnlyLess, OnlyLess>);
static_assert(callable<std::ranges::less, Full, Full>);
static_assert(callable<std::ranges::equal_to, OnlyLess, OnlyLess>);
static_assert(!callable<std::ranges::equal_to, EqNoBool, EqNoBool>);
static_assert(!callable<std::ranges::equal_to, int, int*>);
static_assert(callable<std::ranges::less, int, long>);
static_assert(callable<std::compare_three_way, Full, Full>);
static_assert(!callable<std::compare_three_way, OnlyLess, OnlyLess>);
static_assert(callable<std::compare_three_way, int, double>);
static_assert(transparent<std::ranges::less>);
static_assert(transparent<std::ranges::equal_to>);
static_assert(transparent<std::ranges::not_equal_to>);
static_assert(transparent<std::ranges::greater>);
static_assert(transparent<std::ranges::less_equal>);
static_assert(transparent<std::ranges::greater_equal>);
static_assert(transparent<std::compare_three_way>);
static_assert(std::is_same_v<decltype(std::ranges::less{}(1, 2)), bool>);
static_assert(std::is_same_v<decltype(std::compare_three_way{}(1, 2)), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::compare_three_way{}(1, 2.0)), std::partial_ordering>);

constexpr bool test() {
  if (!std::ranges::equal_to{}(1, 1L) || std::ranges::not_equal_to{}(2, 2)) return false;
  if (!std::ranges::less{}(Full{1}, Full{2}) || std::ranges::greater{}(Full{1}, Full{2})) return false;
  if (!std::ranges::less_equal{}(3, 3) || !std::ranges::greater_equal{}(4, 3)) return false;
  if (std::compare_three_way{}(Full{2}, Full{1}) != std::strong_ordering::greater) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
