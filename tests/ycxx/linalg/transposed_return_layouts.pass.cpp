// [linalg.transp.transposed]/3: the layout of transposed(a) for each input layout:
//   (3.1) layout_left -> layout_right, (3.2) layout_right -> layout_left,
//   (3.3) layout_left_padded<P> -> layout_right_padded<P>, (3.4) and back,
//   (3.5) layout_stride -> layout_stride,
//   (3.6) layout_blas_packed<T, SO> -> layout_blas_packed<opposite T, opposite SO>,
//   (3.7) layout_transpose<L> -> L, (3.8) any other layout L -> layout_transpose<L>;
// with ReturnExtents = transpose-extents-t<Extents> (static extents swapped).
// /4: the returned mdspan has the same data handle and accessor; its mapping is built from the
// transposed extents (4.1), with the padding stride (4.2, 4.3), with the swapped strides (4.4),
// from the nested mapping (4.5) or wraps the input mapping (4.6). In every case
// transposed(a)[j, i] designates the same element as a[i, j] (/1, and Example 1).
#include <linalg>
#include <array>
#include <cstddef>
#include <mdspan>
#include <type_traits>
#include "check.hpp"

namespace la = std::linalg;
using std::dynamic_extent;
using std::extents;

template <class M>
constexpr bool same_elements_transposed(const M& a) {
  auto t = la::transposed(a);
  if (t.extent(0) != a.extent(1) || t.extent(1) != a.extent(0)) return false;
  using I = M::index_type;
  for (I i = 0; i < a.extent(0); ++i)
    for (I j = 0; j < a.extent(1); ++j)
      if (&t[j, i] != &a[i, j]) return false;
  return t.data_handle() == a.data_handle();
}

// A layout not named in /3: the identity on a 2-D index space stored column-major, but a
// distinct type, so transposed must wrap it in layout_transpose.
struct my_layout {
  template <class E>
  struct mapping {
    using extents_type = E;
    using index_type = E::index_type;
    using size_type = E::size_type;
    using rank_type = E::rank_type;
    using layout_type = my_layout;
    E e{};
    constexpr mapping() = default;
    constexpr explicit mapping(const E& x) : e(x) {}
    constexpr const E& extents() const noexcept { return e; }
    constexpr index_type required_span_size() const noexcept { return e.extent(0) * e.extent(1); }
    constexpr index_type operator()(index_type i, index_type j) const noexcept { return i + j * e.extent(0); }
    static constexpr bool is_always_unique() noexcept { return true; }
    static constexpr bool is_always_exhaustive() noexcept { return true; }
    static constexpr bool is_always_strided() noexcept { return true; }
    static constexpr bool is_unique() noexcept { return true; }
    static constexpr bool is_exhaustive() noexcept { return true; }
    static constexpr bool is_strided() noexcept { return true; }
    constexpr index_type stride(rank_type r) const noexcept { return r == 0 ? 1 : e.extent(0); }
    friend constexpr bool operator==(const mapping& a, const mapping& b) noexcept { return a.e == b.e; }
  };
};

