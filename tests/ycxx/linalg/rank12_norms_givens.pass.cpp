// [linalg.algs.blas2.rank1]/6-/13: matrix_rank_1_update(x, y, [E,] A) computes A = [E +] x y^T;
// matrix_rank_1_update_c is the same with conjugated(y) (x y^H); A may alias E.
// [linalg.algs.blas2.symherrank1]: symmetric_matrix_rank_1_update(alpha, x, [E,] A, t) computes
// A = [E +] alpha x x^T, hermitian_... A = [E +] real-if-needed(alpha) x x^H; t applies to A
// (only its t-triangle is accessed; [linalg.general]/4), and to E for the E overloads.
// [linalg.algs.blas2.rank2]: symmetric_matrix_rank_2_update(x, y, [E,] A, t) computes
// A = [E +] x y^T + y x^T; hermitian_... A = [E +] x y^H + y x^H.
// [linalg.algs.blas1.matonenorm]/3, [linalg.algs.blas1.matinfnorm]/3: matrix_one_norm(A, init)
// is init plus the largest column sum of abs-if-needed(A[i, j]) (init if there are no columns),
// matrix_inf_norm the largest row sum (init if there are no rows); the one-argument forms use
// T{} with T = decltype(abs-if-needed(value_type)).
// [linalg.algs.blas1.givens.lartg]: setup_givens_rotation(a, b) is noexcept and returns c, s,
// r with c real, c^2 + |s|^2 = 1, [c s; -conj(s) c] [a; b] = [r; 0], r the Euclidean norm of
// (a, b). [linalg.algs.blas1.givens.rot]/4: apply_givens_rotation(x, y, c, s) applies that 2x2
// matrix to the rows x and y: x' = c x + s y, y' = -conj(s) x + c y.
#include <linalg>
#include <cmath>
#include <complex>
#include <limits>
#include <mdspan>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace la = std::linalg;
using cd = std::complex<double>;
using M2 = std::dextents<int, 2>;
using V1 = std::dextents<int, 1>;
const double qnan = std::numeric_limits<double>::quiet_NaN();
constexpr int N = 4;

bool near(double a, double b) { return std::abs(a - b) <= 1e-12 * (1 + std::abs(b)); }
bool near(cd a, cd b) { return std::abs(a - b) <= 1e-12 * (1 + std::abs(b)); }
bool upper_tri(bool upper, int i, int j) { return upper ? i <= j : i >= j; }

template <class T>
T cj(T x) {
  if constexpr (std::is_same_v<T, cd>) return std::conj(x); else return x;
}

template <class T, class Upd, class Want>
void check_tri_update(bool upper, bool with_e, bool hermitian, Upd upd, Want want) {
  T A[N * N], E[N * N], Efull[N * N];
  for (int i = 0; i < N * N; ++i) A[i] = T(100 + i);
  for (int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j) E[i * N + j] = T(i - 2 * j + 1);
  if constexpr (std::is_same_v<T, cd>)
    for (int i = 0; i < N * N; ++i) E[i] += cd(0, (i * 7) % 5 - 2);
  for (int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j) {
      if (i == j) Efull[i * N + j] = hermitian ? T(std::real(E[i * N + i])) : E[i * N + i];
      else if (upper_tri(upper, i, j)) Efull[i * N + j] = E[i * N + j];
      else Efull[i * N + j] = hermitian ? cj(E[j * N + i]) : E[j * N + i];
    }
  for (int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j) {
      if (!upper_tri(upper, i, j)) E[i * N + j] = T(qnan);  // outside t: not accessed
    }
  if constexpr (std::is_same_v<T, cd>)
    if (hermitian)
      for (int i = 0; i < N; ++i) E[i * N + i] = cd(E[i * N + i].real(), 999);
  std::mdspan<T, M2> a(A, N, N), e(E, N, N);
  upd(e, a);
  for (int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j) {
      if (upper_tri(upper, i, j)) {
        T w = want(i, j) + (with_e ? Efull[i * N + j] : T(0));
        CHECK(near(A[i * N + j], w));
      } else {
        CHECK(A[i * N + j] == T(100 + i * N + j));  // the other triangle is not accessed
      }
    }
}

