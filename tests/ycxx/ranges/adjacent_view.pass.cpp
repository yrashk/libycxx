// [range.adjacent]: views::adjacent<N> gives tuples of N consecutive references;
// adjacent<0> of a forward range is empty_view<tuple<>> (/2.1); pairwise is adjacent<2>;
// size() is size - (N - 1) clamped at zero; at least forward, never input; iterator_category
// is input_iterator_tag; borrowed as its view; adjacent_view requires a forward range.
#include <iterator>
#include <ranges>
#include <tuple>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

static_assert(std::is_same_v<decltype(std::declval<int (&)[3]>() | vw::adjacent<0>), rg::empty_view<std::tuple<>>>);
using AD = decltype(std::declval<int (&)[4]>() | vw::adjacent<3>);
static_assert(std::is_same_v<AD, rg::adjacent_view<rg::ref_view<int[4]>, 3>>);
static_assert(std::is_same_v<decltype(std::declval<int (&)[4]>() | vw::pairwise), rg::adjacent_view<rg::ref_view<int[4]>, 2>>);
static_assert(rg::random_access_range<AD> && rg::common_range<AD> && rg::sized_range<AD> && rg::borrowed_range<AD>);
static_assert(std::is_same_v<rg::range_reference_t<AD>, std::tuple<int&, int&, int&>>);
static_assert(std::is_same_v<rg::range_value_t<AD>, std::tuple<int, int, int>>);
static_assert(std::is_same_v<rg::iterator_t<AD>::iterator_category, std::input_iterator_tag>);
static_assert(std::is_same_v<rg::iterator_t<AD>::iterator_concept, std::random_access_iterator_tag>);

using FwdNC = ArchetypeView<ForwardIter<int>, PtrSentinel<int>>;
using AF = rg::adjacent_view<FwdNC, 2>;
static_assert(rg::forward_range<AF> && !rg::common_range<AF> && !rg::bidirectional_range<AF>);
static_assert(std::is_same_v<rg::iterator_t<AF>::iterator_concept, std::forward_iterator_tag>);

template <class V, std::size_t N>
concept adj_ok = requires { typename rg::adjacent_view<V, N>; };
static_assert(!adj_ok<ArchetypeView<InputIter<int>, PtrSentinel<int>>, 2>);
static_assert(!adj_ok<rg::ref_view<int[2]>, 0>);

constexpr bool test() {
  int a[4] = {1, 2, 3, 4};
  {
    auto p = a | vw::pairwise;
    CHECK(p.size() == 3u);
    int k = 0;
    for (auto [x, y] : p) {
      CHECK(x == a[k] && y == a[k + 1]);
      ++k;
    }
    CHECK(k == 3);
    std::get<1>(p[0]) = 20;
    CHECK(a[1] == 20);
    auto e = p.end();
    --e;
    CHECK(std::get<0>(*e) == 3 && std::get<1>(*e) == 4);
    CHECK(p.end() - p.begin() == 3);
  }
  {
    auto t = a | vw::adjacent<5>;
    CHECK(t.size() == 0u && t.begin() == t.end());
    auto f = a | vw::adjacent<4>;
    CHECK(f.size() == 1u && std::get<3>(f.front()) == 4);
    auto one = a | vw::adjacent<1>;
    static_assert(std::is_same_v<rg::range_reference_t<decltype(one)>, std::tuple<int&>>);
    CHECK(one.size() == 4u);
  }
  {
    int b[3] = {5, 6, 7};
    AF f(FwdNC(ForwardIter<int>(b), PtrSentinel<int>{b + 3}));
    CHECK(count_elements(f) == 2);
    int c[1] = {1};
    AF g(FwdNC(ForwardIter<int>(c), PtrSentinel<int>{c + 1}));
    CHECK(g.begin() == g.end());
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
