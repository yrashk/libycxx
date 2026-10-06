// The [linalg] algorithms take any mdspan meeting the argument concepts
// ([linalg.helpers.concepts]): any layout (layout_left, layout_right, padded, layout_stride,
// the results of transposed and submdspan) and any accessor (scaled_accessor,
// conjugated_accessor, aligned_accessor). The results depend only on the elements:
// [linalg.algs.blas3.gemm] C = A B and C = E + A B (/5: C may alias E),
// [linalg.algs.blas2.gemv] y = A x and z = y + A x (z may alias y), [linalg.algs.blas1.copy],
// [linalg.algs.blas1.add] (z may alias x or y), [linalg.algs.blas1.scal],
// [linalg.algs.blas1.dot], [linalg.algs.blas1.matfrobnorm], all against naive loops over the
// same elements. The values are small integers, so every result is exact.
#include <linalg>
#include <array>
#include <cmath>
#include <complex>
#include <mdspan>
#include "check.hpp"

namespace la = std::linalg;
using std::dextents;
using std::full_extent;

template <class A, class B>
bool same(const A& a, const B& b) {
  if (a.extent(0) != b.extent(0) || a.extent(1) != b.extent(1)) return false;
  for (int i = 0; i < a.extent(0); ++i)
    for (int j = 0; j < a.extent(1); ++j)
      if (a[i, j] != b[i, j]) return false;
  return true;
}
template <class A, class B, class C>
void naive_gemm(const A& a, const B& b, C& c) {
  for (int i = 0; i < a.extent(0); ++i)
    for (int j = 0; j < b.extent(1); ++j) {
      double s = 0;
      for (int k = 0; k < a.extent(1); ++k) s += a[i, k] * b[k, j];
      c[i, j] = s;
    }
}

int main() {
  // a 6 x 8 base array; A is a strided 3 x 4 window of it, B a transposed 4 x 2 one
  double base[48];
  for (int i = 0; i < 48; ++i) base[i] = (i * 7) % 11 - 5;
  std::mdspan<double, std::extents<int, 6, 8>> M(base);
  auto A = std::submdspan(M, std::range_slice{0, 6, 2}, std::pair{1, 5});  // rows 0, 2, 4
  static_assert(decltype(A)::rank() == 2);
  auto Bsrc = std::submdspan(M, std::pair{2, 4}, std::pair{4, 8});  // 2 x 4
  auto B = la::transposed(Bsrc);                                      // 4 x 2
  CHECK(A.extent(0) == 3 && A.extent(1) == 4 && B.extent(0) == 4 && B.extent(1) == 2);

  double refd[6], cd[16];
  std::mdspan<double, dextents<int, 2>> R(refd, 3, 2);
  naive_gemm(A, B, R);
  // C: layout_left_padded with padding 4
  using LP = std::layout_left_padded<4>;
  std::mdspan<double, dextents<int, 2>, LP> C(cd, LP::mapping<dextents<int, 2>>(dextents<int, 2>(3, 2)));
  la::matrix_product(A, B, C);
  CHECK(same(C, R));
  // C = E + A B with C aliasing E, through a layout_stride view of C's storage
  std::mdspan<double, dextents<int, 2>, std::layout_stride> Cs(
      cd, std::layout_stride::mapping<dextents<int, 2>>(dextents<int, 2>(3, 2), std::array{1, 4}));
  la::matrix_product(A, B, Cs, Cs);
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 2; ++j) CHECK(C[i, j] == 2 * R[i, j]);
  // scaled operands
  la::matrix_product(la::scaled(3.0, A), B, C);
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 2; ++j) CHECK(C[i, j] == 3 * R[i, j]);

  // gemv with a column of M as x (stride 8), y a row of a padded matrix
  auto x = std::submdspan(M, std::pair{1, 5}, 3);  // M[1..4, 3]
  double yd[3], zd[3], rd[3];
  std::mdspan<double, dextents<int, 1>> y(yd, 3), z(zd, 3), r(rd, 3);
  for (int i = 0; i < 3; ++i) {
    rd[i] = 0;
    for (int k = 0; k < 4; ++k) rd[i] += A[i, k] * x[k];
  }
  la::matrix_vector_product(A, x, y);
  for (int i = 0; i < 3; ++i) CHECK(yd[i] == rd[i]);
  la::matrix_vector_product(A, x, y, y);  // y = y + A x, z aliasing y
  for (int i = 0; i < 3; ++i) CHECK(yd[i] == 2 * rd[i]);
  la::matrix_vector_product(la::transposed(la::transposed(A)), x, la::scaled(1.0, y), z);
  for (int i = 0; i < 3; ++i) CHECK(zd[i] == 3 * rd[i]);

  // blas1 on strided views
  double vd[4], wd[4];
  std::mdspan<double, dextents<int, 1>> v(vd, 4), w(wd, 4);
  la::copy(x, v);
  for (int k = 0; k < 4; ++k) CHECK(vd[k] == M[1 + k, 3]);
  la::add(x, v, v);  // z aliasing y
  for (int k = 0; k < 4; ++k) CHECK(vd[k] == 2 * M[1 + k, 3]);
  la::add(v, la::scaled(-2.0, x), w);
  for (int k = 0; k < 4; ++k) CHECK(wd[k] == 0);
  double dref = 0;
  for (int k = 0; k < 4; ++k) dref += x[k] * A[1, k];
  CHECK(la::dot(x, std::submdspan(A, 1, full_extent)) == dref);
  double fro = 0;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 4; ++j) fro += A[i, j] * A[i, j];
  CHECK(std::fabs(la::matrix_frob_norm(A) - std::sqrt(fro)) < 1e-12);
  la::scale(2.0, std::submdspan(M, 0, full_extent));  // only row 0 changes
  CHECK(base[0] == 2 * ((0 * 7) % 11 - 5) && base[8] == (8 * 7) % 11 - 5);

  // conjugated, complex, with aligned_accessor
  using Cx = std::complex<double>;
  alignas(64) Cx ca[4] = {{1, 2}, {3, -1}, {0, 1}, {2, 2}};
  std::mdspan<Cx, std::extents<int, 4>, std::layout_right, std::aligned_accessor<Cx, 64>> cv(ca);
  Cx cdot = la::dot(la::conjugated(cv), cv);  // sum |c|^2
  CHECK(cdot == Cx(5 + 10 + 1 + 8, 0));
  CHECK(la::dotc(cv, cv) == cdot);
  return 0;
}
