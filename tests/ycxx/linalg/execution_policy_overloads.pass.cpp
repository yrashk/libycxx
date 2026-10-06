// [linalg.algs.reqs]/1.1 and Note 1: every [linalg] algorithm has an overload taking an
// ExecutionPolicy first (a parallel algorithm, [algorithms.parallel.defns]); its effects are those
// of the overload without it. Checked for the BLAS 1, 2 and 3 functions with each standard
// policy object, against the sequential overloads. The values are small integers, so the
// results are exact. (The policies are passed as the standard's lvalue objects, as with every
// other parallel algorithm, [algorithms.parallel.defns]/2; read literally, /1.1's
// is_execution_policy<ExecutionPolicy> would reject the deduced reference type, which cannot be
// the intent: no portable program could call these overloads.)
#include <linalg>
#include <execution>
#include <mdspan>
#include "check.hpp"

namespace la = std::linalg;
constexpr int n = 3;
using M = std::mdspan<double, std::extents<int, n, n>>;
using V = std::mdspan<double, std::extents<int, n>>;

template <class X>
bool eq(X a, X b) {
  if constexpr (X::rank() == 1) {
    for (int i = 0; i < n; ++i)
      if (a[i] != b[i]) return false;
  } else {
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < n; ++j)
        if (a[i, j] != b[i, j]) return false;
  }
  return true;
}

template <class Pol>
void run(const Pol& pol) {
  double ad[n * n] = {2, -1, 3, 0, 4, 1, -2, 5, 3}, bd[n * n] = {1, 2, 0, -1, 3, 2, 4, 0, 1};
  double xd[n] = {1, -2, 3}, yd[n] = {4, 0, -1};
  M A(ad), B(bd);
  V x(xd), y(yd);
  CHECK(la::dot(pol, x, y) == la::dot(x, y));
  CHECK(la::dot(pol, x, y, 1.0) == la::dot(x, y, 1.0));
  CHECK(la::dotc(pol, x, y) == la::dotc(x, y));
  CHECK(la::vector_abs_sum(pol, x) == la::vector_abs_sum(x));
  CHECK(la::vector_two_norm(pol, y) == la::vector_two_norm(y));
  CHECK(la::vector_idx_abs_max(pol, x) == la::vector_idx_abs_max(x));
  CHECK(la::matrix_frob_norm(pol, A) == la::matrix_frob_norm(A));
  CHECK(la::matrix_one_norm(pol, A) == la::matrix_one_norm(A));
  CHECK(la::matrix_inf_norm(pol, A) == la::matrix_inf_norm(A));

  double p1[n], p2[n], q1[n * n], q2[n * n];
  V r1(p1), r2(p2);
  M S1(q1), S2(q2);
  la::copy(pol, x, r1);
  la::copy(x, r2);
  CHECK(eq(r1, r2));
  la::scale(pol, 3.0, r1);
  la::scale(3.0, r2);
  CHECK(eq(r1, r2));
  la::add(pol, x, y, r1);
  la::add(x, y, r2);
  CHECK(eq(r1, r2));
  la::swap_elements(pol, r1, r2);
  CHECK(eq(r1, r2));

  la::matrix_vector_product(pol, A, x, r1);
  la::matrix_vector_product(A, x, r2);
  CHECK(eq(r1, r2));
  la::matrix_vector_product(pol, A, x, y, r1);
  la::matrix_vector_product(A, x, y, r2);
  CHECK(eq(r1, r2));
  la::symmetric_matrix_vector_product(pol, A, la::upper_triangle, x, r1);
  la::symmetric_matrix_vector_product(A, la::upper_triangle, x, r2);
  CHECK(eq(r1, r2));
  la::triangular_matrix_vector_product(pol, A, la::lower_triangle, la::explicit_diagonal, x, r1);
  la::triangular_matrix_vector_product(A, la::lower_triangle, la::explicit_diagonal, x, r2);
  CHECK(eq(r1, r2));
  la::triangular_matrix_vector_solve(pol, A, la::upper_triangle, la::implicit_unit_diagonal, x, r1);
  la::triangular_matrix_vector_solve(A, la::upper_triangle, la::implicit_unit_diagonal, x, r2);
  CHECK(eq(r1, r2));
  la::matrix_rank_1_update(pol, x, y, S1);
  la::matrix_rank_1_update(x, y, S2);
  CHECK(eq(S1, S2));
  la::symmetric_matrix_rank_2_update(pol, x, y, S1, la::lower_triangle);
  la::symmetric_matrix_rank_2_update(x, y, S2, la::lower_triangle);
  CHECK(eq(S1, S2));

  la::matrix_product(pol, A, B, S1);
  la::matrix_product(A, B, S2);
  CHECK(eq(S1, S2));
  la::matrix_product(pol, A, B, B, S1);
  la::matrix_product(A, B, B, S2);
  CHECK(eq(S1, S2));
  la::symmetric_matrix_product(pol, A, la::lower_triangle, B, S1);
  la::symmetric_matrix_product(A, la::lower_triangle, B, S2);
  CHECK(eq(S1, S2));
  la::triangular_matrix_product(pol, A, la::upper_triangle, la::explicit_diagonal, B, S1);
  la::triangular_matrix_product(A, la::upper_triangle, la::explicit_diagonal, B, S2);
  CHECK(eq(S1, S2));
  la::triangular_matrix_matrix_left_solve(pol, A, la::lower_triangle, la::implicit_unit_diagonal, B, S1);
  la::triangular_matrix_matrix_left_solve(A, la::lower_triangle, la::implicit_unit_diagonal, B, S2);
  CHECK(eq(S1, S2));
  la::symmetric_matrix_rank_k_update(pol, 2.0, A, S1, la::upper_triangle);
  la::symmetric_matrix_rank_k_update(2.0, A, S2, la::upper_triangle);
  CHECK(eq(S1, S2));
  la::symmetric_matrix_rank_2k_update(pol, A, B, S1, la::lower_triangle);
  la::symmetric_matrix_rank_2k_update(A, B, S2, la::lower_triangle);
  CHECK(eq(S1, S2));
}

int main() {
  run(std::execution::seq);
  run(std::execution::par);
  run(std::execution::par_unseq);
  run(std::execution::unseq);
  return 0;
}
