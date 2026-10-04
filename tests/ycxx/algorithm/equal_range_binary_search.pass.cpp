// [equal.range]: returns {lower_bound(first, last, value, comp), upper_bound(first, last,
// value, comp)} (ranges: a subrange of the two, with proj); "At most 2 * log2(last - first)
// + O(1) comparisons and projections". [binary.search]: "true if and only if for some
// iterator i in the range [first, last), !bool(invoke(comp, invoke(proj, *i), value)) &&
// !bool(invoke(comp, value, invoke(proj, *i))) is true"; "At most log2(last - first) + O(1)
// comparisons and projections".
#include <algorithm>
#include <functional>
#include <ranges>
#include <type_traits>
#include <utility>
#include "sort_support.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

struct Key {
  int k;
};

constexpr bool test() {
  int a[] = {1, 3, 3, 3, 5, 8};
  auto er = std::equal_range(a, a + 6, 3);
  static_assert(std::is_same_v<decltype(er), std::pair<int*, int*>>);
  if (er.first != a + 1 || er.second != a + 4) return false;
  er = std::equal_range(a, a + 6, 4);
  if (er.first != a + 4 || er.second != a + 4) return false;
  er = std::equal_range(a, a + 6, 0);
  if (er.first != a || er.second != a) return false;
  er = std::equal_range(a, a + 6, 9);
  if (er.first != a + 6 || er.second != a + 6) return false;
  er = std::equal_range(a, a, 1);
  if (er.first != a || er.second != a) return false;
  int d[] = {8, 5, 5, 1};
  er = std::equal_range(d, d + 4, 5, std::greater<>{});
  if (er.first != d + 1 || er.second != d + 3) return false;
  auto fer = std::equal_range(ForwardIter<int>(a), ForwardIter<int>(a + 6), 3);
  if (fer.first.p != a + 1 || fer.second.p != a + 4) return false;

  if (!std::binary_search(a, a + 6, 5) || std::binary_search(a, a + 6, 4)) return false;
  if (std::binary_search(a, a, 1)) return false;
  if (!std::binary_search(a, a + 6, 1) || !std::binary_search(a, a + 6, 8)) return false;
  if (!std::binary_search(d, d + 4, 1, std::greater<>{}) || std::binary_search(d, d + 4, 2, std::greater<>{}))
    return false;
  if (!std::binary_search(ForwardIter<int>(a), ForwardIter<int>(a + 6), 3)) return false;
  // equivalence, not equality: comparator on tens digit
  int t[] = {11, 15, 23, 37};
  auto tens = [](int x, int y) { return x / 10 < y / 10; };
  if (!std::binary_search(t, t + 4, 19, tens) || std::binary_search(t, t + 4, 41, tens)) return false;
  er = std::equal_range(t, t + 4, 10, tens);
  if (er.first != t || er.second != t + 2) return false;

  // ranges
  Key ks[] = {{1}, {2}, {2}, {4}};
  auto rs = std::ranges::equal_range(ks, 2, {}, &Key::k);
  static_assert(std::is_same_v<decltype(rs), std::ranges::subrange<Key*>>);
  if (rs.begin() != ks + 1 || rs.end() != ks + 3) return false;
  auto rs2 = std::ranges::equal_range(a, a + 6, 3);
  if (rs2.begin() != a + 1 || rs2.end() != a + 4) return false;
  if (!std::ranges::binary_search(ks, 4, {}, &Key::k) || std::ranges::binary_search(ks, 3, {}, &Key::k)) return false;
  if (!std::ranges::binary_search(a, a + 6, 8)) return false;
  ForwardRange<int> fr{a, a + 6};
  auto frs = std::ranges::equal_range(fr, 3);
  if (frs.begin().p != a + 1 || frs.end().p != a + 4) return false;
  if (!std::ranges::binary_search(fr, 1)) return false;
  return true;
}

static_assert(test());

constexpr int N = 1000;
int big[N];

int main() {
  CHECK(test());
  for (int i = 0; i < N; ++i) big[i] = i / 3;
  const int lg = floor_log2(N);
  for (int v = -1; v <= N / 3 + 1; ++v) {
    int comps = 0;
    auto er = std::equal_range(big, big + N, v, CountingLess{&comps});
    CHECK(comps <= 2 * lg + 4);
    CHECK(er.first == std::lower_bound(big, big + N, v));
    CHECK(er.second == std::upper_bound(big, big + N, v));
    comps = 0;
    bool found = std::binary_search(big, big + N, v, CountingLess{&comps});
    CHECK(comps <= lg + 2);
    CHECK(found == (v >= 0 && v <= (N - 1) / 3));
    comps = 0;
    int projs = 0;
    auto rs = std::ranges::equal_range(big, v, CountingLess{&comps}, CountingProj{&projs});
    CHECK(comps <= 2 * lg + 4 && projs <= 2 * lg + 4);
    CHECK(rs.begin() == er.first && rs.end() == er.second);
    comps = projs = 0;
    CHECK(std::ranges::binary_search(big, v, CountingLess{&comps}, CountingProj{&projs}) == found);
    CHECK(comps <= lg + 2 && projs <= lg + 2);
  }
  return 0;
}
