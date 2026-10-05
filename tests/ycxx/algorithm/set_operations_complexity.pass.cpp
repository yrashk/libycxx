// [set.union]/5, [set.intersection]/5, [set.difference]/5, [set.symmetric.difference]/5:
// "Complexity: At most 2 * ((last1 - first1) + (last2 - first2)) - 1 comparisons and
// applications of each projection." Checked on random multisets, including empty inputs
// and very unbalanced sizes; the outputs are checked against the multiset definitions of
// [alg.set.operations.general] (counts: union max, intersection min, difference m - n,
// symmetric difference |m - n|).
// COUNTERPART: libcxx:algorithms/alg.sorting/alg.set.operations/set.intersection/set_intersection_complexity.pass.cpp
#include <algorithm>
#include "sort_support.hpp"
#include "check.hpp"

int x[600], y[600], out[1200];

int count(const int* b, const int* e, int v) {
  int c = 0;
  for (; b != e; ++b) c += *b == v;
  return c;
}

int main() {
  const int sizes[][2] = {{0, 0}, {0, 50}, {50, 0}, {1, 600}, {600, 1}, {300, 300}, {600, 600}};
  for (unsigned seed = 1; seed <= 4; ++seed) {
    for (auto [n1, n2] : sizes) {
      fill_pattern(x, n1, Pattern::few_values, seed);
      fill_pattern(y, n2, Pattern::random, seed + 7);
      for (int i = 0; i < n2; ++i) y[i] %= 6;
      std::sort(x, x + n1);
      std::sort(y, y + n2);
      const int bound = n1 + n2 == 0 ? 0 : 2 * (n1 + n2) - 1;
      for (int op = 0; op < 4; ++op) {
        int comps = 0, p1 = 0, p2 = 0;
        CountingLess c{&comps};
        CountingProj q1{&p1}, q2{&p2};
        int* e = nullptr;
        switch (op) {
          case 0: e = std::ranges::set_union(x, x + n1, y, y + n2, out, c, q1, q2).out; break;
          case 1: e = std::ranges::set_intersection(x, x + n1, y, y + n2, out, c, q1, q2).out; break;
          case 2: e = std::ranges::set_difference(x, x + n1, y, y + n2, out, c, q1, q2).out; break;
          case 3: e = std::ranges::set_symmetric_difference(x, x + n1, y, y + n2, out, c, q1, q2).out; break;
        }
        CHECK(comps <= bound);
        CHECK(p1 <= bound && p2 <= bound);
        CHECK(sorted_by(out, e));
        for (int v = 0; v < 6; ++v) {
          int m = count(x, x + n1, v), n = count(y, y + n2, v), k = count(out, e, v);
          int want = op == 0 ? std::max(m, n) : op == 1 ? std::min(m, n) : op == 2 ? std::max(m - n, 0) : (m > n ? m - n : n - m);
          CHECK(k == want);
        }
        // std overloads: same count bound
        comps = 0;
        switch (op) {
          case 0: std::set_union(x, x + n1, y, y + n2, out, c); break;
          case 1: std::set_intersection(x, x + n1, y, y + n2, out, c); break;
          case 2: std::set_difference(x, x + n1, y, y + n2, out, c); break;
          case 3: std::set_symmetric_difference(x, x + n1, y, y + n2, out, c); break;
        }
        CHECK(comps <= bound);
      }
    }
  }
  return 0;
}
