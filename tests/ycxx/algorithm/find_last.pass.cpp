// [alg.find.last]: ranges::find_last / find_last_if / find_last_if_not return {i, last}
// where i is the last iterator in [first, last) satisfying the condition, or {last, last}
// if none; the range overloads return borrowed_subrange_t<R>. They require forward
// iterators. T defaults to the projected value type.
#include <algorithm>
#include <ranges>
#include <type_traits>
#include "test_iterators.hpp"
#include "check.hpp"

struct Pt {
  int x;
  int y;
  constexpr bool operator==(const Pt&) const = default;
};

constexpr bool test() {
  int a[] = {1, 2, 3, 2, 1};
  auto r = std::ranges::find_last(a, 2);
  static_assert(std::is_same_v<decltype(r), std::ranges::subrange<int*>>);
  if (r.begin() != a + 3 || r.end() != a + 5) return false;
  auto n = std::ranges::find_last(a, a + 5, 7);
  if (n.begin() != a + 5 || n.end() != a + 5 || !n.empty()) return false;
  auto r2 = std::ranges::find_last_if(a, [](int x) { return x < 3; });
  if (r2.begin() != a + 4) return false;
  auto r3 = std::ranges::find_last_if_not(a, [](int x) { return x < 3; });
  if (r3.begin() != a + 2 || r3.size() != 3) return false;
  auto r4 = std::ranges::find_last_if_not(a, [](int) { return true; });
  if (r4.begin() != a + 5) return false;
  Pt pts[] = {{1, 0}, {2, 0}, {1, 1}};
  if (std::ranges::find_last(pts, {1, 0}).begin() != pts) return false;  // braced value
  if (std::ranges::find_last(pts, 1, &Pt::x).begin() != pts + 2) return false;
  if (std::ranges::find_last_if(pts, [](int y) { return y == 0; }, &Pt::y).begin() != pts + 1) return false;
  // forward, non-common range: the subrange's end is the iterator reached at the sentinel
  ForwardRange<int> fr{a, a + 5};
  auto f = std::ranges::find_last(fr, 1);
  if (f.begin().p != a + 4 || f.end().p != a + 5) return false;
  auto g = std::ranges::find_last(fr.begin(), fr.end(), 9);
  if (g.begin().p != a + 5) return false;
  int e[1] = {0};
  if (!std::ranges::find_last(e, e, 0).empty()) return false;
  return true;
}
static_assert(test());

struct Owning {
  int v[2] = {1, 2};
  constexpr int* begin() { return v; }
  constexpr int* end() { return v + 2; }
};
static_assert(std::is_same_v<decltype(std::ranges::find_last(Owning{}, 1)), std::ranges::dangling>);

template <class R>
concept can_find_last = requires(R r) { std::ranges::find_last(r, 1); };
static_assert(can_find_last<ForwardRange<int>>);
static_assert(!can_find_last<InputRange<int>>);

int main() {
  CHECK(test());
  return 0;
}
