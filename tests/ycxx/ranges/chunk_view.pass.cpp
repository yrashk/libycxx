// [range.chunk]: chunk_view splits into non-overlapping chunks of n elements (the last one
// possibly shorter); for forward ranges each chunk is views::take(subrange(...), n), the
// iterator is as strong as the base (random access for random-access bases) with
// iterator_category input_iterator_tag, and end() is an iterator for common sized bases;
// for input ranges chunks are input ranges over the shared position; size() is
// div-ceil(distance, n); borrowed iff the base is forward and borrowed.
#include <iterator>
#include <ranges>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

using C1 = decltype(std::declval<int (&)[5]>() | vw::chunk(2));
static_assert(std::is_same_v<C1, rg::chunk_view<rg::ref_view<int[5]>>>);
static_assert(rg::random_access_range<C1> && rg::common_range<C1> && rg::sized_range<C1> && rg::borrowed_range<C1>);
static_assert(std::is_same_v<rg::iterator_t<C1>::iterator_category, std::input_iterator_tag>);
static_assert(std::is_same_v<rg::iterator_t<C1>::iterator_concept, std::random_access_iterator_tag>);
static_assert(std::is_same_v<rg::range_value_t<C1>, rg::subrange<int*>>); // views::take of a subrange<int*>
static_assert(rg::random_access_range<const C1>);

using FwdNC = ArchetypeView<ForwardIter<int>, PtrSentinel<int>>;
using C2 = rg::chunk_view<FwdNC>;
static_assert(rg::forward_range<C2> && !rg::bidirectional_range<C2> && !rg::common_range<C2>);
static_assert(std::is_same_v<rg::sentinel_t<C2>, std::default_sentinel_t>);
using FwdC = ArchetypeView<ForwardIter<int>>;
static_assert(rg::common_range<rg::chunk_view<FwdC>>); // common, not bidirectional

using InV = ArchetypeView<InputIter<int>, PtrSentinel<int>>;
using C3 = rg::chunk_view<InV>;
static_assert(rg::input_range<C3> && !rg::forward_range<C3> && !rg::borrowed_range<C3>);
static_assert(std::is_same_v<rg::sentinel_t<C3>, std::default_sentinel_t>);
static_assert(rg::input_range<rg::range_reference_t<C3>> && !rg::forward_range<rg::range_reference_t<C3>>);
static_assert(!rg::range<const C3>);
static_assert(!rg::borrowed_range<rg::chunk_view<rg::owning_view<rg::single_view<int>>>>);

constexpr bool test() {
  int a[5] = {1, 2, 3, 4, 5};
  {
    auto c = a | vw::chunk(2);
    CHECK(c.size() == 3u && c.end() - c.begin() == 3);
    CHECK(range_equals(c[0], {1, 2}) && range_equals(c[1], {3, 4}) && range_equals(c[2], {5}));
    auto e = c.end();
    --e;
    CHECK(range_equals(*e, {5}));
    --e;
    CHECK(range_equals(*e, {3, 4}));
    auto i = c.begin() + 2;
    CHECK(range_equals(*i, {5}) && i - c.begin() == 2);
    CHECK((a | vw::chunk(5)).size() == 1u && (a | vw::chunk(7)).size() == 1u);
    CHECK((a | vw::chunk(1)).size() == 5u);
  }
  {
    int b[4] = {1, 2, 3, 4};
    auto c = b | vw::chunk(2);
    auto e = c.end();
    --e;
    CHECK(range_equals(*e, {3, 4}));
  }
  {
    C2 c(FwdNC(ForwardIter<int>(a), PtrSentinel<int>{a + 5}), 3);
    auto i = c.begin();
    CHECK(range_equals(*i, {1, 2, 3}));
    ++i;
    CHECK(range_equals(*i, {4, 5}));
    ++i;
    CHECK(i == std::default_sentinel);
  }
  {
    C3 c(InV(InputIter<int>(a), PtrSentinel<int>{a + 5}), 2);
    int total = 0, chunks = 0;
    for (auto&& inner : c) {
      ++chunks;
      for (int v : inner) total = total * 10 + v;
    }
    CHECK(chunks == 3 && total == 12345);
  }
  {
    // size() of an input chunk_view over a sized range.
    auto c = vw::iota(0, 10) | vw::chunk(4);
    CHECK(c.size() == 3u);
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