template <class T>
void rank_updates() {
  T xb[N], yb[N];
  for (int i = 0; i < N; ++i) {
    xb[i] = T(i + 1);
    yb[i] = T(2 - i);
    if constexpr (std::is_same_v<T, cd>) {
      xb[i] += cd(0, i - 1);
      yb[i] += cd(0, 3 - i);
    }
  }
  std::mdspan<T, V1> x(xb, N), y(yb, N);
  constexpr bool cplx = std::is_same_v<T, cd>;

  // general rank-1 (all elements written)
  {
    T A[N * N], E[N * N];
    for (int i = 0; i < N * N; ++i) {
      A[i] = T(qnan);
      E[i] = T(i);
    }
    std::mdspan<T, M2> a(A, N, N), e(E, N, N);
    la::matrix_rank_1_update(x, y, a);
    for (int i = 0; i < N; ++i)
      for (int j = 0; j < N; ++j) CHECK(near(A[i * N + j], xb[i] * yb[j]));
    la::matrix_rank_1_update(x, y, e, a);
    for (int i = 0; i < N; ++i)
      for (int j = 0; j < N; ++j) CHECK(near(A[i * N + j], T(i * N + j) + xb[i] * yb[j]));
    la::matrix_rank_1_update(x, y, e, e);
    for (int i = 0; i < N; ++i)
      for (int j = 0; j < N; ++j) CHECK(near(E[i * N + j], T(i * N + j) + xb[i] * yb[j]));
    la::matrix_rank_1_update_c(x, y, a);
    for (int i = 0; i < N; ++i)
      for (int j = 0; j < N; ++j) CHECK(near(A[i * N + j], xb[i] * cj(yb[j])));
    for (int i = 0; i < N * N; ++i) E[i] = T(i);
    la::matrix_rank_1_update_c(x, y, e, a);
    for (int i = 0; i < N; ++i)
      for (int j = 0; j < N; ++j) CHECK(near(A[i * N + j], T(i * N + j) + xb[i] * cj(yb[j])));
  }

  for (bool upper : {true, false}) {
    auto run = [&](auto t) {
      // symmetric rank-1
      check_tri_update<T>(upper, false, false,
                          [&](auto, auto a) { la::symmetric_matrix_rank_1_update(T(2), x, a, t); },
                          [&](int i, int j) { return T(2) * xb[i] * xb[j]; });
      check_tri_update<T>(upper, true, false,
                          [&](auto e, auto a) { la::symmetric_matrix_rank_1_update(T(2), x, e, a, t); },
                          [&](int i, int j) { return T(2) * xb[i] * xb[j]; });
      // symmetric rank-2
      check_tri_update<T>(upper, false, false, [&](auto, auto a) { la::symmetric_matrix_rank_2_update(x, y, a, t); },
                          [&](int i, int j) { return xb[i] * yb[j] + yb[i] * xb[j]; });
      check_tri_update<T>(upper, true, false,
                          [&](auto e, auto a) { la::symmetric_matrix_rank_2_update(x, y, e, a, t); },
                          [&](int i, int j) { return xb[i] * yb[j] + yb[i] * xb[j]; });
      if constexpr (cplx) {
        // hermitian: the scalar is real-if-needed(alpha)
        check_tri_update<T>(upper, false, true,
                            [&](auto, auto a) { la::hermitian_matrix_rank_1_update(cd(2, 5), x, a, t); },
                            [&](int i, int j) { return T(2) * xb[i] * cj(xb[j]); });
        check_tri_update<T>(upper, true, true,
                            [&](auto e, auto a) { la::hermitian_matrix_rank_1_update(2.0, x, e, a, t); },
                            [&](int i, int j) { return T(2) * xb[i] * cj(xb[j]); });
        check_tri_update<T>(upper, false, true,
                            [&](auto, auto a) { la::hermitian_matrix_rank_2_update(x, y, a, t); },
                            [&](int i, int j) { return xb[i] * cj(yb[j]) + yb[i] * cj(xb[j]); });
        check_tri_update<T>(upper, true, true,
                            [&](auto e, auto a) { la::hermitian_matrix_rank_2_update(x, y, e, a, t); },
                            [&](int i, int j) { return xb[i] * cj(yb[j]) + yb[i] * cj(xb[j]); });
      }
    };
    if (upper) run(la::upper_triangle);
    else run(la::lower_triangle);
  }
}

void norms() {
  double A[6] = {1, -2, 3,
                 -4, 5, -6};  // 2x3
  std::mdspan<double, M2> a(A, 2, 3);
  // columns: 5, 7, 9; rows: 6, 15
  CHECK(la::matrix_one_norm(a) == 9.0);
  CHECK(la::matrix_inf_norm(a) == 15.0);
  CHECK(la::matrix_one_norm(a, 1.5) == 10.5);
  CHECK(la::matrix_inf_norm(a, 1.5) == 16.5);
  static_assert(std::is_same_v<decltype(la::matrix_one_norm(a)), double>);
  static_assert(std::is_same_v<decltype(la::matrix_inf_norm(a, 1.0f)), float>);
  // no columns: init (even with rows); no rows: init for the infinity norm
  std::mdspan<double, M2> nocols(A, 2, 0), norows(A, 0, 3);
  CHECK(la::matrix_one_norm(nocols, 4.0) == 4.0);
  CHECK(la::matrix_inf_norm(norows, 4.0) == 4.0);
  CHECK(la::matrix_inf_norm(nocols, 4.0) == 4.0);
  CHECK(la::matrix_one_norm(norows, 4.0) == 4.0);
  // integers
  int I[4] = {-1, 2, -3, -4};
  std::mdspan<int, M2> ia(I, 2, 2);
  CHECK(la::matrix_one_norm(ia) == 6);
  CHECK(la::matrix_inf_norm(ia) == 7);
  // complex: abs-if-needed is the modulus (3+4i -> 5)
  cd C[4] = {cd(3, 4), cd(0, 1), cd(-6, 8), cd(1, 0)};
  std::mdspan<cd, M2> c(C, 2, 2);
  static_assert(std::is_same_v<decltype(la::matrix_one_norm(c)), double>);
  CHECK(near(la::matrix_one_norm(c), 15.0));  // columns: 5 + 10, 1 + 1
  CHECK(near(la::matrix_inf_norm(c), 11.0));  // rows: 5 + 1, 10 + 1
}

