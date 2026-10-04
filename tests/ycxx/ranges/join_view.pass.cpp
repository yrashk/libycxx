// [range.join]: join_view flattens a range of ranges, skipping empty inner ranges
// ([range.join.iterator]/6); iterator_concept (/1) is bidirectional only for glvalue inner
// ranges that are bidirectional and common, forward for glvalue forward inners, input for
// prvalue inner ranges; iterator_category (/2) is present only for glvalue forward inners;
// end() is an iterator when everything is forward and common; begin() const requires a
// glvalue inner range.
#include <array>
#include <iterator>
#include <ranges>
#include <string_view>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

using J1 = rg::join_view<rg::ref_view<int[3][2]>>;
static_assert(rg::bidirectional_range<J1> && rg::common_range<J1> && !rg::sized_range<J1>);
static_assert(rg::bidirectional_range<const J1> && !rg::random_access_range<J1>);
static_assert(std::is_same_v<rg::range_reference_t<J1>, int&> && std::is_same_v<rg::range_value_t<J1>, int>);
static_assert(std::is_same_v<rg::iterator_t<J1>::iterator_concept, std::bidirectional_iterator_tag>);
static_assert(std::is_same_v<rg::iterator_t<J1>::iterator_category, std::bidirectional_iterator_tag>);
static_assert(std::is_same_v<rg::iterator_t<J1>, rg::sentinel_t<J1>>);
static_assert(!rg::borrowed_range<J1>);

// Inner ranges that are forward but not bidirectional/common.
using FwdV = ArchetypeView<ForwardIter<int>, PtrSentinel<int>>;
using J2 = rg::join_view<rg::ref_view<FwdV[2]>>;
static_assert(rg::forward_range<J2> && !rg::bidirectional_range<J2> && !rg::common_range<J2>);
static_assert(std::is_same_v<rg::iterator_t<J2>::iterator_category, std::forward_iterator_tag>);

// prvalue inner ranges: input only, no iterator_category, no const begin().
inline constexpr auto upto = [](int n) { return vw::iota(0, n); };
using J3 = decltype(vw::iota(0, 4) | vw::transform(upto) | vw::join);
static_assert(rg::input_range<J3> && !rg::forward_range<J3>);
static_assert(std::is_same_v<rg::iterator_t<J3>::iterator_concept, std::input_iterator_tag>);
static_assert(!has_iterator_category<rg::iterator_t<J3>>);
static_assert(!rg::range<const J3>);

static_assert(std::is_same_v<decltype(vw::join(std::declval<int (&)[3][2]>())), J1>);

constexpr bool test() {
  int m[3][2] = {{1, 2}, {3, 4}, {5, 6}};
  {
    auto j = m | vw::join;
    CHECK(range_equals(j, {1, 2, 3, 4, 5, 6}));
    auto e = j.end();
    --e;
    CHECK(*e == 6);
    --e;
    --e;
    CHECK(*e == 4);
    CHECK(j.front() == 1 && j.back() == 6);
    *j.begin() = 10;
    CHECK(m[0][0] == 10);
    auto it = j.begin();
    auto old = it++;
    CHECK(*old == 10 && *it == 2);
    static_assert(std::is_same_v<decltype(rg::iter_move(it)), int&&>);
  }
  {
    std::array<std::string_view, 5> words = {"", "ab", "", "", "c"};
    auto j = words | vw::join;
    CHECK(range_equals(j, {'a', 'b', 'c'}));
    auto e = j.end();
    --e;
    CHECK(*e == 'c');
    --e;
    CHECK(*e == 'b'); // skips the empty strings backwards
  }
  {
    std::array<std::string_view, 2> empties = {"", ""};
    auto j = empties | vw::join;
    CHECK(j.begin() == j.end());
  }
  {
    auto j = vw::iota(0, 4) | vw::transform(upto) | vw::join;
    CHECK(range_equals(j, {0, 0, 1, 0, 1, 2}));
  }
  {
    int a[2] = {1, 2}, b[1] = {3};
    FwdV inner[2] = {FwdV(ForwardIter<int>(a), PtrSentinel<int>{a + 2}), FwdV(ForwardIter<int>(b), PtrSentinel<int>{b + 1})};
    auto j = inner | vw::join;
    CHECK(range_equals(j, {1, 2, 3}));
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
