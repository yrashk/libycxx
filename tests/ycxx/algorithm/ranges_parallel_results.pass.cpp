// Results of ranges:: parallel algorithm overloads that do not take a bounded output; where the
// draft gives them a return type different from the sequential overload's, that type:
// [alg.foreach]/15-/17: for_each(exec, first, last, f, proj) returns I (last), for the range
// overload borrowed_iterator_t<R>, not for_each_result; /34-/37: for_each_n returns first + n.
// [alg.find.last]/2: find_last(_if(_not)) return {i, last} (or {last, last}).
// [alg.min.max]/6, /14, /22: min, max and minmax over a range: leftmost smallest, leftmost
// largest, {leftmost smallest, rightmost largest}.
// [alg.swap]/1: swap_ranges swaps M = min(last1 - first1, last2 - first2) elements.
// [alg.fill]: fill(exec, r, value) returns borrowed_iterator_t<R>; fill_n returns first + n.
// [alg.sort], [stable.sort] (stability), [partial.sort], [alg.nth.element], [alg.partitions]
// (partition returns the subrange of the second group; stable_partition is stable),
// [alg.remove], [alg.unique] (subrange of the removed tail), [alg.reverse], [alg.rotate]
// (subrange {first + (last - middle), last}), [alg.shift], plus the queries count, all_of,
// mismatch, equal, search, adjacent_find, is_sorted_until, includes, contains,
// starts_with / ends_with and lexicographical_compare: the same results as the sequential ones.
#include <algorithm>
#include <execution>
#include <functional>
#include <ranges>
#include <type_traits>
#include <vector>
#include "check.hpp"

namespace rg = std::ranges;
using V = std::vector<int>;
using It = V::iterator;

struct P {
  int k, id;
  bool operator==(const P&) const = default;
};

