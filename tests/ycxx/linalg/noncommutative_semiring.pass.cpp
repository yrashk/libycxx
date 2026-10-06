// [linalg.reqs.alg]/2.3: "Each function in [linalg.algs.blas1], [linalg.algs.blas2], and
// [linalg.algs.blas3] that is not a triangular solve algorithm will use a sequence of
// evaluations of *, *=, +, +=, and = operators that would produce the result specified by the
// algorithm's Effects and Remarks when operating on elements of a semiring with noncommutative
// multiplication." So the products keep their factors in order. Checked with integer
// quaternions (noncommutative: i j = k, j i = -k) against naive loops:
// [linalg.algs.blas1.scal] x = alpha x; [linalg.algs.blas1.dot] v1[i] * v2[i];
// [linalg.algs.blas2.gemv] y = A x, z = y + A x; [linalg.algs.blas3.gemm] C = A B, C = E + A B;
// [linalg.algs.blas2.trmv] y = A x for a triangular A; [linalg.algs.blas3.trmm]
// triangular_matrix_left_product C = A C, triangular_matrix_right_product C = C A,
// triangular_matrix_product(A, t, d, B, C) C = A B and (B, A, t, d, C) C = B A;
// [linalg.algs.blas2.rank1] A = x y^T (x[i] * y[j]); [linalg.algs.blas3.xxmm] symmetric
// products, the structured matrix on the left and on the right; [linalg.scaled.scaledaccessor]/6.
// [linalg.reqs.val]/3: a value-initialized object is the additive identity.
#include <linalg>
#include <mdspan>
#include "check.hpp"

namespace la = std::linalg;

struct Q {
  long a = 0, b = 0, c = 0, d = 0;  // a + b i + c j + d k
  friend constexpr Q operator+(const Q& x, const Q& y) { return {x.a + y.a, x.b + y.b, x.c + y.c, x.d + y.d}; }
  friend constexpr Q operator*(const Q& x, const Q& y) {
    return {x.a * y.a - x.b * y.b - x.c * y.c - x.d * y.d, x.a * y.b + x.b * y.a + x.c * y.d - x.d * y.c,
            x.a * y.c - x.b * y.d + x.c * y.a + x.d * y.b, x.a * y.d + x.b * y.c - x.c * y.b + x.d * y.a};
  }
  constexpr Q& operator+=(const Q& y) { return *this = *this + y; }
  constexpr Q& operator*=(const Q& y) { return *this = *this * y; }
  friend constexpr bool operator==(const Q&, const Q&) = default;
};
constexpr Q I{0, 1, 0, 0}, J{0, 0, 1, 0}, K{0, 0, 0, 1};
static_assert(I * J == K && J * I == Q{0, 0, 0, -1});

constexpr int n = 3;
using M = std::mdspan<Q, std::extents<int, n, n>>;
using V = std::mdspan<Q, std::extents<int, n>>;

// distinct, pairwise noncommuting entries
Q gen(int s) { return Q{s % 3 - 1, (s * 7) % 5 - 2, (s * 3) % 4 - 1, (s * 5) % 7 - 3}; }

bool eq(M a, M b) {
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j)
      if (!(a[i, j] == b[i, j])) return false;
  return true;
}
bool eq(V a, V b) {
  for (int i = 0; i < n; ++i)
    if (!(a[i] == b[i])) return false;
  return true;
}

