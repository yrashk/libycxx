// [lower.bound]: "Returns: The furthermost iterator i in the range [first, last] such that
// for every iterator j in the range [first, i), bool(invoke(comp, invoke(proj, *j), value))
// is true." [upper.bound]: the same with !bool(invoke(comp, value, invoke(proj, *j))). Both:
// "At most log2(last - first) + O(1) comparisons and projections" (checked as
// floor(log2 N) + 2), also for forward iterators ([alg.binary.search.general]/1: the number
// of comparisons "will be logarithmic for all types of iterators"). The precondition is only
// that the range is partitioned, not sorted. For the std overloads lower_bound only calls
// comp(element, value) and upper_bound only comp(value, element), so a comparator callable in
// only that order works.
#include <algorithm>
#include <functional>
#include <ranges>
#include <type_traits>
#include "sort_support.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

struct Key {
  int k;
};
struct ElemLessKey {  // only (element, key)
  constexpr bool operator()(const Key& e, int v) const { return e.k < v; }
};
struct KeyLessElem {  // only (key, element)
  constexpr bool operator()(int v, const Key& e) const { return v < e.k; }
};

constexpr bool test() {
  int a[] = {1, 2, 2, 2, 4, 6, 6, 9};
  if (std::lower_bound(a, a + 8, 2) != a + 1) return false;
  if (std::upper_bound(a, a + 8, 2) != a + 4) return false;
  if (std::lower_bound(a, a + 8, 3) != a + 4) return false;
  if (std::upper_bound(a, a + 8, 3) != a + 4) return false;
  if (std::lower_bound(a, a + 8, 0) != a) return false;
  if (std::upper_bound(a, a + 8, 0) != a) return false;
  if (std::lower_bound(a, a + 8, 10) != a + 8) return false;
  if (std::upper_bound(a, a + 8, 9) != a + 8) return false;
  if (std::lower_bound(a, a, 1) != a || std::upper_bound(a, a, 1) != a) return false;

  // descending order with greater
  int d[] = {9, 7, 7, 3, 1};
  if (std::lower_bound(d, d + 5, 7, std::greater<>{}) != d + 1) return false;
  if (std::upper_bound(d, d + 5, 7, std::greater<>{}) != d + 3) return false;

  // only partitioned, not sorted: elements < 5 first, then the rest in any order
  int part[] = {3, 1, 4, 2, 8, 5, 9, 7};
  if (std::lower_bound(part, part + 8, 5) != part + 4) return false;

  // one-directional heterogeneous comparators
  Key ks[] = {{1}, {3}, {3}, {5}};
  if (std::lower_bound(ks, ks + 4, 3, ElemLessKey{}) != ks + 1) return false;
  if (std::upper_bound(ks, ks + 4, 3, KeyLessElem{}) != ks + 3) return false;

  // forward iterators
  ForwardIter<int> f = std::lower_bound(ForwardIter<int>(a), ForwardIter<int>(a + 8), 6);
  if (f.p != a + 5) return false;
  f = std::upper_bound(ForwardIter<int>(a), ForwardIter<int>(a + 8), 6);
  if (f.p != a + 7) return false;

  // ranges forms, projection, sentinel
  if (std::ranges::lower_bound(a, 2) != a + 1) return false;
  if (std::ranges::upper_bound(a, a + 8, 2) != a + 4) return false;
  auto pk = std::ranges::lower_bound(ks, 3, {}, &Key::k);
  static_assert(std::is_same_v<decltype(pk), Key*>);
  if (pk != ks + 1) return false;
  if (std::ranges::upper_bound(ks, 3, {}, &Key::k) != ks + 3) return false;
  ForwardRange<int> fr{a, a + 8};
  if (std::ranges::lower_bound(fr, 4).p != a + 4) return false;
  if (std::ranges::upper_bound(fr.begin(), fr.end(), 4).p != a + 5) return false;
  return true;
}

static_assert(test());

constexpr int N = 1000;
int big[N];

int main() {
  CHECK(test());
  for (int i = 0; i < N; ++i) big[i] = i / 2;
  const int bound = floor_log2(N) + 2;
  for (int v = -1; v <= N / 2 + 1; ++v) {
    int comps = 0;
    int* lb = std::lower_bound(big, big + N, v, CountingLess{&comps});
    CHECK(comps <= bound);
    CHECK(lb == big + (v < 0 ? 0 : v > N / 2 ? N : 2 * v));
    comps = 0;
    int* ub = std::upper_bound(big, big + N, v, CountingLess{&comps});
    CHECK(comps <= bound);
    CHECK(ub == big + (v < 0 ? 0 : v >= N / 2 ? N : 2 * v + 2));
    // forward iterators: still logarithmic comparisons
    comps = 0;
    ForwardIter<int> f = std::lower_bound(ForwardIter<int>(big), ForwardIter<int>(big + N), v, CountingLess{&comps});
    CHECK(comps <= bound);
    CHECK(f.p == lb);
    // ranges with a projection: comparisons and projections
    comps = 0;
    int projs = 0;
    int* r = std::ranges::upper_bound(big, v, CountingLess{&comps}, CountingProj{&projs});
    CHECK(r == ub);
    CHECK(comps <= bound);
    CHECK(projs <= bound);
  }
  return 0;
}
