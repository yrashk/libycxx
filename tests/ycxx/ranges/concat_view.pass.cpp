// [range.concat]: views::concat of one range is views::all (/2.1); concat_view's reference,
// value and difference types are the common ones; iterator_concept (/1) requires all but the
// last range to be common for bidirectional/random access (iterator_category: see
// concat_view_iterator_category); end() is default_sentinel unless the last
// range is common; size() is the sum; random-access arithmetic crosses range boundaries and
// empty ranges are skipped.
#include <array>
#include <iterator>
#include <ranges>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;
using A2 = std::array<int, 2>;

static_assert(std::is_same_v<decltype(vw::concat(std::declval<int (&)[3]>())), rg::ref_view<int[3]>>);

using C1 = decltype(vw::concat(std::declval<int (&)[3]>(), std::declval<A2&>()));
static_assert(std::is_same_v<C1, rg::concat_view<rg::ref_view<int[3]>, rg::ref_view<A2>>>);
static_assert(rg::random_access_range<C1> && rg::common_range<C1> && rg::sized_range<C1>);
static_assert(!rg::contiguous_range<C1> && !rg::borrowed_range<C1> && rg::random_access_range<const C1>);
static_assert(std::is_same_v<rg::range_reference_t<C1>, int&>);
static_assert(std::is_same_v<rg::iterator_t<C1>::iterator_concept, std::random_access_iterator_tag>);

// int& and long& have the common reference long, a prvalue.
using C2 = decltype(vw::concat(std::declval<int (&)[2]>(), std::declval<long (&)[2]>()));
static_assert(std::is_same_v<rg::range_reference_t<C2>, long> && std::is_same_v<rg::range_value_t<C2>, long>);
static_assert(std::is_same_v<rg::iterator_t<C2>::iterator_concept, std::random_access_iterator_tag>);

// The last range is not common: still random access, but end() is default_sentinel.
using C3 = decltype(vw::concat(std::declval<int (&)[2]>(), vw::iota(0)));
static_assert(rg::random_access_range<C3> && !rg::common_range<C3> && !rg::sized_range<C3>);
static_assert(std::is_same_v<rg::sentinel_t<C3>, std::default_sentinel_t>);
static_assert(std::is_same_v<rg::range_reference_t<C3>, int>);

// A non-common range before the last one: only forward.
using FwdV = ArchetypeView<ForwardIter<int>, PtrSentinel<int>>;
using RandNC = ArchetypeView<RandomIter<int>, PtrSentinel<int>>;
using C4 = rg::concat_view<RandNC, rg::ref_view<int[2]>>;
static_assert(rg::forward_range<C4> && !rg::bidirectional_range<C4>);
static_assert(std::is_same_v<rg::iterator_t<C4>::iterator_concept, std::forward_iterator_tag>);
using C5 = rg::concat_view<rg::ref_view<int[2]>, FwdV>;
static_assert(rg::forward_range<C5> && !rg::bidirectional_range<C5>);

// Input ranges: input only, no iterator_category.
using InV = ArchetypeView<InputIter<int>, PtrSentinel<int>>;
using C6 = rg::concat_view<InV, rg::ref_view<int[2]>>;
static_assert(rg::input_range<C6> && !rg::forward_range<C6> && !has_iterator_category<rg::iterator_t<C6>>);

// Non-concatable element types.
struct X {};
template <class... R>
concept can_concat = requires(R&... r) { vw::concat(r...); };
static_assert(can_concat<int[2], long[2]> && !can_concat<int[2], X[2]>);
static_assert(!can_concat<>); // [range.concat.view]: sizeof...(Views) > 0

constexpr bool test() {
  int a[3] = {1, 2, 3};
  A2 b = {4, 5};
  int empty_arr[1] = {99};
  {
    auto c = vw::concat(a, b);
    CHECK(range_equals(c, {1, 2, 3, 4, 5}) && c.size() == 5u);
    auto i = c.begin();
    i += 4;
    CHECK(*i == 5);
    i -= 3;
    CHECK(*i == 2 && i[2] == 4 && *(i + 3) == 5);
    CHECK(c.end() - c.begin() == 5 && c.begin() - c.end() == -5);
    CHECK((c.begin() + 3) - (c.begin() + 1) == 2);
    CHECK(c.begin() < i && (i <=> c.begin()) > 0);
    auto e = c.end();
    --e;
    CHECK(*e == 5);
    --e;
    --e;
    CHECK(*e == 3);
    CHECK(c[3] == 4 && c.back() == 5);
    c[3] = 40;
    CHECK(b[0] == 40);
    rg::iter_swap(c.begin(), c.begin() + 4);
    CHECK(a[0] == 5 && b[1] == 1);
  }
  {
    auto c = vw::concat(a, vw::take(empty_arr, 0), b, vw::single(6));
    CHECK(range_equals(c, {5, 2, 3, 40, 1, 6}) && c.size() == 6u);
    auto it = c.begin() + 3;
    CHECK(*it == 40);
    --it;
    CHECK(*it == 3);
  }
  {
    auto c = vw::concat(a, vw::iota(100));
    auto i = c.begin();
    i += 5;
    CHECK(*i == 102 && !(i == std::default_sentinel));
  }
  {
    int x[2] = {7, 8};
    long y[1] = {9};
    auto c = vw::concat(x, y);
    CHECK(range_equals(c, {7L, 8L, 9L}));
  }
  {
    auto c = vw::concat(vw::iota(0, 2), vw::iota(5, 7));
    auto i = c.begin() + 2;
    CHECK(range_equals(c, {0, 1, 5, 6}) && c.end() - i == 2);
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
