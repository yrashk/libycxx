// [sort]: sort "Sorts the elements in the range [first, last) with respect to comp and
// proj"; ranges::sort "Returns: last" (borrowed_iterator_t<R> for the range form). The
// overloads are constexpr and take random-access iterators (class types, not only
// pointers) with any sentinel_for<I> for ranges::sort. [alg.sorting.general]/5 defines
// "sorted with respect to comp and proj".
// COUNTERPART: libcxx:algorithms/alg.sorting/alg.sort/sort/sort_constexpr(_comp)?.pass.cpp
#include <algorithm>
#include <functional>
#include <ranges>
#include <type_traits>
#include "sort_support.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

struct Rec {
  int a;
  int b;
};

template <int N>
constexpr bool is_ascending(const int (&x)[N]) {
  for (int i = 1; i < N; ++i)
    if (x[i] < x[i - 1]) return false;
  return true;
}

constexpr bool test() {
  {
    int a[] = {5, 3, 9, 1, 3, 7, 0, 2, 8, 6};
    std::sort(a, a + 10);
    int want[] = {0, 1, 2, 3, 3, 5, 6, 7, 8, 9};
    for (int i = 0; i < 10; ++i)
      if (a[i] != want[i]) return false;
  }
  {
    int a[] = {5, 3, 9, 1, 3, 7, 0, 2, 8, 6};
    std::sort(a, a + 10, std::greater<>{});
    for (int i = 1; i < 10; ++i)
      if (a[i] > a[i - 1]) return false;
  }
  {
    // empty and single-element ranges
    int a[] = {4};
    std::sort(a, a);
    std::sort(a, a + 1);
    if (a[0] != 4) return false;
  }
  {
    // class-type random-access iterators
    int a[] = {3, 1, 2, 5, 4, 0};
    std::sort(RandomIter<int>(a), RandomIter<int>(a + 6));
    if (!is_ascending(a)) return false;
  }
  {
    // a larger input exercising whatever partitioning scheme is used
    int a[300];
    fill_pattern(a, 300, Pattern::random, 7);
    std::sort(a, a + 300);
    if (!is_ascending(a)) return false;
    fill_pattern(a, 300, Pattern::few_values, 3);
    std::sort(a, a + 300);
    if (!is_ascending(a)) return false;
  }
  {
    // ranges::sort: iterator/sentinel form returns last
    int a[] = {9, 8, 7, 1, 2, 3};
    RandomIter<int> r = std::ranges::sort(RandomIter<int>(a), PtrSentinel<int>{a + 6});
    if (r.p != a + 6 || !is_ascending(a)) return false;
    // range form with a non-common range
    int b[] = {4, 2, 6, 1};
    RandomRange<int> rr{b, b + 4};
    auto rb = std::ranges::sort(rr);
    static_assert(std::is_same_v<decltype(rb), RandomIter<int>>);
    if (rb.p != b + 4 || !is_ascending(b)) return false;
  }
  {
    // ranges::sort with comparator and projection
    Rec rs[] = {{3, 0}, {1, 1}, {2, 2}, {0, 3}};
    auto it = std::ranges::sort(rs, std::ranges::greater{}, &Rec::a);
    static_assert(std::is_same_v<decltype(it), Rec*>);
    if (it != rs + 4) return false;
    if (rs[0].a != 3 || rs[1].a != 2 || rs[2].a != 1 || rs[3].a != 0) return false;
    // projection to a member via a lambda; comparator is ranges::less by default
    std::ranges::sort(rs, {}, [](const Rec& r) { return r.b; });
    for (int i = 0; i < 4; ++i)
      if (rs[i].b != i) return false;
  }
  {
    // std::sort with a function pointer comparator
    int a[] = {1, 4, 2, 3};
    bool (*gt)(int, int) = [](int x, int y) { return x > y; };
    std::sort(a, a + 4, gt);
    if (a[0] != 4 || a[3] != 1) return false;
  }
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
