// [array.tuple]: tuple_size<array<T,N>> is integral_constant<size_t, N>; tuple_element is T
// (with const propagated by the generic const specialization [tuple.helper]); get<I> for all
// four value categories, noexcept, constexpr; structured bindings work.
#include <array>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"

using A = std::array<int, 3>;
static_assert(std::is_base_of_v<std::integral_constant<std::size_t, 3>, std::tuple_size<A>>);
static_assert(std::tuple_size_v<A> == 3);
static_assert(std::tuple_size_v<const A> == 3);
static_assert(std::is_same_v<std::tuple_element_t<0, A>, int>);
static_assert(std::is_same_v<std::tuple_element_t<2, const A>, const int>);
static_assert(std::is_same_v<decltype(std::get<0>(std::declval<A&>())), int&>);
static_assert(std::is_same_v<decltype(std::get<0>(std::declval<A&&>())), int&&>);
static_assert(std::is_same_v<decltype(std::get<0>(std::declval<const A&>())), const int&>);
static_assert(std::is_same_v<decltype(std::get<0>(std::declval<const A&&>())), const int&&>);
static_assert(noexcept(std::get<1>(std::declval<A&>())));
static_assert(noexcept(std::get<1>(std::declval<const A&&>())));

constexpr bool test() {
  A a{1, 2, 3};
  std::get<1>(a) = 20;
  if (a[1] != 20 || std::get<2>(std::as_const(a)) != 3) return false;
  auto [x, y, z] = a;
  if (x != 1 || y != 20 || z != 3) return false;
  auto& [rx, ry, rz] = a;
  rz = 30;
  if (a[2] != 30) return false;
  (void)rx; (void)ry;
  int&& m = std::get<0>(std::move(a));
  if (m != 1) return false;
  // usable with tuple algorithms
  if (std::apply([](int p, int q, int r) { return p + q + r; }, a) != 51) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