template <class Pol>
void run(Pol&& pol) {
  // for_each / for_each_n
  {
    V v{1, 2, 3};
    int sum = 0;
    auto r = rg::for_each(pol, v, [&sum](int x) { sum += x; });
    static_assert(std::is_same_v<decltype(r), It>);
    CHECK(r == v.end() && sum == 6);
    auto r2 = rg::for_each(pol, v.begin(), v.end(), [](int& x) { x *= 2; });
    static_assert(std::is_same_v<decltype(r2), It>);
    CHECK(r2 == v.end() && v[2] == 6);
    auto r3 = rg::for_each_n(pol, v.begin(), 2, [](int& x) { ++x; }, std::identity{});
    static_assert(std::is_same_v<decltype(r3), It>);
    CHECK(r3 == v.begin() + 2 && v[0] == 3 && v[1] == 5 && v[2] == 6);
    auto r4 = rg::for_each(pol, V{1}, [](int) {});
    static_assert(std::is_same_v<decltype(r4), rg::dangling>);
  }
  // finds
  {
    V v{5, 1, 4, 1, 5, 9};
    CHECK(rg::find(pol, v, 1) == v.begin() + 1);
    CHECK(rg::find_if(pol, v, [](int x) { return x > 5; }) == v.begin() + 5);
    CHECK(rg::find_if_not(pol, v.begin(), v.end(), [](int x) { return x < 9; }) == v.begin() + 5);
    auto fl = rg::find_last(pol, v, 1);
    CHECK(fl.begin() == v.begin() + 3 && fl.end() == v.end());
    auto fl2 = rg::find_last(pol, v, 7);
    CHECK(fl2.begin() == v.end() && fl2.end() == v.end());
    auto fl3 = rg::find_last_if_not(pol, v, [](int x) { return x > 2; });
    CHECK(fl3.begin() == v.begin() + 3);
    CHECK(rg::adjacent_find(pol, v, std::greater<>{}) == v.begin());
    CHECK(rg::count(pol, v, 5) == 2 && rg::count_if(pol, v, [](int x) { return x % 2; }) == 5);
    CHECK(rg::all_of(pol, v, [](int x) { return x > 0; }) && !rg::any_of(pol, v, [](int x) { return x > 9; }));
    CHECK(rg::none_of(pol, v, [](int x) { return x == 0; }) && rg::contains(pol, v, 9));
    V pat{1, 5};
    auto s = rg::search(pol, v, pat);
    CHECK(s.begin() == v.begin() + 3 && s.end() == v.begin() + 5);
    CHECK(rg::starts_with(pol, v, V{5, 1}) && rg::ends_with(pol, v, V{5, 9}) && !rg::ends_with(pol, v, V{9, 5}));
    V w{5, 1, 4, 2};
    auto mm = rg::mismatch(pol, v, w);
    CHECK(mm.in1 == v.begin() + 3 && mm.in2 == w.begin() + 3);
    CHECK(!rg::equal(pol, v, w) && rg::equal(pol, v, v));
    CHECK(rg::lexicographical_compare(pol, w, v) == false && rg::lexicographical_compare(pol, v, w));
    CHECK(rg::is_sorted_until(pol, v) == v.begin() + 1);
    CHECK(rg::includes(pol, V{1, 2, 3, 4}, V{2, 4}) && !rg::includes(pol, V{1, 2}, V{3}));
  }
  // min / max / minmax: which of the equivalent elements
  {
    std::vector<P> v{{2, 0}, {1, 1}, {3, 2}, {1, 3}, {3, 4}};
    CHECK((rg::min(pol, v, {}, &P::k) == P{1, 1}));
    CHECK((rg::max(pol, v, {}, &P::k) == P{3, 2}));
    auto mm = rg::minmax(pol, v, {}, &P::k);
    CHECK((mm.min == P{1, 1}) && (mm.max == P{3, 4}));
    CHECK(rg::min_element(pol, v, {}, &P::k) == v.begin() + 1);
    CHECK(rg::max_element(pol, v, {}, &P::k) == v.begin() + 2);
    auto mme = rg::minmax_element(pol, v, {}, &P::k);
    CHECK(mme.min == v.begin() + 1 && mme.max == v.begin() + 4);
  }
  // modifying in place
  {
    V a{1, 2, 3, 4}, b{9, 8};
    auto sw = rg::swap_ranges(pol, a, b);
    CHECK(sw.in1 == a.begin() + 2 && sw.in2 == b.end() && a[0] == 9 && a[1] == 8 && b[1] == 2);
    auto f = rg::fill(pol, a, 7);
    static_assert(std::is_same_v<decltype(f), It>);
    CHECK(f == a.end() && a[3] == 7);
    CHECK(rg::fill_n(pol, a.begin(), 2, 0) == a.begin() + 2 && a[1] == 0 && a[2] == 7);
    rg::replace(pol, a, 7, 1);
    CHECK((a == V{0, 0, 1, 1}));
    rg::replace_if(pol, a, [](int x) { return x == 0; }, 5);
    CHECK((a == V{5, 5, 1, 1}));
  }
  {
    std::vector<P> v{{3, 0}, {1, 1}, {2, 2}, {1, 3}, {3, 4}, {2, 5}};
    auto st = v;
    CHECK(rg::stable_sort(pol, st, {}, &P::k) == st.end());
    CHECK((st == std::vector<P>{{1, 1}, {1, 3}, {2, 2}, {2, 5}, {3, 0}, {3, 4}}));
    auto so = v;
    CHECK(rg::sort(pol, so, rg::greater{}, &P::k) == so.end());
    CHECK(so[0].k == 3 && so[1].k == 3 && so[5].k == 1);
    auto ps = v;
    CHECK(rg::partial_sort(pol, ps, ps.begin() + 2, {}, &P::k) == ps.end());
    CHECK(ps[0].k == 1 && ps[1].k == 1);
    auto ne = v;
    CHECK(rg::nth_element(pol, ne, ne.begin() + 4, {}, &P::k) == ne.end());
    CHECK(ne[4].k == 3);
    for (int i = 0; i < 4; ++i) CHECK(ne[i].k <= 3);
    auto sp = v;
    auto sr = rg::stable_partition(pol, sp, [](const P& p) { return p.k == 2; });
    CHECK(sr.begin() == sp.begin() + 2 && sr.end() == sp.end());
    CHECK(sp[0].id == 2 && sp[1].id == 5 && sp[2].id == 0 && sp[3].id == 1 && sp[5].id == 4);
    auto pa = v;
    auto pr = rg::partition(pol, pa, [](int k) { return k < 2; }, &P::k);
    CHECK(pr.begin() == pa.begin() + 2 && pr.end() == pa.end() && pa[0].k == 1 && pa[1].k == 1);
  }
  {
    V v{1, 1, 2, 3, 3, 3, 1};
    auto u = rg::unique(pol, v);
    CHECK(u.begin() == v.begin() + 4 && u.end() == v.end() && v[3] == 1);
    V w{1, 0, 2, 0};
    auto rm = rg::remove(pol, w, 0);
    CHECK(rm.begin() == w.begin() + 2 && w[0] == 1 && w[1] == 2);
    auto ri = rg::remove_if(pol, w.begin(), w.begin() + 2, [](int x) { return x == 1; });
    CHECK(ri.begin() == w.begin() + 1 && ri.end() == w.begin() + 2 && w[0] == 2);
    V r{1, 2, 3, 4, 5};
    CHECK(rg::reverse(pol, r) == r.end() && r[0] == 5);
    auto ro = rg::rotate(pol, r, r.begin() + 2);  // 3 2 1 5 4
    CHECK(ro.begin() == r.begin() + 3 && ro.end() == r.end() && (r == V{3, 2, 1, 5, 4}));
    auto sl = rg::shift_left(pol, r, 2);
    CHECK(sl.begin() == r.begin() && sl.end() == r.begin() + 3 && r[0] == 1 && r[2] == 4);
    V t{1, 2, 3, 4, 5};
    auto sr = rg::shift_right(pol, t, 3);
    CHECK(sr.begin() == t.begin() + 3 && sr.end() == t.end() && t[3] == 1 && t[4] == 2);
  }
}

int main() {
  run(std::execution::seq);
  run(std::execution::par);
  run(std::execution::par_unseq);
  run(std::execution::unseq);
  return 0;
}
