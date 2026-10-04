// [range.cartesian]: views::cartesian_product() is views::single(tuple()) (/2.1);
// cartesian_product_view enumerates tuples with the last range varying fastest; it is random
// access when the first is and the others are sized random access, common when the first is
// a cartesian-product-common-arg; it is empty when any range other than the first is empty
// (/4.2); size() is the product; iterator_category is input_iterator_tag.
#include <array>
#include <iterator>
#include <ranges>
#include <tuple>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

static_assert(std::is_same_v<decltype(vw::cartesian_product()), rg::single_view<std::tuple<>>>);
using CP = decltype(vw::cartesian_product(std::declval<int (&)[2]>(), std::declval<char (&)[3]>()));
static_assert(std::is_same_v<CP, rg::cartesian_product_view<rg::ref_view<int[2]>, rg::ref_view<char[3]>>>);
static_assert(rg::random_access_range<CP> && rg::common_range<CP> && rg::sized_range<CP>);
static_assert(!rg::borrowed_range<CP>);
static_assert(std::is_same_v<rg::range_reference_t<CP>, std::tuple<int&, char&>>);
static_assert(std::is_same_v<rg::range_value_t<CP>, std::tuple<int, char>>);
static_assert(std::is_same_v<rg::iterator_t<CP>::iterator_category, std::input_iterator_tag>);

// The first range is not common: not common, end() is default_sentinel.
using CPu = decltype(vw::cartesian_product(vw::iota(0), std::declval<int (&)[2]>()));
static_assert(!rg::common_range<CPu> && rg::random_access_range<CPu>);
using FwdNC = ArchetypeView<ForwardIter<int>, PtrSentinel<int>>;
using CPf = rg::cartesian_product_view<rg::ref_view<int[2]>, FwdNC>;
static_assert(rg::forward_range<CPf> && !rg::bidirectional_range<CPf>);
using InV = ArchetypeView<InputIter<int>, PtrSentinel<int>>;
static_assert(rg::input_range<rg::cartesian_product_view<InV, rg::ref_view<int[2]>>>);
template <class F, class S>
concept cp_ok = requires { typename rg::cartesian_product_view<F, S>; };
static_assert(!cp_ok<rg::ref_view<int[2]>, InV>); // later ranges must be forward

constexpr bool test() {
  int a[2] = {1, 2};
  char c[3] = {'x', 'y', 'z'};
  {
    auto p = vw::cartesian_product(a, c);
    CHECK(p.size() == 6u && p.end() - p.begin() == 6);
    int k = 0;
    for (auto [i, ch] : p) {
      CHECK(i == a[k / 3] && ch == c[k % 3]);
      ++k;
    }
    CHECK(k == 6);
    auto it = p.begin() + 4;
    CHECK(std::get<0>(*it) == 2 && std::get<1>(*it) == 'y');
    --it;
    CHECK(std::get<0>(*it) == 2 && std::get<1>(*it) == 'x');
    --it;
    CHECK(std::get<0>(*it) == 1 && std::get<1>(*it) == 'z');
    CHECK(std::get<1>(p[5]) == 'z');
    std::get<1>(*p.begin()) = 'X';
    CHECK(c[0] == 'X');
  }
  {
    int e[1] = {0};
    auto p = vw::cartesian_product(a, vw::take(e, 0));
    CHECK(p.empty() && p.size() == 0u && p.begin() == p.end());
    auto q = vw::cartesian_product(vw::take(e, 0), a);
    CHECK(q.size() == 0u && q.begin() == q.end());
  }
  {
    auto t = vw::cartesian_product(a, a, a);
    CHECK(t.size() == 8u);
    auto last = *(t.end() - 1);
    CHECK(std::get<0>(last) == 2 && std::get<1>(last) == 2 && std::get<2>(last) == 2);
    auto one = vw::cartesian_product(a);
    static_assert(std::is_same_v<rg::range_reference_t<decltype(one)>, std::tuple<int&>>);
    CHECK(one.size() == 2u);
  }
  {
    auto e = vw::cartesian_product();
    CHECK(e.size() == 1u);
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
