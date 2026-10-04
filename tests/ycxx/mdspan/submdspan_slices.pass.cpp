// [mdspan.sub.range.slices], [mdspan.sub.canonical], [mdspan.sub.extents]: extent_slice
// {offset, extent, stride} selects `extent` indices offset, offset + stride, ...; range_slice
// {first, last, stride} selects the indices of [first, last) spaced by stride (Note 1:
// extent_slice{1, 4, 3} and range_slice{1, 11, 3} both give 1, 4, 7, 10). subextents gives the
// extents of the result, with a static extent for an extent_slice whose extent is a
// constant_wrapper; canonical_slices returns a tuple of canonical slices (full_extent_t, the
// index type or a constant_wrapper, extent_slice). The result's stride for an extent_slice
// with extent > 1 is stride(k) * s.stride ([mdspan.sub.map.common]/6).
#include <mdspan>
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"

using std::dynamic_extent;
using std::full_extent;

static_assert(std::is_aggregate_v<std::extent_slice<int, int, int>>);
static_assert(std::is_aggregate_v<std::range_slice<int, int>>);
static_assert(std::is_same_v<decltype(std::range_slice<int, int>{}.stride), std::constant_wrapper<1zu>>);
static_assert(std::is_same_v<std::submdspan_mapping_result<std::layout_right::mapping<std::extents<int, 2>>>,
                             decltype(std::submdspan_mapping_result<std::layout_right::mapping<std::extents<int, 2>>>{})>);

constexpr bool run() {
  int data[36];
  for (int i = 0; i < 36; ++i) data[i] = i;
  std::mdspan<int, std::extents<int, 3, 12>> m(data);  // row-major 3 x 12

  auto a = std::submdspan(m, full_extent, std::extent_slice{1, 4, 3});
  if (a.extent(0) != 3 || a.extent(1) != 4) return false;
  if (a[0, 0] != 1 || a[0, 1] != 4 || a[0, 2] != 7 || a[0, 3] != 10 || a[2, 3] != m[2, 10]) return false;
  if (a.stride(1) != 3 || a.stride(0) != 12) return false;

  auto b = std::submdspan(m, 1, std::range_slice{1, 11, 3});
  if (b.extent(0) != 4 || b[0] != m[1, 1] || b[3] != m[1, 10]) return false;
  auto c = std::submdspan(m, std::range_slice{0, 3}, 5);  // default stride 1
  if (c.extent(0) != 3 || c[2] != m[2, 5]) return false;
  auto d = std::submdspan(m, 2, std::range_slice{2, 7, 2});  // 2, 4, 6
  if (d.extent(0) != 3 || d[2] != m[2, 6]) return false;

  // Static extents from constant_wrapper.
  auto e = std::submdspan(m, full_extent, std::extent_slice{std::cw<2>, std::cw<3>, std::cw<2>});
  static_assert(std::is_same_v<decltype(e)::extents_type, std::extents<int, 3, 3>>);
  if (e[1, 2] != m[1, 6]) return false;
  // Zero extent.
  auto z = std::submdspan(m, full_extent, std::extent_slice{5, 0, 1});
  if (z.extent(1) != 0 || z.size() != 0) return false;

  // subextents
  auto se = std::subextents(m.extents(), 1, std::extent_slice{0, std::cw<4>, 2});
  static_assert(std::is_same_v<decltype(se), std::extents<int, 4>>);
  auto se2 = std::subextents(std::extents<int, 5, dynamic_extent>(9), full_extent, std::pair{2, 7});
  static_assert(std::is_same_v<decltype(se2), std::extents<int, 5, dynamic_extent>>);
  if (se2.extent(1) != 5) return false;

  // canonical_slices
  auto cs = std::canonical_slices(m.extents(), full_extent, std::pair{2, 5});
  static_assert(std::tuple_size_v<decltype(cs)> == 2);
  static_assert(std::is_same_v<std::tuple_element_t<0, decltype(cs)>, std::full_extent_t>);
  auto s1 = std::get<1>(cs);
  if (s1.offset != 2 || s1.extent != 3) return false;
  auto ci = std::canonical_slices(m.extents(), 2, std::cw<3>);
  static_assert(std::is_same_v<std::tuple_element_t<0, decltype(ci)>, int>);
  static_assert(std::is_same_v<std::tuple_element_t<1, decltype(ci)>, std::constant_wrapper<3>>);
  return true;
}
static_assert(run());

int main() {
  CHECK(run());
  return 0;
}
