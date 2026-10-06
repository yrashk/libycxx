// [linalg.algs.blas2.trsv]/3.1: "Mandates: If InMat has layout_blas_packed layout, then the
// layout's Triangle template argument has the same type as the function's Triangle template
// argument". An upper-packed A solved as lower triangular is ill-formed.
#include <linalg>
#include <mdspan>

namespace la = std::linalg;
using P = la::layout_blas_packed<la::upper_triangle_t, la::column_major_t>;

void f(std::mdspan<const double, std::extents<int, 3, 3>, P> A, std::mdspan<double, std::extents<int, 3>> b) {
  la::triangular_matrix_vector_solve(A, la::lower_triangle, la::explicit_diagonal, b);
}
// EXPECT-ERROR: static assert|static_assert
