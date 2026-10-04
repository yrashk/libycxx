// [iterators.common]: common_iterator<I, S> holds either an iterator or a sentinel; == between
// an iterator state and a sentinel state uses I == S; two iterator states compare their
// iterators when equality_comparable_with<I, I2>, otherwise (input iterators) only the
// alternatives matter; operator- for sized sentinels; [common.iter.types]: iterator_traits
// iterator_concept is forward_iterator_tag if I models forward_iterator, else
// input_iterator_tag; iterator_category is forward_iterator_tag or input_iterator_tag;
// pointer is decltype(a.operator->()) or void; incrementable_traits difference_type.
#include <iterator>
#include <cstddef>
#include <type_traits>
#include "check.hpp"

struct End {
  const int* e;
  friend constexpr bool operator==(const int* p, End s) { return p == s.e; }
  friend constexpr std::ptrdiff_t operator-(End s, const int* p) { return s.e - p; }
  friend constexpr std::ptrdiff_t operator-(const int* p, End s) { return p - s.e; }
};
using CI = std::common_iterator<const int*, End>;

static_assert(std::forward_iterator<CI>);
static_assert(std::is_same_v<std::iterator_traits<CI>::iterator_concept, std::forward_iterator_tag>);
static_assert(std::is_same_v<std::iterator_traits<CI>::iterator_category, std::forward_iterator_tag>);
static_assert(std::is_same_v<std::iterator_traits<CI>::value_type, int>);
static_assert(std::is_same_v<std::iterator_traits<CI>::reference, const int&>);
static_assert(std::is_same_v<std::iterator_traits<CI>::pointer, const int*>);
static_assert(std::is_same_v<std::iter_difference_t<CI>, std::ptrdiff_t>);
static_assert(std::sized_sentinel_for<CI, CI>);

using CC = std::common_iterator<std::counted_iterator<int*>, std::default_sentinel_t>;
static_assert(std::is_same_v<std::iterator_traits<CC>::iterator_concept, std::forward_iterator_tag>);

constexpr bool test() {
  const int a[4] = {1, 2, 3, 4};
  CI first(a), last(End{a + 4});
  int sum = 0;
  for (CI it = first; it != last; ++it) sum += *it;
  if (sum != 10) return false;
  if (last - first != 4 || first - last != -4) return false;
  CI mid = first;
  ++mid;
  if (mid == first || !(mid != first) || *mid != 2) return false;
  CI copy = mid++;
  if (*copy != 2 || *mid != 3) return false;
  if (mid.operator->() != a + 2) return false;
  CI s2(End{a + 4});
  if (!(s2 == last)) return false;  // two sentinel states compare equal
  int b[3] = {5, 6, 7};
  CC c(std::counted_iterator<int*>(b, 3)), ce(std::default_sentinel);
  int n = 0;
  for (; c != ce; ++c) n += *c;
  if (n != 18) return false;
  return true;
}
static_assert(test());

int main() {
  CHECK(test());
  return 0;
}
