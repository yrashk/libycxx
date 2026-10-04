// [range.enumerate]: enumerate_view's elements are tuple<difference_type, reference>, its
// value_type tuple<difference_type, value>; iterator_category is always input_iterator_tag;
// iterator_concept follows the base (/1); end() is an iterator for forward, common and sized
// bases; index(); comparisons and difference by position; iter_move; borrowed as its view.
#include <compare>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

using E1 = decltype(std::declval<int (&)[3]>() | vw::enumerate);
static_assert(std::is_same_v<E1, rg::enumerate_view<rg::ref_view<int[3]>>>);
static_assert(rg::random_access_range<E1> && rg::common_range<E1> && rg::sized_range<E1> && rg::borrowed_range<E1>);
static_assert(std::is_same_v<rg::range_reference_t<E1>, std::tuple<std::ptrdiff_t, int&>>);
static_assert(std::is_same_v<rg::range_value_t<E1>, std::tuple<std::ptrdiff_t, int>>);
static_assert(std::is_same_v<rg::range_rvalue_reference_t<E1>, std::tuple<std::ptrdiff_t, int&&>>);
using I1 = rg::iterator_t<E1>;
static_assert(std::is_same_v<I1::iterator_category, std::input_iterator_tag>);
static_assert(std::is_same_v<I1::iterator_concept, std::random_access_iterator_tag>);
static_assert(noexcept(std::declval<const I1&>().index()));
static_assert(noexcept(std::declval<const I1&>() == std::declval<const I1&>()));
static_assert(noexcept(std::declval<const I1&>() - std::declval<const I1&>()));
static_assert(std::is_same_v<decltype(std::declval<const I1&>() <=> std::declval<const I1&>()), std::strong_ordering>);

// Forward, common, but not sized: end() is a sentinel.
using FwdC = ArchetypeView<ForwardIter<int>>;
using E2 = rg::enumerate_view<FwdC>;
static_assert(rg::forward_range<E2> && !rg::common_range<E2> && !rg::sized_range<E2>);
static_assert(std::is_same_v<rg::iterator_t<E2>::iterator_concept, std::forward_iterator_tag>);
using BidiC = ArchetypeView<BidiIter<int>>;
static_assert(std::is_same_v<rg::iterator_t<rg::enumerate_view<BidiC>>::iterator_concept, std::bidirectional_iterator_tag>);
using InV = ArchetypeView<InputIter<int>, PtrSentinel<int>>;
static_assert(std::is_same_v<rg::iterator_t<rg::enumerate_view<InV>>::iterator_concept, std::input_iterator_tag>);
static_assert(!rg::borrowed_range<rg::enumerate_view<rg::owning_view<rg::single_view<int>>>>);

constexpr bool test() {
  int a[3] = {10, 20, 30};
  {
    auto e = a | vw::enumerate;
    std::ptrdiff_t expect = 0;
    for (auto [i, v] : e) {
      CHECK(i == expect && v == a[expect]);
      ++expect;
    }
    CHECK(expect == 3 && e.size() == 3u);
    auto it = e.begin() + 2;
    CHECK(it.index() == 2 && std::get<1>(*it) == 30 && it - e.begin() == 2);
    CHECK(std::get<0>(it[-1]) == 1 && std::get<1>(it[-1]) == 20);
    CHECK(e.end().index() == 3 && e.end() - e.begin() == 3);
    CHECK(e.begin() < it && (it <=> e.begin()) == std::strong_ordering::greater);
    std::get<1>(*e.begin()) = 11;
    CHECK(a[0] == 11);
    auto m = rg::iter_move(it);
    static_assert(std::is_same_v<decltype(m), std::tuple<std::ptrdiff_t, int&&>>);
    CHECK(std::get<0>(m) == 2);
    --it;
    CHECK(it.index() == 1 && *it.base() == 20);
  }
  {
    int b[2] = {5, 6};
    E2 e(FwdC(ForwardIter<int>(b), ForwardIter<int>(b + 2)));
    auto it = e.begin();
    ++it;
    CHECK(it.index() == 1 && std::get<1>(*it) == 6);
    ++it;
    CHECK(it == e.end());
  }
  {
    auto e = vw::iota(5) | vw::enumerate | vw::take(2);
    auto it = e.begin();
    CHECK(std::get<0>(*it) == 0 && std::get<1>(*it) == 5);
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
