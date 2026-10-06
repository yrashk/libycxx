// [linalg.transp.layout.transpose]: layout_transpose<Layout>::mapping<Extents> wraps a
// Layout::mapping<transpose-extents-t<Extents>> (/1: it "swaps the two indices, extents, and
// strides"). /4: the constructor stores the nested mapping and the transposed extents;
// operator()(i, j) is nested(j, i); required_span_size, is_[always_]unique/exhaustive/strided are
// the nested mapping's; /6: stride(r) is nested.stride(r == 0 ? 1 : 0); /8: == compares the
// nested mappings. nested_layout_type is Layout and layout_type is layout_transpose.
// [linalg.helpers]: transpose-extents-t swaps the static extents and keeps the index type.
#include <linalg>
#include <array>
#include <mdspan>
#include <type_traits>
#include "check.hpp"

namespace la = std::linalg;
using std::dynamic_extent;
using std::extents;

constexpr bool run() {
  using LT = la::layout_transpose<std::layout_right>;
  static_assert(std::is_same_v<LT::nested_layout_type, std::layout_right>);
  using M = LT::mapping<extents<short, 5, dynamic_extent>>;
  static_assert(std::is_same_v<M::extents_type, extents<short, 5, dynamic_extent>>);
  static_assert(std::is_same_v<M::index_type, short>);
  static_assert(std::is_same_v<M::size_type, std::make_unsigned_t<short>>);
  static_assert(std::is_same_v<M::layout_type, LT>);
  // the constructor is explicit and takes the nested mapping (of the transposed extents)
  using NM = std::layout_right::mapping<extents<short, dynamic_extent, 5>>;
  static_assert(std::is_constructible_v<M, const NM&> && !std::is_convertible_v<const NM&, M>);

  NM nm(extents<short, dynamic_extent, 5>(3));  // 3 x 5, row-major
  M m(nm);
  if (m.extents().extent(0) != 5 || m.extents().extent(1) != 3) return false;
  if (!(m.nested_mapping() == nm)) return false;
  if (m.required_span_size() != 15) return false;
  for (short i = 0; i < 5; ++i)
    for (short j = 0; j < 3; ++j)
      if (m(i, j) != nm(j, i) || m(i, j) != j * 5 + i) return false;
  if (m.stride(0) != 1 || m.stride(1) != 5) return false;
  static_assert(M::is_always_unique() && M::is_always_exhaustive() && M::is_always_strided());
  if (!m.is_unique() || !m.is_exhaustive() || !m.is_strided()) return false;

  // over layout_stride: non-exhaustive, strides swapped
  using LS = la::layout_transpose<std::layout_stride>;
  using SN = std::layout_stride::mapping<extents<int, 2, 3>>;
  SN sn({}, std::array{7, 2});
  LS::mapping<extents<int, 3, 2>> ms(sn);
  if (ms.stride(0) != 2 || ms.stride(1) != 7) return false;
  if (ms.is_exhaustive() || !ms.is_unique() || ms.required_span_size() != sn.required_span_size()) return false;
  if (ms(2, 1) != 7 + 4) return false;
  static_assert(!decltype(ms)::is_always_exhaustive());

  // over layout_blas_packed: neither unique nor strided for n >= 2
  using PK = la::layout_blas_packed<la::lower_triangle_t, la::row_major_t>;
  using LP = la::layout_transpose<PK>;
  LP::mapping<extents<int, 3, 3>> mp(PK::mapping<extents<int, 3, 3>>{});
  if (mp.is_unique() || mp.is_strided() || !mp.is_exhaustive() || mp.required_span_size() != 6) return false;
  if (mp(0, 2) != 3 || mp(2, 0) != 3) return false;  // lower, row-major: (2, 0) is offset 3

  // == compares the nested mappings, also across extents types
  M m2(NM(extents<short, dynamic_extent, 5>(3)));
  M m3(NM(extents<short, dynamic_extent, 5>(4)));
  if (!(m == m2) || m == m3) return false;
  LT::mapping<extents<short, 5, 3>> m4(std::layout_right::mapping<extents<short, 3, 5>>{});
  if (!(m == m4)) return false;

  // as the layout of an mdspan: element [i, j] is the nested layout's [j, i]
  int d[6] = {0, 1, 2, 3, 4, 5};
  std::mdspan<int, extents<int, 3, 2>, la::layout_transpose<std::layout_right>> t(
      d, la::layout_transpose<std::layout_right>::mapping<extents<int, 3, 2>>(
             std::layout_right::mapping<extents<int, 2, 3>>{}));
  if (t[2, 0] != 2 || t[0, 1] != 3 || t[2, 1] != 5 || t.size() != 6) return false;
  return true;
}

int main() {
  CHECK(run());
  static_assert(run());
  return 0;
}
