// [range.as.rvalue]: views::as_rvalue is views::all when the rvalue reference type already
// equals the reference type (/2.1), otherwise as_rvalue_view, whose iterators are
// move_iterators and whose sentinel is a move_sentinel for non-common views; borrowed as its
// view.
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

static_assert(std::is_same_v<decltype(vw::iota(0, 3) | vw::as_rvalue), rg::iota_view<int, int>>);
using R1 = decltype(std::declval<int (&)[3]>() | vw::as_rvalue);
static_assert(std::is_same_v<R1, rg::as_rvalue_view<rg::ref_view<int[3]>>>);
static_assert(std::is_same_v<rg::iterator_t<R1>, std::move_iterator<int*>>);
static_assert(std::is_same_v<rg::range_reference_t<R1>, int&&>);
static_assert(rg::random_access_range<R1> && !rg::contiguous_range<R1> && rg::common_range<R1>);
static_assert(rg::sized_range<R1> && rg::borrowed_range<R1>);
static_assert(std::is_same_v<decltype(std::declval<R1>() | vw::as_rvalue), R1>); // already rvalues

using FwdNC = ArchetypeView<ForwardIter<int>, PtrSentinel<int>>;
using R2 = rg::as_rvalue_view<FwdNC>;
static_assert(std::is_same_v<rg::sentinel_t<R2>, std::move_sentinel<PtrSentinel<int>>>);
static_assert(rg::forward_range<R2> && !rg::common_range<R2>);

struct Tracker {
  int v = 0;
  bool moved_from = false;
  constexpr Tracker() = default;
  constexpr Tracker(int x) : v(x) {}
  constexpr Tracker(const Tracker& o) : v(o.v) {}
  constexpr Tracker(Tracker&& o) : v(o.v) { o.moved_from = true; }
  constexpr Tracker& operator=(const Tracker&) = default;
  constexpr Tracker& operator=(Tracker&&) = default;
};

constexpr bool test() {
  Tracker t[2] = {Tracker(1), Tracker(2)};
  auto r = t | vw::as_rvalue;
  int sum = 0;
  for (auto i = r.begin(); i != r.end(); ++i) {
    Tracker moved = *i;
    sum += moved.v;
  }
  CHECK(sum == 3 && t[0].moved_from && t[1].moved_from);
  CHECK(r.size() == 2u);
  int a[3] = {1, 2, 3};
  auto ri = a | vw::as_rvalue;
  CHECK(range_equals(ri, {1, 2, 3}) && ri.begin().base() == a);
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
