// [alg.partitions]: stable_partition "Places all the elements e in [first, last) that
// satisfy E(e) before all the elements that do not. The relative order of the elements in
// both groups is preserved." Returns the boundary i (ranges: {i, last}). Bidirectional
// iterators suffice; "Exactly N applications of the predicate and projection". The
// non-parallel overloads are constexpr.
#include <algorithm>
#include <ranges>
#include <type_traits>
#include "sort_support.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

constexpr bool check(const KV* a, int n, int mid, int (*cls)(const KV&)) {
  // trues first, falses after, each group in increasing id order
  for (int i = 0; i < n; ++i)
    if ((cls(a[i]) != 0) != (i < mid)) return false;
  for (int i = 1; i < n; ++i)
    if (i != mid && a[i].id < a[i - 1].id) return false;
  return true;
}
constexpr int odd_key(const KV& x) { return x.key % 2; }

constexpr bool test() {
  {
    KV a[20];
    Lcg g{3};
    int trues = 0;
    for (int i = 0; i < 20; ++i) {
      a[i] = {static_cast<int>(g() % 10), i};
      trues += a[i].key % 2;
    }
    int calls = 0;
    KV* m = std::stable_partition(a, a + 20, [&calls](const KV& x) {
      ++calls;
      return x.key % 2 == 1;
    });
    if (calls != 20 || m != a + trues || !check(a, 20, trues, odd_key)) return false;
  }
  {
    // bidirectional iterators
    KV a[] = {{1, 0}, {2, 1}, {3, 2}, {4, 3}, {5, 4}, {6, 5}};
    BidiIter<KV> m = std::stable_partition(BidiIter<KV>(a), BidiIter<KV>(a + 6), [](const KV& x) { return x.key % 2 == 1; });
    if (m.p != a + 3 || !check(a, 6, 3, odd_key)) return false;
  }
  {
    // edge cases
    KV a[] = {{2, 0}, {4, 1}};
    if (std::stable_partition(a, a + 2, [](const KV& x) { return x.key % 2 == 1; }) != a) return false;
    if (a[0].id != 0 || a[1].id != 1) return false;
    if (std::stable_partition(a, a + 2, [](const KV&) { return true; }) != a + 2) return false;
    if (std::stable_partition(a, a, [](const KV&) { return true; }) != a) return false;
  }
  {
    // ranges: projection, {i, last}, bidirectional non-common range
    KV a[] = {{1, 0}, {8, 1}, {3, 2}, {6, 3}, {7, 4}};
    auto r = std::ranges::stable_partition(a, [](int k) { return k % 2 == 1; }, &KV::key);
    static_assert(std::is_same_v<decltype(r), std::ranges::subrange<KV*>>);
    if (r.begin() != a + 3 || r.end() != a + 5 || !check(a, 5, 3, odd_key)) return false;
    KV b[] = {{2, 0}, {1, 1}, {4, 2}, {3, 3}};
    BidiRange<KV> br{b, b + 4};
    auto rb = std::ranges::stable_partition(br, [](int k) { return k % 2 == 1; }, &KV::key);
    if (rb.begin().p != b + 2 || rb.end().p != b + 4 || !check(b, 4, 2, odd_key)) return false;
  }
  return true;
}

static_assert(test());

KV big[3000];

int main() {
  CHECK(test());
  Lcg g{17};
  int trues = 0;
  for (int i = 0; i < 3000; ++i) {
    big[i] = {static_cast<int>(g() % 100), i};
    trues += big[i].key % 2;
  }
  int pc = 0, jc = 0;
  auto r = std::ranges::stable_partition(big, [&pc](int k) { ++pc; return k % 2 == 1; },
                                         [&jc](const KV& x) { ++jc; return x.key; });
  CHECK(pc == 3000 && jc == 3000);
  CHECK(r.begin() == big + trues);
  CHECK(check(big, 3000, trues, odd_key));
  return 0;
}
