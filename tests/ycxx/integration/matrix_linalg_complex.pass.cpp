// Whole-program integration: a small complex matrix toolkit built from <mdspan>, <linalg> and
// <complex>, checked against naive loops. All values are small Gaussian integers, so every
// result is exact in double.
//   [mdspan.layout.right], [mdspan.layout.left], [mdspan.sub] (submdspan of a column has
//   layout_stride or layout_left/right as specified; only element access is relied on).
//   [linalg.algs.blas3.gemm]/(matrix_product: C = A B), [linalg.algs.blas2.gemv] (y = A x),
//   [linalg.conjtransposed] (conjugate_transposed(A)[i, j] == conj(A[j, i])),
//   [linalg.algs.blas1.dot]/10-13 (dotc(v1, v2) = sum of conj(v1[i]) * v2[i]),
//   [linalg.algs.blas2.trsv] (triangular_matrix_vector_solve with lower_triangle and
//   implicit_unit_diagonal solves A x = b reading only the strictly lower triangle),
//   [linalg.algs.blas1.scal] (scale), [linalg.algs.blas1.add] (add),
//   [linalg.algs.blas1.nrm2] (vector_two_norm: sqrt of the sum of |x[i]|^2).
#include <cmath>
#include <complex>
#include <linalg>
#include <mdspan>
#include <vector>
#include "check.hpp"

namespace la = std::linalg;
using C = std::complex<double>;

template <class M1, class M2, class M3>
void naive_product(M1 a, M2 b, M3 c) {
  for (std::size_t i = 0; i < c.extent(0); ++i)
    for (std::size_t j = 0; j < c.extent(1); ++j) {
      C s{};
      for (std::size_t k = 0; k < a.extent(1); ++k) s += C(a[i, k]) * C(b[k, j]);
      c[i, j] = s;
    }
}

int main() {
  // A: 3 x 4, row-major.
  std::vector<C> ad;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 4; ++j) ad.emplace_back(i - j, (i * j) % 3 - 1);
  std::mdspan A(ad.data(), std::dextents<std::size_t, 2>(3, 4));
  // B: 4 x 2, column-major.
  std::vector<C> bd = {{1, 0}, {0, 1}, {2, -1}, {-1, 0}, {0, 0}, {3, 2}, {1, 1}, {0, -2}};
  std::mdspan<C, std::dextents<std::size_t, 2>, std::layout_left> B(bd.data(), 4, 2);
  CHECK(B[2, 0] == C(2, -1) && B[0, 1] == C(0, 0) && B[3, 1] == C(0, -2));

  std::vector<C> cd(6), nd(6);
  std::mdspan Cm(cd.data(), std::dextents<std::size_t, 2>(3, 2));
  std::mdspan N(nd.data(), std::dextents<std::size_t, 2>(3, 2));
  la::matrix_product(A, B, Cm);
  naive_product(A, B, N);
  CHECK(cd == nd);
  CHECK(cd[0] != C{});

  // A * A^H is Hermitian.
  std::vector<C> hd(9), hn(9);
  std::mdspan H(hd.data(), std::extents<std::size_t, 3, 3>());
  std::mdspan HN(hn.data(), std::extents<std::size_t, 3, 3>());
  auto AH = la::conjugate_transposed(A);
  CHECK(AH.extent(0) == 4 && AH.extent(1) == 3);
  CHECK(C(AH[1, 2]) == std::conj(A[2, 1]));
  la::matrix_product(A, AH, H);
  naive_product(A, AH, HN);
  CHECK(hd == hn);
  for (std::size_t i = 0; i < 3; ++i) {
    CHECK(H[i, i].imag() == 0);
    for (std::size_t j = 0; j < 3; ++j) CHECK(H[i, j] == std::conj(H[j, i]));
  }

  // dotc of a row with itself is its squared norm; that is H's diagonal.
  for (std::size_t i = 0; i < 3; ++i) {
    auto row = std::submdspan(A, i, std::full_extent);
    CHECK(row.rank() == 1 && row.extent(0) == 4);
    C d = la::dotc(row, row);
    CHECK(d == H[i, i]);
    C plain = la::dot(row, row);
    C want{};
    for (std::size_t k = 0; k < 4; ++k) want += row[k] * row[k];
    CHECK(plain == want);
  }

  // A column (a strided view) times a vector.
  auto col = std::submdspan(A, std::full_extent, 2);
  CHECK(col.extent(0) == 3 && col[1] == A[1, 2] && &col[2] == &A[2, 2]);
  std::vector<C> xd = {{1, 1}, {0, -1}, {2, 0}, {-1, 3}}, yd(3);
  std::mdspan x(xd.data(), 4), y(yd.data(), 3);
  la::matrix_vector_product(A, x, y);
  for (std::size_t i = 0; i < 3; ++i) {
    C s{};
    for (std::size_t k = 0; k < 4; ++k) s += A[i, k] * x[k];
    CHECK(y[i] == s);
  }

  // Lower unit-triangular solve: L z = b, where b = L z0. The upper triangle and the diagonal
  // hold garbage that must not be read.
  std::vector<C> ld = {{99, 99}, {77, 0}, {55, 0},  //
                       {2, 1},   {99, 0}, {66, 0},  //
                       {-1, 0},  {3, -2}, {99, 0}};
  std::mdspan L(ld.data(), std::extents<int, 3, 3>());
  std::vector<C> z0 = {{1, 1}, {2, 0}, {0, -1}};
  std::vector<C> b = {z0[0], C(2, 1) * z0[0] + z0[1], C(-1, 0) * z0[0] + C(3, -2) * z0[1] + z0[2]};
  std::vector<C> z(3);
  la::triangular_matrix_vector_solve(L, la::lower_triangle, la::implicit_unit_diagonal,
                                     std::mdspan(b.data(), 3), std::mdspan(z.data(), 3));
  CHECK(z == z0);

  // scale and add, then the two-norm of (3+4i, 0, 0): 5.
  std::vector<C> v = {{3, 4}, {1, -1}, {2, 2}}, w = {{0, 0}, {-1, 1}, {-2, -2}}, u(3);
  la::add(std::mdspan(v.data(), 3), std::mdspan(w.data(), 3), std::mdspan(u.data(), 3));
  CHECK((u == std::vector<C>{{3, 4}, {0, 0}, {0, 0}}));
  CHECK(std::abs(la::vector_two_norm(std::mdspan(u.data(), 3)) - 5.0) < 1e-12);
  la::scale(C(0, 1), std::mdspan(u.data(), 3));
  CHECK(u[0] == C(-4, 3) && u[1] == C{} && u[2] == C{});
  return 0;
}