void givens() {
  static_assert(noexcept(la::setup_givens_rotation(1.0, 2.0)));
  static_assert(noexcept(la::setup_givens_rotation(std::declval<cd>(), std::declval<cd>())));  // complex(re, im) is not noexcept
  static_assert(std::is_same_v<decltype(la::setup_givens_rotation(1.0, 2.0)), la::setup_givens_rotation_result<double>>);
  static_assert(std::is_same_v<decltype(la::setup_givens_rotation(cd(1), cd(2)).c), double>);
  static_assert(std::is_same_v<decltype(la::setup_givens_rotation(cd(1), cd(2)).s), cd>);
  const double pairs[][2] = {{3, 4}, {-3, 4}, {3, -4}, {-3, -4}, {0, 5}, {5, 0}, {-5, 0}, {0, -2}, {1e-3, 7}, {6, 1e-3}};
  for (auto& p : pairs) {
    double a = p[0], b = p[1];
    auto [c, s, r] = la::setup_givens_rotation(a, b);
    CHECK(near(c * c + s * s, 1.0));
    CHECK(near(c * a + s * b, r));
    CHECK(std::abs(-s * a + c * b) <= 1e-12 * std::hypot(a, b));
    CHECK(near(std::abs(r), std::hypot(a, b)));
    CHECK(r >= 0);  // "r is the Euclidean norm of the two-component vector"
  }
  const cd cpairs[][2] = {{cd(3, 4), cd(1, -2)}, {cd(0, 0), cd(2, 1)}, {cd(-1, 2), cd(0, 0)}, {cd(0, 3), cd(4, 0)}};
  for (auto& p : cpairs) {
    cd a = p[0], b = p[1];
    auto res = la::setup_givens_rotation(a, b);
    double c = res.c;
    cd s = res.s, r = res.r;
    CHECK(near(c * c + std::norm(s), 1.0));
    CHECK(near(c * a + s * b, r));
    CHECK(std::abs(-std::conj(s) * a + c * b) <= 1e-12 * (1 + std::abs(a) + std::abs(b)));
    CHECK(near(std::abs(r), std::sqrt(std::norm(a) + std::norm(b))));
  }

  double xb[3] = {1, 2, 3}, yb[3] = {4, -5, 6};
  std::mdspan<double, V1> x(xb, 3), y(yb, 3);
  la::apply_givens_rotation(x, y, 0.6, 0.8);
  const double x0[3] = {1, 2, 3}, y0[3] = {4, -5, 6};
  for (int i = 0; i < 3; ++i) {
    CHECK(near(xb[i], 0.6 * x0[i] + 0.8 * y0[i]));
    CHECK(near(yb[i], -0.8 * x0[i] + 0.6 * y0[i]));
  }
  // rotating (a, b) by its own rotation gives (r, 0)
  double ab[1] = {3}, bb[1] = {4};
  auto g = la::setup_givens_rotation(3.0, 4.0);
  la::apply_givens_rotation(std::mdspan<double, V1>(ab, 1), std::mdspan<double, V1>(bb, 1), g.c, g.s);
  CHECK(near(ab[0], g.r) && std::abs(bb[0]) < 1e-12);

  cd cx[2] = {cd(1, 1), cd(2, -1)}, cy[2] = {cd(0, 3), cd(-1, 0)};
  const cd cx0[2] = {cd(1, 1), cd(2, -1)}, cy0[2] = {cd(0, 3), cd(-1, 0)};
  const double c = 0.6;
  const cd s(0.0, 0.8);
  la::apply_givens_rotation(std::mdspan<cd, V1>(cx, 2), std::mdspan<cd, V1>(cy, 2), c, s);
  for (int i = 0; i < 2; ++i) {
    CHECK(near(cx[i], c * cx0[i] + s * cy0[i]));
    CHECK(near(cy[i], -std::conj(s) * cx0[i] + c * cy0[i]));
  }
}

int main() {
  rank_updates<double>();
  rank_updates<cd>();
  norms();
  givens();
}
