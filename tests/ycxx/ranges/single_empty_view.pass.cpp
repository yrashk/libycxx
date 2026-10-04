// [range.single], [range.empty]: single_view and empty_view, their concepts, their static
// members, views::single's decay ([range.single.overview]/2) and the in_place constructor.
#include <concepts>
#include <cstddef>
#include <ranges>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "range_support.hpp"

namespace rg = std::ranges;

using S = rg::single_view<int>;
static_assert(rg::view<S> && rg::contiguous_range<S> && rg::sized_range<S> && rg::common_range<S>);
static_assert(!rg::borrowed_range<S>);
static_assert(std::is_same_v<rg::iterator_t<S>, int*> && std::is_same_v<rg::iterator_t<const S>, const int*>);
static_assert(noexcept(S::size()) && noexcept(S::empty()) && S::size() == 1 && !S::empty());
static_assert(std::is_same_v<decltype(S::size()), std::size_t>);
static_assert(noexcept(std::declval<S&>().begin()) && noexcept(std::declval<const S&>().data()));
static_assert(std::is_same_v<decltype(std::views::single(1)), S>);
static_assert(std::is_same_v<decltype(std::views::single("abc")), rg::single_view<const char*>>);
static_assert(std::is_same_v<decltype(rg::single_view(2.0)), rg::single_view<double>>);
static_assert(!std::is_convertible_v<int, S>); // explicit

using E = rg::empty_view<long>;
static_assert(rg::view<E> && rg::contiguous_range<E> && rg::sized_range<E> && rg::borrowed_range<E>);
static_assert(std::is_same_v<decltype(std::views::empty<long>), const E>);
static_assert(E::size() == 0 && E::empty() && E::begin() == nullptr && E::data() == nullptr);
static_assert(noexcept(E::begin()) && noexcept(E::end()) && noexcept(E::size()));
static_assert(std::is_same_v<decltype(E::begin()), long*>);
static_assert(rg::empty(std::views::empty<int>));

struct Point {
  int x, y;
  constexpr Point(int a, int b) : x(a), y(b) {}
};

constexpr bool test() {
  S s(42);
  CHECK(*s.begin() == 42 && s.end() == s.begin() + 1 && s.data() == s.begin() && s.front() == 42);
  *s.data() = 7;
  CHECK(range_equals(s, {7}));
  const S cs = s;
  CHECK(*cs.data() == 7 && cs.back() == 7);
  rg::single_view<Point> p(std::in_place, 3, 4);
  CHECK(p.begin()->x == 3 && p.begin()->y == 4);
  int n = 0;
  for (int v : std::views::single(5)) n += v;
  CHECK(n == 5);
  int c = 0;
  for (long v : std::views::empty<long>) c += static_cast<int>(v) + 1;
  CHECK(c == 0);
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
