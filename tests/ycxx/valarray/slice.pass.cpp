// [class.slice]: slice() is (0, 0, 0); slice(start, size, stride) with the accessors start(),
// size(), stride() and operator== (equal when all three are equal). [valarray.sub]: the const
// operator[](slice) returns a valarray with the selected elements; the non-const one returns a
// slice_array referring to them. [template.slice.array]: slice_array has the assignment and
// compound assignment operators from valarray<T>, a fill assignment from T, a copy
// assignment that copies the referred elements, and no default constructor.
#include <valarray>
#include <initializer_list>
#include <type_traits>
#include "check.hpp"

template <class V, class T>
bool eq(const V& v, std::initializer_list<T> il) {
  if (v.size() != il.size()) return false;
  std::size_t i = 0;
  for (T x : il)
    if (v[i++] != x) return false;
  return true;
}

static_assert(!std::is_default_constructible_v<std::slice_array<int>>);
static_assert(std::is_copy_constructible_v<std::slice_array<int>>);
static_assert(std::is_same_v<std::slice_array<int>::value_type, int>);

int main() {
  std::slice s0;
  CHECK(s0.start() == 0 && s0.size() == 0 && s0.stride() == 0);
  std::slice s(1, 3, 2);
  CHECK(s.start() == 1 && s.size() == 3 && s.stride() == 2);
  CHECK(s == std::slice(1, 3, 2) && !(s == std::slice(1, 3, 3)) && s != std::slice(0, 3, 2));
  static_assert(std::is_same_v<decltype(s.start()), std::size_t>);

  std::valarray<int> v{0, 1, 2, 3, 4, 5, 6, 7};
  const std::valarray<int>& cv = v;
  std::valarray<int> sub = cv[std::slice(1, 3, 2)];
  CHECK(eq(sub, {1, 3, 5}));
  CHECK(eq(std::valarray<int>(v[std::slice(0, 4, 2)]), {0, 2, 4, 6}));
  static_assert(std::is_same_v<decltype(v[std::slice()]), std::slice_array<int>>);

  v[std::slice(0, 3, 3)] = std::valarray<int>{10, 20, 30};  // elements 0, 3, 6
  CHECK(eq(v, {10, 1, 2, 20, 4, 5, 30, 7}));
  v[std::slice(1, 2, 1)] = 9;  // fill
  CHECK(eq(v, {10, 9, 9, 20, 4, 5, 30, 7}));
  v[std::slice(0, 4, 2)] += std::valarray<int>{1, 1, 1, 1};
  CHECK(eq(v, {11, 9, 10, 20, 5, 5, 31, 7}));
  v[std::slice(4, 2, 1)] *= std::valarray<int>{3, 4};
  CHECK(eq(v, {11, 9, 10, 20, 15, 20, 31, 7}));
  v[std::slice(0, 2, 1)] -= std::valarray<int>{1, 1};
  v[std::slice(2, 2, 1)] /= std::valarray<int>{5, 4};
  v[std::slice(4, 2, 1)] %= std::valarray<int>{4, 6};
  CHECK(eq(v, {10, 8, 2, 5, 3, 2, 31, 7}));
  v[std::slice(6, 2, 1)] ^= std::valarray<int>{1, 1};
  v[std::slice(0, 2, 1)] &= std::valarray<int>{6, 12};
  v[std::slice(2, 2, 1)] |= std::valarray<int>{1, 2};
  CHECK(eq(v, {2, 8, 3, 7, 3, 2, 30, 6}));
  v[std::slice(0, 2, 1)] <<= std::valarray<int>{1, 2};
  v[std::slice(6, 2, 1)] >>= std::valarray<int>{1, 1};
  CHECK(eq(v, {4, 32, 3, 7, 3, 2, 15, 3}));

  // slice_array copy assignment copies the referred elements, not the reference.
  std::valarray<int> w{1, 2, 3, 4, 5, 6};
  std::slice_array<int> left = w[std::slice(0, 3, 1)];
  const std::slice_array<int> right = w[std::slice(3, 3, 1)];
  left = right;
  CHECK(eq(w, {4, 5, 6, 4, 5, 6}));
  // A valarray constructed from / assigned a slice_array.
  std::valarray<int> from(w[std::slice(1, 2, 3)]);
  CHECK(eq(from, {5, 5}));
  std::valarray<int> to(2);
  to = w[std::slice(0, 2, 2)];
  CHECK(eq(to, {4, 6}));
  return 0;
}
