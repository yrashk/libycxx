// Conjugation in [linalg] goes through the exposition-only helpers, which find conj and real by
// argument-dependent lookup ([linalg.helpers.conj], [linalg.helpers.real]), so a user complex
// type is conjugated like std::complex:
// [linalg.general]/4: a hermitian function uses real-if-needed(m[i, i]) for a diagonal element
// and conj-if-needed(m[j, i]) for an element outside the triangle t;
// [linalg.algs.blas2.hemv]: hermitian_matrix_vector_product y = A x;
// [linalg.algs.blas3.xxmm]: hermitian_matrix_product C = A B;
// [linalg.algs.blas2.symherrank1]/13: hermitian_matrix_rank_1_update(alpha, x, A, t) computes
// A = alpha x x^H with alpha real-if-needed(alpha);
// [linalg.algs.blas1.dot]: dotc conjugates v1; [linalg.algs.blas2.rank1]: matrix_rank_1_update_c
// computes A = x y^H; [linalg.conjtransposed]: conjugate_transposed.
// The type is a Gaussian integer; the diagonal holds a nonzero imaginary part that a hermitian
// function must not use, and the other triangle holds values that must not be read.
#include <linalg>
#include <mdspan>
#include "check.hpp"

namespace la = std::linalg;

namespace user {
struct G {
  long re = 0, im = 0;
  constexpr G() = default;
  constexpr G(long r, long i = 0) : re(r), im(i) {}
  friend constexpr G operator+(const G& a, const G& b) { return {a.re + b.re, a.im + b.im}; }
  friend constexpr G operator-(const G& a, const G& b) { return {a.re - b.re, a.im - b.im}; }
  friend constexpr G operator*(const G& a, const G& b) { return {a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re}; }
  constexpr G& operator+=(const G& b) { return *this = *this + b; }
  constexpr G& operator*=(const G& b) { return *this = *this * b; }
  friend constexpr bool operator==(const G&, const G&) = default;
};
constexpr G conj(const G& g) { return {g.re, -g.im}; }
constexpr long real(const G& g) { return g.re; }
constexpr long imag(const G& g) { return g.im; }
}  // namespace user
using user::G;

constexpr int n = 3;
using M = std::mdspan<G, std::extents<int, n, n>>;
using V = std::mdspan<G, std::extents<int, n>>;
constexpr G junk{1000, 1000};

int main() {
  // H, the Hermitian matrix: upper triangle stored, diagonal with junk imaginary parts
  G hd[n * n] = {{2, 9}, {1, 2}, {0, -1},  //
                 junk, {3, 7}, {4, 1},     //
                 junk, junk, {-1, 5}};
  M A(hd);
  G full[n][n];
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j)
      full[i][j] = i == j ? G(A[i, i].re) : i < j ? A[i, j] : user::conj(A[j, i]);

  G xd[n] = {{1, 1}, {0, 2}, {-1, 0}}, yd[n], wd[n];
  V x(xd), y(yd);
  la::hermitian_matrix_vector_product(A, la::upper_triangle, x, y);
  for (int i = 0; i < n; ++i) {
    G s;
    for (int j = 0; j < n; ++j) s += full[i][j] * x[j];
    CHECK(y[i] == s);
  }

  G bd[n * n], cd[n * n];
  for (int i = 0; i < n * n; ++i) bd[i] = G(i - 3, i % 2);
  M B(bd), C(cd);
  la::hermitian_matrix_product(A, la::upper_triangle, B, C);
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) {
      G s;
      for (int k = 0; k < n; ++k) s += full[i][k] * B[k, j];
      CHECK((C[i, j] == s));
    }

  // dotc: conj(v1[i]) * v2[i]
  G dref;
  for (int i = 0; i < n; ++i) dref += user::conj(x[i]) * y[i];
  CHECK(la::dotc(x, y) == dref);

  // A = x w^H
  for (int i = 0; i < n; ++i) wd[i] = G(i, 1 - i);
  V w(wd);
  la::matrix_rank_1_update_c(x, w, C);
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) CHECK((C[i, j] == x[i] * user::conj(w[j])));

  // A = alpha x x^H in the lower triangle, alpha = real-if-needed(alpha)
  for (int i = 0; i < n * n; ++i) cd[i] = junk;
  la::hermitian_matrix_rank_1_update(G(2, 5), x, C, la::lower_triangle);
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) {
      if (j <= i)
        CHECK((C[i, j] == G(2) * x[i] * user::conj(x[j])));
      else
        CHECK((C[i, j] == junk));  // the other triangle is not written
    }

  // conjugate_transposed
  auto ct = la::conjugate_transposed(B);
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) CHECK((G(ct[i, j]) == user::conj(B[j, i])));
  return 0;
}
