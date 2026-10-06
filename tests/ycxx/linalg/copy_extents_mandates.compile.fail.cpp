// [linalg.algs.blas1.copy]/3: "Mandates: For all r in the range [0, x.rank()),
// compatible-static-extents<InObj, OutObj>(r, r) is true." A 3-vector into a static 4-vector
// is ill-formed (not merely a precondition violation).
#include <linalg>
#include <mdspan>

void f(std::mdspan<const double, std::extents<int, 3>> x, std::mdspan<double, std::extents<int, 4>> y) {
  std::linalg::copy(x, y);
}
// EXPECT-ERROR: static assert|static_assert
