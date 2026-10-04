// [range.stride]: stride_view yields every nth element; it is as strong as the base;
// iterator_category is random_access_iterator_tag for random-access C, otherwise C, and only
// for forward bases; end() of a common sized forward base is an iterator that accounts for
// the missing steps, so reverse iteration works; size() is div-ceil(size, n); stride() is
// noexcept; borrowed as its base.
#include <iterator>
#include <ranges>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

using S1 = decltype(vw::iota(0, 12) | vw::stride(3));
static_assert(std::is_same_v<S1, rg::stride_view<rg::iota_view<int, int>>>);
static_assert(rg::random_access_range<S1> && rg::common_range<S1> && rg::sized_range<S1> && rg::borrowed_range<S1>);
static_assert(std::is_same_v<rg::iterator_t<S1>::iterator_category, std::input_iterator_tag>); // iota's C
using S2 = decltype(std::declval<int (&)[5]>() | vw::stride(2));
static_assert(std::is_same_v<rg::iterator_t<S2>::iterator_category, std::random_access_iterator_tag>);
static_assert(!rg::contiguous_range<S2>);
static_assert(noexcept(std::declval<const S2&>().stride()));

using BidiC = ArchetypeView<BidiIter<int>>;
using S3 = rg::stride_view<BidiC>;
static_assert(rg::bidirectional_range<S3> && !rg::common_range<S3>); // not sized: default_sentinel
static_assert(std::is_same_v<rg::iterator_t<S3>::iterator_category, std::bidirectional_iterator_tag>);
using FwdC = ArchetypeView<ForwardIter<int>>;
static_assert(rg::common_range<rg::stride_view<FwdC>>);
using InV = ArchetypeView<InputIter<int>, PtrSentinel<int>>;
static_assert(!has_iterator_category<rg::iterator_t<rg::stride_view<InV>>>);

constexpr bool test() {
  {
    auto s = vw::iota(0, 12) | vw::stride(3);
    CHECK(range_equals(s, {0, 3, 6, 9}) && s.size() == 4u);
    CHECK(range_equals(s | vw::reverse, {9, 6, 3, 0}));
    CHECK(s[2] == 6 && s.stride() == 3);
  }
  {
    auto s = vw::iota(0, 10) | vw::stride(3);
    CHECK(range_equals(s, {0, 3, 6, 9}) && s.size() == 4u);
    CHECK(range_equals(s | vw::reverse, {9, 6, 3, 0}));
    auto t = vw::iota(0, 11) | vw::stride(3);
    CHECK(range_equals(t | vw::reverse, {9, 6, 3, 0}) && t.end() - t.begin() == 4);
  }
  {
    int a[5] = {1, 2, 3, 4, 5};
    auto s = a | vw::stride(2);
    CHECK(range_equals(s, {1, 3, 5}) && s.size() == 3u);
    auto i = s.begin();
    i += 2;
    CHECK(*i == 5 && i - s.begin() == 2);
    ++i;
    CHECK(i == s.end());
    CHECK(range_equals(a | vw::stride(10), {1}));
    S3 b(BidiC(BidiIter<int>(a), BidiIter<int>(a + 5)), 2);
    CHECK(range_equals(b, {1, 3, 5}));
    InV in(InputIter<int>(a), PtrSentinel<int>{a + 5});
    CHECK(range_equals(rg::stride_view<InV>(in, 4), {1, 5}));
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
