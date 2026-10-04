// [range.zip]: views::zip() is empty_view<tuple<>> (/2.1); zip_view's reference is a tuple
// of references and its value_type a tuple of values; it is common per zip-is-common (so
// two bidirectional, non-random-access common ranges do not give a common zip); size() is
// the minimum; iterator_category (input_iterator_tag) only for forward views (/2); the
// iterator's == is true if any component is equal unless all are bidirectional (/13);
// borrowed iff all views are.
#include <array>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <tuple>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;
using A = std::array<char, 2>;

static_assert(std::is_same_v<decltype(vw::zip()), rg::empty_view<std::tuple<>>>);

using Z1 = decltype(vw::zip(std::declval<int (&)[3]>(), std::declval<A&>()));
static_assert(std::is_same_v<Z1, rg::zip_view<rg::ref_view<int[3]>, rg::ref_view<A>>>);
static_assert(rg::random_access_range<Z1> && rg::common_range<Z1> && rg::sized_range<Z1> && rg::borrowed_range<Z1>);
static_assert(std::is_same_v<rg::range_reference_t<Z1>, std::tuple<int&, char&>>);
static_assert(std::is_same_v<rg::range_value_t<Z1>, std::tuple<int, char>>);
static_assert(std::is_same_v<rg::range_rvalue_reference_t<Z1>, std::tuple<int&&, char&&>>);
static_assert(std::is_same_v<rg::iterator_t<Z1>::iterator_category, std::input_iterator_tag>);
static_assert(std::is_same_v<rg::iterator_t<Z1>::iterator_concept, std::random_access_iterator_tag>);
static_assert(!rg::borrowed_range<rg::zip_view<rg::ref_view<int[3]>, rg::owning_view<A>>>);

using BidiC = ArchetypeView<BidiIter<int>>;
using Z2 = rg::zip_view<BidiC, BidiC>;
static_assert(rg::bidirectional_range<Z2> && !rg::common_range<Z2>);
static_assert(rg::common_range<rg::zip_view<BidiC>>); // a single common range

using FwdC = ArchetypeView<ForwardIter<int>>;
using Z3 = rg::zip_view<FwdC, FwdC>;
static_assert(rg::forward_range<Z3> && rg::common_range<Z3> && !rg::sized_range<Z3>);
static_assert(std::is_same_v<rg::iterator_t<Z3>::iterator_concept, std::forward_iterator_tag>);

using InV = ArchetypeView<InputIter<int>, PtrSentinel<int>>;
using Z4 = rg::zip_view<InV, rg::ref_view<int[2]>>;
static_assert(rg::input_range<Z4> && !rg::forward_range<Z4> && !has_iterator_category<rg::iterator_t<Z4>>);

using Z5 = decltype(vw::zip(vw::iota(0), std::declval<int (&)[2]>()));
static_assert(!rg::common_range<Z5> && !rg::sized_range<Z5> && rg::random_access_range<Z5>);

static_assert(std::is_same_v<decltype(rg::zip_view(std::declval<int (&)[3]>())), rg::zip_view<rg::ref_view<int[3]>>>);

constexpr bool test() {
  int a[3] = {1, 2, 3};
  A b = {'a', 'b'};
  {
    auto z = vw::zip(a, b);
    CHECK(z.size() == 2u && z.end() - z.begin() == 2);
    auto [x, y] = z.front();
    CHECK(x == 1 && y == 'a');
    std::get<0>(z[1]) = 20;
    CHECK(a[1] == 20);
    auto it = z.begin() + 1;
    CHECK(std::get<1>(*it) == 'b' && it - z.begin() == 1 && z.begin() < it);
    auto m = rg::iter_move(it);
    static_assert(std::is_same_v<decltype(m), std::tuple<int&&, char&&>>);
    rg::iter_swap(z.begin(), it);
    CHECK(a[0] == 20 && a[1] == 1 && b[0] == 'b' && b[1] == 'a');
    auto e = z.end();
    --e;
    CHECK(std::get<0>(*e) == 1);
  }
  {
    int c[5] = {10, 20, 30, 40, 50};
    Z3 z(FwdC(ForwardIter<int>(a), ForwardIter<int>(a + 3)), FwdC(ForwardIter<int>(c), ForwardIter<int>(c + 5)));
    CHECK(count_elements(z) == 3); // stops at the shorter: == compares true if any component is equal
  }
  {
    int n = 0;
    for (auto [i, v] : vw::zip(vw::iota(0), a)) {
      CHECK(i == n);
      (void)v;
      ++n;
    }
    CHECK(n == 3);
  }
  {
    auto z = vw::zip(a);
    static_assert(std::is_same_v<rg::range_reference_t<decltype(z)>, std::tuple<int&>>);
    CHECK(z.size() == 3u);
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
