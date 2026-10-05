// [alg.heap.operations]: a range [a, b) is a heap with respect to comp and proj if "for all
// i, 0 < i < N, bool(invoke(comp, invoke(proj, a[floor((i - 1)/2)]), invoke(proj, a[i])))
// is false". push_heap "Places the value in the location last - 1 into the resulting heap
// [first, last)"; pop_heap "Swaps the value in the location first with the value in the
// location last - 1 and makes [first, last - 1) into a heap"; make_heap constructs a heap;
// sort_heap sorts a heap; is_heap_until returns "The last iterator i in [first, last] for
// which the range [first, i) is a heap"; ranges forms return last.
// COUNTERPART: libcxx:algorithms/alg.sorting/alg.heap.operations/(make.heap/make_heap|sort.heap/sort_heap)(_comp)?.pass.cpp
// COUNTERPART: libcxx:algorithms/robust_against_proxy_iterators_lifetime_bugs.pass.cpp
#include <algorithm>
#include <functional>
#include <ranges>
#include <type_traits>
#include "sort_support.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

template <class Comp = std::less<>>
constexpr bool heap_def(const int* a, int n, Comp comp = {}) {
  for (int i = 1; i < n; ++i)
    if (comp(a[(i - 1) / 2], a[i])) return false;
  return true;
}

constexpr bool test() {
  {
    int a[] = {3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
    int orig[11];
    for (int i = 0; i < 11; ++i) orig[i] = a[i];
    std::make_heap(a, a + 11);
    if (!heap_def(a, 11) || a[0] != 9 || !same_multiset(a, orig, 11)) return false;
    if (!std::is_heap(a, a + 11) || std::is_heap_until(a, a + 11) != a + 11) return false;
    // pop_heap moves the largest to the back
    std::pop_heap(a, a + 11);
    if (a[10] != 9 || !heap_def(a, 10) || a[0] != 6) return false;
    // push_heap integrates the back element
    a[10] = 10;
    std::push_heap(a, a + 11);
    if (a[0] != 10 || !heap_def(a, 11)) return false;
    std::sort_heap(a, a + 11);
    if (!sorted_by(a, a + 11)) return false;
  }
  {
    // repeated pops give descending order
    int a[] = {4, 8, 1, 7, 3, 3};
    std::make_heap(a, a + 6);
    int want[] = {8, 7, 4, 3, 3, 1};
    for (int n = 6; n > 0; --n) {
      if (a[0] != want[6 - n]) return false;
      std::pop_heap(a, a + n);
      if (a[n - 1] != want[6 - n]) return false;
    }
  }
  {
    // is_heap_until on non-heaps
    int a[] = {9, 5, 8, 6, 1};  // a[3] = 6 > a[1] = 5
    if (std::is_heap_until(a, a + 5) != a + 3 || std::is_heap(a, a + 5)) return false;
    if (!std::is_heap(a, a) || !std::is_heap(a, a + 1) || std::is_heap_until(a, a) != a) return false;
    int eq[] = {2, 2, 2};
    if (!std::is_heap(eq, eq + 3)) return false;
  }
  {
    // comparator (min-heap) and class-type iterators
    int a[] = {5, 2, 8, 1, 9};
    std::make_heap(RandomIter<int>(a), RandomIter<int>(a + 5), std::greater<>{});
    if (a[0] != 1 || !heap_def(a, 5, std::greater<>{})) return false;
    if (!std::is_heap(a, a + 5, std::greater<>{}) || std::is_heap(a, a + 5)) return false;
    if (std::is_heap_until(a, a + 5, std::greater<>{}) != a + 5) return false;
    std::pop_heap(RandomIter<int>(a), RandomIter<int>(a + 5), std::greater<>{});
    if (a[4] != 1 || a[0] != 2) return false;
    std::push_heap(RandomIter<int>(a), RandomIter<int>(a + 5), std::greater<>{});
    std::sort_heap(RandomIter<int>(a), RandomIter<int>(a + 5), std::greater<>{});
    if (a[0] != 9 || a[4] != 1) return false;
  }
  {
    // trivial sizes
    int a[] = {1};
    std::make_heap(a, a);
    std::make_heap(a, a + 1);
    std::push_heap(a, a + 1);
    std::pop_heap(a, a + 1);
    std::sort_heap(a, a + 1);
    if (a[0] != 1) return false;
  }
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
