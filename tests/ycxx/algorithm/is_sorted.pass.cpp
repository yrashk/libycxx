// [is.sorted]: is_sorted_until "Returns: The last iterator i in [first, last] for which the
// range [first, i) is sorted with respect to comp and proj"; is_sorted is
// is_sorted_until(...) == last. "Complexity: Linear" (at most last - first - 1 comparisons
// suffice; checked as <= N). Works with forward iterators; ranges forms take projections.
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
  int a[] = {1, 2, 2, 3, 1, 4};
  if (std::is_sorted_until(a, a + 6) != a + 4) return false;
  if (std::is_sorted(a, a + 6)) return false;
  if (!std::is_sorted(a, a + 4)) return false;
  if (std::is_sorted_until(a, a) != a || !std::is_sorted(a, a)) return false;
  if (std::is_sorted_until(a, a + 1) != a + 1) return false;
  // equal neighbours are sorted
  int e[] = {5, 5, 5};
  if (!std::is_sorted(e, e + 3)) return false;
  // comparator
  int d[] = {5, 4, 4, 1, 2};
  if (std::is_sorted_until(d, d + 5, std::greater<>{}) != d + 4) return false;
  if (!std::is_sorted(d, d + 4, std::greater<>{})) return false;
  // forward iterators
  ForwardIter<int> fi = std::is_sorted_until(ForwardIter<int>(a), ForwardIter<int>(a + 6));
  if (fi.p != a + 4) return false;
  if (std::is_sorted(ForwardIter<int>(a), ForwardIter<int>(a + 6))) return false;

  // ranges
  P ps[] = {{1}, {3}, {2}};
  auto r = std::ranges::is_sorted_until(ps, {}, &P::k);
  static_assert(std::is_same_v<decltype(r), P*>);
  if (r != ps + 2) return false;
  if (std::ranges::is_sorted(ps, {}, &P::k)) return false;
  if (!std::ranges::is_sorted(ps, ps + 2, {}, &P::k)) return false;
  if (!std::ranges::is_sorted(ps, std::ranges::greater{}, [](const P& p) { return p.k == 1 ? 9 : p.k; })) return false;
  ForwardRange<int> fr{a, a + 6};
  auto fr_it = std::ranges::is_sorted_until(fr);
  if (fr_it.p != a + 4) return false;
  if (std::ranges::is_sorted(fr)) return false;
  return true;
}

static_assert(test());

int main() {
  CHECK(test());
  int big[500];
  for (int i = 0; i < 500; ++i) big[i] = i / 3;
  int comps = 0, projs = 0;
  CHECK(std::ranges::is_sorted(big, CountingLess{&comps}, CountingProj{&projs}));
  CHECK(comps <= 500);
  CHECK(projs <= 1000);
  comps = 0;
  CHECK(std::is_sorted_until(big, big + 500, CountingLess{&comps}) == big + 500);
  CHECK(comps <= 500);
  return 0;
}
