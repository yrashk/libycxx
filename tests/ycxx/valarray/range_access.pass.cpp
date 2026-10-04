// [valarray.range]: valarray has iterator and const_iterator types (mutable / constant
// random access iterators modelling contiguous_iterator, value_type T, reference T& and
// const T&) and begin()/end() for const and non-const arrays, referring to the first element
// and one past the last. So valarray is a contiguous range usable with range-for and the
// standard algorithms.
#include <valarray>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <ranges>
#include <type_traits>
#include "check.hpp"

using V = std::valarray<int>;
static_assert(std::contiguous_iterator<V::iterator>);
static_assert(std::contiguous_iterator<V::const_iterator>);
static_assert(std::is_same_v<std::iter_value_t<V::iterator>, int>);
static_assert(std::is_same_v<std::iter_reference_t<V::iterator>, int&>);
static_assert(std::is_same_v<std::iter_reference_t<V::const_iterator>, const int&>);
static_assert(std::is_same_v<decltype(std::declval<V&>().begin()), V::iterator>);
static_assert(std::is_same_v<decltype(std::declval<const V&>().end()), V::const_iterator>);
static_assert(std::ranges::contiguous_range<V> && std::ranges::contiguous_range<const V>);
static_assert(std::ranges::sized_range<V>);

int main() {
  V v{3, 1, 2};
  CHECK(v.end() - v.begin() == 3);
  CHECK(&*v.begin() == &v[0] && std::to_address(v.end()) == &v[0] + 3);
  std::sort(v.begin(), v.end());
  CHECK(v[0] == 1 && v[1] == 2 && v[2] == 3);
  int sum = 0;
  for (int& x : v) sum += x++;
  CHECK(sum == 6 && v[2] == 4);
  const V& cv = v;
  CHECK(std::accumulate(cv.begin(), cv.end(), 0) == 9);
  CHECK(std::begin(v) == v.begin() && std::end(cv) == cv.end());
  CHECK(std::ranges::size(v) == 3 && std::ranges::data(v) == &v[0]);
  V empty;
  CHECK(empty.begin() == empty.end());
  return 0;
}
