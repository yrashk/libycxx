// [alg.find.first.of]: find_first_of returns the first i in [first1, last1) such that *i
// == *j (or pred(*i, *j)) for some j in [first2, last2), or last1 if none (in particular if
// [first2, last2) is empty). The first range may be a single-pass input range; the second
// must be forward. ranges::find_first_of uses projections proj1 / proj2.
#include <algorithm>
#include <ranges>
#include "test_iterators.hpp"
#include "check.hpp"

struct Pt {
  int x;
  int y;
};

constexpr bool test() {
  int a[] = {5, 4, 3, 2, 1};
  int s[] = {9, 2, 3};
  int none[] = {7, 8};
  if (std::find_first_of(a, a + 5, s, s + 3) != a + 2) return false;
  if (std::find_first_of(a, a + 5, none, none + 2) != a + 5) return false;
  if (std::find_first_of(a, a + 5, s, s) != a + 5) return false;
  if (std::find_first_of(a, a + 5, s, s + 3, [](int l, int r) { return l == r + 2; }) != a) return false;
  auto in = std::find_first_of(InputIter<int>(a), InputIter<int>(a + 5), ForwardIter<int>(s), ForwardIter<int>(s + 3));
  if (in.p != a + 2) return false;

  if (std::ranges::find_first_of(a, s) != a + 2) return false;
  if (std::ranges::find_first_of(a, a + 5, none, none + 2) != a + 5) return false;
  Pt pts[] = {{1, 10}, {2, 20}, {3, 30}};
  int ys[] = {30, 20};
  if (std::ranges::find_first_of(pts, ys, {}, &Pt::y) != pts + 1) return false;
  if (std::ranges::find_first_of(pts, ys, {}, &Pt::x, [](int v) { return v / 10; }) != pts + 1) return false;
  InputRange<int> ir{a, a + 5};
  if (std::ranges::find_first_of(ir, s).p != a + 2) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
