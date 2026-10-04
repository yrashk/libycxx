// [range.common]: views::common is views::all for common ranges (/3.1), otherwise
// common_view; common_view keeps the iterator of a sized random-access view (end() is
// begin() + distance) and uses common_iterator otherwise; common_view requires a non-common
// view with copyable iterators.
#include <iterator>
#include <ranges>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

static_assert(std::is_same_v<decltype(vw::common(std::declval<int (&)[2]>())), rg::ref_view<int[2]>>);
static_assert(std::is_same_v<decltype(vw::common(vw::iota(0, 3))), rg::iota_view<int, int>>);

// Sized random access, not common.
using C1 = decltype(vw::iota(0, 5L) | vw::common);
static_assert(std::is_same_v<C1, rg::common_view<rg::iota_view<int, long>>>);
static_assert(rg::common_range<C1> && rg::random_access_range<C1> && rg::sized_range<C1>);
static_assert(std::is_same_v<rg::iterator_t<C1>, rg::iterator_t<rg::iota_view<int, long>>>);
static_assert(rg::borrowed_range<C1>);

// Not sized: common_iterator (at most forward).
using RandNC = ArchetypeView<RandomIter<int>, PtrSentinel<int>>;
using C2 = rg::common_view<RandNC>;
static_assert(std::is_same_v<rg::iterator_t<C2>, std::common_iterator<RandomIter<int>, PtrSentinel<int>>>);
static_assert(rg::common_range<C2> && rg::forward_range<C2> && !rg::bidirectional_range<C2>);
static_assert(!rg::borrowed_range<C2>);

template <class V>
concept common_ok = requires { typename rg::common_view<V>; };
static_assert(!common_ok<rg::ref_view<int[2]>>); // already common
static_assert(common_ok<RandNC>);
// Move-only iterators are not supported.
static_assert(std::is_same_v<decltype(rg::common_view(std::declval<RandNC>())), C2>);

constexpr bool test() {
  {
    auto c = vw::iota(0, 5L) | vw::common;
    CHECK(range_equals(c, {0, 1, 2, 3, 4}) && c.size() == 5u && *(c.end() - 1) == 4);
  }
  {
    int a[4] = {1, 2, 3, 4};
    C2 c(RandNC(RandomIter<int>(a), PtrSentinel<int>{a + 4}));
    CHECK(range_equals(c, {1, 2, 3, 4}));
    int sum = 0;
    for (auto i = c.begin(), e = c.end(); i != e; ++i) sum += *i; // same-typed pair
    CHECK(sum == 10);
    CHECK(c.base().b.p == a);
  }
  {
    auto t = vw::iota(1) | vw::take(3) | vw::common;
    static_assert(rg::common_range<decltype(t)>);
    CHECK(range_equals(t, {1, 2, 3}));
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
