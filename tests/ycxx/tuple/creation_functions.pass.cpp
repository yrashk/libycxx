// [tuple.creation]/1: make_tuple(TTypes&&... t) returns tuple<unwrap_ref_decay_t<TTypes>...>(
// std::forward<TTypes>(t)...) [Example 1: make_tuple(1, ref(i), cref(j)) is tuple<int, int&,
// const float&>]. /3-4: forward_as_tuple(TTypes&&... t) noexcept returns tuple<TTypes&&...>(
// std::forward<TTypes>(t)...). /5: tie(TTypes&... t) noexcept returns tuple<TTypes&...>(t...);
// [Example 2] tie(i, ignore, s) = make_tuple(42, 3.14, "C++"). All constexpr.
// COUNTERPART: libcxx:utilities/utility/ignore/ignore.include.compile.pass.cpp
#include <tuple>
#include <functional>
#include <type_traits>
#include <utility>
#include "check.hpp"

struct Counted {
  static inline int copies = 0;
  static inline int moves = 0;
  int v;
  constexpr explicit Counted(int x) : v(x) {}
  constexpr Counted(const Counted& o) : v(o.v) {
    if !consteval { ++copies; }
  }
  constexpr Counted(Counted&& o) noexcept : v(o.v) {
    if !consteval { ++moves; }
  }
};
int fn(int x) { return x; }

int i = 0;
const float j = 0;
static_assert(std::is_same_v<decltype(std::make_tuple(1, std::ref(i), std::cref(j))),
                             std::tuple<int, int&, const float&>>);
static_assert(std::is_same_v<decltype(std::make_tuple(i, j, "ab", fn)),
                             std::tuple<int, float, const char*, int (*)(int)>>);
static_assert(std::is_same_v<decltype(std::make_tuple()), std::tuple<>>);
static_assert(std::is_same_v<decltype(std::forward_as_tuple(i, j, 1, std::move(i))),
                             std::tuple<int&, const float&, int&&, int&&>>);
static_assert(noexcept(std::forward_as_tuple(i, 1)));
static_assert(std::is_same_v<decltype(std::forward_as_tuple()), std::tuple<>>);
static_assert(std::is_same_v<decltype(std::tie(i, j)), std::tuple<int&, const float&>>);
static_assert(noexcept(std::tie(i)));
template <class T>
concept tie_rvalue = requires { std::tie(std::declval<T>()); };
static_assert(!tie_rvalue<int>);  // tie takes lvalue references

constexpr bool test() {
  int a = 1;
  long b = 2;
  auto t = std::make_tuple(a, std::ref(b));
  std::get<0>(t) = 10;
  std::get<1>(t) = 20;
  if (a != 1 || b != 20) return false;

  auto f = std::forward_as_tuple(a, std::move(b));
  if (&std::get<0>(f) != &a || &std::get<1>(f) != &b) return false;

  std::tie(a, b) = std::make_tuple(3, 4L);
  if (a != 3 || b != 4) return false;
  std::tie(a, std::ignore, b) = std::make_tuple(5, 3.14, 6L);
  if (a != 5 || b != 6) return false;
  auto tt = std::tie(a, b);
  if (&std::get<0>(tt) != &a) return false;

  // swapping through tie
  int x = 1, y = 2;
  std::tie(x, y) = std::make_tuple(y, x);
  return x == 2 && y == 1;
}
static_assert(test());

int main() {
  CHECK(test());
  // make_tuple copies lvalues and moves rvalues; forward_as_tuple and tie never copy
  Counted c(1);
  Counted::copies = Counted::moves = 0;
  auto t1 = std::make_tuple(c);
  CHECK(Counted::copies == 1 && std::get<0>(t1).v == 1);
  Counted::copies = Counted::moves = 0;
  auto t2 = std::make_tuple(std::move(c));
  CHECK(Counted::copies == 0 && Counted::moves == 1 && std::get<0>(t2).v == 1);
  Counted::copies = Counted::moves = 0;
  auto t3 = std::forward_as_tuple(c, Counted(2));
  (void)t3;
  auto t4 = std::tie(c);
  CHECK(&std::get<0>(t4) == &c);
  CHECK(Counted::copies == 0 && Counted::moves == 0);
  return 0;
}
