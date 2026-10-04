// [range.drop.view]: drop_view's begin() is ranges::next(begin, count, end) (/3), cached for
// forward ranges by the non-const overload (/4); begin() const exists only for sized
// random-access const V; size() clamps at zero; borrowed as its view.
#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;

using D1 = rg::drop_view<BorrowedView<int>>;
static_assert(rg::contiguous_range<D1> && rg::common_range<D1> && rg::sized_range<D1> && rg::borrowed_range<D1>);
static_assert(rg::range<const D1>);
static_assert(std::is_same_v<rg::iterator_t<D1>, int*>);

// A forward, non-sized view: no begin() const ([range.drop.view]: begin() const requires
// random_access_range<const V> && sized_range<const V>).
using FV = ArchetypeView<ForwardIter<int>, PtrSentinel<int>>;
using D2 = rg::drop_view<FV>;
static_assert(rg::forward_range<D2> && !rg::range<const D2> && !rg::sized_range<D2>);
static_assert(!rg::borrowed_range<D2>);

// Counts how often its iterator is incremented, to observe the begin() cache.
struct CountingIter {
  using value_type = int;
  using difference_type = std::ptrdiff_t;
  int* p = nullptr;
  int* incs = nullptr;
  constexpr int& operator*() const { return *p; }
  constexpr CountingIter& operator++() {
    ++p;
    ++*incs;
    return *this;
  }
  constexpr CountingIter operator++(int) {
    auto t = *this;
    ++*this;
    return t;
  }
  friend constexpr bool operator==(const CountingIter& a, const CountingIter& b) { return a.p == b.p; }
};
static_assert(std::forward_iterator<CountingIter>);
struct CountingView : rg::view_base {
  int* b = nullptr;
  int* e = nullptr;
  int* incs = nullptr;
  constexpr CountingIter begin() const { return {b, incs}; }
  constexpr CountingIter end() const { return {e, incs}; }
};

constexpr bool test() {
  int a[6] = {0, 1, 2, 3, 4, 5};
  {
    D1 d(BorrowedView<int>(a, a + 6), 2);
    CHECK(range_equals(d, {2, 3, 4, 5}) && d.size() == 4u && d.begin() == a + 2);
    const D1& cd = d;
    CHECK(cd.begin() == a + 2 && cd.size() == 4u);
    D1 over(BorrowedView<int>(a, a + 6), 9);
    CHECK(over.empty() && over.size() == 0u && over.begin() == a + 6);
    D1 none(BorrowedView<int>(a, a + 6), 0);
    CHECK(none.size() == 6u);
  }
  {
    D2 d(FV(ForwardIter<int>(a), PtrSentinel<int>{a + 6}), 4);
    CHECK(range_equals(d, {4, 5}));
    D2 over(FV(ForwardIter<int>(a), PtrSentinel<int>{a + 3}), 4);
    CHECK(over.begin() == over.end());
  }
  {
    int incs = 0;
    rg::drop_view<CountingView> d(CountingView{{}, a, a + 6, &incs}, 3);
    auto b1 = d.begin();
    CHECK(incs == 3 && *b1 == 3);
    auto b2 = d.begin();
    CHECK(incs == 3 && b2 == b1); // cached (/4)
  }
  {
    auto d = std::views::iota(0) | std::views::take(10) | std::views::drop(5);
    CHECK(range_equals(d, {5, 6, 7, 8, 9}));
  }
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
