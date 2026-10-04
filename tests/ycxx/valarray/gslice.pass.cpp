// [class.gslice]: gslice(start, lengths, strides) maps multidimensional indices to
// k = start + sum(i_j * stride_j), the highest-ordered index turning fastest (Example 1:
// start 3, lengths {2, 4, 3}, strides {19, 4, 1} gives 3, 4, 5, 7, 8, 9, 11, ..., 17, 22, 23,
// ..., 36); degenerate gslices may repeat an index. [template.gslice.array]: gslice_array has
// the valarray assignment/compound assignment operators and a fill assignment;
// [valarray.sub]: const operator[](const gslice&) returns a valarray.
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

static_assert(!std::is_default_constructible_v<std::gslice_array<int>>);

int main() {
  std::gslice g0;
  CHECK(g0.start() == 0 && g0.size().size() == 0 && g0.stride().size() == 0);

  const std::size_t len[] = {2, 4, 3}, str[] = {19, 4, 1};
  std::gslice g(3, std::valarray<std::size_t>(len, 3), std::valarray<std::size_t>(str, 3));
  CHECK(g.start() == 3);
  CHECK(eq(g.size(), {std::size_t(2), std::size_t(4), std::size_t(3)}));
  CHECK(eq(g.stride(), {std::size_t(19), std::size_t(4), std::size_t(1)}));

  std::valarray<int> idx(40);
  for (int i = 0; i < 40; ++i) idx[i] = i;
  const std::valarray<int>& cidx = idx;
  std::valarray<int> sel = cidx[g];
  CHECK(sel.size() == 24);
  CHECK(eq(sel, {3, 4, 5, 7, 8, 9, 11, 12, 13, 15, 16, 17, 22, 23, 24, 26, 27, 28, 30, 31, 32, 34, 35, 36}));

  // Degenerate: strides {1, 1, 1} repeat indices (read-only use).
  const std::size_t ones[] = {1, 1, 1};
  std::valarray<int> deg = cidx[std::gslice(3, std::valarray<std::size_t>(len, 3), std::valarray<std::size_t>(ones, 3))];
  CHECK(deg.size() == 24 && deg[0] == 3 && deg[1] == 4 && deg[2] == 5 && deg[3] == 4 && deg[4] == 5);

  // gslice_array: a 3x3 matrix stored row-major; select column 1 and the 2x2 lower-right block.
  std::valarray<int> m{1, 2, 3, 4, 5, 6, 7, 8, 9};
  const std::size_t l1[] = {3}, s1[] = {3};
  m[std::gslice(1, std::valarray<std::size_t>(l1, 1), std::valarray<std::size_t>(s1, 1))] = 0;
  CHECK(eq(m, {1, 0, 3, 4, 0, 6, 7, 0, 9}));
  const std::size_t l2[] = {2, 2}, s2[] = {3, 1};
  std::gslice block(4, std::valarray<std::size_t>(l2, 2), std::valarray<std::size_t>(s2, 2));
  m[block] = std::valarray<int>{50, 60, 80, 90};
  CHECK(eq(m, {1, 0, 3, 4, 50, 60, 7, 80, 90}));
  m[block] += std::valarray<int>{1, 2, 3, 4};
  CHECK(eq(m, {1, 0, 3, 4, 51, 62, 7, 83, 94}));
  m[block] -= std::valarray<int>{1, 2, 3, 4};
  m[block] *= std::valarray<int>{2, 2, 2, 2};
  m[block] /= std::valarray<int>{10, 10, 10, 10};
  CHECK(eq(m, {1, 0, 3, 4, 10, 12, 7, 16, 18}));
  m[block] %= std::valarray<int>{4, 5, 6, 7};
  CHECK(eq(m, {1, 0, 3, 4, 2, 2, 7, 4, 4}));
  std::valarray<int> copy(m[block]);
  CHECK(eq(copy, {2, 2, 4, 4}));
  std::valarray<int> asg(4);
  asg = m[block];
  CHECK(eq(asg, {2, 2, 4, 4}));
  return 0;
}
