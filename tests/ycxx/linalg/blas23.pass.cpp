// [linalg.algs.blas2.gemv]: matrix_vector_product(A, x, y) computes y = A x and (A, x, y, z)
// computes z = y + A x. [linalg.algs.blas3.gemm]: matrix_product(A, B, C) computes C = A B and
// (A, B, E, C) computes C = E + A B. [linalg.scaled], [linalg.transp], [linalg.conj]:
// scaled(alpha, x) is a read-only view of alpha * x, transposed(A) swaps the extents (and
// layout_right becomes layout_left), conjugated(a) conjugates complex elements and is the
// identity for real ones.
#include <linalg>
#include <complex>
#include <mdspan>
#include <type_traits>
#include "check.hpp"

namespace la = std::linalg;

int main() {
  double ad[6] = {1, 2, 3, 4, 5, 6};  // 2 x 3 row-major: [1 2 3; 4 5 6]
  std::mdspan<double, std::extents<int, 2, 3>> A(ad);
  double xd[3] = {1, 0, -1}, yd[2] = {}, wd[2] = {10, 20}, zd[2] = {};
  std::mdspan<double, std::extents<int, 3>> x(xd);
  std::mdspan<double, std::extents<int, 2>> y(yd), w(wd), z(zd);
  la::matrix_vector_product(A, x, y);
  CHECK(yd[0] == -2 && yd[1] == -2);
  la::matrix_vector_product(A, x, w, z);
  CHECK(zd[0] == 8 && zd[1] == 18);

  // transposed
  auto At = la::transposed(A);
  static_assert(std::is_same_v<decltype(At)::extents_type, std::extents<int, 3, 2>>);
  static_assert(std::is_same_v<decltype(At)::layout_type, std::layout_left>);
  CHECK(At[2, 1] == 6 && At[0, 1] == 4 && &At[1, 0] == &A[0, 1]);
  double t2[3] = {};
  std::mdspan<double, std::extents<int, 3>> r(t2);
  double ones[2] = {1, 1};
  la::matrix_vector_product(At, std::mdspan<double, std::extents<int, 2>>(ones), r);
  CHECK(t2[0] == 5 && t2[1] == 7 && t2[2] == 9);

  // scaled
  auto sx = la::scaled(2.0, x);
  CHECK(sx[0] == 2 && sx[2] == -2 && xd[0] == 1);
  la::matrix_vector_product(A, la::scaled(3.0, x), y);
  CHECK(yd[0] == -6 && yd[1] == -6);

  // matrix_product
  double bd[6] = {1, 0, 0, 1, 1, 1};  // 3 x 2: [1 0; 0 1; 1 1]
  std::mdspan<double, std::extents<int, 3, 2>> B(bd);
  double cd[4] = {}, ed[4] = {1, 1, 1, 1}, fd[4] = {};
  std::mdspan<double, std::extents<int, 2, 2>> Cm(cd), E(ed), F(fd);
  la::matrix_product(A, B, Cm);
  CHECK(cd[0] == 4 && cd[1] == 5 && cd[2] == 10 && cd[3] == 11);
  la::matrix_product(A, B, E, F);
  CHECK(fd[0] == 5 && fd[3] == 12);

  // conjugated
  using C = std::complex<double>;
  C cdat[2] = {{1, 2}, {3, -4}};
  std::mdspan<C, std::extents<int, 2>> cv(cdat);
  auto cc = la::conjugated(cv);
  CHECK(C(cc[0]) == C(1, -2) && C(cc[1]) == C(3, 4));
  auto rc = la::conjugated(x);
  CHECK(rc[0] == 1 && rc[2] == -1);
  return 0;
}