int main() {
  Q ad[n * n], bd[n * n], ed[n * n], xd[n], yd[n], wd[n];
  for (int i = 0; i < n * n; ++i) ad[i] = gen(i + 1), bd[i] = gen(2 * i + 5), ed[i] = gen(3 * i + 2);
  for (int i = 0; i < n; ++i) xd[i] = gen(11 * i + 3), yd[i] = gen(5 * i + 7), wd[i] = gen(i + 13);
  M A(ad), B(bd), E(ed);
  V x(xd), y(yd), w(wd);
  CHECK(!(A[0, 1] * B[1, 0] == B[1, 0] * A[0, 1]));  // the data do not commute

  // dot: sum of v1[i] * v2[i]
  Q dref{};
  for (int i = 0; i < n; ++i) dref += x[i] * y[i];
  CHECK(la::dot(x, y) == dref);
  CHECK(la::dot(x, y, Q{}) == dref);

  // scale and scaled: alpha on the left
  {
    Q sd[n];
    V s(sd);
    la::copy(x, s);
    la::scale(J, s);
    for (int i = 0; i < n; ++i) CHECK(s[i] == J * x[i]);
    auto sx = la::scaled(K, x);
    for (int i = 0; i < n; ++i) CHECK(sx[i] == K * x[i]);
  }
  // matrix_vector_product
  {
    Q rd[n], ref[n], zd[n], zref[n];
    V r(rd), rf(ref), z(zd), zr(zref);
    for (int i = 0; i < n; ++i) {
      ref[i] = Q{};
      for (int j = 0; j < n; ++j) ref[i] += A[i, j] * x[j];
      zref[i] = w[i] + ref[i];
    }
    la::matrix_vector_product(A, x, r);
    CHECK(eq(r, rf));
    la::matrix_vector_product(A, x, w, z);
    CHECK(eq(z, zr));
    // x^T A through transposed: (A^T x)[j] = sum A[i, j] * x[i]
    for (int j = 0; j < n; ++j) {
      ref[j] = Q{};
      for (int i = 0; i < n; ++i) ref[j] += A[i, j] * x[i];
    }
    la::matrix_vector_product(la::transposed(A), x, r);
    CHECK(eq(r, rf));
  }
  // matrix_product
  {
    Q cd[n * n], refd[n * n], fd[n * n], frefd[n * n];
    M C(cd), R(refd), F(fd), FR(frefd);
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < n; ++j) {
        R[i, j] = Q{};
        for (int k = 0; k < n; ++k) R[i, j] += A[i, k] * B[k, j];
        FR[i, j] = E[i, j] + R[i, j];
      }
    la::matrix_product(A, B, C);
    CHECK(eq(C, R));
    la::matrix_product(A, B, E, F);
    CHECK(eq(F, FR));
  }
  // triangular products (lower, explicit diagonal): only the triangle is used
  {
    Q lrefd[n * n];
    M LR(lrefd);  // A's lower triangle, zeros elsewhere
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < n; ++j) LR[i, j] = j <= i ? A[i, j] : Q{};
    Q rd[n], ref[n];
    V r(rd), rf(ref);
    for (int i = 0; i < n; ++i) {
      ref[i] = Q{};
      for (int j = 0; j < n; ++j) ref[i] += LR[i, j] * x[j];
    }
    la::triangular_matrix_vector_product(A, la::lower_triangle, la::explicit_diagonal, x, r);
    CHECK(eq(r, rf));

    Q cd[n * n], refd[n * n];
    M C(cd), R(refd);
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < n; ++j) {
        R[i, j] = Q{};
        for (int k = 0; k < n; ++k) R[i, j] += LR[i, k] * B[k, j];
      }
    la::triangular_matrix_product(A, la::lower_triangle, la::explicit_diagonal, B, C);
    CHECK(eq(C, R));
    la::copy(B, C);
    la::triangular_matrix_left_product(A, la::lower_triangle, la::explicit_diagonal, C);  // C = A C
    CHECK(eq(C, R));

    for (int i = 0; i < n; ++i)
      for (int j = 0; j < n; ++j) {
        R[i, j] = Q{};
        for (int k = 0; k < n; ++k) R[i, j] += B[i, k] * LR[k, j];
      }
    la::triangular_matrix_product(B, A, la::lower_triangle, la::explicit_diagonal, C);
    CHECK(eq(C, R));
    la::copy(B, C);
    la::triangular_matrix_right_product(A, la::lower_triangle, la::explicit_diagonal, C);  // C = C A
    CHECK(eq(C, R));
  }
  // symmetric products: the matrix is A's upper triangle mirrored
  {
    Q srefd[n * n];
    M SR(srefd);
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < n; ++j) SR[i, j] = i <= j ? A[i, j] : A[j, i];
    Q cd[n * n], refd[n * n];
    M C(cd), R(refd);
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < n; ++j) {
        R[i, j] = Q{};
        for (int k = 0; k < n; ++k) R[i, j] += SR[i, k] * B[k, j];
      }
    la::symmetric_matrix_product(A, la::upper_triangle, B, C);
    CHECK(eq(C, R));
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < n; ++j) {
        R[i, j] = Q{};
        for (int k = 0; k < n; ++k) R[i, j] += B[i, k] * SR[k, j];
      }
    la::symmetric_matrix_product(B, A, la::upper_triangle, C);
    CHECK(eq(C, R));
    Q rd[n], ref[n];
    V r(rd), rf(ref);
    for (int i = 0; i < n; ++i) {
      ref[i] = Q{};
      for (int j = 0; j < n; ++j) ref[i] += SR[i, j] * x[j];
    }
    la::symmetric_matrix_vector_product(A, la::upper_triangle, x, r);
    CHECK(eq(r, rf));
  }
  // rank-1 update: A = x y^T, element x[i] * y[j]
  {
    Q cd[n * n], refd[n * n];
    M C(cd), R(refd);
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < n; ++j) R[i, j] = x[i] * y[j], C[i, j] = Q{};
    la::matrix_rank_1_update(x, y, C);
    CHECK(eq(C, R));
  }
  return 0;
}
