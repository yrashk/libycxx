// [alg.find.end]: find_end returns the last iterator i in [first1, last1 - (last2 - first2)]
// such that the subsequence starting at i equals [first2, last2) (under pred), or last1 if
// [first2, last2) is empty or no such subsequence exists. ranges::find_end returns {i, i +
// (last2 - first2)}, or {last1, last1} if none; with projections proj1/proj2.
#include <algorithm>
#include <functional>
#include <ranges>
#include "test_iterators.hpp"
#include "check.hpp"

constexpr bool test() {
  int a[] = {1, 2, 3, 1, 2, 3, 1};
  int n[] = {1, 2, 3};
  int m[] = {3, 1};
  int x[] = {4};
  if (std::find_end(a, a + 7, n, n + 3) != a + 3) return false;
  if (std::find_end(a, a + 7, m, m + 2) != a + 5) return false;
  if (std::find_end(a, a + 7, x, x + 1) != a + 7) return false;
  if (std::find_end(a, a + 7, n, n) != a + 7) return false;  // empty needle: last1
  if (std::find_end(a, a + 2, n, n + 3) != a + 2) return false;  // needle longer than haystack
  if (std::find_end(a, a + 7, a, a + 7) != a) return false;  // whole range
  // with a predicate
  int d[] = {10, 20, 30};
  auto by_tens = [](int l, int r) { return l * 10 == r; };
  if (std::find_end(a, a + 7, d, d + 3, by_tens) != a + 3) return false;
  // forward iterators
  ForwardIter<int> f = std::find_end(ForwardIter<int>(a), ForwardIter<int>(a + 7), ForwardIter<int>(m),
                                     ForwardIter<int>(m + 2));
  if (f.p != a + 5) return false;

  auto r = std::ranges::find_end(a, n);
  if (r.begin() != a + 3 || r.end() != a + 6) return false;
  auto r2 = std::ranges::find_end(a, x);
  if (r2.begin() != a + 7 || r2.end() != a + 7) return false;
  auto r3 = std::ranges::find_end(a, a + 7, n, n);
  if (r3.begin() != a + 7 || r3.end() != a + 7) return false;
  auto r4 = std::ranges::find_end(a, d, std::ranges::equal_to{}, [](int v) { return v * 10; });
  if (r4.begin() != a + 3) return false;
  auto r5 = std::ranges::find_end(a, d, {}, {}, [](int v) { return v / 10; });
  if (r5.begin() != a + 3 || r5.size() != 3) return false;
  ForwardRange<int> fr{a, a + 7};
  auto r6 = std::ranges::find_end(fr, m);
  if (r6.begin().p != a + 5 || r6.end().p != a + 7) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
