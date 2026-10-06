// [set.intersection]/3-/4: the parallel ranges::set_intersection returns
// {first1 + A, first2 + B, result + N}, A and B the copied or skipped elements of each input, even
// when the whole intersection fits (4.3; the sequential overloads return {last1, last2, ...},
// 4.2). Of m and n equivalent elements, the first min(m, n) of the first range are in the
// intersection; if k of them are copied, the first k of the second range are skipped; a
// non-copied element is also skipped if it compares less than the min(M, N + 1)th element of the
// intersection.
// [partial.sort.copy]/1, /5-/6: N = min(last - first, result_last - result_first); the N smallest
// elements, sorted, are placed in [result_first, result_first + N); returns
// {last, result_first + N}.
#include <algorithm>
#include <execution>
#include <functional>
#include <vector>
#include "check.hpp"

namespace rg = std::ranges;
using V = std::vector<int>;

template <class Pol>
void run(Pol&& pol) {
  {
    V a{1, 2, 3, 5}, b{2, 3, 4};  // intersection: 2 3 (M = 2)
    V out(5, 0);
    auto r = rg::set_intersection(pol, a, b, out);
    CHECK(out[0] == 2 && out[1] == 3 && out[2] == 0);
    // a: 1 skipped (less than 3, the 2nd element), 2 and 3 copied, 5 not -> A = 3
    // b: 2 and 3 skipped (equivalent to copied ones), 4 not less than 3 -> B = 2
    CHECK(r.in1 == a.begin() + 3 && r.in2 == b.begin() + 2 && r.out == out.begin() + 2);
    V one(1, 0);
    auto r2 = rg::set_intersection(pol, a, b, one);
    // N = 1: 2 copied; the (N + 1)th element is 3: 1 skipped, 3 not -> A = 2; b: 2 skipped -> B = 1
    CHECK(one[0] == 2 && r2.in1 == a.begin() + 2 && r2.in2 == b.begin() + 1 && r2.out == one.end());
    // the sequential overload goes to the ends
    V out2(5, 0);
    auto s = rg::set_intersection(a, b, out2.begin());
    CHECK(s.in1 == a.end() && s.in2 == b.end() && s.out == out2.begin() + 2);
    // equivalent runs: 1 1 1 vs 1 1 -> the first two 1s of a
    V c{1, 1, 1}, d{1, 1};
    V o(3, 0);
    auto r3 = rg::set_intersection(pol, c, d, o);
    CHECK(r3.out == o.begin() + 2 && r3.in1 == c.begin() + 2 && r3.in2 == d.end());
  }
  {
    V in{5, 3, 9, 1, 7, 3};
    V out(3, 0);
    auto r = rg::partial_sort_copy(pol, in, out);
    CHECK((out == V{1, 3, 3}) && r.in == in.end() && r.out == out.end());
    V big(8, -1);
    auto r2 = rg::partial_sort_copy(pol, in, big, rg::greater{});
    CHECK(r2.in == in.end() && r2.out == big.begin() + 6 && big[0] == 9 && big[5] == 1 && big[6] == -1);
    V none;
    auto r3 = rg::partial_sort_copy(pol, in, none);
    CHECK(r3.in == in.end() && r3.out == none.end());
    V keys(2, 0);
    auto r4 = rg::partial_sort_copy(pol, in.begin(), in.begin() + 2, keys.begin(), keys.end(), {}, std::negate<>{}, std::negate<>{});  // descending
    CHECK(keys[0] == 5 && keys[1] == 3 && r4.in == in.begin() + 2);
  }
}

int main() {
  run(std::execution::seq);
  run(std::execution::par);
  run(std::execution::par_unseq);
  return 0;
}
