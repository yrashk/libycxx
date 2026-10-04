// [range.elements]: elements_view<V, N> projects the Nth tuple element; keys_view and
// values_view are elements_view<R, 0> and <R, 1>; for a prvalue tuple reference the element
// is returned by value (get-element, /3) and iterator_category is input_iterator_tag (/2.1),
// otherwise it is random_access_iterator_tag for random-access C (/2.2) or C; the
// has-tuple-element constraint; borrowed as its view.
#include <array>
#include <iterator>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;
using P = std::pair<int, char>;

static_assert(std::is_same_v<rg::keys_view<rg::ref_view<P[2]>>, rg::elements_view<rg::ref_view<P[2]>, 0>>);
static_assert(std::is_same_v<rg::values_view<rg::ref_view<P[2]>>, rg::elements_view<rg::ref_view<P[2]>, 1>>);

using E1 = decltype(std::declval<P (&)[3]>() | vw::values);
static_assert(std::is_same_v<E1, rg::elements_view<rg::ref_view<P[3]>, 1>>);
static_assert(rg::random_access_range<E1> && rg::common_range<E1> && rg::sized_range<E1> && rg::borrowed_range<E1>);
static_assert(!rg::contiguous_range<E1>);
static_assert(std::is_same_v<rg::range_reference_t<E1>, char&> && std::is_same_v<rg::range_value_t<E1>, char>);
static_assert(std::is_same_v<rg::iterator_t<E1>::iterator_category, std::random_access_iterator_tag>);
static_assert(std::is_same_v<rg::iterator_t<E1>::iterator_concept, std::random_access_iterator_tag>);

inline constexpr auto mk = [](int i) { return std::tuple<int, long, char>(i, i * 10L, char('a' + i)); };
using E2 = decltype(vw::iota(0, 3) | vw::transform(mk) | vw::elements<1>);
static_assert(std::is_same_v<rg::range_reference_t<E2>, long>);
static_assert(std::is_same_v<rg::iterator_t<E2>::iterator_category, std::input_iterator_tag>);
static_assert(rg::random_access_range<E2>);

using FwdV = ArchetypeView<ForwardIter<P>, PtrSentinel<P>>;
using E3 = rg::elements_view<FwdV, 0>;
static_assert(rg::forward_range<E3> && !rg::common_range<E3>);
static_assert(std::is_same_v<rg::iterator_t<E3>::iterator_category, std::forward_iterator_tag>);
using InV = ArchetypeView<InputIter<P>, PtrSentinel<P>>;
static_assert(!has_iterator_category<rg::iterator_t<rg::elements_view<InV, 0>>>);

template <class V, std::size_t N>
concept has_elements = requires { typename rg::elements_view<V, N>; };
static_assert(has_elements<rg::ref_view<P[2]>, 1> && !has_elements<rg::ref_view<P[2]>, 2>);
static_assert(!has_elements<rg::ref_view<int[2]>, 0>);
static_assert(has_elements<rg::ref_view<std::array<int, 3>[2]>, 2>);

constexpr bool test() {
  P ps[3] = {{1, 'x'}, {2, 'y'}, {3, 'z'}};
  CHECK(range_equals(ps | vw::keys, {1, 2, 3}));
  CHECK(range_equals(ps | vw::values, {'x', 'y', 'z'}));
  auto v = ps | vw::values;
  v[1] = 'Y';
  CHECK(ps[1].second == 'Y' && v.size() == 3u && v.back() == 'z');
  CHECK(v.begin().base() == ps + 0);
  auto it = v.begin() + 2;
  CHECK(*it == 'z' && it - v.begin() == 2 && it[-1] == 'Y');
  CHECK(range_equals(vw::iota(0, 3) | vw::transform(mk) | vw::elements<2>, {'a', 'b', 'c'}));
  std::array<int, 3> arrs[2] = {{1, 2, 3}, {4, 5, 6}};
  CHECK(range_equals(arrs | vw::elements<2>, {3, 6}));
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
