// [stable.sort]: stable_sort sorts [first, last) with respect to comp and proj and is
// "Stable ([algorithm.stable])": the relative order of equivalent elements is preserved.
// ranges::stable_sort returns last. The non-parallel overloads are constexpr.
#include <algorithm>
#include <functional>
#include <ranges>
#include <type_traits>
#include "sort_support.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

constexpr bool check_stable(const KV* a, int n) {
  for (int i = 1; i < n; ++i) {
    if (a[i].key < a[i - 1].key) return false;
    if (a[i].key == a[i - 1].key && a[i].id < a[i - 1].id) return false;
  }
  return true;
}

constexpr bool test() {
  {
    KV a[40];
    Lcg g{5};
    for (int i = 0; i < 40; ++i) a[i] = {static_cast<int>(g() % 5), i};
    std::stable_sort(a, a + 40);
    if (!check_stable(a, 40)) return false;
  }
  {
    // with a comparator (descending keys), still stable
    KV a[30];
    Lcg g{9};
    for (int i = 0; i < 30; ++i) a[i] = {static_cast<int>(g() % 4), i};
    std::stable_sort(a, a + 30, [](const KV& x, const KV& y) { return x.key > y.key; });
    for (int i = 1; i < 30; ++i) {
      if (a[i].key > a[i - 1].key) return false;
      if (a[i].key == a[i - 1].key && a[i].id < a[i - 1].id) return false;
    }
  }
  {
    // class-type random-access iterators
    KV a[] = {{2, 0}, {1, 1}, {2, 2}, {1, 3}, {0, 4}};
    std::stable_sort(RandomIter<KV>(a), RandomIter<KV>(a + 5));
    if (!check_stable(a, 5)) return false;
  }
  {
    // ranges::stable_sort with projection; iterator/sentinel and range forms return last
    KV a[] = {{3, 0}, {1, 1}, {3, 2}, {1, 3}, {2, 4}, {1, 5}};
    auto r = std::ranges::stable_sort(a, {}, &KV::key);
    static_assert(std::is_same_v<decltype(r), KV*>);
    if (r != a + 6 || !check_stable(a, 6)) return false;
    KV b[] = {{1, 0}, {0, 1}, {1, 2}, {0, 3}};
    RandomIter<KV> rb = std::ranges::stable_sort(RandomIter<KV>(b), PtrSentinel<KV>{b + 4}, {}, &KV::key);
    if (rb.p != b + 4 || !check_stable(b, 4)) return false;
    // projection onto id with greater: reverses identity order, stability irrelevant
    std::ranges::stable_sort(b, std::ranges::greater{}, &KV::id);
    for (int i = 0; i < 4; ++i)
      if (b[i].id != 3 - i) return false;
  }
  {
    // all-equivalent input stays in its original order
    KV a[] = {{1, 0}, {1, 1}, {1, 2}, {1, 3}};
    std::ranges::stable_sort(a, {}, &KV::key);
    for (int i = 0; i < 4; ++i)
      if (a[i].id != i) return false;
  }
  {
    // empty range
    KV a[1] = {{0, 0}};
    std::stable_sort(a, a);
    if (std::ranges::stable_sort(a + 0, a + 0, {}, &KV::key) != a) return false;
  }
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  // a larger run-time input
  static KV big[3000];
  Lcg g{11};
  for (int i = 0; i < 3000; ++i) big[i] = {static_cast<int>(g() % 50), i};
  std::stable_sort(big, big + 3000);
  CHECK(check_stable(big, 3000));
  return 0;
}
