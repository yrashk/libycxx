// [mdspan.sub.map.left]/1, [mdspan.sub.map.right]/1, [mdspan.sub.map.leftpad]/1: the layout of a
// submdspan result. Pair slices {first, last} canonicalize to extent_slice with stride cw<1>
// ([mdspan.sub.helpers]), a unit-stride slice type ([mdspan.sub.overview]/6), as is full_extent.
//  - layout_left: full_extent slices followed by one unit-stride slice keep layout_left (1.3);
//    a leading unit-stride slice, then (after index slices) full_extent slices ending with a
//    unit-stride slice give layout_left_padded<S_static> with padding stride(u + 1), S_static the
//    product of the static extents [0, u + 1) or dynamic_extent (1.4); otherwise layout_stride.
//  - layout_right: the mirror image, giving layout_right_padded with stride(rank - u - 2).
//  - layout_left_padded<P> source: one unit-stride leading slice of a rank-1 result gives
//    layout_left (1.3); the padded case keeps layout_left_padded with S_static = P times the
//    static extents [1, u + 1) (1.4.5).
// [mdspan.sub.sub]: the result's accessor is AccessorPolicy::offset_policy(src.accessor()), so an
// aligned_accessor source gives default_accessor; elements are src's elements at the slice
// lower bounds plus the result's indices.
#include <mdspan>
#include <type_traits>
#include <utility>
#include "check.hpp"

using std::full_extent;

int main() {
  int data[120];
  for (int i = 0; i < 120; ++i) data[i] = i;

  {  // layout_left, dynamic extents
    std::mdspan<int, std::dextents<int, 2>, std::layout_left> m(data, 4, 6);
    auto s = std::submdspan(m, std::pair{1, 3}, std::pair{2, 5});
    static_assert(std::is_same_v<decltype(s)::layout_type, std::layout_left_padded<std::dynamic_extent>>);
    CHECK(s.extent(0) == 2 && s.extent(1) == 3 && s.stride(0) == 1 && s.stride(1) == 4);
    for (int i = 0; i < 2; ++i)
      for (int j = 0; j < 3; ++j) CHECK((s[i, j] == m[1 + i, 2 + j]));
    auto col = std::submdspan(m, full_extent, std::pair{1, 3});  // full_extent then unit-stride
    static_assert(std::is_same_v<decltype(col)::layout_type, std::layout_left>);
    CHECK((col[3, 1] == m[3, 2]));
    auto strided = std::submdspan(m, std::extent_slice{0, 2, 2}, full_extent);
    static_assert(std::is_same_v<decltype(strided)::layout_type, std::layout_stride>);
    CHECK(strided.stride(0) == 2 && strided.stride(1) == 4 && (strided[1, 5] == m[2, 5]));
  }
  {  // layout_left, static extents: S_static = 4
    std::mdspan<int, std::extents<int, 4, 6>, std::layout_left> m(data);
    auto s = std::submdspan(m, std::pair{0, 2}, std::pair{1, 3});
    static_assert(std::is_same_v<decltype(s)::layout_type, std::layout_left_padded<4>>);
    CHECK(s.stride(1) == 4 && (s[1, 1] == m[1, 2]));
  }
  {  // layout_left, rank 3 with an index slice in between: u = 1, padding stride(2) = 20
    std::mdspan<int, std::dextents<int, 3>, std::layout_left> m(data, 4, 5, 6);
    auto s = std::submdspan(m, full_extent, 2, std::pair{1, 3});
    static_assert(std::is_same_v<decltype(s)::layout_type, std::layout_left_padded<std::dynamic_extent>>);
    CHECK(s.extent(0) == 4 && s.extent(1) == 2 && s.stride(1) == 20);
    CHECK((s[3, 1] == m[3, 2, 2]));
    std::mdspan<int, std::extents<int, 4, 5, 6>, std::layout_left> ms(data);
    auto ss = std::submdspan(ms, full_extent, 2, std::pair{1, 3});
    static_assert(std::is_same_v<decltype(ss)::layout_type, std::layout_left_padded<20>>);
    auto keep = std::submdspan(ms, full_extent, full_extent, std::pair{1, 3});
    static_assert(std::is_same_v<decltype(keep)::layout_type, std::layout_left>);
    CHECK((keep[3, 4, 1] == ms[3, 4, 2]));
  }
  {  // layout_right mirror image
    std::mdspan<int, std::extents<int, 4, 6>> m(data);
    auto s = std::submdspan(m, std::pair{1, 3}, std::pair{2, 5});
    static_assert(std::is_same_v<decltype(s)::layout_type, std::layout_right_padded<6>>);
    CHECK(s.stride(0) == 6 && s.stride(1) == 1 && (s[1, 2] == m[2, 4]));
    std::mdspan<int, std::dextents<int, 3>> d(data, 4, 5, 6);
    auto t = std::submdspan(d, std::pair{1, 3}, 2, full_extent);
    static_assert(std::is_same_v<decltype(t)::layout_type, std::layout_right_padded<std::dynamic_extent>>);
    CHECK(t.stride(0) == 30 && (t[1, 5] == d[2, 2, 5]));
  }
  {  // layout_left_padded source
    using P = std::layout_left_padded<8>;
    std::mdspan<int, std::dextents<int, 2>, P> p(data, P::mapping<std::dextents<int, 2>>(std::dextents<int, 2>(5, 3)));
    CHECK(p.stride(1) == 8);
    auto c = std::submdspan(p, full_extent, 1);
    static_assert(std::is_same_v<decltype(c)::layout_type, std::layout_left>);
    CHECK((c[4] == p[4, 1]) && c[4] == 12);
    auto b = std::submdspan(p, std::pair{1, 4}, std::pair{0, 2});
    static_assert(std::is_same_v<decltype(b)::layout_type, std::layout_left_padded<8>>);
    CHECK(b.stride(1) == 8 && (b[2, 1] == p[3, 1]) && b[2, 1] == 11);
  }
  {  // accessor: aligned_accessor's offset_policy
    alignas(64) static int buf[16] = {};
    for (int i = 0; i < 16; ++i) buf[i] = i * 2;
    std::mdspan<int, std::dextents<int, 2>, std::layout_right, std::aligned_accessor<int, 64>> a(buf, 4, 4);
    auto s = std::submdspan(a, 1, std::pair{1, 3});
    static_assert(std::is_same_v<decltype(s)::accessor_type, std::default_accessor<int>>);
    CHECK(s[0] == 10 && s[1] == 12);
  }
  return 0;
}
