// submdspan with degenerate slices: empty slices (also at the end of an extent), extent-1
// slices with a stride, zero-sized source extents. [mdspan.sub.map.common]/6: sub_strides is
// stride(k) * s.stride "if the type of s is a specialization of extent_slice and
// s.extent > 1 is true", otherwise stride(k); /8: if a lower bound ls...[k] equals
// extents().extent(k) for any k, the offset is required_span_size(), otherwise
// operator()(ls...). [mdspan.sub.overview]/7: the slice range of an extent_slice with extent
// 0 is [offset, offset); /9.3.2: the stride only has to be positive when the extent is at
// least two (so extent_slice{0, 0, 0} and {1, 1, 0} are valid). [mdspan.sub.helpers]/7:
// canonical-range-slice uses stride 1 when the span is 0 and extent 1 + (span - 1) / stride
// otherwise, a constant_wrapper when the span and stride types are; range_slice's default
// StrideType is constant_wrapper<1zu> ([mdspan.sub.range.slices]), so a range_slice without
// stride is a unit-stride slice ([mdspan.sub.overview]/6). [mdspan.sub.map.left]/1 and
// [mdspan.sub.map.right]: layout_left/right results (1.3), padded results (1.4), otherwise
// layout_stride (1.5); [mdspan.sub.map.stride]: layout_stride. [mdspan.sub.extents]/5-6:
// static extents of the result.
#include <array>
#include <cstddef>
#include <mdspan>
#include <type_traits>
#include <utility>
#include "check.hpp"

using std::cw;
using std::extent_slice;
using std::full_extent;
using std::range_slice;

int d[64];

template <class M>
bool strides_are(const M& m, std::size_t s0, std::size_t s1) {
  return static_cast<std::size_t>(m.stride(0)) == s0 && static_cast<std::size_t>(m.stride(1)) == s1;
}

int main() {
  std::mdspan<int, std::dextents<int, 2>, std::layout_left> L(d, 4, 5);    // strides 1, 4
  std::mdspan<int, std::dextents<int, 2>> R(d, 4, 5);                      // strides 5, 1
  std::mdspan<int, std::dextents<int, 2>, std::layout_stride> S(
      d, std::layout_stride::mapping(std::dextents<int, 2>(4, 5), std::array<int, 2>{7, 1}));

  // Extent-1 and extent-0 extent_slices keep the source stride.
  auto a = std::submdspan(L, extent_slice{1, 2, 2}, extent_slice{0, 1, 3});
  static_assert(std::is_same_v<decltype(a)::layout_type, std::layout_stride>);
  CHECK(a.extent(0) == 2 && a.extent(1) == 1 && strides_are(a, 2, 4) && a.data_handle() == d + 1);
  auto b = std::submdspan(S, extent_slice{1, 1, 5}, full_extent);
  CHECK(b.extent(0) == 1 && strides_are(b, 7, 1) && b.data_handle() == d + 7);
  auto c = std::submdspan(S, extent_slice{0, 0, 0}, full_extent);
  CHECK(c.extent(0) == 0 && c.extent(1) == 5 && strides_are(c, 7, 1) && c.data_handle() == d);
  auto c2 = std::submdspan(S, extent_slice{2, 1, 0}, full_extent);
  CHECK(c2.extent(0) == 1 && strides_are(c2, 7, 1) && c2.data_handle() == d + 14 && &c2[0, 4] == d + 18);

  // A lower bound equal to the extent: the offset is required_span_size() (20).
  auto e = std::submdspan(R, extent_slice{4, 0, 1}, full_extent);
  CHECK(e.extent(0) == 0 && e.extent(1) == 5 && e.data_handle() == d + 20);
  auto f = std::submdspan(R, full_extent, extent_slice{5, 0, 1});
  CHECK(f.extent(0) == 4 && f.extent(1) == 0 && f.data_handle() == d + 20);
  auto g = std::submdspan(R, 3, extent_slice{5, 0, 1});
  static_assert(decltype(g)::rank() == 1);
  CHECK(g.extent(0) == 0 && g.data_handle() == d + 20);
  // An empty slice inside the extent: the offset is mapping(ls...).
  auto h = std::submdspan(R, range_slice{3, 3}, full_extent);
  static_assert(std::is_same_v<decltype(h)::layout_type, std::layout_right>);
  CHECK(h.extent(0) == 0 && h.extent(1) == 5 && h.data_handle() == d + 15);
  // A zero-sized source extent: every lower bound 0 equals that extent.
  std::mdspan<int, std::dextents<int, 2>> Z(d + 3, 0, 5);
  auto z1 = std::submdspan(Z, full_extent, extent_slice{2, 2, 1});
  CHECK(z1.extent(0) == 0 && z1.extent(1) == 2 && z1.data_handle() == d + 3);
  CHECK(z1.mapping().required_span_size() == 0);

  // range_slice: extent 1 + (span - 1) / stride; strides multiplied when the extent is > 1.
  auto r = std::submdspan(R, range_slice{0, 4, 3}, range_slice{1, 5, 2});
  CHECK(r.extent(0) == 2 && r.extent(1) == 2 && strides_are(r, 15, 2) && r.data_handle() == d + 1);
  CHECK(&r[1, 1] == d + 3 * 5 + 3);
  auto r1 = std::submdspan(R, range_slice{0, 1, 3}, full_extent);  // extent 1: stride kept
  CHECK(r1.extent(0) == 1 && strides_are(r1, 5, 1));

  // Static extents from constant_wrapper slices, including an empty constant range.
  std::mdspan<int, std::extents<int, 4, 5>> RS(d);
  auto s = std::submdspan(RS, extent_slice{cw<1>, cw<2>, cw<2>}, range_slice{cw<0>, cw<5>, cw<2>});
  static_assert(decltype(s)::static_extent(0) == 2 && decltype(s)::static_extent(1) == 3);
  CHECK(strides_are(s, 10, 2) && s.data_handle() == d + 5);
  auto t = std::submdspan(RS, range_slice{cw<2>, cw<2>, 5}, full_extent);
  static_assert(decltype(t)::static_extent(0) == 0);
  CHECK(t.extent(0) == 0);

  // layout_left: unit-stride leading slices give layout_left or layout_left_padded.
  auto l1 = std::submdspan(L, full_extent, 1);
  static_assert(std::is_same_v<decltype(l1)::layout_type, std::layout_left>);
  CHECK(l1.data_handle() == d + 4);
  auto l2 = std::submdspan(L, 1, full_extent);
  static_assert(std::is_same_v<decltype(l2)::layout_type, std::layout_stride>);
  CHECK(l2.stride(0) == 4 && l2.data_handle() == d + 1);
  auto l3 = std::submdspan(L, extent_slice{1, 2, cw<1>}, full_extent);
  static_assert(std::is_same_v<decltype(l3)::layout_type, std::layout_left_padded<std::dynamic_extent>>);
  CHECK(strides_are(l3, 1, 4) && l3.data_handle() == d + 1);
  std::mdspan<int, std::dextents<int, 3>, std::layout_left> L3(d, 2, 3, 4);
  auto l4 = std::submdspan(L3, extent_slice{0, 1, cw<1>}, full_extent, full_extent);
  static_assert(std::is_same_v<decltype(l4)::layout_type, std::layout_left_padded<std::dynamic_extent>>);
  CHECK(l4.extent(0) == 1 && l4.stride(1) == 2 && l4.stride(2) == 6);
  auto l5 = std::submdspan(L3, 1, 1, 1);
  static_assert(decltype(l5)::rank() == 0);
  CHECK(&l5[] == d + 1 + 2 + 6);
  return 0;
}
