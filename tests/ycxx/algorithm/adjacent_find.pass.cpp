// [alg.adjacent.find]: adjacent_find returns the first i such that both i and i + 1 are in
// [first, last) and *i == *(i + 1) (or pred(*i, *(i + 1))), or last if none. Complexity:
// for a non-empty range exactly min((i - first) + 1, (last - first) - 1) applications.
// ranges::adjacent_find compares invoke(proj, *i) and invoke(proj, *(i + 1)).
#include <algorithm>
#include <functional>
#include "test_iterators.hpp"
#include "check.hpp"

struct Pt {
  int x;
  int y;
};

constexpr bool test() {
  int a[] = {1, 2, 3, 3, 4, 4};
  if (std::adjacent_find(a, a + 6) != a + 2) return false;
  if (std::adjacent_find(a, a + 3) != a + 3) return false;
  if (std::adjacent_find(a, a) != a) return false;
  if (std::adjacent_find(a, a + 1) != a + 1) return false;
  if (std::adjacent_find(a, a + 6, std::greater<>{}) != a + 6) return false;
  if (std::adjacent_find(a, a + 6, [](int l, int r) { return r == l + 1; }) != a) return false;
  if (std::adjacent_find(ForwardIter<int>(a + 3), ForwardIter<int>(a + 6)).p != a + 4) return false;

  if (std::ranges::adjacent_find(a) != a + 2) return false;
  Pt pts[] = {{1, 5}, {2, 6}, {3, 6}};
  if (std::ranges::adjacent_find(pts, {}, &Pt::y) != pts + 1) return false;
  if (std::ranges::adjacent_find(pts, pts + 3, std::ranges::less{}, &Pt::x) != pts) return false;
  ForwardRange<int> fr{a, a + 6};
  if (std::ranges::adjacent_find(fr).p != a + 2) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  int a[] = {1, 2, 3, 3, 4};
  int calls = 0;
  std::adjacent_find(a, a + 5, [&](int l, int r) { ++calls; return l == r; });
  CHECK(calls == 3);  // min((i - first) + 1, (last - first) - 1) = min(3, 4)
  calls = 0;
  std::adjacent_find(a, a + 3, [&](int l, int r) { ++calls; return l == r; });
  CHECK(calls == 2);  // not found: (last - first) - 1
  return 0;
}
