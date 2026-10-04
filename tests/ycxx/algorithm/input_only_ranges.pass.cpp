// Algorithms on input-only, move-only iterators (views::istream: [range.istream] - an
// input_range whose iterator is move-only and whose operator++(int) returns void), which
// must neither copy iterators nor read an element twice.
// [alg.unique]/6-12 (unique_copy): E(i) is false for i == first, else comp(proj(*(i - 1)),
// proj(*i)); copies the first N elements with E(i) false; ranges: returns {last, result + N};
// the constraint "forward_iterator<I> || (input_iterator<O> && same_as<iter_value_t<I>,
// iter_value_t<O>>) || indirectly_copyable_storable<I, O>" admits input sources with an input
// iterator output (read back) or a stored copy; "At most last - first - 1 applications of the
// corresponding predicate".
// [alg.min.max]: ranges::min / max / minmax of an input_range return copies of values: max
// "Returns: The largest value in the input range. Returns a copy of the leftmost element when
// several elements are equivalent to the largest"; min likewise the leftmost smallest; minmax
// "Returns: Let X be the return type. Returns X{x, y}, where x is a copy of the leftmost element
// with the smallest value and y a copy of the rightmost element with the largest value".
// [alg.merge]/1-5: merge of two input ranges, stable (equivalent elements of the first range
// first), "At most N - 1 comparisons". [set.union], [set.intersection], [set.difference],
// [set.symmetric.difference]: input ranges, "At most 2 * ((last1 - first1) + (last2 -
// first2)) - 1 comparisons and applications of each projection"; union/intersection copy
// equivalent elements from the first range. [alg.fold]: fold_left_first(r, f) on an input
// range. [alg.copy] copy_if, [alg.partitions] partition_copy, [alg.transform] binary
// transform with input ranges.
#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>
#include <sstream>
#include <utility>
#include <vector>
#include "check.hpp"

namespace rg = std::ranges;
using V = std::vector<int>;

int main() {
  static_assert(!rg::forward_range<rg::istream_view<int>>);
  static_assert(!std::copyable<rg::iterator_t<rg::istream_view<int>>>);
  {
    long calls = 0;
    auto eq = [&](int a, int b) { ++calls; return a == b; };
    std::istringstream is("1 1 2 2 2 3 1 1 4");
    auto v = rg::istream_view<int>(is);
    V out;
    auto r = rg::unique_copy(v, std::back_inserter(out), eq);  // stored-copy branch
    CHECK((out == V{1, 2, 3, 1, 4}) && calls <= 8 && r.in == std::default_sentinel);
  }
  {
    long calls = 0;
    auto eq = [&](int a, int b) { ++calls; return a == b; };
    std::istringstream is("5 5 6 7 7 7 5");
    auto v = rg::istream_view<int>(is);
    V out(10, -1);
    auto r = rg::unique_copy(v, out.begin(), eq);  // input_iterator<O> branch
    CHECK(r.out == out.begin() + 4 && (V(out.begin(), r.out) == V{5, 6, 7, 5}) && calls <= 6);
    CHECK(out[4] == -1);
  }
  {
    // std::unique_copy with istream_iterator and ostream_iterator (neither forward).
    std::istringstream is("9 9 8 8 9");
    std::ostringstream os;
    std::unique_copy(std::istream_iterator<int>(is), std::istream_iterator<int>(), std::ostream_iterator<int>(os, ","));
    CHECK(os.str() == "9,8,9,");
    std::istringstream is2("4 4 4 3");
    V out(3, 0);
    auto e = std::unique_copy(std::istream_iterator<int>(is2), std::istream_iterator<int>(), out.begin());
    CHECK(e == out.begin() + 2 && out[0] == 4 && out[1] == 3);
  }
  {
    auto tens = [](int x) { return x / 10; };
    std::istringstream a("15 12 31 38 11 33"), b("15 12 31 38 11 33"), c("15 12 31 38 11 33");
    CHECK(rg::max(rg::istream_view<int>(a), {}, tens) == 31);
    CHECK(rg::min(rg::istream_view<int>(b), {}, tens) == 15);
    auto mm = rg::minmax(rg::istream_view<int>(c), {}, tens);
    CHECK(mm.min == 15 && mm.max == 33);
    std::istringstream d("7");
    auto one = rg::minmax(rg::istream_view<int>(d));
    CHECK(one.min == 7 && one.max == 7);
    std::istringstream e2("3 1 4 1 5 9 2 6");
    CHECK(rg::max(rg::istream_view<int>(e2)) == 9);
  }
  {
    // merge: stability between the ranges, comparison count.
    long calls = 0;
    auto lt = [&](const std::pair<int, int>& x, const std::pair<int, int>& y) {
      ++calls;
      return x.first < y.first;
    };
    std::istringstream a("1 3 3 5"), b("2 3 4");
    std::vector<std::pair<int, int>> out;
    auto av = rg::istream_view<int>(a) | std::views::transform([](int x) { return std::pair{x, 1}; });
    auto bv = rg::istream_view<int>(b) | std::views::transform([](int x) { return std::pair{x, 2}; });
    rg::merge(av, bv, std::back_inserter(out), lt);
    const std::vector<std::pair<int, int>> want = {{1, 1}, {2, 2}, {3, 1}, {3, 1}, {3, 2}, {4, 2}, {5, 1}};
    CHECK(out == want && calls <= 6);
  }
  {
    auto run = [](auto algo, const char* x, const char* y) {
      std::istringstream a(x), b(y);
      V out;
      algo(rg::istream_view<int>(a), rg::istream_view<int>(b), std::back_inserter(out));
      return out;
    };
    auto uni = [](auto&& r1, auto&& r2, auto o) { rg::set_union(r1, r2, o); };
    auto inter = [](auto&& r1, auto&& r2, auto o) { rg::set_intersection(r1, r2, o); };
    auto diff = [](auto&& r1, auto&& r2, auto o) { rg::set_difference(r1, r2, o); };
    auto sym = [](auto&& r1, auto&& r2, auto o) { rg::set_symmetric_difference(r1, r2, o); };
    CHECK((run(uni, "1 2 2 2 5", "2 2 3 6") == V{1, 2, 2, 2, 3, 5, 6}));
    CHECK((run(inter, "1 2 2 2 5", "2 2 3 6") == V{2, 2}));
    CHECK((run(diff, "1 2 2 2 5", "2 2 3 6") == V{1, 2, 5}));
    CHECK((run(sym, "1 2 2 2 5", "2 2 3 6") == V{1, 2, 3, 5, 6}));
    CHECK((run(uni, "", "1") == V{1}) && run(inter, "1 2", "").empty());
  }
  {
    std::istringstream a("1 2 3 4 5 6"), b("10 20 30"), c("1 2 3 4 5"), d("4 5 6 7");
    V evens, odds, sums;
    rg::partition_copy(rg::istream_view<int>(a), std::back_inserter(evens), std::back_inserter(odds),
                       [](int x) { return x % 2 == 0; });
    CHECK((evens == V{2, 4, 6}) && (odds == V{1, 3, 5}));
    rg::transform(rg::istream_view<int>(b), rg::istream_view<int>(c), std::back_inserter(sums), std::plus<>{});
    CHECK((sums == V{11, 22, 33}));
    CHECK(rg::fold_left_first(rg::istream_view<int>(d), std::multiplies<>{}) == 840);
    std::istringstream e2("3 1 2"), f("3 1 2 0");
    CHECK(rg::lexicographical_compare(rg::istream_view<int>(e2), rg::istream_view<int>(f)));
  }
  return 0;
}
