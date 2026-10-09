// Libycxx performance policy: 12 N comparisons averaged over the fixed seeds and nth positions.
// The finite comparison budget is a regression heuristic, not an exact draft bound or
// proof of asymptotic/average-case complexity. Normative effects remain independent.
// [alg.nth.element]: "After nth_element the element in the position pointed to by nth is
// the element that would be in that position if the whole range were sorted with respect to
// comp and proj, unless nth == last. Also for every iterator i in the range [first, nth) and
// every iterator j in the range [nth, last) it holds that: bool(invoke(comp, invoke(proj,
// *j), invoke(proj, *i))) is false." ranges forms return last. "Complexity: For the
// non-parallel algorithm overloads, linear on average" (checked on random inputs with a
// generous constant of 12 N comparisons, averaged over several seeds).
// COUNTERPART: libcxx:algorithms/alg.sorting/alg.nth.element/nth_element(_comp)?.pass.cpp
#include <algorithm>
#include <functional>
#include <ranges>
#include <type_traits>
#include "sort_support.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

struct P {
  int k;
};

template <class Comp = std::less<>>
constexpr bool check_nth(const int* a, int n, int nth, const int* sorted, Comp comp = {}) {
  if (a[nth] != sorted[nth]) return false;
  for (int i = 0; i < nth; ++i)
    for (int j = nth; j < n; ++j)
      if (comp(a[j], a[i])) return false;
  return true;
}

constexpr bool test() {
  {
    int base[] = {5, 9, 1, 7, 3, 3, 8, 0, 2, 6, 4};
    int sorted[] = {0, 1, 2, 3, 3, 4, 5, 6, 7, 8, 9};
    for (int nth = 0; nth < 11; ++nth) {
      int a[11];
      for (int i = 0; i < 11; ++i) a[i] = base[i];
      std::nth_element(a, a + nth, a + 11);
      if (!check_nth(a, 11, nth, sorted)) return false;
      if (!same_multiset(a, base, 11)) return false;
    }
  }
  {
    // nth == last: valid, no requirement other than a permutation
    int a[] = {3, 1, 2};
    std::nth_element(a, a + 3, a + 3);
    int b[] = {3, 1, 2};
    if (!same_multiset(a, b, 3)) return false;
    std::nth_element(a, a, a);  // empty
  }
  {
    // comparator and class-type iterators
    int a[] = {1, 2, 3, 4, 5, 6, 7};
    int sorted[] = {7, 6, 5, 4, 3, 2, 1};
    std::nth_element(RandomIter<int>(a), RandomIter<int>(a + 2), RandomIter<int>(a + 7), std::greater<>{});
    if (!check_nth(a, 7, 2, sorted, std::greater<>{})) return false;
  }
  {
    // all equal
    int a[] = {4, 4, 4, 4, 4};
    std::nth_element(a, a + 2, a + 5);
    for (int x : a)
      if (x != 4) return false;
  }
  {
    // ranges forms with projection
    P ps[] = {{5}, {1}, {4}, {2}, {3}};
    auto r = std::ranges::nth_element(ps, ps + 1, {}, &P::k);
    static_assert(std::is_same_v<decltype(r), P*>);
    if (r != ps + 5 || ps[1].k != 2 || ps[0].k != 1) return false;
    int a[] = {9, 7, 8, 1};
    RandomIter<int> ri = std::ranges::nth_element(RandomIter<int>(a), RandomIter<int>(a + 3), PtrSentinel<int>{a + 4});
    if (ri.p != a + 4 || a[3] != 9) return false;
    // nth == last through ranges
    if (std::ranges::nth_element(a, a + 4) != a + 4) return false;
  }
  return true;
}

static_assert(test());

constexpr int N = 4000;
int big[N], sorted_big[N];

int main() {
  CHECK(test());
  long long total = 0;
  int runs = 0;
  for (unsigned seed = 1; seed <= 8; ++seed) {
    for (int nth : {0, N / 4, N / 2, N - 1}) {
      fill_pattern(big, N, Pattern::random, seed);
      for (int i = 0; i < N; ++i) sorted_big[i] = big[i];
      std::sort(sorted_big, sorted_big + N);
      int comps = 0;
      std::nth_element(big, big + nth, big + N, CountingLess{&comps});
      total += comps;
      ++runs;
      CHECK(big[nth] == sorted_big[nth]);
      CHECK(same_multiset(big, sorted_big, N));
      for (int i = 0; i < nth; ++i) CHECK(!(big[nth] < big[i]));
      for (int j = nth + 1; j < N; ++j) CHECK(!(big[j] < big[nth]));
    }
  }
  CHECK(total / runs <= 12LL * N);
  // other shapes: only the effects
  for (Pattern p : all_patterns) {
    fill_pattern(big, N, p);
    for (int i = 0; i < N; ++i) sorted_big[i] = big[i];
    std::sort(sorted_big, sorted_big + N);
    std::ranges::nth_element(big, big + N / 3);
    CHECK(big[N / 3] == sorted_big[N / 3]);
    CHECK(same_multiset(big, sorted_big, N));
  }
  return 0;
}
