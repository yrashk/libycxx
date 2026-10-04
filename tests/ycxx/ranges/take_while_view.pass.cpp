// [range.take.while]: take_while_view produces [begin(r), find_if_not(r, pred)); it is never
// common (its sentinel tests the predicate, [range.take.while.sentinel]/3) nor sized; const
// iteration requires the predicate to be invocable as const; pred() returns the predicate.
#include <concepts>
#include <iterator>
#include <ranges>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;

struct Less {
  int n;
  constexpr bool operator()(int x) const { return x < n; }
};
using TW = rg::take_while_view<BorrowedView<int>, Less>;
static_assert(rg::view<TW> && rg::contiguous_range<TW> && !rg::common_range<TW> && !rg::sized_range<TW>);
static_assert(!rg::borrowed_range<TW>);
static_assert(rg::contiguous_range<const TW>);
static_assert(std::is_same_v<rg::iterator_t<TW>, int*>);
static_assert(std::is_same_v<decltype(std::declval<const TW&>().pred()), const Less&>);

using FV = ArchetypeView<ForwardIter<int>, PtrSentinel<int>>;
static_assert(rg::forward_range<rg::take_while_view<FV, Less>>);

using TWM = rg::take_while_view<MutableOnlyView<int>, Less>;
static_assert(rg::range<TWM> && !rg::range<const TWM>);

// The predicate must be callable as const (indirect_unary_predicate<const Pred, ...>).
struct NonConstPred {
  bool operator()(int) { return true; }
};
template <class P>
concept can_take_while = requires { typename rg::take_while_view<BorrowedView<int>, P>; };
static_assert(can_take_while<Less> && !can_take_while<NonConstPred>);

constexpr bool test() {
  int a[7] = {1, 2, 3, 9, 1, 2, 3};
  auto t = a | std::views::take_while(Less{5});
  CHECK(range_equals(t, {1, 2, 3}));
  CHECK(t.pred().n == 5 && t.front() == 1 && !t.empty());
  auto e = t.end();
  CHECK(e.base() == a + 7);
  auto none = a | std::views::take_while(Less{0});
  CHECK(none.empty());
  auto all = a | std::views::take_while(Less{100});
  CHECK(count_elements(all) == 7);
  auto lam = std::views::iota(1) | std::views::take_while([](int x) { return x * x < 30; });
  CHECK(range_equals(lam, {1, 2, 3, 4, 5}));
  const auto& ct = t;
  CHECK(count_elements(ct) == 3);
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
