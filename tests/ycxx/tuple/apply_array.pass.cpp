// [tuple.apply]/1: apply(f, t) for any tuple-like t, including std::array ([tuple.like]):
// INVOKE(std::forward<F>(f), get<I>(std::forward<Tuple>(t))...) for I in
// [0, tuple_size_v<remove_reference_t<Tuple>>), so a std::array<T, N> supplies N arguments
// with the array's value category; std::array<T, 0> supplies none. /3: make_from_tuple<T>(t)
// is T(get<I>(std::forward<Tuple>(t))...).
#include <tuple>
#include <array>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Cat {
  constexpr int operator()(int&) const { return 1; }
  constexpr int operator()(const int&) const { return 2; }
  constexpr int operator()(int&&) const { return 3; }
  constexpr int operator()(const int&&) const { return 4; }
};
struct Three {
  int a, b, c;
  constexpr Three(int x, int y, int z) : a(x), b(y), c(z) {}
};

static_assert(std::is_same_v<decltype(std::apply([](auto&... xs) -> decltype(auto) { return (xs, ...); },
                                                 std::declval<std::array<long, 2>&>())),
                             long&>);

constexpr bool test() {
  std::array<int, 3> a{1, 2, 3};
  if (std::apply([](int x, int y, int z) { return x * 100 + y * 10 + z; }, a) != 123) return false;
  if (std::apply([] { return 7; }, std::array<int, 0>{}) != 7) return false;
  std::array<int, 1> one{0};
  if (std::apply(Cat{}, one) != 1) return false;
  if (std::apply(Cat{}, std::as_const(one)) != 2) return false;
  if (std::apply(Cat{}, std::move(one)) != 3) return false;
  if (std::apply(Cat{}, std::move(std::as_const(one))) != 4) return false;
  // modification through the references get yields
  std::apply([](int& x, int&, int& z) { x = 9; z = 8; }, a);
  if (a[0] != 9 || a[2] != 8) return false;
  Three t = std::make_from_tuple<Three>(std::array<int, 3>{4, 5, 6});
  return t.a == 4 && t.b == 5 && t.c == 6;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
