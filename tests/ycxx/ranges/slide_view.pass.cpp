// [range.slide]: slide_view's Mth element is views::counted(current, n) over elements M to
// M+n-1; it is empty when the base has fewer than n elements; size() is size - n + 1
// clamped at zero; iterator_category input_iterator_tag; const-iterable only for sized
// random-access bases (slide-caches-nothing); common for sized random-access and
// bidirectional common bases.
#include <iterator>
#include <ranges>
#include <span>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

using S1 = decltype(std::declval<int (&)[4]>() | vw::slide(2));
static_assert(std::is_same_v<S1, rg::slide_view<rg::ref_view<int[4]>>>);
static_assert(rg::random_access_range<S1> && rg::common_range<S1> && rg::sized_range<S1> && rg::borrowed_range<S1>);
static_assert(rg::random_access_range<const S1>);
static_assert(std::is_same_v<rg::range_value_t<S1>, std::span<int>>); // views::counted(int*, n)
static_assert(std::is_same_v<rg::iterator_t<S1>::iterator_category, std::input_iterator_tag>);

using BidiC = ArchetypeView<BidiIter<int>>;
using S2 = rg::slide_view<BidiC>;
static_assert(rg::bidirectional_range<S2> && rg::common_range<S2> && !rg::range<const S2>);

using FwdNC = ArchetypeView<ForwardIter<int>, PtrSentinel<int>>;
using S3 = rg::slide_view<FwdNC>;
static_assert(rg::forward_range<S3> && !rg::common_range<S3> && !rg::range<const S3>);

template <class V>
concept slidable = requires { typename rg::slide_view<V>; };
static_assert(!slidable<ArchetypeView<InputIter<int>, PtrSentinel<int>>>);

constexpr bool test() {
  int a[4] = {1, 2, 3, 4};
  {
    auto s = a | vw::slide(2);
    CHECK(s.size() == 3u);
    CHECK(range_equals(s[0], {1, 2}) && range_equals(s[2], {3, 4}));
    auto w = a | vw::slide(3);
    CHECK(w.size() == 2u && range_equals(w.back(), {2, 3, 4}));
    auto none = a | vw::slide(5);
    CHECK(none.size() == 0u && none.empty());
    auto all = a | vw::slide(4);
    CHECK(all.size() == 1u);
    CHECK(s.end() - s.begin() == 3);
  }
  {
    S2 s(BidiC(BidiIter<int>(a), BidiIter<int>(a + 4)), 3);
    CHECK(count_elements(s) == 2);
    auto e = s.end();
    --e;
    CHECK(range_equals(*e, {2, 3, 4}));
  }
  {
    S3 s(FwdNC(ForwardIter<int>(a), PtrSentinel<int>{a + 4}), 2);
    auto i = s.begin();
    CHECK(range_equals(*i, {1, 2}));
    CHECK(count_elements(s) == 3);
    S3 n(FwdNC(ForwardIter<int>(a), PtrSentinel<int>{a + 1}), 2);
    CHECK(n.begin() == n.end());
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
