// [array.syn], [container.reqmts]: operator== (element-wise) and operator<=> returning
// synth-three-way-result<T> (lexicographical, falling back to < when T has no <=>).
#include <array>
#include <compare>
#include <type_traits>
#include "check.hpp"

struct OnlyLess {
  int v;
  constexpr friend bool operator<(OnlyLess a, OnlyLess b) { return a.v < b.v; }
  constexpr friend bool operator==(OnlyLess a, OnlyLess b) { return a.v == b.v; }
};

static_assert(std::is_same_v<decltype(std::array<int, 2>{} <=> std::array<int, 2>{}), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::array<double, 2>{} <=> std::array<double, 2>{}), std::partial_ordering>);
static_assert(std::is_same_v<decltype(std::array<OnlyLess, 2>{} <=> std::array<OnlyLess, 2>{}), std::weak_ordering>);
static_assert(std::is_same_v<decltype(std::array<int, 2>{} == std::array<int, 2>{}), bool>);

constexpr bool test() {
  std::array<int, 3> a{1, 2, 3}, b{1, 2, 4}, c{1, 2, 3};
  if (!(a == c) || a == b || !(a != b)) return false;
  if (!(a < b) || b < a || !(b > a) || !(a <= c) || !(a >= c)) return false;
  if ((a <=> b) != std::strong_ordering::less || (a <=> c) != std::strong_ordering::equal) return false;
  std::array<OnlyLess, 2> x{{{1}, {5}}}, y{{{1}, {6}}};
  if ((x <=> y) != std::weak_ordering::less || (y <=> x) != std::weak_ordering::greater) return false;
  if ((x <=> x) != std::weak_ordering::equivalent) return false;
  if (!(x < y)) return false;
  std::array<double, 1> n{__builtin_nan("")};
  if ((n <=> n) != std::partial_ordering::unordered || n == n) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
