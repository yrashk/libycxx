// [mdspan.copy]: copy(src, dst) assigns each element of src to the corresponding element of
// dst (same multidimensional index; layouts may differ); fill(dst, value) assigns value to
// every element. Constraints: mdspan specializations with assignable references and
// compatible extents. Both are constexpr.
#include <mdspan>
#include <type_traits>
#include "check.hpp"

using std::dynamic_extent;

template <class S, class D>
concept can_copy = requires(const S& s, const D& d) { std::copy(s, d); };
using MR = std::mdspan<int, std::extents<int, 2, 3>>;
using ML = std::mdspan<int, std::dextents<int, 2>, std::layout_left>;
using MC = std::mdspan<const int, std::extents<int, 2, 3>>;
static_assert(can_copy<MR, ML> && can_copy<MC, ML>);
static_assert(!can_copy<MR, MC>);                                            // const destination
static_assert(!can_copy<MR, std::mdspan<int, std::extents<int, 3, 2>>>);    // incompatible extents
static_assert(!can_copy<MR, std::mdspan<int, std::extents<int, 6>>>);       // rank

constexpr bool run() {
  int a[6] = {1, 2, 3, 4, 5, 6}, b[6] = {};
  MR src(a);
  ML dst(b, 2, 3);
  std::copy(src, dst);
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 3; ++j)
      if (dst[i, j] != src[i, j]) return false;
  if (b[0] != 1 || b[1] != 4 || b[2] != 2 || b[5] != 6) return false;  // column-major storage
  std::fill(dst, 9);
  for (int x : b)
    if (x != 9) return false;
  std::fill(std::submdspan(src, 1, std::full_extent), 0);
  if (a[2] != 3 || a[3] != 0 || a[5] != 0) return false;
  double d[4] = {};
  std::fill(std::mdspan<double, std::extents<int, 4>>(d), 1);  // T = value_type by default
  if (d[3] != 1.0) return false;
  return true;
}
static_assert(run());

int main() {
  CHECK(run());
  return 0;
}
