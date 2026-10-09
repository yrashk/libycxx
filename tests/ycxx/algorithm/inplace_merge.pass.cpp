// Libycxx performance policy: N ceil(log2 N) comparisons for the fixed merge workloads.
// The finite comparison budget is a regression heuristic, not an exact draft bound or
// proof of asymptotic/average-case complexity. Normative effects remain independent.
// [alg.merge]: inplace_merge "Merges two sorted consecutive ranges [first, middle) and
// [middle, last), putting the result of the merge into the range [first, last). The
// resulting range is sorted with respect to comp and proj." "Remarks: Stable." Bidirectional
// iterators suffice; ranges forms return last; the non-parallel overloads are constexpr.
// Complexity: at most N - 1 comparisons with enough memory, otherwise O(N log N); checked as
// at most N log2 N comparisons, and twice as many projections as comparisons.
#include <algorithm>
#include <functional>
#include <ranges>
#include <type_traits>
#include "sort_support.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

constexpr bool stable_sorted(const KV* a, int n) {
  for (int i = 1; i < n; ++i) {
    if (a[i].key < a[i - 1].key) return false;
    if (a[i].key == a[i - 1].key && a[i].id < a[i - 1].id) return false;
  }
  return true;
}

constexpr bool test() {
  {
    // ids increase from the first half to the second, so stability = increasing ids
    KV a[] = {{1, 0}, {3, 1}, {3, 2}, {5, 3}, {0, 4}, {3, 5}, {4, 6}, {6, 7}};
    std::inplace_merge(a, a + 4, a + 8);
    if (!stable_sorted(a, 8)) return false;
  }
  {
    // bidirectional iterators with a comparator
    int a[] = {9, 6, 2, 8, 7, 1};
    std::inplace_merge(BidiIter<int>(a), BidiIter<int>(a + 3), BidiIter<int>(a + 6), std::greater<>{});
    int want[] = {9, 8, 7, 6, 2, 1};
    for (int i = 0; i < 6; ++i)
      if (a[i] != want[i]) return false;
  }
  {
    // empty halves
    int a[] = {1, 2, 3};
    std::inplace_merge(a, a, a + 3);
    std::inplace_merge(a, a + 3, a + 3);
    if (a[0] != 1 || a[2] != 3) return false;
  }
  {
    // ranges: projection, iterator/sentinel and range forms return last
    KV a[] = {{2, 0}, {4, 1}, {1, 2}, {2, 3}, {4, 4}};
    auto r = std::ranges::inplace_merge(a, a + 2, {}, &KV::key);
    static_assert(std::is_same_v<decltype(r), KV*>);
    if (r != a + 5 || !stable_sorted(a, 5)) return false;
    KV b[] = {{1, 0}, {5, 1}, {0, 2}, {5, 3}};
    BidiIter<KV> rb = std::ranges::inplace_merge(BidiIter<KV>(b), BidiIter<KV>(b + 2), PtrSentinel<KV>{b + 4}, {}, &KV::key);
    if (rb.p != b + 4 || !stable_sorted(b, 4)) return false;
    KV c[] = {{3, 0}, {1, 1}};
    BidiRange<KV> cr{c, c + 2};
    auto rc = std::ranges::inplace_merge(cr, BidiIter<KV>(c + 1), {}, &KV::key);
    if (rc.p != c + 2 || c[0].id != 1) return false;
  }
  return true;
}

static_assert(test());

KV big[2000];

int main() {
  CHECK(test());
  for (unsigned seed = 1; seed <= 4; ++seed) {
    for (int mid : {1, 500, 1000, 1999}) {
      Lcg g{seed};
      for (int i = 0; i < 2000; ++i) big[i] = {static_cast<int>(g() % 64), i};
      std::stable_sort(big, big + mid);
      std::stable_sort(big + mid, big + 2000);
      for (int i = 0; i < 2000; ++i) big[i].id = i;
      int key_for_id[2000];
      for (int i = 0; i < 2000; ++i) key_for_id[i] = big[i].key;
      int comps = 0, projs = 0;
      std::ranges::inplace_merge(big, big + mid, [&comps](int x, int y) { ++comps; return x < y; },
                                 [&projs](const KV& v) { ++projs; return v.key; });
      CHECK(stable_sorted(big, 2000));
      bool seen[2000] = {};
      for (const KV& e : big) {
        CHECK(0 <= e.id && e.id < 2000 && !seen[e.id]);
        seen[e.id] = true;
        CHECK(e.key == key_for_id[e.id]);
      }
      CHECK(comps <= 2000 * ceil_log2(2000));
      CHECK(projs <= 2 * comps);
    }
  }
  return 0;
}
