// [linalg.reqs.alg]/2.2: the triangular solve algorithms take a BinaryDivideOp divide, interpret
// divide(a, b) as a times the multiplicative inverse of b, and use *, divide, +, -, ... "that
// would produce the result specified by the algorithm's Effects and Remarks when operating on
// elements of a field with noncommutative multiplication". [linalg.algs.blas3.trsm] Note 2 /
// Note 3: with the triangular matrix on the left the divide to pass is y^-1 x, on the right
// x y^-1.
// [linalg.algs.blas2.trsv]/6, /11: x with b = A x (into x, or in place into b);
// [linalg.algs.blas3.trsm]/5, /13: X with A X = B and X A = B;
// [linalg.algs.blas3.inplacetrsm]: the same, written into B.
// [linalg.general]/4-/5: only the triangle t of A is accessed; with implicit_unit_diagonal the
// diagonal is not accessed and taken as one.
// The values are integer quaternions; the diagonal entries are units (+-1, +-i, +-j, +-k), whose
// inverse is their conjugate, so every solution is exact. The elements outside the triangle (and
// the diagonal, for implicit_unit_diagonal) are huge, so using them would show.
// The element type has no operator/, so only the divide argument can divide.
#include <linalg>
#include <mdspan>
#include <type_traits>
#include "check.hpp"

namespace la = std::linalg;

struct Q {
  long a = 0, b = 0, c = 0, d = 0;
  friend constexpr Q operator+(const Q& x, const Q& y) { return {x.a + y.a, x.b + y.b, x.c + y.c, x.d + y.d}; }
  friend constexpr Q operator-(const Q& x, const Q& y) { return {x.a - y.a, x.b - y.b, x.c - y.c, x.d - y.d}; }
  friend constexpr Q operator-(const Q& x) { return {-x.a, -x.b, -x.c, -x.d}; }
  friend constexpr Q operator+(const Q& x) { return x; }
  friend constexpr Q operator*(const Q& x, const Q& y) {
    return {x.a * y.a - x.b * y.b - x.c * y.c - x.d * y.d, x.a * y.b + x.b * y.a + x.c * y.d - x.d * y.c,
            x.a * y.c - x.b * y.d + x.c * y.a + x.d * y.b, x.a * y.d + x.b * y.c - x.c * y.b + x.d * y.a};
  }
  constexpr Q& operator+=(const Q& y) { return *this = *this + y; }
  constexpr Q& operator-=(const Q& y) { return *this = *this - y; }
  constexpr Q& operator*=(const Q& y) { return *this = *this * y; }
  friend constexpr bool operator==(const Q&, const Q&) = default;
};
constexpr Q conjq(const Q& q) { return {q.a, -q.b, -q.c, -q.d}; }
constexpr Q one{1, 0, 0, 0};
// divide by a unit quaternion
inline constexpr auto left_div = [](const Q& x, const Q& y) { return conjq(y) * x; };   // y^-1 x
inline constexpr auto right_div = [](const Q& x, const Q& y) { return x * conjq(y); };  // x y^-1

constexpr int n = 4;
using M = std::mdspan<Q, std::extents<int, n, n>>;
using V = std::mdspan<Q, std::extents<int, n>>;
constexpr Q huge{1000000, 1000000, 1000000, 1000000};
constexpr Q units[] = {{0, 1, 0, 0}, {0, 0, -1, 0}, {0, 0, 0, 1}, {-1, 0, 0, 0}};

Q gen(int s) { return Q{s % 3 - 1, (s * 7) % 5 - 2, (s * 3) % 4 - 1, (s * 5) % 7 - 3}; }

// A with the given triangle; the other entries huge; the diagonal units (or huge if implicit)
void make(M A, bool lower, bool implicit) {
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) {
      bool in = lower ? j < i : j > i;
      A[i, j] = i == j ? (implicit ? huge : units[i]) : in ? gen(5 * i + j + 1) : huge;
    }
}
// the triangular matrix A stands for
Q tri(M A, int i, int j, bool lower, bool implicit) {
  if (i == j) return implicit ? one : A[i, j];
  return (lower ? j < i : j > i) ? A[i, j] : Q{};
}

template <class T, class D>
void vector_case(T t, D d) {
  constexpr bool lower = std::is_same_v<T, la::lower_triangle_t>;
  constexpr bool implicit = std::is_same_v<D, la::implicit_unit_diagonal_t>;
  Q ad[n * n], x0d[n], bd[n], xd[n];
  M A(ad);
  V x0(x0d), b(bd), x(xd);
  make(A, lower, implicit);
  for (int i = 0; i < n; ++i) x0[i] = gen(3 * i + 2);
  for (int i = 0; i < n; ++i) {
    b[i] = Q{};
    for (int j = 0; j < n; ++j) b[i] += tri(A, i, j, lower, implicit) * x0[j];
  }
  la::triangular_matrix_vector_solve(A, t, d, b, x, left_div);
  for (int i = 0; i < n; ++i) CHECK(x[i] == x0[i]);
  la::triangular_matrix_vector_solve(A, t, d, b, left_div);  // in place
  for (int i = 0; i < n; ++i) CHECK(b[i] == x0[i]);
}

template <class T, class D>
void matrix_case(T t, D d) {
  constexpr bool lower = std::is_same_v<T, la::lower_triangle_t>;
  constexpr bool implicit = std::is_same_v<D, la::implicit_unit_diagonal_t>;
  Q ad[n * n], x0d[n * n], bd[n * n], xd[n * n], b2d[n * n];
  M A(ad), X0(x0d), B(bd), X(xd), B2(b2d);
  make(A, lower, implicit);
  for (int i = 0; i < n * n; ++i) x0d[i] = gen(7 * i + 3);
  // left: B = A X0
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) {
      B[i, j] = Q{};
      for (int k = 0; k < n; ++k) B[i, j] += tri(A, i, k, lower, implicit) * X0[k, j];
    }
  la::triangular_matrix_matrix_left_solve(A, t, d, B, X, left_div);
  for (int i = 0; i < n * n; ++i) CHECK(xd[i] == x0d[i]);
  la::copy(B, B2);
  la::triangular_matrix_matrix_left_solve(A, t, d, B2, left_div);
  for (int i = 0; i < n * n; ++i) CHECK(b2d[i] == x0d[i]);
  // right: B = X0 A
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) {
      B[i, j] = Q{};
      for (int k = 0; k < n; ++k) B[i, j] += X0[i, k] * tri(A, k, j, lower, implicit);
    }
  la::triangular_matrix_matrix_right_solve(A, t, d, B, X, right_div);
  for (int i = 0; i < n * n; ++i) CHECK(xd[i] == x0d[i]);
  la::copy(B, B2);
  la::triangular_matrix_matrix_right_solve(A, t, d, B2, right_div);
  for (int i = 0; i < n * n; ++i) CHECK(b2d[i] == x0d[i]);
}

int main() {
  vector_case(la::lower_triangle, la::explicit_diagonal);
  vector_case(la::lower_triangle, la::implicit_unit_diagonal);
  vector_case(la::upper_triangle, la::explicit_diagonal);
  vector_case(la::upper_triangle, la::implicit_unit_diagonal);
  matrix_case(la::lower_triangle, la::explicit_diagonal);
  matrix_case(la::lower_triangle, la::implicit_unit_diagonal);
  matrix_case(la::upper_triangle, la::explicit_diagonal);
  matrix_case(la::upper_triangle, la::implicit_unit_diagonal);
  return 0;
}
