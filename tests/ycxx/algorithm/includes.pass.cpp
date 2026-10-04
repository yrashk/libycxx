// [includes]: "Returns: true if and only if [first2, last2) is a subsequence of [first1,
// last1)" (multiset semantics per [alg.set.operations.general]: repeated elements must be
// repeated at least as often in the first range). "At most 2 * ((last1 - first1) + (last2 -
// first2)) - 1 comparisons and applications of each projection". Input iterators suffice;
// ranges::includes takes proj1 and proj2.
#include <algorithm>
#include <functional>
#include <ranges>
#include "sort_support.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

struct A {
  int k;
};
struct B {
  long k;
};

constexpr bool test() {
  int a[] = {1, 2, 2, 4, 7, 9};
  int b[] = {2, 2, 7};
  int c[] = {2, 2, 2};
  int d[] = {3};
  if (!std::includes(a, a + 6, b, b + 3)) return false;
  if (std::includes(a, a + 6, c, c + 3)) return false;  // three 2s but only two in a
  if (std::includes(a, a + 6, d, d + 1)) return false;
  if (!std::includes(a, a + 6, a, a)) return false;  // empty is a subsequence
  if (std::includes(a, a, b, b + 1)) return false;
  if (!std::includes(a, a, a, a)) return false;
  if (!std::includes(a, a + 6, a, a + 6)) return false;
  // comparator, input iterators
  int ra[] = {9, 5, 3, 1};
  int rb[] = {5, 1};
  if (!std::includes(InputIter<int>(ra), InputIter<int>(ra + 4), InputIter<int>(rb), InputIter<int>(rb + 2),
                     std::greater<>{}))
    return false;
  // equivalence, not equality
  int t1[] = {11, 25};
  int t2[] = {19};
  if (!std::includes(t1, t1 + 2, t2, t2 + 1, [](int x, int y) { return x / 10 < y / 10; })) return false;
  // ranges with projections of different types
  A as[] = {{1}, {3}, {3}, {8}};
  B bs[] = {{3}, {8}};
  if (!std::ranges::includes(as, bs, {}, &A::k, &B::k)) return false;
  B bs2[] = {{3}, {3}, {3}};
  if (std::ranges::includes(as, bs2, {}, &A::k, &B::k)) return false;
  InputRange<int> ir1{a, a + 6}, ir2{b, b + 3};
  if (!std::ranges::includes(ir1, ir2)) return false;
  if (!std::ranges::includes(a, a + 6, b, b + 3)) return false;
  return true;
}

static_assert(test());

int x[800], y[300];

int main() {
  CHECK(test());
  for (unsigned seed = 1; seed <= 6; ++seed) {
    fill_pattern(x, 800, Pattern::few_values, seed);
    fill_pattern(y, 300, Pattern::few_values, seed + 10);
    std::sort(x, x + 800);
    std::sort(y, y + 300);
    for (int n2 : {0, 1, 50, 300}) {
      int comps = 0, p1 = 0, p2 = 0;
      std::ranges::includes(x, x + 800, y, y + n2, CountingLess{&comps}, CountingProj{&p1}, CountingProj{&p2});
      const int bound = 2 * (800 + n2) - 1;
      CHECK(comps <= bound && p1 <= bound && p2 <= bound);
    }
  }
  return 0;
}
