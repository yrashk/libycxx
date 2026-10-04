// [range.take.view], [range.take.sentinel]: take_view's iterator and sentinel types follow
// begin()/end(): the underlying iterator for sized random-access ranges (common), a
// counted_iterator with default_sentinel for other sized ranges, counted_iterator with
// take_view::sentinel otherwise; size() is min(size, count); borrowed as its view.
#include <concepts>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;

// Sized random access: plain pointers, common, contiguous.
using T1 = rg::take_view<BorrowedView<int>>;
static_assert(std::is_same_v<rg::iterator_t<T1>, int*> && std::is_same_v<rg::sentinel_t<T1>, int*>);
static_assert(rg::contiguous_range<T1> && rg::common_range<T1> && rg::sized_range<T1> && rg::borrowed_range<T1>);

// Sized bidirectional (sized via a sized sentinel? no: a sized non-random range).
struct SizedBidi : rg::view_base {
  int* b = nullptr;
  int* e = nullptr;
  constexpr BidiIter<int> begin() const { return BidiIter<int>(b); }
  constexpr BidiIter<int> end() const { return BidiIter<int>(e); }
  constexpr std::size_t size() const { return static_cast<std::size_t>(e - b); }
};
using T2 = rg::take_view<SizedBidi>;
static_assert(std::is_same_v<rg::iterator_t<T2>, std::counted_iterator<BidiIter<int>>>);
static_assert(std::is_same_v<rg::sentinel_t<T2>, std::default_sentinel_t>);
static_assert(rg::bidirectional_range<T2> && rg::sized_range<T2> && !rg::common_range<T2>);
static_assert(!rg::borrowed_range<T2>);

// Not sized, no sized sentinel: counted_iterator with the take_view sentinel.
using FV = ArchetypeView<ForwardIter<int>, PtrSentinel<int>>;
using T3 = rg::take_view<FV>;
static_assert(std::is_same_v<rg::iterator_t<T3>, std::counted_iterator<ForwardIter<int>>>);
static_assert(!std::is_same_v<rg::sentinel_t<T3>, std::default_sentinel_t>);
static_assert(rg::forward_range<T3> && !rg::sized_range<T3> && !rg::common_range<T3>);

// Unbounded iota is not sized: taking from it gives a non-sized take_view ([range.take.overview]
// /2.4 applies only to sized iota_views).
using T4 = decltype(std::views::iota(0) | std::views::take(5));
static_assert(std::is_same_v<T4, rg::take_view<rg::iota_view<int>>>);
static_assert(!rg::sized_range<T4> && rg::random_access_range<T4>);

// Const iteration requires range<const V>.
using T5 = rg::take_view<MutableOnlyView<int>>;
static_assert(rg::range<T5> && !rg::range<const T5>);

// Deduction guide and views::take.
static_assert(std::is_same_v<decltype(rg::take_view(std::declval<int (&)[3]>(), 1)), rg::take_view<rg::ref_view<int[3]>>>);
// F must be convertible to the difference type.
template <class R, class N>
concept can_take = requires(R&& r, N n) { std::views::take(std::forward<R>(r), n); };
static_assert(can_take<int (&)[3], int> && can_take<int (&)[3], long> && !can_take<int (&)[3], int*>);

constexpr bool test() {
  int a[6] = {0, 1, 2, 3, 4, 5};
  {
    T1 t(BorrowedView<int>(a, a + 6), 3);
    CHECK(range_equals(t, {0, 1, 2}) && t.size() == 3u && t.end() == a + 3);
    T1 big(BorrowedView<int>(a, a + 6), 10);
    CHECK(big.size() == 6u && big.end() == a + 6);
    CHECK(t.base().b == a);
  }
  {
    T2 t(SizedBidi{{}, a, a + 6}, 4);
    CHECK(range_equals(t, {0, 1, 2, 3}) && t.size() == 4u);
    T2 s(SizedBidi{{}, a, a + 2}, 4);
    CHECK(range_equals(s, {0, 1}) && s.size() == 2u);
  }
  {
    T3 t(FV(ForwardIter<int>(a), PtrSentinel<int>{a + 6}), 2);
    CHECK(range_equals(t, {0, 1}));
    T3 all(FV(ForwardIter<int>(a), PtrSentinel<int>{a + 3}), 8); // fewer than N elements
    CHECK(range_equals(all, {0, 1, 2}));
    auto e = t.end();
    CHECK(e.base().p == a + 6);
    const T3& ct = t;
    auto ce = ct.end(); // sentinel<true>, convertible from sentinel<false>
    decltype(ce) conv = e;
    CHECK(conv.base().p == a + 6 && t.begin() != ce);
  }
  {
    auto t = std::views::iota(0) | std::views::take(4);
    CHECK(range_equals(t, {0, 1, 2, 3}));
  }
  {
    auto t = a | std::views::take(0);
    CHECK(t.empty());
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
