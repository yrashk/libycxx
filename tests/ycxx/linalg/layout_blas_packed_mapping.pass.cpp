// [linalg.layout.packed]: layout_blas_packed<Triangle, StorageOrder>::mapping.
// overview/2: column_major_t packs the stored triangle column by column, each column from its
// top entry; /3: row_major_t packs it row by row, each row from its leftmost entry.
// overview/4: is_always_unique is true iff a static extent is < 2; is_always_exhaustive true;
// is_always_strided == is_always_unique; is_unique and is_strided iff extent(0) < 2.
// overview/6: the mapping is trivially copyable and models regular.
// [linalg.layout.packed.cons]/3: the converting constructor is explicit iff the extents are not
// convertible. [linalg.layout.packed.obs]/1: required_span_size is N (N + 1) / 2; /5:
// operator()(i, j) is (*this)(j, i) for i > j (both triangles map to the stored one), else
// i + j (j + 1) / 2 for (column-major, upper) and (row-major, lower), else j + N i - i (i + 1) / 2;
// /7: stride is 1; /8: == compares the extents. All noexcept.
#include <linalg>
#include <concepts>
#include <mdspan>
#include <type_traits>
#include "check.hpp"

namespace la = std::linalg;
using std::dynamic_extent;
using std::extents;

template <class T, class SO>
constexpr bool check_order(int n) {
  using P = la::layout_blas_packed<T, SO>;
  using E = std::dextents<int, 2>;
  typename P::template mapping<E> m(E(n, n));
  if (m.required_span_size() != n * (n + 1) / 2) return false;
  constexpr bool upper = std::is_same_v<T, la::upper_triangle_t>;
  constexpr bool colmaj = std::is_same_v<SO, la::column_major_t>;
  int next = 0;
  // walk the stored triangle in the order /2 or /3 describes; offsets must be 0, 1, 2, ...
  for (int outer = 0; outer < n; ++outer) {
    for (int inner = 0; inner < n; ++inner) {
      int r = colmaj ? inner : outer, c = colmaj ? outer : inner;
      bool stored = upper ? r <= c : r >= c;
      if (!stored) continue;
      if (m(r, c) != next) return false;
      if (m(c, r) != next) return false;  // /5.1: the other triangle maps to the same element
      ++next;
    }
  }
  return next == n * (n + 1) / 2;
}

constexpr bool run() {
  for (int n = 0; n <= 6; ++n) {
    if (!check_order<la::upper_triangle_t, la::column_major_t>(n)) return false;
    if (!check_order<la::upper_triangle_t, la::row_major_t>(n)) return false;
    if (!check_order<la::lower_triangle_t, la::column_major_t>(n)) return false;
    if (!check_order<la::lower_triangle_t, la::row_major_t>(n)) return false;
  }

  using P = la::layout_blas_packed<la::upper_triangle_t, la::row_major_t>;
  static_assert(std::is_same_v<P::triangle_type, la::upper_triangle_t>);
  static_assert(std::is_same_v<P::storage_order_type, la::row_major_t>);
  using M5 = P::mapping<extents<int, 5, 5>>;
  using MD = P::mapping<std::dextents<int, 2>>;
  using M1 = P::mapping<extents<int, 1, dynamic_extent>>;
  using M0 = P::mapping<extents<int, dynamic_extent, 0>>;
  static_assert(std::is_same_v<M5::layout_type, P> && std::is_same_v<M5::index_type, int>);
  static_assert(!M5::is_always_unique() && !M5::is_always_strided() && M5::is_always_exhaustive());
  static_assert(!MD::is_always_unique() && !MD::is_always_strided() && MD::is_always_exhaustive());
  static_assert(M1::is_always_unique() && M1::is_always_strided());
  static_assert(M0::is_always_unique() && M0::is_always_strided());
  static_assert(std::is_trivially_copyable_v<M5> && std::regular<M5> && std::regular<MD>);
  static_assert(M5().required_span_size() == 15);  // obs/1 note: a 5 x 5 matrix stores 15
  static_assert(noexcept(M5().required_span_size()) && noexcept(M5()(1, 2)) && noexcept(M5().stride(0)));
  static_assert(noexcept(M5() == MD()));

  MD m1(std::dextents<int, 2>(1, 1)), m3(std::dextents<int, 2>(3, 3));
  if (!m1.is_unique() || !m1.is_strided() || m1.stride(0) != 1 || m1.stride(1) != 1) return false;
  if (m3.is_unique() || m3.is_strided() || !m3.is_exhaustive()) return false;
  MD m0(std::dextents<int, 2>(0, 0));
  if (!m0.is_unique() || m0.required_span_size() != 0) return false;

  // cons/3: implicit from static to dynamic extents, explicit the other way
  static_assert(std::is_convertible_v<M5, MD>);
  static_assert(std::is_constructible_v<M5, MD> && !std::is_convertible_v<MD, M5>);
  static_assert(!std::is_constructible_v<M5, P::mapping<extents<int, 4, 4>>>);
  MD fromstatic = M5();
  if (fromstatic.extents().extent(0) != 5 || !(fromstatic == M5())) return false;
  // obs/8: == compares the extents only
  if (m3 == MD(std::dextents<int, 2>(4, 4)) || !(m3 == P::mapping<extents<int, 3, 3>>())) return false;

  // as an mdspan layout: a symmetric view of the packed data
  double d[6] = {1, 2, 3, 4, 5, 6};  // upper, row-major: [1 2 3; . 4 5; . . 6]
  std::mdspan<double, extents<int, 3, 3>, P> s(d);
  if (s[0, 2] != 3 || s[2, 0] != 3 || s[1, 2] != 5 || s[2, 1] != 5 || s[2, 2] != 6) return false;
  if (s.size() != 9 || s.mapping().required_span_size() != 6) return false;
  return true;
}

int main() {
  CHECK(run());
  static_assert(run());
  return 0;
}
