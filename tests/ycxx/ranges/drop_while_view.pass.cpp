// [range.drop.while]: drop_while_view produces [find_if_not(r, pred), end(r)); it has no
// const begin(); begin() caches its result for forward ranges ([range.drop.while.view]/5),
// so the predicate is not called again by a second begin(); borrowed as its view.
#include <iterator>
#include <ranges>
#include <string_view>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;

struct Small {
  int* calls = nullptr;
  constexpr bool operator()(int x) const {
    if (calls) ++*calls;
    return x < 3;
  }
};
using DW = rg::drop_while_view<BorrowedView<int>, Small>;
static_assert(rg::view<DW> && rg::contiguous_range<DW> && rg::common_range<DW> && rg::sized_range<DW>);
static_assert(rg::borrowed_range<DW>);
static_assert(!rg::range<const DW>);
static_assert(std::is_same_v<decltype(std::declval<const DW&>().pred()), const Small&>);
static_assert(rg::borrowed_range<rg::drop_while_view<rg::ref_view<int[2]>, Small>>);
static_assert(!rg::borrowed_range<rg::drop_while_view<rg::owning_view<rg::single_view<int>>, Small>>);

constexpr bool test() {
  int a[6] = {0, 1, 2, 3, 0, 1};
  int calls = 0;
  auto d = a | std::views::drop_while(Small{&calls});
  CHECK(calls == 0); // lazy
  auto b = d.begin();
  CHECK(*b == 3 && calls == 4);
  CHECK(d.begin() == b && calls == 4); // cached
  CHECK(range_equals(d, {3, 0, 1}));
  CHECK(calls == 4);
  auto all = a | std::views::drop_while([](int) { return true; });
  CHECK(all.empty());
  constexpr std::string_view src = "  \t  hello there";
  auto skip = src | std::views::drop_while([](char c) { return c == ' ' || c == '\t'; });
  CHECK(*skip.begin() == 'h' && count_elements(skip) == 11);
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
