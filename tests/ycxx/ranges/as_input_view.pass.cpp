// [range.as.input]: views::as_input is views::all for input-only non-common ranges (/2.1),
// otherwise as_input_view, an input-only, non-common view whose end() is the base's end()
// ([range.as.input.view]/4); sized as its base.
#include <iterator>
#include <ranges>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

using AI = decltype(std::declval<int (&)[3]>() | vw::as_input);
static_assert(std::is_same_v<AI, rg::as_input_view<rg::ref_view<int[3]>>>);
static_assert(rg::input_range<AI> && !rg::forward_range<AI> && !rg::common_range<AI>);
static_assert(rg::sized_range<AI> && rg::input_range<const AI>); // borrowed_range: as_input_view_borrowed
static_assert(std::is_same_v<rg::sentinel_t<AI>, int*>);
static_assert(std::is_same_v<rg::range_reference_t<AI>, int&>);
static_assert(std::is_same_v<rg::range_rvalue_reference_t<AI>, int&&>);
static_assert(std::is_same_v<rg::iterator_t<AI>::iterator_concept, std::input_iterator_tag>);

using InV = ArchetypeView<InputIter<int>, PtrSentinel<int>>;
static_assert(std::is_same_v<decltype(std::declval<InV>() | vw::as_input), InV>);
using InC = ArchetypeView<InputIter<int>>; // input, but common
static_assert(std::is_same_v<decltype(std::declval<InC>() | vw::as_input), rg::as_input_view<InC>>);

constexpr bool test() {
  int a[3] = {1, 2, 3};
  auto v = a | vw::as_input;
  CHECK(range_equals(v, {1, 2, 3}) && v.size() == 3u);
  auto it = v.begin();
  *it = 10;
  CHECK(a[0] == 10 && *it.base() == 10);
  ++it;
  it++;
  CHECK(*it == 3);
  CHECK(v.end() - v.begin() == 3);
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
