// [alg.min.max]: min_element returns "The first iterator i in the range [first, last) such
// that for every iterator j in the range [first, last), bool(invoke(comp, invoke(proj, *j),
// invoke(proj, *i))) is false. Returns last if first == last."; max_element likewise with
// comp(*i, *j): the FIRST largest. Both "Exactly max(last - first - 1, 0) comparisons and
// twice as many projections". minmax_element "Returns: {first, first} if [first, last) is
// empty, otherwise {m, M}, where m is the first iterator ... [to the smallest] and where M
// is the last iterator ... [to the largest]" ("This behavior intentionally differs from
// max_element"), "At most max(floor(3/2 (N - 1)), 0) comparisons and twice as many
// applications of the projection". ranges::minmax_element returns minmax_element_result.
#include <algorithm>
#include <functional>
#include <ranges>
#include <type_traits>
#include <utility>
#include "sort_support.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

constexpr bool test() {
  int a[] = {4, 1, 7, 1, 7, 3};
  if (std::min_element(a, a + 6) != a + 1 || std::max_element(a, a + 6) != a + 2) return false;
  auto mm = std::minmax_element(a, a + 6);
  static_assert(std::is_same_v<decltype(mm), std::pair<int*, int*>>);
  if (mm.first != a + 1 || mm.second != a + 4) return false;
  if (std::min_element(a, a) != a || std::max_element(a, a) != a) return false;
  mm = std::minmax_element(a, a);
  if (mm.first != a || mm.second != a) return false;
  mm = std::minmax_element(a, a + 1);
  if (mm.first != a || mm.second != a) return false;
  int e[] = {2, 2, 2, 2};
  mm = std::minmax_element(e, e + 4);
  if (mm.first != e || mm.second != e + 3) return false;
  if (std::min_element(e, e + 4) != e || std::max_element(e, e + 4) != e) return false;
  // comparator
  if (std::min_element(a, a + 6, std::greater<>{}) != a + 2) return false;
  if (std::max_element(a, a + 6, std::greater<>{}) != a + 1) return false;
  mm = std::minmax_element(a, a + 6, std::greater<>{});
  if (mm.first != a + 2 || mm.second != a + 3) return false;
  // forward iterators
  auto fm = std::minmax_element(ForwardIter<int>(a), ForwardIter<int>(a + 6));
  if (fm.first.p != a + 1 || fm.second.p != a + 4) return false;
  if (std::max_element(ForwardIter<int>(a), ForwardIter<int>(a + 6)).p != a + 2) return false;
  // ranges with projection
  KV ks[] = {{3, 0}, {1, 1}, {4, 2}, {1, 3}, {4, 4}};
  if (std::ranges::min_element(ks, {}, &KV::key) != ks + 1) return false;
  if (std::ranges::max_element(ks, {}, &KV::key) != ks + 2) return false;
  auto rm = std::ranges::minmax_element(ks, {}, &KV::key);
  static_assert(std::is_same_v<decltype(rm), std::ranges::minmax_element_result<KV*>>);
  static_assert(std::is_same_v<std::ranges::minmax_element_result<KV*>, std::ranges::min_max_result<KV*>>);
  if (rm.min != ks + 1 || rm.max != ks + 4) return false;
  ForwardRange<int> fr{a, a + 6};
  auto frm = std::ranges::minmax_element(fr);
  if (frm.min.p != a + 1 || frm.max.p != a + 4) return false;
  if (std::ranges::min_element(fr).p != a + 1 || std::ranges::max_element(a, a + 6) != a + 2) return false;
  auto erm = std::ranges::minmax_element(a, a);
  if (erm.min != a || erm.max != a) return false;
  return true;
}

static_assert(test());

int big[1001];

int main() {
  CHECK(test());
  for (int n : {0, 1, 2, 3, 4, 100, 1001}) {
    for (Pattern p : all_patterns) {
      fill_pattern(big, n, p);
      int comps = 0, projs = 0;
      std::ranges::min_element(big, big + n, CountingLess{&comps}, CountingProj{&projs});
      CHECK(comps == (n > 0 ? n - 1 : 0));
      CHECK(projs <= 2 * comps);
      comps = projs = 0;
      std::ranges::max_element(big, big + n, CountingLess{&comps}, CountingProj{&projs});
      CHECK(comps == (n > 0 ? n - 1 : 0));
      CHECK(projs <= 2 * comps);
      comps = 0;
      std::max_element(big, big + n, CountingLess{&comps});
      CHECK(comps == (n > 0 ? n - 1 : 0));
      comps = projs = 0;
      auto r = std::ranges::minmax_element(big, big + n, CountingLess{&comps}, CountingProj{&projs});
      CHECK(comps <= (n > 0 ? 3 * (n - 1) / 2 : 0));
      CHECK(projs <= 2 * comps);
      if (n > 0) {
        CHECK(r.min == std::min_element(big, big + n));
        // the last largest
        int* last_max = big;
        for (int i = 0; i < n; ++i)
          if (!(big[i] < *last_max)) last_max = big + i;
        CHECK(r.max == last_max);
      }
      comps = 0;
      std::minmax_element(big, big + n, CountingLess{&comps});
      CHECK(comps <= (n > 0 ? 3 * (n - 1) / 2 : 0));
    }
  }
  return 0;
}