constexpr bool run() {
  double d[64]{};
  for (int i = 0; i < 64; ++i) d[i] = i;

  // (3.1), (3.2), static extents swapped
  std::mdspan<double, extents<int, 3, dynamic_extent>, std::layout_left> L(d, 4);
  auto Lt = la::transposed(L);
  static_assert(std::is_same_v<decltype(Lt),
                               std::mdspan<double, extents<int, dynamic_extent, 3>, std::layout_right>>);
  if (!same_elements_transposed(L)) return false;
  std::mdspan<double, extents<long, 2, 5>, std::layout_right> R(d);
  static_assert(std::is_same_v<decltype(la::transposed(R)),
                               std::mdspan<double, extents<long, 5, 2>, std::layout_left>>);
  if (!same_elements_transposed(R)) return false;

  // (3.3) / (4.2): left_padded<4> with 3 rows: column stride 4
  using LP = std::layout_left_padded<4>;
  std::mdspan<double, extents<int, 3, 5>, LP> lp(d);
  if (lp.stride(1) != 4) return false;
  auto lpt = la::transposed(lp);
  static_assert(std::is_same_v<decltype(lpt)::layout_type, std::layout_right_padded<4>>);
  static_assert(std::is_same_v<decltype(lpt)::extents_type, extents<int, 5, 3>>);
  if (lpt.stride(0) != 4 || lpt.stride(1) != 1 || !same_elements_transposed(lp)) return false;
  // dynamic padding value
  using LPD = std::layout_left_padded<dynamic_extent>;
  std::mdspan<double, std::dextents<int, 2>, LPD> lpd(d, LPD::mapping<std::dextents<int, 2>>(std::dextents<int, 2>(3, 4), 8));
  if (lpd.stride(1) != 8) return false;
  auto lpdt = la::transposed(lpd);
  static_assert(std::is_same_v<typename decltype(lpdt)::layout_type, std::layout_right_padded<dynamic_extent>>);
  if (lpdt.stride(0) != 8 || !same_elements_transposed(lpd)) return false;

  // (3.4) / (4.3)
  using RP = std::layout_right_padded<dynamic_extent>;
  std::mdspan<double, std::dextents<int, 2>, RP> rp(d, RP::mapping<std::dextents<int, 2>>(std::dextents<int, 2>(4, 3), 5));
  if (rp.stride(0) != 5) return false;
  auto rpt = la::transposed(rp);
  static_assert(std::is_same_v<typename decltype(rpt)::layout_type, std::layout_left_padded<dynamic_extent>>);
  if (rpt.stride(1) != 5 || rpt.stride(0) != 1 || !same_elements_transposed(rp)) return false;

  // (3.5) / (4.4)
  using SM = std::layout_stride::mapping<extents<int, 3, 4>>;
  std::mdspan<double, extents<int, 3, 4>, std::layout_stride> s(d, SM({}, std::array{2, 9}));
  auto st = la::transposed(s);
  static_assert(std::is_same_v<decltype(st), std::mdspan<double, extents<int, 4, 3>, std::layout_stride>>);
  if (st.stride(0) != 9 || st.stride(1) != 2 || !same_elements_transposed(s)) return false;

  // (3.6) / (4.1): only (i, j) and (j, i) designate the same element, so compare values via
  // the symmetric mapping: transposed(p)[j, i] is p[i, j].
  using PK = la::layout_blas_packed<la::upper_triangle_t, la::column_major_t>;
  std::mdspan<double, extents<int, 4, 4>, PK> p(d);
  auto pt = la::transposed(p);
  static_assert(std::is_same_v<decltype(pt)::layout_type,
                               la::layout_blas_packed<la::lower_triangle_t, la::row_major_t>>);
  if (!same_elements_transposed(p)) return false;
  using PK2 = la::layout_blas_packed<la::lower_triangle_t, la::column_major_t>;
  std::mdspan<double, std::dextents<int, 2>, PK2> p2(d, 4, 4);
  static_assert(std::is_same_v<decltype(la::transposed(p2))::layout_type,
                               la::layout_blas_packed<la::upper_triangle_t, la::row_major_t>>);
  if (!same_elements_transposed(p2)) return false;

  // (3.8) / (4.6) and (3.7) / (4.5)
  std::mdspan<double, extents<int, 3, 5>, my_layout> m(d);
  auto mt = la::transposed(m);
  static_assert(std::is_same_v<decltype(mt),
                               std::mdspan<double, extents<int, 5, 3>, la::layout_transpose<my_layout>>>);
  if (!same_elements_transposed(m)) return false;
  if (!(mt.mapping().nested_mapping() == m.mapping())) return false;
  auto mtt = la::transposed(mt);
  static_assert(std::is_same_v<decltype(mtt), decltype(m)>);
  if (!(mtt.mapping() == m.mapping()) || mtt.data_handle() != d) return false;

  // transposing twice gives back the original layout for the standard layouts too
  static_assert(std::is_same_v<decltype(la::transposed(la::transposed(L))), decltype(L)>);
  static_assert(std::is_same_v<decltype(la::transposed(la::transposed(lp))), decltype(lp)>);
  static_assert(std::is_same_v<decltype(la::transposed(la::transposed(p))), decltype(p)>);
  return true;
}

int main() {
  CHECK(run());
  static_assert(run());
  // the accessor is kept: transposed of a scaled view is still scaled
  double d[6] = {1, 2, 3, 4, 5, 6};
  std::mdspan<double, extents<int, 2, 3>> A(d);
  auto sA = la::transposed(la::scaled(2.0, A));
  static_assert(std::is_same_v<decltype(sA)::accessor_type, la::scaled_accessor<double, std::default_accessor<double>>>);
  CHECK(sA[2, 1] == 12 && sA[0, 1] == 8);
  return 0;
}
