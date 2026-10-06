// More ranges:: parallel algorithm overloads ([algorithms.parallel.overloads]: the same
// results as the overloads without a policy unless stated otherwise):
// [alg.find.end], [alg.find.first.of], [alg.search] (search_n), [alg.contains]
// (contains_subrange), [alg.adjacent.find], [alg.mismatch], [alg.equal] with projections,
// [includes], [is.heap], [alg.partitions] (is_partitioned), [alg.lex.comparison],
// [alg.starts.with], [alg.ends.with].
// [set.symmetric.difference]/3, /4.3: with an output shorter than the symmetric difference
// (N < M), the parallel overload returns {first1 + A, first2 + B, result_last}, A and B the
// copied or skipped elements, where a non-copied element is skipped "if it compares less than or
// equivalent to the (N+1)th element of the sorted symmetric difference, unless it is from the
// same range as that element and does not precede it". (Only N < M is checked: /4.2's
// condition "N is equal to M+K" names a K the paragraph does not define.)
#include <algorithm>
#include <execution>
#include <functional>
#include <vector>
#include "check.hpp"

namespace rg = std::ranges;
using V = std::vector<int>;

template <class Pol>
void run(Pol&& pol) {
  V v{1, 2, 3, 1, 2, 3, 4, 4, 4, 5};
  {
    auto fe = rg::find_end(pol, v, V{1, 2, 3});
    CHECK(fe.begin() == v.begin() + 3 && fe.end() == v.begin() + 6);
    auto fe2 = rg::find_end(pol, v, V{-1, -2}, {}, std::negate<>{});
    CHECK(fe2.begin() == v.begin() + 3 && fe2.end() == v.begin() + 5);
    auto none = rg::find_end(pol, v, V{9});
    CHECK(none.begin() == v.end() && none.end() == v.end());
    CHECK(rg::find_first_of(pol, v, V{5, 4}) == v.begin() + 6);
    auto sn = rg::search_n(pol, v, 3, 4);
    CHECK(sn.begin() == v.begin() + 6 && sn.end() == v.begin() + 9);
    auto sn0 = rg::search_n(pol, v, 0, 4);
    CHECK(sn0.begin() == v.begin() && sn0.end() == v.begin());
    CHECK(rg::search_n(pol, v, 4, 4).begin() == v.end());
    CHECK(rg::contains_subrange(pol, v, V{3, 4, 4}) && !rg::contains_subrange(pol, v, V{3, 3}));
    CHECK(rg::adjacent_find(pol, v) == v.begin() + 6);
    CHECK(rg::adjacent_find(pol, v, {}, [](int x) { return x / 2; }) == v.begin() + 1);  // 2/2 == 3/2
  }
  {
    V a{1, 2, 3, 4}, b{1, 2, 7};
    auto m = rg::mismatch(pol, a, b);
    CHECK(m.in1 == a.begin() + 2 && m.in2 == b.begin() + 2);
    auto m2 = rg::mismatch(pol, a, V{1, 2});
    CHECK(m2.in1 == a.begin() + 2);
    CHECK(rg::equal(pol, a, V{-1, -2, -3, -4}, {}, std::identity{}, std::negate<>{}));
    CHECK(!rg::equal(pol, a, V{1, 2, 3}));
    CHECK(rg::lexicographical_compare(pol, V{1, 2}, a) && !rg::lexicographical_compare(pol, a, V{1, 2}));
    CHECK(rg::lexicographical_compare(pol, a, b, {}, std::negate<>{}, std::negate<>{}) == false);
    CHECK(rg::starts_with(pol, a, V{1, 2}) && rg::ends_with(pol, a, V{3, 4}) && !rg::ends_with(pol, b, a));
    CHECK(rg::includes(pol, a, V{2, 4}) && !rg::includes(pol, a, V{2, 5}));
    CHECK(rg::is_partitioned(pol, a, [](int x) { return x < 3; }) && !rg::is_partitioned(pol, b, [](int x) { return x > 1; }));
    V h{9, 5, 8, 1, 2, 10};
    CHECK(rg::is_heap_until(pol, h) == h.begin() + 5 && !rg::is_heap(pol, h));
    CHECK(rg::is_heap(pol, h.begin(), h.begin() + 5));
    CHECK(rg::is_sorted(pol, a) && !rg::is_sorted(pol, h));
  }
  {
    V a{1, 3, 5, 7}, b{2, 3, 6};  // symmetric difference: 1 2 5 6 7 (M = 5)
    V o2(2, 0);
    auto r = rg::set_symmetric_difference(pol, a, b, o2);
    CHECK(o2[0] == 1 && o2[1] == 2);
    // (N+1)th element: 5, from a. a: 1 copied, 3 skipped (precedes 5), 5 not -> A = 2;
    // b: 2 copied, 3 skipped (less than 5) -> B = 2
    CHECK(r.in1 == a.begin() + 2 && r.in2 == b.begin() + 2 && r.out == o2.end());
    V o3(3, 0);
    auto r3 = rg::set_symmetric_difference(pol, a, b, o3);
    CHECK(o3[2] == 5);
    // (N+1)th element: 6, from b. a: 1, 5 copied, 3 skipped, 7 not -> A = 3;
    // b: 2 copied, 3 skipped, 6 not (same range, does not precede itself) -> B = 2
    CHECK(r3.in1 == a.begin() + 3 && r3.in2 == b.begin() + 2 && r3.out == o3.end());
  }
}

int main() {
  run(std::execution::seq);
  run(std::execution::par);
  run(std::execution::par_unseq);
  return 0;
}
