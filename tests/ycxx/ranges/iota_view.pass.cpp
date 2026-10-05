// [range.iota.view], [range.iota.iterator], [range.iota.sentinel]: iota_view's elements,
// size() (/16, including negative values and unsigned W), iterator arithmetic (/9-/11,
// /21 for unsigned W), comparisons, the (first, last) constructor (/10), and empty().
// COUNTERPART: libcxx:ranges/range.factories/range.iota.view/(iterator|sentinel)/ctor.value.pass.cpp
#include <climits>
#include <compare>
#include <cstddef>
#include <iterator>
#include <ranges>
#include "check.hpp"
#include "range_support.hpp"

namespace rg = std::ranges;

constexpr bool test() {
  {
    auto v = std::views::iota(1, 6);
    CHECK(range_equals(v, {1, 2, 3, 4, 5}));
    CHECK(v.size() == 5 && !v.empty() && v[2] == 3 && v.front() == 1 && v.back() == 5);
    auto i = v.begin();
    CHECK(*i == 1 && i[3] == 4 && *(i + 4) == 5 && *(4 + i) == 5);
    i += 3;
    CHECK(*i == 4);
    i -= 2;
    CHECK(*i == 2);
    CHECK(v.end() - v.begin() == 5 && v.begin() - v.end() == -5);
    CHECK(v.begin() < v.end() && v.end() > v.begin() && v.begin() <= v.begin() && v.end() >= v.begin());
    CHECK((v.begin() <=> v.end()) == std::strong_ordering::less);
    auto j = v.end();
    --j;
    CHECK(*j == 5);
    auto k = j--;
    CHECK(*k == 5 && *j == 4);
  }
  // Negative values and size().
  {
    rg::iota_view<int, int> v(-3, 4);
    CHECK(v.size() == 7u && range_equals(v, {-3, -2, -1, 0, 1, 2, 3}));
    rg::iota_view<int, int> w(-10, -4);
    CHECK(w.size() == 6u);
    rg::iota_view<int, int> x(INT_MIN + 1, INT_MAX);
    CHECK(x.size() == 0xFFFFFFFEu);
    CHECK(x.end() - x.begin() == static_cast<long long>(INT_MAX) - (INT_MIN + 1));
  }
  // Unsigned W: negative offsets and differences (/9, /10, /21).
  {
    rg::iota_view<unsigned, unsigned> v(2u, 12u);
    CHECK(v.size() == 10u);
    auto b = v.begin();
    auto e = v.end();
    CHECK(b - e == -10 && e - b == 10);
    auto m = e;
    m += -3;
    CHECK(*m == 9u);
    m -= -2;
    CHECK(*m == 11u);
    CHECK(*(b - -1) == 3u && b[5] == 7u);
    rg::iota_view<unsigned char, unsigned char> c(250, 255);
    CHECK(c.size() == 5u && c.back() == 254);
  }
  // Empty and unbounded.
  {
    rg::iota_view<int, int> e(7, 7);
    CHECK(e.empty() && e.size() == 0u && !e);
    auto u = std::views::iota(10);
    auto i = u.begin();
    for (int n = 0; n < 100; ++n) ++i;
    CHECK(*i == 110 && !(i == u.end()));
  }
  // Mixed W/Bound with the sentinel; sized because both are integer-like (/17).
  {
    auto v = std::views::iota(0, 4L);
    CHECK(range_equals(v, {0, 1, 2, 3}));
    CHECK(v.size() == 4u);
  }
  // Construction from an iterator/sentinel pair (/10).
  {
    rg::iota_view<int, int> v(0, 10);
    rg::iota_view<int, int> w(v.begin() + 2, v.begin() + 5);
    CHECK(range_equals(w, {2, 3, 4}));
    auto ml = std::views::iota(0, 10L);
    rg::iota_view<int, long> mw(ml.begin() + 7, ml.end());
    CHECK(range_equals(mw, {7, 8, 9}));
    auto ub = std::views::iota(5);
    rg::iota_view<int> uw(ub.begin(), std::unreachable_sentinel);
    CHECK(*uw.begin() == 5);
  }
  // Pointer W.
  {
    int a[4] = {};
    rg::iota_view<int*, int*> p(a, a + 4);
    CHECK(p.size() == 4u && p[1] == a + 1 && p.back() == a + 3);
  }
  // Postfix ++ for an incrementable W returns the old iterator.
  {
    auto v = std::views::iota(0, 3);
    auto i = v.begin();
    auto old = i++;
    CHECK(*old == 0 && *i == 1);
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
