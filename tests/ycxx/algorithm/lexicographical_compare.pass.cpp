// [alg.lex.comparison]: lexicographical_compare returns true iff [first1, last1) is
// lexicographically less than [first2, last2): equal-length sequences of equivalent
// elements are not less; "If one sequence is a proper prefix of the other, then the shorter
// sequence is lexicographically less"; otherwise the first non-equivalent pair decides. "An
// empty sequence is lexicographically less than any non-empty sequence, but not less than
// any empty sequence." "At most 2 min(last1 - first1, last2 - first2) applications of the
// corresponding comparison and each projection". Input iterators suffice; the ranges form
// takes proj1 and proj2.
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
  int a[] = {1, 2, 3};
  int b[] = {1, 2, 4};
  int c[] = {1, 2};
  if (!std::lexicographical_compare(a, a + 3, b, b + 3) || std::lexicographical_compare(b, b + 3, a, a + 3)) return false;
  if (std::lexicographical_compare(a, a + 3, a, a + 3)) return false;
  if (!std::lexicographical_compare(c, c + 2, a, a + 3) || std::lexicographical_compare(a, a + 3, c, c + 2)) return false;
  if (!std::lexicographical_compare(a, a, a, a + 1) || std::lexicographical_compare(a, a, b, b)) return false;
  if (std::lexicographical_compare(a, a + 1, a, a)) return false;
  // a later difference does not matter once an earlier one decides
  int d[] = {2, 0, 0};
  if (!std::lexicographical_compare(b, b + 3, d, d + 3)) return false;
  // comparator: equivalence, not equality
  int t1[] = {11, 25};
  int t2[] = {19, 21};
  auto tens = [](int x, int y) { return x / 10 < y / 10; };
  if (std::lexicographical_compare(t1, t1 + 2, t2, t2 + 2, tens)) return false;
  if (std::lexicographical_compare(t2, t2 + 2, t1, t1 + 2, tens)) return false;
  if (!std::lexicographical_compare(b, b + 3, a, a + 3, std::greater<>{})) return false;
  // input iterators
  if (!std::lexicographical_compare(InputIter<int>(a), InputIter<int>(a + 3), InputIter<int>(b), InputIter<int>(b + 3)))
    return false;
  // ranges, with projections of different types
  A as[] = {{1}, {5}};
  B bs[] = {{1}, {6}};
  if (!std::ranges::lexicographical_compare(as, bs, {}, &A::k, &B::k)) return false;
  if (std::ranges::lexicographical_compare(bs, as, {}, &B::k, &A::k)) return false;
  if (!std::ranges::lexicographical_compare(a, a + 2, a, a + 3)) return false;
  InputRange<int> r1{a, a + 3}, r2{b, b + 3};
  if (!std::ranges::lexicographical_compare(r1, r2)) return false;
  if (std::ranges::lexicographical_compare(a, a, std::ranges::greater{})) return false;
  return true;
}

static_assert(test());

int x[100], y[100];

int main() {
  CHECK(test());
  for (int n1 : {0, 1, 50, 100})
    for (int n2 : {0, 1, 50, 100}) {
      for (int i = 0; i < 100; ++i) x[i] = y[i] = i % 7;
      int comps = 0, p1 = 0, p2 = 0;
      std::ranges::lexicographical_compare(x, x + n1, y, y + n2, CountingLess{&comps}, CountingProj{&p1}, CountingProj{&p2});
      int m = n1 < n2 ? n1 : n2;
      CHECK(comps <= 2 * m && p1 <= 2 * m && p2 <= 2 * m);
      comps = 0;
      std::lexicographical_compare(x, x + n1, y, y + n2, CountingLess{&comps});
      CHECK(comps <= 2 * m);
    }
  return 0;
}
