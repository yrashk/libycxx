// [partial.sort]: "Places the first middle - first elements from the range [first, last)
// as sorted with respect to comp and proj into the range [first, middle). The rest of the
// elements in the range [middle, last) are placed in an unspecified order." ranges forms
// return last. "Complexity: Approximately (last - first) * log(middle - first)
// comparisons, and twice as many projections" (checked at run time with a generous
// constant: 3 N log2 M + 3 N).
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

constexpr bool test() {
  {
    int a[] = {9, 1, 8, 2, 7, 3, 6, 4, 5, 0};
    int orig[10];
    for (int i = 0; i < 10; ++i) orig[i] = a[i];
    std::partial_sort(a, a + 4, a + 10);
    if (a[0] != 0 || a[1] != 1 || a[2] != 2 || a[3] != 3) return false;
    // the rest is a permutation of the remaining values
    for (int i = 4; i < 10; ++i)
      if (a[i] < 4) return false;
    if (!same_multiset(a, orig, 10)) return false;
  }
  {
    int a[] = {3, 1, 2};
    std::partial_sort(a, a, a + 3);  // middle == first: no requirement on the order
    int b[] = {3, 1, 2};
    if (!same_multiset(a, b, 3)) return false;
    std::partial_sort(a, a + 3, a + 3);  // middle == last: fully sorted
    if (a[0] != 1 || a[1] != 2 || a[2] != 3) return false;
  }
  {
    int a[] = {1, 5, 2, 4, 3};
    std::partial_sort(RandomIter<int>(a), RandomIter<int>(a + 2), RandomIter<int>(a + 5), std::greater<>{});
    if (a[0] != 5 || a[1] != 4) return false;
  }
  {
    // duplicates among the smallest elements
    int a[] = {2, 2, 9, 1, 1, 8, 1};
    std::partial_sort(a, a + 4, a + 7);
    if (a[0] != 1 || a[1] != 1 || a[2] != 1 || a[3] != 2) return false;
  }
  {
    // ranges forms
    P ps[] = {{4}, {2}, {5}, {1}, {3}};
    auto r = std::ranges::partial_sort(ps, ps + 3, {}, &P::k);
    static_assert(std::is_same_v<decltype(r), P*>);
    if (r != ps + 5 || ps[0].k != 1 || ps[1].k != 2 || ps[2].k != 3) return false;
    int a[] = {6, 5, 4, 3, 2, 1};
    RandomIter<int> ri =
        std::ranges::partial_sort(RandomIter<int>(a), RandomIter<int>(a + 2), PtrSentinel<int>{a + 6});
    if (ri.p != a + 6 || a[0] != 1 || a[1] != 2) return false;
  }
  return true;
}

static_assert(test());

constexpr int N = 2000;
int big[N], orig[N];

int main() {
  CHECK(test());
  for (int m : {1, 10, 100, 1000, 2000}) {
    for (Pattern p : all_patterns) {
      fill_pattern(big, N, p);
      for (int i = 0; i < N; ++i) orig[i] = big[i];
      int comps = 0, projs = 0;
      std::ranges::partial_sort(big, big + m, CountingLess{&comps}, CountingProj{&projs});
      CHECK(comps <= 3LL * N * ceil_log2(m) + 3LL * N);
      CHECK(projs <= 2 * comps);
      CHECK(sorted_by(big, big + m));
      for (int i = m; i < N; ++i) CHECK(!(big[i] < big[m - 1]));
      CHECK(same_multiset(big, orig, N));
    }
  }
  return 0;
}
