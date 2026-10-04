// [linalg.transp.transposed]/3-4: transposed(a) swaps the extents; layout_left <-> layout_right,
// layout_left_padded<P> <-> layout_right_padded<P> (keeping the padding stride), layout_stride
// with the strides swapped; the accessor is kept.
// [linalg.conj.conjugated]: for a real element type conjugated(a) returns a itself (/2.2); for
// complex, an mdspan with conjugated_accessor<Accessor>; conjugating that again restores the
// nested accessor (/1.1, /2.1).
// [linalg.scaled.scaled]: scaled(alpha, x) has accessor scaled_accessor<ScalingFactor, Accessor>
// and elements alpha * x[i]; scaled of scaled multiplies both factors.
// [linalg.general]/4-5: with a Triangle argument only that triangle is accessed; with
// implicit_unit_diagonal the diagonal is not accessed and taken as 1.
// [linalg.algs.blas2.trsv]/6, /11: triangular_matrix_vector_solve computes x with b = A x (or
// overwrites b). [linalg.algs.blas2.rank1]/6, /8: matrix_rank_1_update(x, y, A) computes
// A = x y^T and (x, y, E, A) computes A = E + x y^T (A may alias E).
// [linalg.algs.blas2.symv]: symmetric_matrix_vector_product(A, t, x, y) computes y = A x using
// only the t triangle of A.
#include <linalg>
#include <cmath>
#include <complex>
#include <limits>
#include <mdspan>
#include <type_traits>
#include "check.hpp"

namespace la = std::linalg;
using M2 = std::dextents<int, 2>;
using V1 = std::dextents<int, 1>;
const double qnan = std::numeric_limits<double>::quiet_NaN();

int main() {
  double buf[12];
  for (int i = 0; i < 12; ++i) buf[i] = i;

  // transposed
  std::mdspan<double, std::extents<int, 3, 4>, std::layout_left> L(buf);
  auto LT = la::transposed(L);
  static_assert(std::is_same_v<decltype(LT)::layout_type, std::layout_right>);
  static_assert(std::is_same_v<decltype(LT)::extents_type, std::extents<int, 4, 3>>);
  CHECK((LT[3, 2] == L[2, 3]));
  auto LTT = la::transposed(LT);
  static_assert(std::is_same_v<decltype(LTT)::layout_type, std::layout_left>);
  CHECK((LTT[1, 2] == L[1, 2]));
  std::mdspan<double, M2, std::layout_stride> S(buf, std::layout_stride::mapping<M2>(M2(2, 3), std::array{1, 4}));
  auto ST = la::transposed(S);
  static_assert(std::is_same_v<decltype(ST)::layout_type, std::layout_stride>);
  CHECK(ST.stride(0) == 4 && ST.stride(1) == 1 && (ST[2, 1] == S[1, 2]));
  using LP = std::layout_left_padded<4>;
  std::mdspan<double, M2, LP> P(buf, LP::mapping<M2>(M2(3, 3)));
  auto PT = la::transposed(P);
  static_assert(std::is_same_v<decltype(PT)::layout_type, std::layout_right_padded<4>>);
  CHECK(PT.stride(0) == 4 && (PT[2, 1] == P[1, 2]) && PT[2, 1] == 9);

  // conjugated
  std::mdspan<double, V1> r(buf, 3);
  auto rc = la::conjugated(r);
  static_assert(std::is_same_v<decltype(rc), decltype(r)>);
  std::complex<double> cbuf[2] = {{1, 2}, {3, -4}};
  std::mdspan<std::complex<double>, V1> c(cbuf, 2);
  auto cc = la::conjugated(c);
  static_assert(std::is_same_v<decltype(cc)::accessor_type, la::conjugated_accessor<std::default_accessor<std::complex<double>>>>);
  CHECK(cc[0] == std::complex<double>(1, -2) && cc[1] == std::complex<double>(3, 4));
  auto ccc = la::conjugated(cc);
  static_assert(std::is_same_v<decltype(ccc)::accessor_type, std::default_accessor<std::complex<double>>>);
  CHECK(ccc[1] == cbuf[1]);
  auto ch = la::conjugate_transposed(std::mdspan<std::complex<double>, M2>(cbuf, 1, 2));
  CHECK(ch.extent(0) == 2 && ch.extent(1) == 1 && (ch[1, 0] == std::complex<double>(3, 4)));

  // scaled
  auto s = la::scaled(2.0, r);
  static_assert(std::is_same_v<decltype(s)::accessor_type, la::scaled_accessor<double, std::default_accessor<double>>>);
  CHECK(s[2] == 4.0);
  auto ss = la::scaled(3.0, s);
  CHECK(ss[1] == 6.0);
  int ibuf[3] = {1, 2, 3};
  auto si = la::scaled(0.5, std::mdspan<int, V1>(ibuf, 3));
  CHECK(si[2] == 1.5);

  // triangular solve: lower, explicit diagonal; the upper part is NaN and must not be read.
  double A[9] = {2, qnan, qnan,
                 1, 4, qnan,
                 3, 2, 5};  // row-major
  std::mdspan<const double, M2> Am(A, 3, 3);
  double xs[3] = {1, -1, 2};  // b = A xs = {2, -3, 11}
  double b[3] = {2, -3, 11}, x[3] = {};
  la::triangular_matrix_vector_solve(Am, la::lower_triangle, la::explicit_diagonal,
                                     std::mdspan<const double, V1>(b, 3), std::mdspan<double, V1>(x, 3));
  CHECK(x[0] == xs[0] && x[1] == xs[1] && x[2] == xs[2]);
  // upper, implicit unit diagonal (the NaN diagonal must not be read), in place
  double U[9] = {qnan, 2, 3,
                 0, qnan, 4,
                 qnan, qnan, qnan};
  double bu[3] = {1 + 2 * 2 + 3 * 3, 2 + 4 * 3, 3};  // for x = {1, 2, 3}
  la::triangular_matrix_vector_solve(std::mdspan<const double, M2>(U, 3, 3), la::upper_triangle,
                                     la::implicit_unit_diagonal, std::mdspan<double, V1>(bu, 3));
  CHECK(bu[0] == 1 && bu[1] == 2 && bu[2] == 3);

  // rank-1 update
  double xv[2] = {1, 2}, yv[3] = {3, 4, 5};
  double R[6] = {qnan, qnan, qnan, qnan, qnan, qnan};
  std::mdspan<double, M2> Rm(R, 2, 3);
  la::matrix_rank_1_update(std::mdspan<const double, V1>(xv, 2), std::mdspan<const double, V1>(yv, 3), Rm);
  CHECK((Rm[0, 0] == 3 && Rm[0, 2] == 5 && Rm[1, 1] == 8 && Rm[1, 2] == 10));
  la::matrix_rank_1_update(std::mdspan<const double, V1>(xv, 2), std::mdspan<const double, V1>(yv, 3), Rm, Rm);
  CHECK((Rm[0, 0] == 6 && Rm[1, 2] == 20));

  // symmetric matrix-vector product from the upper triangle
  double Sy[9] = {1, 2, 3,
                  qnan, 4, 5,
                  qnan, qnan, 6};
  double sx[3] = {1, 1, 1}, sy[3] = {};
  la::symmetric_matrix_vector_product(std::mdspan<const double, M2>(Sy, 3, 3), la::upper_triangle,
                                      std::mdspan<const double, V1>(sx, 3), std::mdspan<double, V1>(sy, 3));
  CHECK(sy[0] == 6 && sy[1] == 11 && sy[2] == 14);
  return 0;
}
