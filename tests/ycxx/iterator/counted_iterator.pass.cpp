// [counted.iterator] and [counted.iter.*]: counted_iterator tracks the distance to the end
// of its range; it compares equal to default_sentinel when count() == 0; differences and
// <=> are computed from the counts.
#include <compare>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include "check.hpp"

using CI = std::counted_iterator<int*>;
using CCI = std::counted_iterator<const int*>;

static_assert(std::is_same_v<CI::iterator_type, int*>);
static_assert(std::is_same_v<CI::value_type, int>);
static_assert(std::is_same_v<CI::difference_type, std::ptrdiff_t>);
static_assert(std::is_same_v<std::iterator_traits<CI>::pointer, int*>);
static_assert(std::is_same_v<std::iterator_traits<CI>::iterator_category, std::random_access_iterator_tag>);
static_assert(std::contiguous_iterator<CI>);
static_assert(std::sized_sentinel_for<std::default_sentinel_t, CI>);
static_assert(std::is_same_v<decltype(std::declval<CI&>() <=> std::declval<CCI&>()), std::strong_ordering>);
static_assert(std::is_same_v<decltype(std::declval<const CI&>().count()), std::ptrdiff_t>);
static_assert(noexcept(std::declval<const CI&>().count()));
static_assert(noexcept(std::declval<const CI&>().base()));
static_assert(noexcept(std::declval<const CI&>() == std::default_sentinel));
static_assert(noexcept(std::declval<const CI&>() - std::default_sentinel));
static_assert(std::is_convertible_v<CI, CCI> && !std::is_convertible_v<CCI, CI>);
static_assert(std::is_same_v<decltype(std::declval<CI&>().operator->()), int*>);

constexpr bool test() {
  int a[6] = {0, 1, 2, 3, 4, 5};
  CI it(a + 1, 4);                           // elements 1..4
  if (it.count() != 4 || it.base() != a + 1 || *it != 1) return false;
  if (it == std::default_sentinel || std::default_sentinel == it) return false;
  if (std::default_sentinel - it != 4 || it - std::default_sentinel != -4) return false;
  int sum = 0;
  for (CI i = it; i != std::default_sentinel; ++i) sum += *i;
  if (sum != 1 + 2 + 3 + 4) return false;
  CI end = it + 4;
  if (end.count() != 0 || !(end == std::default_sentinel) || end.base() != a + 5) return false;
  if (end - it != 4 || it - end != -4) return false;
  if (!(it < end) || (it <=> end) != std::strong_ordering::less) return false;
  if ((end <=> it) != std::strong_ordering::greater || (it <=> it) != 0) return false;
  // Arithmetic.
  CI j = it;
  j += 2;
  if (j.count() != 2 || *j != 3) return false;
  j -= 1;
  if (j.count() != 3 || *j != 2) return false;
  CI k = 2 + it;
  if (k.count() != 2 || *(k - 1) != 2 || (k - 1).count() != 3) return false;
  if (it[3] != 4) return false;
  CI post = j++;
  if (post.count() != 3 || j.count() != 2) return false;
  --j;
  j--;
  if (j.count() != 4 || j.base() != a + 1) return false;
  // Conversion and heterogeneous comparison with counted_iterator<const int*>.
  CCI c = it;
  if (c.count() != 4 || c.base() != a + 1) return false;
  if (!(c == it) || (it <=> CCI(end)) != std::strong_ordering::less) return false;
  if (CCI(end) - it != 4) return false;
  CCI assigned(a, 0);
  assigned = end;
  if (assigned.count() != 0) return false;
  // Customizations.
  CI x(a, 2), y(a + 4, 2);
  std::ranges::iter_swap(x, y);
  if (a[0] != 4 || a[4] != 0) return false;
  if (std::ranges::iter_move(x) != 4) return false;
  static_assert(std::is_same_v<decltype(std::ranges::iter_move(x)), int&&>);
  // Zero-length counted range; default construction.
  CI z(a, 0);
  if (!(z == std::default_sentinel)) return false;
  CI dflt;
  if (dflt.count() != 0 || dflt.base() != nullptr) return false;
  // Rvalue base().
  if (std::move(it).base() != a + 1) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
