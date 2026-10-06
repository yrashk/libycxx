// The exposition-only helpers on non-floating value types:
// [linalg.helpers.abs]/1: abs-if-needed(E) is E for an unsigned integer type, std::abs(E) for
// another arithmetic type, otherwise abs(E) found with a deleted template<class U> U abs(U) in
// scope (so a user type's ADL abs); [linalg.helpers.real], [linalg.helpers.imag]: real(E) /
// imag(E) for a non-arithmetic type that has them, otherwise E and ((void)E, T{}).
// Through [linalg.algs.blas1.asum]/3, /5: vector_abs_sum(v) is vector_abs_sum(v, T{}) with T the
// value_type, summing abs-if-needed(v[i]) for arithmetic types and
// abs-if-needed(real-if-needed(v[i])) + abs-if-needed(imag-if-needed(v[i])) otherwise;
// [linalg.algs.blas1.iamax]/2-/4: vector_idx_abs_max compares those sums (the first largest);
// [linalg.algs.blas1.matinfnorm]/3, /5, [linalg.algs.blas1.matonenorm]: abs-if-needed of each
// element, the one-argument form using T = decltype(abs-if-needed(value_type)).
#include <linalg>
#include <cstdlib>
#include <mdspan>
#include <type_traits>
#include "check.hpp"

namespace la = std::linalg;

namespace num {
// a real number type with an ADL abs (and no real/imag)
struct fixed {
  long v = 0;
  friend fixed operator+(fixed a, fixed b) { return {a.v + b.v}; }
  fixed& operator+=(fixed b) { return *this = *this + b; }  // [linalg.reqs.alg]/2.3: + and += may be used
  friend auto operator<=>(fixed, fixed) = default;
};
fixed abs(fixed f) { return {f.v < 0 ? -f.v : f.v}; }

// a complex-like type with ADL real, imag and abs (abs must not be used for its |re| + |im|)
struct gauss {
  long re = 0, im = 0;
  friend gauss operator+(gauss a, gauss b) { return {a.re + b.re, a.im + b.im}; }
  gauss& operator+=(gauss b) { return *this = *this + b; }
  friend bool operator==(gauss, gauss) = default;
};
long real(gauss g) { return g.re; }
long imag(gauss g) { return g.im; }
long abs(gauss) { return -999; }
}  // namespace num

int main() {
  // unsigned: E itself
  unsigned ud[4] = {3, 9, 1, 9};
  std::mdspan<unsigned, std::extents<int, 4>> u(ud);
  static_assert(std::is_same_v<decltype(la::vector_abs_sum(u)), unsigned>);
  CHECK(la::vector_abs_sum(u) == 22u);
  CHECK(la::vector_idx_abs_max(u) == 1);
  // signed: std::abs
  int id[5] = {-3, 7, -8, 8, 0};
  std::mdspan<int, std::extents<int, 5>> s(id);
  static_assert(std::is_same_v<decltype(la::vector_abs_sum(s)), int>);
  CHECK(la::vector_abs_sum(s) == 26);
  CHECK(la::vector_abs_sum(s, 100L) == 126L);
  CHECK(la::vector_idx_abs_max(s) == 2);  // the first of -8 and 8
  int md[6] = {1, -5, 2, -3, 4, -1};  // [1 -5 2; -3 4 -1]
  std::mdspan<int, std::extents<int, 2, 3>> m(md);
  static_assert(std::is_same_v<decltype(la::matrix_inf_norm(m)), int>);
  CHECK(la::matrix_inf_norm(m) == 8 && la::matrix_one_norm(m) == 9);
  CHECK(la::matrix_inf_norm(m, 1L) == 9L);

  // a user real type: abs through ADL, imag-if-needed gives fixed{}
  num::fixed fd[3] = {{-4}, {2}, {-5}};
  std::mdspan<num::fixed, std::extents<int, 3>> f(fd);
  static_assert(std::is_same_v<decltype(la::vector_abs_sum(f)), num::fixed>);
  CHECK(la::vector_abs_sum(f) == num::fixed{11});
  CHECK(la::vector_idx_abs_max(f) == 2);
  num::fixed fm[4] = {{-1}, {2}, {3}, {-7}};
  std::mdspan<num::fixed, std::extents<int, 2, 2>> F(fm);
  CHECK(la::matrix_inf_norm(F) == num::fixed{10} && la::matrix_one_norm(F) == num::fixed{9});

  // a complex-like type: |real| + |imag| through ADL real and imag; the result converts to
  // the Scalar given
  num::gauss gd[3] = {{1, -2}, {-3, 0}, {2, 2}};
  std::mdspan<num::gauss, std::extents<int, 3>> g(gd);
  CHECK(la::vector_abs_sum(g, 0L) == 10L);
  CHECK(la::vector_idx_abs_max(g) == 2);  // 3, 3, 4
  num::gauss gt[3] = {{0, -4}, {4, 0}, {1, 1}};
  CHECK(la::vector_idx_abs_max(std::mdspan<num::gauss, std::extents<int, 3>>(gt)) == 0);  // the first of two
  return 0;
}
