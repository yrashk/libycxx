// [range.as.const]: views::as_const returns views::all for constant ranges (/2.1),
// empty_view<const X> (/2.2), span<const X, Extent> (/2.4), a ref_view of the const range
// (/2.5, /2.6), and as_const_view otherwise (/2.7), whose iterators are const iterators.
#include <array>
#include <iterator>
#include <ranges>
#include <span>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;
using A = std::array<int, 3>;

template <class R>
using ac_t = decltype(vw::as_const(std::declval<R>()));

static_assert(std::is_same_v<ac_t<std::span<const int>>, std::span<const int>>);              // 2.1
static_assert(std::is_same_v<ac_t<rg::iota_view<int, int>>, rg::iota_view<int, int>>);        // 2.1
static_assert(std::is_same_v<ac_t<rg::empty_view<int>>, rg::empty_view<const int>>);          // 2.2
static_assert(std::is_same_v<ac_t<std::span<int, 3>>, std::span<const int, 3>>);              // 2.4
static_assert(std::is_same_v<ac_t<std::span<int>&>, std::span<const int>>);
static_assert(std::is_same_v<ac_t<rg::ref_view<A>>, rg::ref_view<const A>>);                  // 2.5
static_assert(std::is_same_v<ac_t<A&>, rg::ref_view<const A>>);                               // 2.6
static_assert(std::is_same_v<ac_t<int (&)[2]>, rg::ref_view<const int[2]>>);
static_assert(std::is_same_v<ac_t<A>, rg::as_const_view<rg::owning_view<A>>>);                // 2.7 (rvalue)

using BV = BorrowedView<int>;
using AC = ac_t<BV>;
static_assert(std::is_same_v<AC, rg::as_const_view<BV>>);
static_assert(rg::constant_range<AC> && rg::contiguous_range<AC> && rg::sized_range<AC> == rg::sized_range<BV>);
static_assert(std::is_same_v<rg::range_reference_t<AC>, const int&>);
static_assert(std::is_same_v<rg::iterator_t<AC>, std::basic_const_iterator<int*>>); // ranges::cbegin of a shallow-const view
static_assert(rg::borrowed_range<AC>);

using RandNC = ArchetypeView<RandomIter<int>, PtrSentinel<int>>;
using AC2 = rg::as_const_view<RandNC>;
static_assert(rg::constant_range<AC2> && rg::random_access_range<AC2> && !rg::common_range<AC2>);
static_assert(std::is_same_v<rg::iterator_t<AC2>, std::basic_const_iterator<RandomIter<int>>>);
static_assert(std::is_same_v<rg::range_reference_t<AC2>, const int&>);
static_assert(!rg::borrowed_range<AC2>);

constexpr bool test() {
  int a[3] = {1, 2, 3};
  auto c = vw::as_const(BV(a, a + 3));
  CHECK(range_equals(c, {1, 2, 3}) && c.size() == 3u && c.begin().base() == a && c.base().b == a);
  RandNC r(RandomIter<int>(a), PtrSentinel<int>{a + 3});
  auto c2 = r | vw::as_const;
  CHECK(range_equals(c2, {1, 2, 3}) && c2[2] == 3);
  A arr = {4, 5, 6};
  auto c3 = vw::as_const(arr);
  CHECK(&c3.base() == &arr && c3[0] == 4);
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
