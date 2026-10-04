// [range.counted]: views::counted(E, F) is a span for contiguous iterators (/2.1), a
// subrange(E, E + F) for random-access iterators (/2.2), and
// subrange(counted_iterator(E, F), default_sentinel) otherwise (/2.3); it is ill-formed when
// F is not convertible to the difference type.
#include <cstddef>
#include <iterator>
#include <ranges>
#include <span>
#include <type_traits>
#include "check.hpp"
#include "range_support.hpp"
#include "test_iterators.hpp"

namespace rg = std::ranges;
namespace vw = std::views;

static_assert(std::is_same_v<decltype(vw::counted(std::declval<int*>(), 2)), std::span<int>>);
static_assert(std::is_same_v<decltype(vw::counted(std::declval<const int*>(), 2)), std::span<const int>>);
static_assert(std::is_same_v<decltype(vw::counted(std::declval<RandomIter<int>>(), 2)), rg::subrange<RandomIter<int>>>);
static_assert(std::is_same_v<decltype(vw::counted(std::declval<BidiIter<int>>(), 2)),
                             rg::subrange<std::counted_iterator<BidiIter<int>>, std::default_sentinel_t>>);
static_assert(std::is_same_v<decltype(vw::counted(std::declval<InputIter<int>>(), 2)),
                             rg::subrange<std::counted_iterator<InputIter<int>>, std::default_sentinel_t>>);

template <class I, class N>
concept countable = requires(I i, N n) { vw::counted(i, n); };
static_assert(countable<int*, int> && countable<int*, long> && !countable<int*, int*>);

constexpr bool test() {
  int a[5] = {1, 2, 3, 4, 5};
  auto s = vw::counted(a + 1, 3);
  CHECK(s.data() == a + 1 && s.size() == 3);
  auto r = vw::counted(RandomIter<int>(a), 2);
  CHECK(range_equals(r, {1, 2}) && r.size() == 2u);
  auto b = vw::counted(BidiIter<int>(a + 2), 3);
  CHECK(range_equals(b, {3, 4, 5}) && b.size() == 3u);
  auto z = vw::counted(BidiIter<int>(a), 0);
  CHECK(z.empty());
  return true;
}

int main() {
  static_assert(test());
  CHECK(test());
}
