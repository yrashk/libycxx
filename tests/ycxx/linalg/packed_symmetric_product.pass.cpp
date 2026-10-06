// Algorithms reading and writing layout_blas_packed matrices.
// [linalg.algs.blas2.symv]: symmetric_matrix_vector_product(A, t, x, y) with A packed in the
// triangle t ([linalg.algs.blas2.symv] Mandates: a packed layout's Triangle is t's type);
// [linalg.algs.blas2.trmv], [linalg.algs.blas2.trsv]: triangular product and solve with a packed
// A; [linalg.algs.blas2.symherrank1]: symmetric_matrix_rank_1_update(alpha, x, A, t) on a packed A
// (possibly-packed-out-matrix, [linalg.helpers.concepts]) computes A = A + alpha x x^T in the
// stored triangle; [linalg.algs.blas2.rank2]: symmetric_matrix_rank_2_update(x, y, A, t), A = A +
// x y^T + y x^T. [linalg.layout.packed.overview]/2-/3: the storage order.
// Each case for both storage orders and both triangles, against a dense oracle.
#include <linalg>
#include <mdspan>
#include <type_traits>
#include "check.hpp"

namespace la = std::linalg;
constexpr int n = 4;
using D = std::mdspan<double, std::extents<int, n, n>>;
using V = std::mdspan<double, std::extents<int, n>>;

template <class T, class SO>
void run(T t) {
  constexpr bool upper = std::is_same_v<T, la::upper_triangle_t>;
  using P = std::mdspan<double, std::extents<int, n, n>, la::layout_blas_packed<T, SO>>;
  double pd[n * (n + 1) / 2];
  P A(pd);
  double dd[n * n];
  D S(dd);  // the dense symmetric matrix A stands for
  for (int i = 0; i < n; ++i)
    for (int j = i; j < n; ++j) {
      double v = 1 + i * 3 + j * j;
      if (upper)
        A[i, j] = v;
      else
        A[j, i] = v;
      S[i, j] = S[j, i] = v;
    }
  double xd[n] = {1, -2, 3, 1}, yd[n], rd[n];
  V x(xd), y(yd), r(rd);

  // symv
  la::symmetric_matrix_vector_product(A, t, x, y);
  la::matrix_vector_product(S, x, r);
  for (int i = 0; i < n; ++i) CHECK(y[i] == r[i]);

  // trmv / trsv with the stored triangle
  la::triangular_matrix_vector_product(A, t, la::explicit_diagonal, x, y);
  for (int i = 0; i < n; ++i) {
    double s = 0;
    for (int j = 0; j < n; ++j)
      if (upper ? j >= i : j <= i) s += S[i, j] * x[j];
    CHECK(y[i] == s);
  }
  la::triangular_matrix_vector_solve(A, t, la::explicit_diagonal, y);  // in place: back to x
  for (int i = 0; i < n; ++i) CHECK(y[i] > x[i] - 1e-9 && y[i] < x[i] + 1e-9);

  // rank-1 and rank-2 updates write the stored triangle
  double ed[n * (n + 1) / 2];
  P E(ed);
  for (int k = 0; k < n * (n + 1) / 2; ++k) ed[k] = pd[k];
  la::symmetric_matrix_rank_1_update(2.0, x, E, A, t);
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) CHECK(A[i, j] == S[i, j] + 2.0 * x[i] * x[j]);
  double wd[n] = {0, 1, -1, 2};
  V w(wd);
  la::symmetric_matrix_rank_2_update(x, w, A, A, t);  // E is A
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) CHECK(A[i, j] == S[i, j] + 2.0 * x[i] * x[j] + x[i] * w[j] + w[i] * x[j]);
  la::symmetric_matrix_rank_1_update(-1.0, w, A, t);  // overwriting
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) CHECK(A[i, j] == -w[i] * w[j]);
}

int main() {
  run<la::upper_triangle_t, la::column_major_t>(la::upper_triangle);
  run<la::upper_triangle_t, la::row_major_t>(la::upper_triangle);
  run<la::lower_triangle_t, la::column_major_t>(la::lower_triangle);
  run<la::lower_triangle_t, la::row_major_t>(la::lower_triangle);
  return 0;
}
