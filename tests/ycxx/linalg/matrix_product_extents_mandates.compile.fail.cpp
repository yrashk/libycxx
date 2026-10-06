// [linalg.algs.blas3.gemm]/2: "Mandates: possibly-multipliable<decltype(A), decltype(B),
// decltype(C)>() is true." ([linalg.helpers.mandates]: compatible-static-extents of A's
// extent 1 and B's extent 0.) A is 2 x 3 and B is 4 x 2, both static: ill-formed.
#include <linalg>
#include <mdspan>

void f(std::mdspan<const double, std::extents<int, 2, 3>> a, std::mdspan<const double, std::extents<int, 4, 2>> b,
       std::mdspan<double, std::extents<int, 2, 2>> c) {
  std::linalg::matrix_product(a, b, c);
}
// EXPECT-ERROR: static assert|static_assert
