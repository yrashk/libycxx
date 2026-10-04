// BLAS 1 reductions on complex vectors and with a wider Scalar.
// [linalg.algs.blas1.asum]/3.3: for a non-arithmetic value_type, vector_abs_sum adds
// abs-if-needed(real-if-needed(v[i])) + abs-if-needed(imag-if-needed(v[i])) (|re| + |im|, as
// BLAS SCASUM/DZASUM), not abs(v[i]); /4: if the value_type and Scalar are floating-point and
// Scalar has higher precision, intermediate terms use Scalar's precision or greater.
// [linalg.algs.blas1.iamax]/4.3: for complex, vector_idx_abs_max is the first index with the
// largest |re| + |im|; /4.1: numeric_limits<size_type>::max() for zero elements.
// [linalg.algs.blas1.nrm2]/3: vector_two_norm(v, init) is the square root of |init|^2 plus the
// squares of |v[i]|; /4: wider Scalar precision. [linalg.algs.blas1.dot]/3, /5 (Remarks):
// dot(v1, v2, init) is GENERALIZED_SUM(plus<>(), init, v1[i] * v2[i]...) with Scalar's precision
// when it is higher. [linalg.algs.blas1.ssq] is not used.
// The float values are chosen so that a float accumulator loses them: 2^24 + 1 + 1 is
// 16777216 in float arithmetic (each +1 rounds back to even) but 16777218 exactly.
#include <cmath>
#include <complex>
#include <cstddef>
#include <limits>
#include <linalg>
#include <mdspan>
#include <type_traits>
#include "check.hpp"

namespace la = std::linalg;

int main() {
  using C = std::complex<double>;
  C ca[3] = {{5, 0}, {3, 3}, {-4, -2}};
  std::mdspan<C, std::dextents<int, 1>> cv(ca, 3);
  // /5: without init, vector_abs_sum(v, T{}) with T the value_type (here complex<double>).
  static_assert(std::is_same_v<decltype(la::vector_abs_sum(cv)), C>);
  CHECK(la::vector_abs_sum(cv) == C(5.0 + 6.0 + 6.0));
  CHECK(la::vector_abs_sum(cv, 1.0) == 18.0);
  // abs: 5, 4.24, 4.47 (index 0 largest); |re| + |im|: 5, 6, 6 (first largest is index 1).
  CHECK(la::vector_idx_abs_max(cv) == 1);
  std::mdspan<C, std::dextents<int, 1>> ce(ca, 0);
  CHECK(la::vector_idx_abs_max(ce) == std::numeric_limits<decltype(ce)::size_type>::max());
  CHECK(la::vector_abs_sum(ce, 2.5) == 2.5);
  C one[1] = {{3, 4}};
  CHECK(la::vector_abs_sum(std::mdspan<C, std::extents<int, 1>>(one), 0.0) == 7.0);
  // two_norm uses |v[i]| (5), and |init|.
  CHECK(std::fabs(la::vector_two_norm(std::mdspan<C, std::extents<int, 1>>(one)) - 5.0) < 1e-12);
  double re[2] = {3, 4};
  std::mdspan<double, std::extents<int, 2>> rv(re);
  CHECK(la::vector_two_norm(rv, -12.0) == 13.0);   // sqrt(144 + 9 + 16)
  CHECK(la::vector_two_norm(rv, C(0, 12)) == C(13.0) || std::abs(la::vector_two_norm(rv, C(0, 12)) - C(13.0)) < 1e-12);

  // Scalar wider than value_type.
  float fa[3] = {16777216.0f, 1.0f, 1.0f};
  std::mdspan<float, std::extents<int, 3>> fv(fa);
  CHECK(la::vector_abs_sum(fv, 0.0) == 16777218.0);
  CHECK(la::vector_abs_sum(fv) == 16777216.0f || la::vector_abs_sum(fv) == 16777218.0f);  // float: either order
  float ones[3] = {1.0f, 1.0f, 1.0f};
  std::mdspan<float, std::extents<int, 3>> ov(ones);
  CHECK(la::dot(fv, ov, 0.0) == 16777218.0);
  static_assert(std::is_same_v<decltype(la::dot(fv, ov, 0.0)), double>);
  float na[2] = {-16777216.0f, -2.0f};
  std::mdspan<float, std::extents<int, 2>> nv(na);
  CHECK(la::vector_abs_sum(nv, 1.0) == 16777219.0);
  return 0;
}
