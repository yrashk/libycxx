// [linalg.algs.blas1]: scale(alpha, x) multiplies in place; copy(x, y); swap_elements(x, y);
// add(x, y, z) sets z = x + y; dot(v1, v2[, init]) is init + sum(v1[i] * v2[i]) with default
// T = decltype(v1[i] * v2[i]); dotc conjugates v1; vector_abs_sum, vector_two_norm,
// vector_idx_abs_max (index of the first element of largest absolute value, or
// numeric_limits<size_type>::max() for an empty vector), matrix_frob_norm.
#include <linalg>
#include <cmath>
#include <complex>
#include <limits>
#include <mdspan>
#include <type_traits>
#include "check.hpp"

namespace la = std::linalg;
using V = std::mdspan<double, std::dextents<int, 1>>;

int main() {
  double xa[4] = {1, -2, 3, -4}, ya[4] = {4, 3, 2, 1}, za[4] = {};
  V x(xa, 4), y(ya, 4), z(za, 4);

  CHECK(la::dot(x, y) == 4 - 6 + 6 - 4);
  CHECK(la::dot(x, y, 10.0) == 10.0);
  static_assert(std::is_same_v<decltype(la::dot(x, y)), double>);
  static_assert(std::is_same_v<decltype(la::dot(x, y, 1.0f)), float>);
  CHECK(la::vector_abs_sum(x) == 10);
  CHECK(std::fabs(la::vector_two_norm(x) - std::sqrt(30.0)) < 1e-14);
  CHECK(la::vector_idx_abs_max(x) == 3);
  double tie[3] = {-5, 5, 1};
  CHECK(la::vector_idx_abs_max(V(tie, 3)) == 0);  // first of the largest
  CHECK(la::vector_idx_abs_max(V(tie, 0)) == std::numeric_limits<V::size_type>::max());
  static_assert(std::is_same_v<decltype(la::vector_idx_abs_max(x)), V::size_type>);

  la::add(x, y, z);
  CHECK(za[0] == 5 && za[1] == 1 && za[2] == 5 && za[3] == -3);
  la::scale(2.0, z);
  CHECK(za[0] == 10 && za[3] == -6);
  la::copy(x, z);
  CHECK(za[0] == 1 && za[1] == -2 && za[3] == -4);
  la::swap_elements(y, z);
  CHECK(ya[0] == 1 && ya[3] == -4 && za[0] == 4 && za[3] == 1);

  // Complex: dot does not conjugate, dotc conjugates the first argument.
  using C = std::complex<double>;
  C ca[2] = {{1, 1}, {0, 2}}, cb[2] = {{2, 0}, {1, -1}};
  std::mdspan<C, std::extents<int, 2>> u(ca), w(cb);
  CHECK(la::dot(u, w) == C(1, 1) * C(2, 0) + C(0, 2) * C(1, -1));
  CHECK(la::dotc(u, w) == C(1, -1) * C(2, 0) + C(0, -2) * C(1, -1));

  double ma[6] = {1, 2, 2, 0, 0, 4};
  std::mdspan<double, std::extents<int, 2, 3>> A(ma);
  CHECK(la::matrix_frob_norm(A) == 5.0);
  return 0;
}
