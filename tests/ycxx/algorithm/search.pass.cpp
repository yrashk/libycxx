// [alg.search]: search returns the first i in [first1, last1 - (last2 - first2)] such that
// the subsequence at i equals [first2, last2), or first1 if [first2, last2) is empty, or last1
// if none. ranges::search returns {i, i + (last2 - first2)}, {first1, first1} for an empty
// needle, or {last1, last1} if none; with projections. search(first, last, searcher) returns
// searcher(first, last).first ([func.search.default]: default_searcher).
#include <algorithm>
#include <functional>
#include <ranges>
#include "test_iterators.hpp"
#include "check.hpp"

constexpr bool test() {
  int a[] = {1, 2, 1, 2, 3, 1, 2, 3};
  int n[] = {1, 2, 3};
  int m[] = {4};
  if (std::search(a, a + 8, n, n + 3) != a + 2) return false;
  if (std::search(a, a + 8, m, m + 1) != a + 8) return false;
  if (std::search(a, a + 8, n, n) != a) return false;  // empty needle: first1
  if (std::search(a, a + 2, n, n + 3) != a + 2) return false;
  if (std::search(a, a + 8, n, n + 3, [](int l, int r) { return l == r; }) != a + 2) return false;
  auto f = std::search(ForwardIter<int>(a), ForwardIter<int>(a + 8), ForwardIter<int>(n), ForwardIter<int>(n + 3));
  if (f.p != a + 2) return false;

  auto r = std::ranges::search(a, n);
  if (r.begin() != a + 2 || r.end() != a + 5) return false;
  auto r2 = std::ranges::search(a, m);
  if (r2.begin() != a + 8 || r2.end() != a + 8) return false;
  auto r3 = std::ranges::search(a, a + 8, n, n);
  if (r3.begin() != a || r3.end() != a) return false;
  int tens[] = {10, 20, 30};
  auto r4 = std::ranges::search(a, tens, {}, {}, [](int v) { return v / 10; });
  if (r4.begin() != a + 2) return false;
  auto r5 = std::ranges::search(a, tens, {}, [](int v) { return v * 10; });
  if (r5.begin() != a + 2) return false;
  ForwardRange<int> fr{a, a + 8};
  auto r6 = std::ranges::search(fr, n);
  if (r6.begin().p != a + 2 || r6.end().p != a + 5) return false;

  // searcher form
  std::default_searcher s(n, n + 3);
  if (std::search(a, a + 8, s) != a + 2) return false;
  std::default_searcher s2(m, m + 1);
  if (std::search(a, a + 8, s2) != a + 8) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
