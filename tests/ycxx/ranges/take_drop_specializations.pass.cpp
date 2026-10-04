// [range.take.overview]/2 and [range.drop.overview]/2: views::take and views::drop return
// the same kind of view for empty_view, span, basic_string_view, subrange, iota_view and
// repeat_view, and take_view/drop_view otherwise.
#include <array>
#include <cstddef>
#include <ranges>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

template <class R>
using take_t = decltype(std::declval<R>() | vw::take(1));
template <class R>
using drop_t = decltype(std::declval<R>() | vw::drop(1));

// /2.1
static_assert(std::is_same_v<take_t<rg::empty_view<int>>, rg::empty_view<int>>);
static_assert(std::is_same_v<drop_t<rg::empty_view<int>>, rg::empty_view<int>>);
// /2.3 span: span<element_type> (dynamic extent)
static_assert(std::is_same_v<take_t<std::span<int, 5>>, std::span<int>>);
static_assert(std::is_same_v<drop_t<std::span<const int, 5>>, std::span<const int>>);
// basic_string_view
static_assert(std::is_same_v<take_t<std::string_view>, std::string_view>);
static_assert(std::is_same_v<drop_t<std::wstring_view>, std::wstring_view>);
// subrange: take gives subrange<iterator_t<T>>; drop gives T
using SR = rg::subrange<int*>;
static_assert(std::is_same_v<take_t<SR>, SR>);
static_assert(std::is_same_v<drop_t<SR>, SR>);
// A sized subrange of a non-sized-sentinel random-access iterator (StoreSize is true).
using SRS = rg::subrange<RandomIter<int>, PtrSentinel<int>, rg::subrange_kind::sized>;
static_assert(std::is_same_v<drop_t<SRS>, SRS>); // /2.4
static_assert(std::is_same_v<take_t<SRS>, rg::subrange<RandomIter<int>>>);
// iota_view
static_assert(std::is_same_v<take_t<rg::iota_view<int, int>>, rg::iota_view<int, int>>);
static_assert(std::is_same_v<drop_t<rg::iota_view<int, int>>, rg::iota_view<int, int>>);
static_assert(std::is_same_v<take_t<rg::iota_view<int>>, rg::take_view<rg::iota_view<int>>>);
static_assert(std::is_same_v<drop_t<rg::iota_view<int>>, rg::drop_view<rg::iota_view<int>>>);
// repeat_view
static_assert(std::is_same_v<take_t<rg::repeat_view<int, int>>, rg::repeat_view<int, int>>);
static_assert(std::is_same_v<drop_t<rg::repeat_view<int, int>>, rg::repeat_view<int, int>>);
static_assert(std::is_same_v<take_t<rg::repeat_view<int>>, rg::repeat_view<int, std::ptrdiff_t>>);
static_assert(std::is_same_v<drop_t<rg::repeat_view<int>>, rg::repeat_view<int>>);
// otherwise
using A = std::array<int, 4>;
static_assert(std::is_same_v<take_t<A&>, rg::take_view<rg::ref_view<A>>>);
static_assert(std::is_same_v<drop_t<A>, rg::drop_view<rg::owning_view<A>>>);
static_assert(std::is_same_v<take_t<rg::single_view<int>>, rg::take_view<rg::single_view<int>>>);

constexpr bool test() {
  int a[5] = {1, 2, 3, 4, 5};
  std::span<int, 5> s(a);
  auto ts = s | vw::take(3);
  CHECK(ts.data() == a && ts.size() == 3u);
  auto ds = s | vw::drop(7);
  CHECK(ds.empty() && ds.data() == a + 5);
  std::string_view sv = "hello";
  CHECK((sv | vw::take(2)) == "he" && (sv | vw::drop(3)) == "lo" && (sv | vw::take(99)) == "hello");
  auto it = vw::iota(10, 20) | vw::take(3);
  CHECK(range_equals(it, {10, 11, 12}));
  auto id = vw::iota(10, 20) | vw::drop(8);
  CHECK(range_equals(id, {18, 19}));
  auto id2 = vw::iota(10, 20) | vw::drop(30);
  CHECK(id2.empty());
  auto rt = vw::repeat(7, 5) | vw::take(2);
  CHECK(rt.size() == 2u);
  auto rd = vw::repeat(7, 5) | vw::drop(2);
  CHECK(rd.size() == 3u && rd[0] == 7);
  auto rd2 = vw::repeat(7, 5) | vw::drop(9);
  CHECK(rd2.size() == 0u);
  auto ru = vw::repeat(7) | vw::take(4);
  CHECK(ru.size() == 4u);
  SR sr(a, a + 5);
  auto st = sr | vw::take(2);
  CHECK(st.begin() == a && st.end() == a + 2);
  auto sd = sr | vw::drop(2);
  CHECK(sd.begin() == a + 2 && sd.end() == a + 5);
  SRS srs(RandomIter<int>(a), PtrSentinel<int>{a + 5}, 5);
  auto srd = srs | vw::drop(1);
  CHECK(srd.size() == 4u && *srd.begin() == 2);
  auto srt = srs | vw::take(3);
  CHECK(srt.size() == 3u && srt.end().p == a + 3);
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
