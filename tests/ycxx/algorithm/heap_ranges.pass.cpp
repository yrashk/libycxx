// [alg.heap.operations]: ranges::push_heap, pop_heap, make_heap and sort_heap return last
// (borrowed_iterator_t<R> for the range forms); ranges::is_heap_until returns the end of
// the longest heap prefix; all take a comparator and a projection, and accept
// random_access_iterator class types with a sentinel.
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
    P ps[] = {{3}, {7}, {1}, {9}, {4}};
    auto m = std::ranges::make_heap(ps, {}, &P::k);
    static_assert(std::is_same_v<decltype(m), P*>);
    if (m != ps + 5 || ps[0].k != 9) return false;
    for (int i = 1; i < 5; ++i)
      if (ps[(i - 1) / 2].k < ps[i].k) return false;
    if (!std::ranges::is_heap(ps, {}, &P::k)) return false;
    if (std::ranges::is_heap_until(ps, {}, &P::k) != ps + 5) return false;
    auto p = std::ranges::pop_heap(ps, {}, &P::k);
    if (p != ps + 5 || ps[4].k != 9 || ps[0].k != 7) return false;
    ps[4].k = 8;
    auto u = std::ranges::push_heap(ps, {}, &P::k);
    if (u != ps + 5 || ps[0].k != 8) return false;
    auto s = std::ranges::sort_heap(ps, {}, &P::k);
    if (s != ps + 5) return false;
    for (int i = 1; i < 5; ++i)
      if (ps[i - 1].k > ps[i].k) return false;
  }
  {
    // iterator/sentinel forms with a class-type iterator and a comparator
    int a[] = {4, 6, 2, 8, 5, 1};
    RandomIter<int> b(a);
    PtrSentinel<int> e{a + 6};
    if (std::ranges::make_heap(b, e, std::ranges::greater{}).p != a + 6) return false;
    if (a[0] != 1 || !std::ranges::is_heap(b, e, std::ranges::greater{})) return false;
    if (std::ranges::pop_heap(b, e, std::ranges::greater{}).p != a + 6 || a[5] != 1) return false;
    if (std::ranges::push_heap(b, e, std::ranges::greater{}).p != a + 6 || a[0] != 1) return false;
    if (std::ranges::sort_heap(b, e, std::ranges::greater{}).p != a + 6) return false;
    for (int i = 1; i < 6; ++i)
      if (a[i - 1] < a[i]) return false;
    // is_heap_until stops at the first violation
    int h[] = {9, 5, 7, 4, 1, 8};  // a[5] = 8 > a[2] = 7
    if (std::ranges::is_heap_until(h) != h + 5) return false;
    if (std::ranges::is_heap_until(RandomIter<int>(h), PtrSentinel<int>{h + 6}).p != h + 5) return false;
  }
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
