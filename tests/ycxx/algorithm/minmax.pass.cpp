// [alg.min.max]: minmax(a, b) "Returns: {b, a} if b is smaller than a, and {a, b}
// otherwise" -- a pair<const T&, const T&> (ranges: minmax_result<const T&>) referring to
// the arguments; so for equivalent arguments min is a and max is b. "Exactly one
// comparison". minmax(initializer_list) / ranges::minmax(r): "Returns X{x, y}, where x is a
// copy of the leftmost element with the smallest value and y a copy of the rightmost
// element with the largest value in the input range"; "At most (3/2) ranges::distance(r)
// applications of the corresponding predicate and twice as many applications of the
// projection".
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <ranges>
#include <type_traits>
#include <utility>
#include "sort_support.hpp"
#include "test_iterators.hpp"
#include "check.hpp"

constexpr bool test() {
  {
    int a = 2, b = 1;
    auto p = std::minmax(a, b);
    static_assert(std::is_same_v<decltype(p), std::pair<const int&, const int&>>);
    if (&p.first != &b || &p.second != &a) return false;
    int c = 3, d = 3;
    auto e = std::minmax(c, d);
    if (&e.first != &c || &e.second != &d) return false;
    auto g = std::minmax(a, b, std::greater<>{});
    if (&g.first != &a || &g.second != &b) return false;
    auto r = std::ranges::minmax(c, d);
    static_assert(std::is_same_v<decltype(r), std::ranges::minmax_result<const int&>>);
    static_assert(std::is_same_v<std::ranges::minmax_result<const int&>, std::ranges::min_max_result<const int&>>);
    if (&r.min != &c || &r.max != &d) return false;
    KV x{5, 0}, y{2, 1};
    auto rk = std::ranges::minmax(x, y, {}, &KV::key);
    if (&rk.min != &y || &rk.max != &x) return false;
    if (std::minmax<long>(3, 2L).first != 2) return false;
  }
  {
    auto p = std::minmax({4, 1, 9, 1, 9, 3});
    static_assert(std::is_same_v<decltype(p), std::pair<int, int>>);
    if (p.first != 1 || p.second != 9) return false;
    // leftmost smallest, rightmost largest
    auto k = std::minmax({KV{2, 0}, KV{1, 1}, KV{3, 2}, KV{1, 3}, KV{3, 4}, KV{2, 5}});
    if (k.first.id != 1 || k.second.id != 4) return false;
    auto s = std::minmax({KV{7, 0}});
    if (s.first.id != 0 || s.second.id != 0) return false;
    auto all = std::minmax({KV{7, 0}, KV{7, 1}, KV{7, 2}});
    if (all.first.id != 0 || all.second.id != 2) return false;
    auto gk = std::minmax({3, 8, 1}, std::greater<>{});
    if (gk.first != 8 || gk.second != 1) return false;
    auto rk = std::ranges::minmax({KV{2, 0}, KV{1, 1}, KV{1, 2}, KV{2, 3}}, {}, &KV::key);
    static_assert(std::is_same_v<decltype(rk), std::ranges::minmax_result<KV>>);
    if (rk.min.id != 1 || rk.max.id != 3) return false;
    KV ks[] = {{5, 0}, {9, 1}, {0, 2}, {9, 3}, {0, 4}};
    auto rr = std::ranges::minmax(ks, {}, &KV::key);
    if (rr.min.id != 2 || rr.max.id != 3) return false;
    int vals[] = {4, 6, 2, 6};
    InputRange<int> ir{vals, vals + 4};
    auto ri = std::ranges::minmax(ir);
    static_assert(std::is_same_v<decltype(ri), std::ranges::minmax_result<int>>);
    if (ri.min != 2 || ri.max != 6) return false;
  }
  return true;
}

static_assert(test());

int a[1001];

int main() {
  CHECK(test());
  int comps = 0, projs = 0;
  int x = 1, y = 2;
  (void)std::minmax(x, y, CountingLess{&comps});
  CHECK(comps == 1);
  comps = 0;
  (void)std::ranges::minmax(x, y, CountingLess{&comps}, CountingProj{&projs});
  CHECK(comps == 1 && projs == 2);
  for (int n : {1, 2, 3, 10, 1000, 1001}) {
    for (Pattern p : all_patterns) {
      fill_pattern(a, n, p);
      comps = projs = 0;
      auto r = std::ranges::minmax(std::ranges::subrange(a, a + n), CountingLess{&comps}, CountingProj{&projs});
      CHECK(2 * comps <= 3 * n);
      CHECK(projs <= 2 * comps);
      CHECK(r.min == *std::min_element(a, a + n) && r.max == *std::max_element(a, a + n));
    }
  }
  comps = 0;
  (void)std::minmax({5, 1, 4, 2, 3, 0}, CountingLess{&comps});
  CHECK(2 * comps <= 3 * 6);
  return 0;
}
