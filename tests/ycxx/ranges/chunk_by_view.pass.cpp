// [range.chunk.by]: chunk_by_view splits between adjacent elements for which the predicate
// is false; elements are subrange<iterator_t<V>>; the iterator is bidirectional for
// bidirectional bases, otherwise forward, with iterator_category input_iterator_tag;
// chunk_by_view is not const-iterable, not sized, not borrowed; begin() is cached.
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

using CB = decltype(std::declval<int (&)[8]>() | vw::chunk_by(rg::less_equal{}));
static_assert(std::is_same_v<CB, rg::chunk_by_view<rg::ref_view<int[8]>, rg::less_equal>>);
static_assert(rg::bidirectional_range<CB> && !rg::random_access_range<CB> && rg::common_range<CB>);
static_assert(!rg::sized_range<CB> && !rg::borrowed_range<CB> && !rg::range<const CB>);
static_assert(std::is_same_v<rg::range_value_t<CB>, rg::subrange<int*>>);
static_assert(std::is_same_v<rg::iterator_t<CB>::iterator_category, std::input_iterator_tag>);
static_assert(std::is_same_v<rg::iterator_t<CB>::iterator_concept, std::bidirectional_iterator_tag>);

using FwdNC = ArchetypeView<ForwardIter<int>, PtrSentinel<int>>;
using CF = rg::chunk_by_view<FwdNC, rg::equal_to>;
static_assert(rg::forward_range<CF> && !rg::bidirectional_range<CF> && !rg::common_range<CF>);
static_assert(std::is_same_v<rg::iterator_t<CF>::iterator_concept, std::forward_iterator_tag>);

struct CountingLE {
  int* calls;
  constexpr bool operator()(int x, int y) const {
    ++*calls;
    return x <= y;
  }
};

constexpr bool test() {
  int v[8] = {1, 2, 2, 3, 0, 4, 5, 2};
  {
    auto c = v | vw::chunk_by(rg::less_equal{});
    auto i = c.begin();
    CHECK(range_equals(*i, {1, 2, 2, 3}));
    ++i;
    CHECK(range_equals(*i, {0, 4, 5}));
    ++i;
    CHECK(range_equals(*i, {2}));
    ++i;
    CHECK(i == c.end());
    --i;
    CHECK(range_equals(*i, {2}));
    --i;
    CHECK(range_equals(*i, {0, 4, 5}));
    --i;
    CHECK(i == c.begin());
    CHECK(c.pred()(1, 2));
  }
  {
    int calls = 0;
    auto c = v | vw::chunk_by(CountingLE{&calls});
    (void)c.begin();
    int after_first = calls;
    CHECK(after_first > 0);
    (void)c.begin();
    CHECK(calls == after_first); // cached ([range.chunk.by.view]: "caches the result")
  }
  {
    int e[1] = {0};
    auto c = vw::take(e, 0) | vw::chunk_by(rg::equal_to{});
    CHECK(c.begin() == c.end());
    int w[5] = {1, 1, 2, 2, 2};
    CF f(FwdNC(ForwardIter<int>(w), PtrSentinel<int>{w + 5}), rg::equal_to{});
    CHECK(count_elements(f) == 2);
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
