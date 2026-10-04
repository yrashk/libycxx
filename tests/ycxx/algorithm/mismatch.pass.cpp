// [mismatch]: mismatch(first1, last1, first2[, pred]) (last2 = first2 + (last1 - first1))
// and mismatch(first1, last1, first2, last2[, pred]) return {first1 + n, first2 + n}
// where n is the smallest index at which the elements differ, or min(last1 - first1,
// last2 - first2) if none. The result type is pair<InputIterator1, InputIterator2>;
// ranges::mismatch returns mismatch_result<I1, I2> (= in_in_result) and uses projections.
#include <algorithm>
#include <functional>
#include <type_traits>
#include <utility>
#include "test_iterators.hpp"
#include "check.hpp"

constexpr bool test() {
  int a[] = {1, 2, 3, 4};
  int b[] = {1, 2, 9, 4};
  int c[] = {1, 2};
  auto p = std::mismatch(a, a + 4, b);
  static_assert(std::is_same_v<decltype(p), std::pair<int*, int*>>);
  if (p.first != a + 2 || p.second != b + 2) return false;
  auto q = std::mismatch(a, a + 4, a);
  if (q.first != a + 4) return false;
  auto r = std::mismatch(a, a + 4, c, c + 2);  // stops at the shorter one
  if (r.first != a + 2 || r.second != c + 2) return false;
  auto s = std::mismatch(c, c + 2, a, a + 4);
  if (s.first != c + 2 || s.second != a + 2) return false;
  auto t = std::mismatch(a, a + 4, b, [](int x, int y) { return x <= y; });
  if (t.first != a + 4) return false;
  auto u = std::mismatch(a, a + 4, b, b + 4, std::equal_to<>{});
  if (u.second != b + 2) return false;
  auto v = std::mismatch(InputIter<int>(a), InputIter<int>(a + 4), InputIter<int>(b));
  if (v.first.p != a + 2 || v.second.p != b + 2) return false;

  auto w = std::ranges::mismatch(a, b);
  static_assert(std::is_same_v<decltype(w), std::ranges::mismatch_result<int*, int*>>);
  static_assert(std::is_same_v<std::ranges::mismatch_result<int*, int*>, std::ranges::in_in_result<int*, int*>>);
  if (w.in1 != a + 2 || w.in2 != b + 2) return false;
  auto x = std::ranges::mismatch(a, a + 4, c, c + 2);
  if (x.in1 != a + 2 || x.in2 != c + 2) return false;
  int tens[] = {10, 20, 30, 40};
  auto y = std::ranges::mismatch(a, tens, {}, [](int v) { return v * 10; });
  if (y.in1 != a + 4 || y.in2 != tens + 4) return false;
  auto z = std::ranges::mismatch(a, tens, {}, {}, [](int v) { return v / 10; });
  if (z.in1 != a + 4) return false;
  InputRange<int> ia{a, a + 4};
  InputRange<int> ib{b, b + 4};
  auto ii = std::ranges::mismatch(ia, ib);
  if (ii.in1.p != a + 2 || ii.in2.p != b + 2) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
