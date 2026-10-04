// [valarray.sub]: operator[](const valarray<bool>&) const returns a valarray of the elements
// whose mask entry is true (in order); the non-const overload returns a mask_array referring
// to them. [template.mask.array]: mask_array has the valarray assignment and compound
// assignment operators (the i-th selected element is combined with element i of the argument),
// a fill assignment, copy construction and no default constructor.
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

static_assert(!std::is_default_constructible_v<std::mask_array<int>>);
static_assert(std::is_same_v<std::mask_array<int>::value_type, int>);

int main() {
  std::valarray<int> v{1, 2, 3, 4, 5, 6};
  const std::valarray<bool> mask{true, false, true, false, false, true};
  const std::valarray<int>& cv = v;
  std::valarray<int> sel = cv[mask];
  CHECK(eq(sel, {1, 3, 6}));
  static_assert(std::is_same_v<decltype(v[mask]), std::mask_array<int>>);

  v[mask] = std::valarray<int>{10, 30, 60};
  CHECK(eq(v, {10, 2, 30, 4, 5, 60}));
  v[mask] += std::valarray<int>{1, 2, 3};
  CHECK(eq(v, {11, 2, 32, 4, 5, 63}));
  v[std::valarray<bool>(v > 10)] = 0;  // a computed mask
  CHECK(eq(v, {0, 2, 0, 4, 5, 0}));
  v[mask] -= std::valarray<int>{1, 1, 1};
  v[mask] *= std::valarray<int>{2, 3, 4};
  CHECK(eq(v, {-2, 2, -3, 4, 5, -4}));
  std::valarray<bool> even = (v % 2) == 0;
  v[even] /= std::valarray<int>{-2, 2, 2, -4};
  CHECK(eq(v, {1, 1, -3, 2, 5, 1}));
  std::valarray<int> copy(v[mask]);
  CHECK(eq(copy, {1, -3, 1}));
  std::valarray<int> asg(3);
  asg = v[mask];
  CHECK(eq(asg, {1, -3, 1}));
  std::valarray<bool> none(false, 6);
  CHECK(cv[none].size() == 0);
  return 0;
}
