// [range.zip.transform]: views::zip_transform(F) with no ranges is an empty_view of
// decay_t<invoke_result_t<FD&>> (/2.1.2); otherwise zip_transform_view, whose
// iterator_category is input_iterator_tag for a non-reference result and otherwise the
// common category of the views (/1); sized and common as the zip; not borrowed.
#include <array>
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

inline constexpr auto zero = [] { return 0L; };
static_assert(std::is_same_v<decltype(vw::zip_transform(zero)), rg::empty_view<long>>);
template <class F>
concept zt_empty_ok = requires(F f) { vw::zip_transform(f); };
static_assert(!zt_empty_ok<void (*)()>); // void result is not an object type

using ZT = decltype(vw::zip_transform(std::plus<>(), std::declval<int (&)[3]>(), std::declval<int (&)[2]>()));
static_assert(rg::random_access_range<ZT> && rg::common_range<ZT> && rg::sized_range<ZT>);
static_assert(!rg::borrowed_range<ZT>);
static_assert(std::is_same_v<rg::range_reference_t<ZT>, int>);
static_assert(std::is_same_v<rg::iterator_t<ZT>::iterator_category, std::input_iterator_tag>);
static_assert(std::is_same_v<rg::iterator_t<ZT>::iterator_concept, std::random_access_iterator_tag>);

inline constexpr auto first = [](int& x, int&) -> int& { return x; };
using ZR = decltype(vw::zip_transform(first, std::declval<int (&)[3]>(), std::declval<int (&)[2]>()));
static_assert(std::is_same_v<rg::range_reference_t<ZR>, int&>);
static_assert(std::is_same_v<rg::iterator_t<ZR>::iterator_category, std::random_access_iterator_tag>);

using BidiC = ArchetypeView<BidiIter<int>>;
using ZB = rg::zip_transform_view<decltype(first), BidiC, rg::ref_view<int[2]>>;
static_assert(std::is_same_v<rg::iterator_t<ZB>::iterator_category, std::bidirectional_iterator_tag>);

using InV = ArchetypeView<InputIter<int>, PtrSentinel<int>>;
using ZI = rg::zip_transform_view<std::plus<>, InV, rg::ref_view<int[2]>>;
static_assert(rg::input_range<ZI> && !has_iterator_category<rg::iterator_t<ZI>>);

constexpr bool test() {
  int a[3] = {1, 2, 3};
  int b[2] = {10, 20};
  auto z = vw::zip_transform(std::plus<>(), a, b);
  CHECK(range_equals(z, {11, 22}) && z.size() == 2u && z[1] == 22);
  auto it = z.begin();
  it += 1;
  CHECK(*it == 22 && it - z.begin() == 1);
  auto r = vw::zip_transform(first, a, b);
  r[0] = 7;
  CHECK(a[0] == 7);
  auto t = vw::zip_transform([](int x) { return x * 2; }, vw::iota(1)) | vw::take(3);
  CHECK(range_equals(t, {2, 4, 6}));
  CHECK(vw::zip_transform(zero).empty());
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
