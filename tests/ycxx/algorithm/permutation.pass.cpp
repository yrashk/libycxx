// [alg.permutation.generators]: next_permutation "transforms it into the next permutation"
// in the lexicographic order with respect to comp and proj; "If no such permutation exists,
// transforms the sequence into the first permutation; that is, the ascendingly-sorted one."
// prev_permutation symmetrically ("the descendingly-sorted one"). Returns B (std) or
// { last, B } (ranges, next_permutation_result / prev_permutation_result =
// in_found_result). Bidirectional iterators suffice; with repeated elements only the
// distinct permutations are visited.
#include <algorithm>
#include <functional>
#include <ranges>
#include <type_traits>
#include "sort_support.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

constexpr bool lex_less(const int* a, const int* b, int n) {
  for (int i = 0; i < n; ++i) {
    if (a[i] < b[i]) return true;
    if (b[i] < a[i]) return false;
  }
  return false;
}

constexpr bool test() {
  {
    // all 24 permutations of 4 distinct values, each strictly greater than the previous
    int a[] = {1, 2, 3, 4};
    int prev[4] = {1, 2, 3, 4};
    int count = 1;
    while (std::next_permutation(a, a + 4)) {
      if (!lex_less(prev, a, 4)) return false;
      for (int i = 0; i < 4; ++i) prev[i] = a[i];
      ++count;
    }
    if (count != 24) return false;
    // wrapped around to ascending order
    if (a[0] != 1 || a[1] != 2 || a[2] != 3 || a[3] != 4) return false;
  }
  {
    int a[] = {4, 3, 2, 1};
    int count = 1;
    while (std::prev_permutation(a, a + 4)) ++count;
    if (count != 24) return false;
    if (a[0] != 4 || a[1] != 3 || a[2] != 2 || a[3] != 1) return false;
  }
  {
    // repeated values: 5! / (2! 2!) = 30 distinct permutations
    int a[] = {1, 1, 2, 2, 3};
    int count = 1;
    while (std::next_permutation(a, a + 5)) ++count;
    if (count != 30) return false;
    int b[] = {3, 2, 2, 1, 1};
    count = 1;
    while (std::prev_permutation(b, b + 5)) ++count;
    if (count != 30) return false;
  }
  {
    // specific steps
    int a[] = {1, 3, 2};
    if (!std::next_permutation(a, a + 3) || a[0] != 2 || a[1] != 1 || a[2] != 3) return false;
    if (!std::prev_permutation(a, a + 3) || a[0] != 1 || a[1] != 3 || a[2] != 2) return false;
    int e[] = {7};
    if (std::next_permutation(e, e) || std::next_permutation(e, e + 1) || std::prev_permutation(e, e + 1)) return false;
  }
  {
    // comparator; bidirectional iterators
    int a[] = {3, 2, 1};  // the first permutation under greater
    int count = 1;
    while (std::next_permutation(BidiIter<int>(a), BidiIter<int>(a + 3), std::greater<>{})) ++count;
    if (count != 6 || a[0] != 3 || a[2] != 1) return false;
    if (std::prev_permutation(BidiIter<int>(a), BidiIter<int>(a + 3), std::greater<>{})) return false;
    if (a[0] != 1 || a[1] != 2 || a[2] != 3) return false;
  }
  {
    // ranges: {last, found}, projection
    KV ks[] = {{2, 0}, {1, 1}, {3, 2}};
    auto r = std::ranges::next_permutation(ks, {}, &KV::key);
    static_assert(std::is_same_v<decltype(r), std::ranges::next_permutation_result<KV*>>);
    static_assert(std::is_same_v<std::ranges::next_permutation_result<KV*>, std::ranges::in_found_result<KV*>>);
    if (r.in != ks + 3 || !r.found) return false;
    if (ks[0].key != 2 || ks[1].key != 3 || ks[2].key != 1) return false;
    auto p = std::ranges::prev_permutation(ks, {}, &KV::key);
    static_assert(std::is_same_v<decltype(p), std::ranges::prev_permutation_result<KV*>>);
    if (p.in != ks + 3 || !p.found || ks[1].key != 1) return false;
    int a[] = {3, 2, 1};
    auto r2 = std::ranges::next_permutation(BidiIter<int>(a), PtrSentinel<int>{a + 3});
    if (r2.in.p != a + 3 || r2.found || a[0] != 1) return false;
    BidiRange<int> br{a, a + 3};
    auto r3 = std::ranges::prev_permutation(br);
    if (r3.in.p != a + 3 || r3.found || a[0] != 3 || a[2] != 1) return false;
  }
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
