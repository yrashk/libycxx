// [pair.astuple]: tuple_size<pair<T1, T2>> is integral_constant<size_t, 2>; tuple_element<I,
// pair<T1, T2>>::type is T1 for I == 0, otherwise T2. get<I>(p) and get<T>(p) return references
// to p.first / p.second with the value category and constness of p, all noexcept.
// [dcl.struct.bind]/4: structured bindings of a tuple-like type use tuple_size, tuple_element
// and get.
#include <utility>
#include <type_traits>
#include "check.hpp"

using P = std::pair<int, double>;
static_assert(std::tuple_size<P>::value == 2);
static_assert(std::tuple_size_v<const P> == 2);
static_assert(std::is_base_of_v<std::integral_constant<std::size_t, 2>, std::tuple_size<P>>);
static_assert(std::is_same_v<std::tuple_element_t<0, P>, int>);
static_assert(std::is_same_v<std::tuple_element_t<1, P>, double>);
static_assert(std::is_same_v<std::tuple_element_t<1, const P>, const double>);
static_assert(std::is_same_v<std::tuple_element_t<0, std::pair<int&, int&&>>, int&>);

static_assert(std::is_same_v<decltype(std::get<0>(std::declval<P&>())), int&>);
static_assert(std::is_same_v<decltype(std::get<1>(std::declval<const P&>())), const double&>);
static_assert(std::is_same_v<decltype(std::get<0>(std::declval<P&&>())), int&&>);
static_assert(std::is_same_v<decltype(std::get<1>(std::declval<const P&&>())), const double&&>);
static_assert(std::is_same_v<decltype(std::get<int>(std::declval<P&>())), int&>);
static_assert(std::is_same_v<decltype(std::get<double>(std::declval<const P&>())), const double&>);
static_assert(std::is_same_v<decltype(std::get<double>(std::declval<P&&>())), double&&>);
static_assert(std::is_same_v<decltype(std::get<int>(std::declval<const P&&>())), const int&&>);
static_assert(noexcept(std::get<0>(std::declval<P&>())));
static_assert(noexcept(std::get<double>(std::declval<P&&>())));
// reference members: get on an rvalue pair yields T&& collapsing
static_assert(std::is_same_v<decltype(std::get<0>(std::declval<std::pair<int&, int>&&>())), int&>);

constexpr bool test() {
  P p(1, 2.5);
  std::get<0>(p) = 3;
  std::get<double>(p) = 4.5;
  if (p.first != 3 || p.second != 4.5) return false;
  auto [a, b] = p;  // copies
  a = 10;
  if (p.first != 3 || b != 4.5) return false;
  auto& [x, y] = p;  // refers to p's members
  x = 20;
  y = 1.0;
  if (p.first != 20 || p.second != 1.0) return false;
  const auto& [cx, cy] = p;
  static_assert(std::is_same_v<decltype(cx), const int>);
  if (cx != 20 || cy != 1.0) return false;
  auto&& [r1, r2] = P(7, 8.0);  // lifetime-extended temporary
  if (r1 != 7 || r2 != 8.0) return false;
  int i = 1, j = 2;
  std::pair<int&, int&> refs(i, j);
  auto [ri, rj] = refs;
  ri = 5;
  rj = 6;
  if (i != 5 || j != 6) return false;
  int&& moved = std::get<0>(std::move(p));
  if (&moved != &p.first) return false;
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
  return 0;
}
