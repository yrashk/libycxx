// [alg.search]/5-12: search_n(first, last, count, value[, pred]) returns the first i in
// [first, last - count] such that the count elements starting at i all equal value (under
// pred(*(i + n), value)), or last if none; for count <= 0 it returns first.
// ranges::search_n returns {i, i + count}, or {last, last} if none; with a projection; T
// defaults to the projected value type.
#include <algorithm>
#include <ranges>
#include "test_iterators.hpp"
#include "check.hpp"

struct Pt {
  int x;
  int y;
  constexpr bool operator==(const Pt&) const = default;
};

constexpr bool test() {
  int a[] = {1, 2, 2, 3, 2, 2, 2, 4};
  if (std::search_n(a, a + 8, 2, 2) != a + 1) return false;
  if (std::search_n(a, a + 8, 3, 2) != a + 4) return false;
  if (std::search_n(a, a + 8, 4, 2) != a + 8) return false;
  if (std::search_n(a, a + 8, 0, 9) != a) return false;
  if (std::search_n(a, a + 8, -1, 9) != a) return false;
  if (std::search_n(a, a + 8, 1, 4) != a + 7) return false;
  if (std::search_n(a, a + 8, 2, 1, [](int x, int v) { return x > v; }) != a + 1) return false;
  if (std::search_n(ForwardIter<int>(a), ForwardIter<int>(a + 8), 3, 2).p != a + 4) return false;

  auto r = std::ranges::search_n(a, 3, 2);
  if (r.begin() != a + 4 || r.end() != a + 7) return false;
  auto r2 = std::ranges::search_n(a, a + 8, 5, 2);
  if (r2.begin() != a + 8 || r2.end() != a + 8) return false;
  auto r3 = std::ranges::search_n(a, 0, 9);
  if (r3.begin() != a || r3.end() != a) return false;
  Pt p[] = {{1, 0}, {2, 5}, {3, 5}, {4, 1}};
  auto r4 = std::ranges::search_n(p, 2, 5, {}, &Pt::y);
  if (r4.begin() != p + 1 || r4.size() != 2) return false;
  auto r5 = std::ranges::search_n(p, 1, {3, 5});  // braced value
  if (r5.begin() != p + 2) return false;
  ForwardRange<int> fr{a, a + 8};
  auto r6 = std::ranges::search_n(fr, 2, 2);
  if (r6.begin().p != a + 1 || r6.end().p != a + 3) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
