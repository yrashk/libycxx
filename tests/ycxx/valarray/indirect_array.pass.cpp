// [valarray.sub]: operator[](const valarray<size_t>&) const returns a valarray of the elements
// at the given indices, in the given order; the non-const overload returns an indirect_array.
// [template.indirect.array], [indirect.array.assign]: assignment and compound assignment
// combine the i-th indexed element with element i of the argument; fill assignment; no default
// constructor.
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

static_assert(!std::is_default_constructible_v<std::indirect_array<int>>);
static_assert(std::is_same_v<std::indirect_array<double>::value_type, double>);

int main() {
  std::valarray<int> v{10, 11, 12, 13, 14};
  const std::valarray<std::size_t> idx{4, 0, 2};
  const std::valarray<int>& cv = v;
  std::valarray<int> sel = cv[idx];
  CHECK(eq(sel, {14, 10, 12}));
  const std::valarray<std::size_t> rep{1, 1, 3};
  CHECK(eq(std::valarray<int>(cv[rep]), {11, 11, 13}));  // repeated indices read fine
  static_assert(std::is_same_v<decltype(v[idx]), std::indirect_array<int>>);

  v[idx] = std::valarray<int>{4, 0, 2};
  CHECK(eq(v, {0, 11, 2, 13, 4}));
  v[idx] += std::valarray<int>{1, 2, 3};
  CHECK(eq(v, {2, 11, 5, 13, 5}));
  v[idx] = 7;
  CHECK(eq(v, {7, 11, 7, 13, 7}));
  v[idx] *= std::valarray<int>{2, 3, 4};  // v[4] *= 2, v[0] *= 3, v[2] *= 4
  CHECK(eq(v, {21, 11, 28, 13, 14}));
  v[idx] %= std::valarray<int>{5, 4, 9};
  CHECK(eq(v, {1, 11, 1, 13, 4}));
  v[idx] <<= std::valarray<int>{1, 2, 3};
  CHECK(eq(v, {4, 11, 8, 13, 8}));
  std::valarray<int> copy(v[idx]);
  CHECK(eq(copy, {8, 4, 8}));
  std::valarray<int> asg(3);
  asg = v[idx];
  CHECK(eq(asg, {8, 4, 8}));
  return 0;
}
