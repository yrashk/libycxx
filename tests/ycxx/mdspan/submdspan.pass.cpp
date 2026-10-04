// [mdspan.sub.sub], [mdspan.sub.overview], [mdspan.sub.extents]: submdspan(src, slices...)
// with integral slices (collapsing a dimension), full_extent (keeping it, with its static
// extent) and pair-like slices {first, last} (keeping [first, last) as a dynamic extent). Each
// element of the result is the element of src at the lower bounds of the slices plus the
// result's index ([mdspan.sub.map.sliceable]/5). For layout_right, full_extent slices after
// the leading unit-stride slice keep layout_right ([mdspan.sub.map.right]).
#include <mdspan>
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"

using std::dynamic_extent;
using std::full_extent;

constexpr bool run() {
  int data[24];
  for (int i = 0; i < 24; ++i) data[i] = i;
  std::mdspan<int, std::extents<int, 2, 3, 4>> m(data);  // row-major 2 x 3 x 4

  // Collapse the first dimension.
  auto s0 = std::submdspan(m, 1, full_extent, full_extent);
  static_assert(decltype(s0)::rank() == 2);
  static_assert(std::is_same_v<decltype(s0)::extents_type, std::extents<int, 3, 4>>);
  static_assert(std::is_same_v<decltype(s0)::layout_type, std::layout_right>);
  if (s0[0, 0] != 12 || s0[2, 3] != 23 || &s0[0, 0] != &data[12]) return false;
  if (&s0[1, 2] != &m[1, 1, 2]) return false;

  // Collapse the last dimension: a strided view.
  auto s2 = std::submdspan(m, full_extent, full_extent, 3);
  static_assert(std::is_same_v<decltype(s2)::extents_type, std::extents<int, 2, 3>>);
  if (s2[1, 2] != m[1, 2, 3] || s2[0, 1] != 7 || s2.stride(0) != 12 || s2.stride(1) != 4) return false;

  // A pair slice keeps [first, last).
  auto sp = std::submdspan(m, full_extent, std::pair{1, 3}, 2);
  static_assert(std::is_same_v<decltype(sp)::extents_type, std::extents<int, 2, dynamic_extent>>);
  if (sp.extent(1) != 2 || sp[0, 0] != m[0, 1, 2] || sp[1, 1] != m[1, 2, 2]) return false;
  auto st = std::submdspan(m, 0, std::tuple{0, 2}, std::pair{1, 4});
  if (st.extent(0) != 2 || st.extent(1) != 3 || st[1, 2] != m[0, 1, 3]) return false;

  // All collapsed: rank 0.
  auto s3 = std::submdspan(m, 1, 2, 3);
  static_assert(decltype(s3)::rank() == 0);
  if (s3[] != 23) return false;

  // Unchanged.
  auto all = std::submdspan(m, full_extent, full_extent, full_extent);
  if (&all[1, 2, 3] != &m[1, 2, 3] || !(all.extents() == m.extents())) return false;

  // layout_left source.
  std::mdspan<int, std::dextents<int, 2>, std::layout_left> l(data, 4, 6);
  auto lc = std::submdspan(l, full_extent, 2);  // a column: contiguous in layout_left
  static_assert(std::is_same_v<decltype(lc)::layout_type, std::layout_left>);
  if (lc.extent(0) != 4 || lc[3] != l[3, 2] || &lc[0] != &data[8]) return false;
  auto lr = std::submdspan(l, 1, full_extent);  // a row: stride 4
  if (lr.extent(0) != 6 || lr[5] != l[1, 5] || lr.stride(0) != 4) return false;

  // Modification through a submdspan.
  std::submdspan(m, 0, 0, full_extent)[1] = -1;
  if (data[1] != -1) return false;
  return true;
}
static_assert(run());

int main() {
  CHECK(run());
  return 0;
}
