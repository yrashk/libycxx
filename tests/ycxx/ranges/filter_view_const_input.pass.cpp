// [range.filter.view]: filter_view has
//   constexpr iterator<true> begin() const
//     requires (input_range<const V> && !forward_range<const V> &&
//               indirect_unary_predicate<const Pred, iterator_t<const V>>);
// and a matching end() const, so a filter over a const-iterable input-only view is
// const-iterable; the const iterator's iterator_concept is input_iterator_tag
// ([range.filter.iterator]/2.1). Over a forward view it is not const-iterable.
#include <iterator>
#include <ranges>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;

struct Odd {
  constexpr bool operator()(int x) const { return x % 2 != 0; }
};
using InV = ArchetypeView<InputIter<int>, PtrSentinel<int>>;
using F = rg::filter_view<InV, Odd>;
static_assert(rg::input_range<const F>);
static_assert(std::is_same_v<rg::iterator_t<const F>::iterator_concept, std::input_iterator_tag>);
static_assert(!rg::range<const rg::filter_view<BorrowedView<int>, Odd>>);

constexpr bool test() {
  int a[5] = {1, 2, 3, 4, 5};
  const F f(InV(InputIter<int>(a), PtrSentinel<int>{a + 5}), Odd{});
  CHECK(range_equals(f, {1, 3, 5}));
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
