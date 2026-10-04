// [alg.find]: find returns the first i in [first, last) with *i == value, find_if the first
// with pred(*i), find_if_not the first with !pred(*i), or last if none. The ranges forms use
// invoke(proj, *i); T defaults to the (projected) value type, so a braced-init-list can be
// passed as the value (P2248); the range overloads return borrowed_iterator_t<R>
// (ranges::dangling for an rvalue non-borrowed range). At most last - first applications.
#include <algorithm>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>
#include "test_iterators.hpp"
#include "check.hpp"

struct Pt {
  int x;
  int y;
  constexpr bool operator==(const Pt&) const = default;
};

constexpr bool test() {
  int a[] = {5, 3, 7, 3, 9};
  if (std::find(a, a + 5, 3) != a + 1 || std::find(a, a + 5, 4) != a + 5) return false;
  if (std::find(InputIter<int>(a), InputIter<int>(a + 5), 9).p != a + 4) return false;
  if (std::find_if(a, a + 5, [](int x) { return x > 6; }) != a + 2) return false;
  if (std::find_if_not(a, a + 5, [](int x) { return x > 2; }) != a + 5) return false;
  if (std::find_if_not(a, a + 5, [](int x) { return x == 5; }) != a + 1) return false;
  if (std::find(a, a, 5) != a) return false;
  // heterogeneous value
  if (std::find(a, a + 5, 7L) != a + 2) return false;

  Pt pts[] = {{1, 2}, {3, 4}, {5, 6}};
  // braced value: T defaults to the value type
  if (std::find(pts, pts + 3, {3, 4}) != pts + 1) return false;
  if (std::ranges::find(pts, {5, 6}) != pts + 2) return false;
  if (std::ranges::find(pts, pts + 3, Pt{9, 9}) != pts + 3) return false;
  // projections
  if (std::ranges::find(pts, 5, &Pt::x) != pts + 2) return false;
  if (std::ranges::find_if(pts, [](int y) { return y > 3; }, &Pt::y) != pts + 1) return false;
  if (std::ranges::find_if_not(pts, pts + 3, [](int y) { return y < 5; }, &Pt::y) != pts + 2) return false;
  InputRange<int> in{a, a + 5};
  if (std::ranges::find(in, 7).p != a + 2) return false;
  ForwardRange<int> fr{a, a + 5};
  if (std::ranges::find_if(fr, [](int x) { return x == 3; }).p != a + 1) return false;
  if (std::ranges::find(fr, 100).p != a + 5) return false;
  // borrowed range (span) rvalue: an iterator, not dangling
  auto it = std::ranges::find(std::span<int>(a), 9);
  if (it != std::span<int>(a).begin() + 4) return false;
  return true;
}
static_assert(test());

struct Owning {
  int v[2] = {1, 2};
  constexpr int* begin() { return v; }
  constexpr int* end() { return v + 2; }
};
static_assert(std::is_same_v<decltype(std::ranges::find(Owning{}, 1)), std::ranges::dangling>);
static_assert(std::is_same_v<decltype(std::ranges::find_if(Owning{}, [](int) { return true; })), std::ranges::dangling>);
static_assert(std::is_same_v<decltype(std::ranges::find(std::declval<Owning&>(), 1)), int*>);

int main() {
  CHECK(test());
  int a[] = {1, 2, 3, 4};
  int calls = 0;
  (void)std::find_if(a, a + 4, [&](int x) { ++calls; return x == 2; });
  CHECK(calls == 2);
  calls = 0;
  (void)std::ranges::find(a, 9, [&](int x) { ++calls; return x; });
  CHECK(calls == 4);  // at most last - first applications of the projection
  return 0;
}
