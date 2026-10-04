// [range.adjacent.transform]: views::adjacent_transform<N>(F) applies F to N consecutive
// elements; adjacent_transform<0> is views::zip_transform(F) (/2.1); pairwise_transform is
// adjacent_transform<2>; iterator_category is input_iterator_tag for a non-reference result.
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

inline constexpr auto zero = [] { return 0; };
static_assert(std::is_same_v<decltype(std::declval<int (&)[3]>() | vw::adjacent_transform<0>(zero)), rg::empty_view<int>>);
using AT = decltype(std::declval<int (&)[4]>() | vw::pairwise_transform(std::multiplies<>()));
static_assert(std::is_same_v<AT, rg::adjacent_transform_view<rg::ref_view<int[4]>, std::multiplies<>, 2>>);
static_assert(rg::random_access_range<AT> && rg::common_range<AT> && rg::sized_range<AT> && !rg::borrowed_range<AT>);
static_assert(std::is_same_v<rg::range_reference_t<AT>, int>);
static_assert(std::is_same_v<rg::iterator_t<AT>::iterator_category, std::input_iterator_tag>);

inline constexpr auto mid = [](int&, int& y, int&) -> int& { return y; };
using AR = decltype(std::declval<int (&)[4]>() | vw::adjacent_transform<3>(mid));
static_assert(std::is_same_v<rg::range_reference_t<AR>, int&>);
static_assert(std::is_same_v<rg::iterator_t<AR>::iterator_category, std::random_access_iterator_tag>);

using FwdNC = ArchetypeView<ForwardIter<int>, PtrSentinel<int>>;
using AF = rg::adjacent_transform_view<FwdNC, decltype(mid), 3>;
static_assert(std::is_same_v<rg::iterator_t<AF>::iterator_category, std::forward_iterator_tag>);

constexpr bool test() {
  int a[4] = {1, 2, 3, 4};
  CHECK(range_equals(a | vw::pairwise_transform(std::multiplies<>()), {2, 6, 12}));
  auto m = a | vw::adjacent_transform<3>(mid);
  CHECK(range_equals(m, {2, 3}) && m.size() == 2u);
  m[0] = 20;
  CHECK(a[1] == 20);
  CHECK((a | vw::adjacent_transform<5>([](int, int, int, int, int) { return 0; })).empty());
  auto s = vw::iota(1, 6) | vw::adjacent_transform<2>([](int x, int y) { return x + y; });
  CHECK(range_equals(s, {3, 5, 7, 9}));
  auto it = s.end();
  --it;
  CHECK(*it == 9 && s.end() - s.begin() == 4);
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
