// [alg.sorting]: sort, stable_sort, partial_sort, nth_element, inplace_merge,
// stable_partition, partition, the heap operations and next/prev_permutation only require
// Cpp17MoveConstructible / Cpp17MoveAssignable and swappable elements (sortable / permutable
// for the ranges forms), so they work with a move-only type, in constant evaluation too.
#include <algorithm>
#include <ranges>
#include "sort_support.hpp"
#include "check.hpp"

template <int N>
constexpr bool asc(const MoveOnly (&a)[N]) {
  for (int i = 1; i < N; ++i)
    if (a[i].v < a[i - 1].v) return false;
  return true;
}

constexpr bool test() {
  {
    MoveOnly a[] = {5, 3, 8, 1, 9, 2, 7};
    std::sort(a, a + 7);
    if (!asc(a)) return false;
  }
  {
    MoveOnly a[] = {5, 3, 8, 1, 9, 2, 7};
    std::ranges::stable_sort(a);
    if (!asc(a)) return false;
  }
  {
    MoveOnly a[] = {5, 3, 8, 1, 9, 2, 7};
    std::partial_sort(a, a + 3, a + 7);
    if (a[0].v != 1 || a[1].v != 2 || a[2].v != 3) return false;
  }
  {
    MoveOnly a[] = {5, 3, 8, 1, 9, 2, 7};
    std::ranges::nth_element(a, a + 3);
    if (a[3].v != 5) return false;
  }
  {
    MoveOnly a[] = {1, 4, 7, 2, 3, 9};
    std::inplace_merge(a, a + 3, a + 6);
    if (!asc(a)) return false;
  }
  {
    MoveOnly a[] = {1, 2, 3, 4, 5, 6};
    auto r = std::ranges::stable_partition(a, [](const MoveOnly& m) { return m.v % 2 == 0; });
    if (r.begin() != a + 3 || a[0].v != 2 || a[1].v != 4 || a[2].v != 6 || a[3].v != 1 || a[5].v != 5) return false;
    std::partition(a, a + 6, [](const MoveOnly& m) { return m.v > 3; });
    if (a[0].v <= 3 || a[1].v <= 3 || a[2].v <= 3) return false;
  }
  {
    MoveOnly a[] = {4, 1, 3, 2};
    std::make_heap(a, a + 4);
    if (a[0].v != 4) return false;
    std::pop_heap(a, a + 4);
    if (a[3].v != 4) return false;
    std::push_heap(a, a + 4);
    std::ranges::sort_heap(a);
    if (!asc(a)) return false;
  }
  {
    MoveOnly a[] = {1, 2, 3};
    int n = 1;
    while (std::ranges::next_permutation(a).found) ++n;
    if (n != 6) return false;
    if (std::prev_permutation(a, a + 3)) return false;
  }
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
