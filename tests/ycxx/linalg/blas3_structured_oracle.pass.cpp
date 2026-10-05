// BLAS 3 algorithms with a structured operand, checked against a dense naive oracle.
// [linalg.general]/4: a parameter m preceding t is accessed only in the triangle t; outside it
// symmetric functions use m[j, i], hermitian ones conj-if-needed(m[j, i]), triangular ones the
// additive identity; hermitian functions use real-if-needed(m[i, i]) on the diagonal. /5: with
// implicit_unit_diagonal the diagonal is not accessed and taken as 1. The unaccessed elements
// are NaN here, so any access shows up in the result.
// [linalg.algs.blas3.xxmm]/9, /13, /17, /22: symmetric_matrix_product, hermitian_matrix_product
// and triangular_matrix_product compute C = A B or C = E + A B, the structured operand on the
// left (A, t, B, ...) or on the right (A, B, t, ...); /18, /23: C may alias E.
// [linalg.algs.blas3.trmm]/4, /8: triangular_matrix_left_product(A, t, d, C) sets C = A C and
// triangular_matrix_right_product sets C = C A.
// [linalg.algs.blas3.rankk]/3, /8-/11: symmetric_matrix_rank_k_update(alpha, A, [E,] C, t)
// computes C = [E +] alpha A A^T, hermitian_... C = [E +] real-if-needed(alpha) A A^H; t applies
// to C (only that triangle is written; the other keeps its values) and to E. /7: C may alias E.
// [linalg.algs.blas3.rank2k]/8-/11: C = [E +] A B^T + B A^T (A B^H + B A^H for hermitian).
// [linalg.algs.blas3.trsm]/5, /13 and [linalg.algs.blas3.inplacetrsm]: left / right solves
// A X = B and X A = B, out of place and in place, with explicit and implicit unit diagonals.
#include <linalg>
#include <cmath>
#include <complex>
#include <limits>
#include <mdspan>
#include <vector>
#include "check.hpp"

namespace la = std::linalg;
using cd = std::complex<double>;
using M2 = std::dextents<int, 2>;
const double qnan = std::numeric_limits<double>::quiet_NaN();

double cj(double x) { return x; }
cd cj(cd x) { return std::conj(x); }
double re(double x) { return x; }
cd re(cd x) { return cd(x.real(), 0); }
double with_imag(double x, double) { return x; }
cd with_imag(cd x, double im) { return cd(x.real(), im); }
template <class T> T poison();
template <> double poison<double>() { return qnan; }
template <> cd poison<cd>() { return cd(qnan, qnan); }

template <class T>
struct Dense {
  int r, c;
  std::vector<T> v;
  Dense(int r_, int c_) : r(r_), c(c_), v(static_cast<std::size_t>(r_ * c_)) {}
  T& operator()(int i, int j) { return v[static_cast<std::size_t>(i * c + j)]; }
  T operator()(int i, int j) const { return v[static_cast<std::size_t>(i * c + j)]; }
  std::mdspan<T, M2> md() { return std::mdspan<T, M2>(v.data(), r, c); }
};

unsigned seed = 12345;
int rnd() {
  seed = seed * 1103515245u + 12345u;
  return static_cast<int>((seed >> 16) % 9) - 4;  // -4 .. 4
}
template <class T> T rval();
template <> double rval<double>() { return rnd(); }
template <> cd rval<cd>() { return cd(rnd(), rnd()); }

template <class T>
Dense<T> random(int r, int c) {
  Dense<T> d(r, c);
  for (auto& x : d.v) x = rval<T>();
  return d;
}

enum Kind { Sym, Herm, TriExplicit, TriUnit };

bool in_tri(bool upper, int i, int j) { return upper ? i <= j : i >= j; }

// The matrix that a function sees through (m, t[, d]), read only from the allowed elements.
template <class T>
Dense<T> interpret(const Dense<T>& m, bool upper, Kind k) {
  Dense<T> out(m.r, m.c);
  for (int i = 0; i < m.r; ++i)
    for (int j = 0; j < m.c; ++j) {
      if (i == j) {
        out(i, j) = k == Herm ? re(m(i, i)) : k == TriUnit ? T(1) : m(i, i);
      } else if (in_tri(upper, i, j)) {
        out(i, j) = m(i, j);
      } else {
        out(i, j) = k == Sym ? m(j, i) : k == Herm ? cj(m(j, i)) : T(0);
      }
    }
  return out;
}

// Overwrite every element the function may not access.
template <class T>
void poison_outside(Dense<T>& m, bool upper, Kind k) {
  for (int i = 0; i < m.r; ++i)
    for (int j = 0; j < m.c; ++j) {
      if (i == j) {
        if (k == TriUnit) m(i, i) = poison<T>();
        if (k == Herm) m(i, i) = with_imag(m(i, i), 999);  // only the real part may be used
      } else if (!in_tri(upper, i, j)) {
        m(i, j) = poison<T>();
      }
    }
}

template <class T>
Dense<T> mul(const Dense<T>& a, const Dense<T>& b) {
  Dense<T> out(a.r, b.c);
  for (int i = 0; i < a.r; ++i)
    for (int j = 0; j < b.c; ++j) {
      T s{};
      for (int k = 0; k < a.c; ++k) s += a(i, k) * b(k, j);
      out(i, j) = s;
    }
  return out;
}
template <class T>
Dense<T> add(const Dense<T>& a, const Dense<T>& b) {
  Dense<T> out = a;
  for (std::size_t i = 0; i < out.v.size(); ++i) out.v[i] += b.v[i];
  return out;
}
template <class T>
Dense<T> trans(const Dense<T>& a, bool conjugate) {
  Dense<T> out(a.c, a.r);
  for (int i = 0; i < a.r; ++i)
    for (int j = 0; j < a.c; ++j) out(j, i) = conjugate ? cj(a(i, j)) : a(i, j);
  return out;
}
template <class T>
Dense<T> scale(T s, Dense<T> a) {
  for (auto& x : a.v) x *= s;
  return a;
}

template <class T>
bool same(const Dense<T>& a, const Dense<T>& b) {
  if (a.r != b.r || a.c != b.c) return false;
  for (std::size_t i = 0; i < a.v.size(); ++i)
    if (!(a.v[i] == b.v[i])) return false;
  return true;
}
template <class T>
bool close(const Dense<T>& a, const Dense<T>& b) {
  if (a.r != b.r || a.c != b.c) return false;
  for (std::size_t i = 0; i < a.v.size(); ++i)
    if (!(std::abs(a.v[i] - b.v[i]) <= 1e-9 * (1 + std::abs(b.v[i])))) return false;
  return true;
}
// Elements of the t-triangle equal want's; the others keep before's.
template <class T>
bool same_in_triangle(const Dense<T>& got, const Dense<T>& want, const Dense<T>& before, bool upper) {
  for (int i = 0; i < got.r; ++i)
    for (int j = 0; j < got.c; ++j) {
      T expect = in_tri(upper, i, j) ? want(i, j) : before(i, j);
      bool both_nan = got(i, j) != got(i, j) && expect != expect;
      if (!(got(i, j) == expect) && !both_nan) return false;
    }
  return true;
}

template <class F>
void both_triangles(F f) {
  f(la::upper_triangle, true);
  f(la::lower_triangle, false);
}
template <class F>
void both_diagonals(F f) {
  f(la::explicit_diagonal, TriExplicit);
  f(la::implicit_unit_diagonal, TriUnit);
}

template <class T>
void structured_products(bool hermitian) {
  const Kind sk = hermitian ? Herm : Sym;
  auto symm_left = [&](auto A, auto t, auto B, auto C) {
    if constexpr (std::is_same_v<T, cd>) {
      if (hermitian) return la::hermitian_matrix_product(A, t, B, C);
    }
    la::symmetric_matrix_product(A, t, B, C);
  };
  auto symm_right = [&](auto A, auto B, auto t, auto C) {
    if constexpr (std::is_same_v<T, cd>) {
      if (hermitian) return la::hermitian_matrix_product(A, B, t, C);
    }
    la::symmetric_matrix_product(A, B, t, C);
  };
  auto symm_left_e = [&](auto A, auto t, auto B, auto E, auto C) {
    if constexpr (std::is_same_v<T, cd>) {
      if (hermitian) return la::hermitian_matrix_product(A, t, B, E, C);
    }
    la::symmetric_matrix_product(A, t, B, E, C);
  };
  auto symm_right_e = [&](auto A, auto B, auto t, auto E, auto C) {
    if constexpr (std::is_same_v<T, cd>) {
      if (hermitian) return la::hermitian_matrix_product(A, B, t, E, C);
    }
    la::symmetric_matrix_product(A, B, t, E, C);
  };
  both_triangles([&](auto t, bool upper) {
    for (int n : {1, 2, 3, 5}) {
      for (int m : {1, 4}) {
        Dense<T> S = random<T>(n, n);
        Dense<T> full = interpret(S, upper, sk);
        poison_outside(S, upper, sk);
        CHECK(same(interpret(S, upper, sk), full));
        // C = S B (S on the left)
        Dense<T> B = random<T>(n, m), C(n, m);
        symm_left(S.md(), t, B.md(), C.md());
        CHECK(same(C, mul(full, B)));
        // C = E + S B, then C aliasing E
        Dense<T> E = random<T>(n, m);
        symm_left_e(S.md(), t, B.md(), E.md(), C.md());
        CHECK(same(C, add(E, mul(full, B))));
        Dense<T> CE = E;
        symm_left_e(S.md(), t, B.md(), CE.md(), CE.md());
        CHECK(same(CE, add(E, mul(full, B))));
        // C = B' S (S on the right)
        Dense<T> B2 = random<T>(m, n), C2(m, n), E2 = random<T>(m, n);
        symm_right(B2.md(), S.md(), t, C2.md());
        CHECK(same(C2, mul(B2, full)));
        symm_right_e(B2.md(), S.md(), t, E2.md(), C2.md());
        CHECK(same(C2, add(E2, mul(B2, full))));
      }
    }
  });
}

template <class T>
void triangular_products() {
  both_triangles([&](auto t, bool upper) {
    both_diagonals([&](auto d, Kind k) {
      for (int n : {1, 2, 4}) {
        for (int m : {1, 3}) {
          Dense<T> A = random<T>(n, n);
          Dense<T> full = interpret(A, upper, k);
          poison_outside(A, upper, k);
          Dense<T> B = random<T>(n, m), C(n, m), E = random<T>(n, m);
          la::triangular_matrix_product(A.md(), t, d, B.md(), C.md());
          CHECK(same(C, mul(full, B)));
          la::triangular_matrix_product(A.md(), t, d, B.md(), E.md(), C.md());
          CHECK(same(C, add(E, mul(full, B))));
          Dense<T> CE = E;
          la::triangular_matrix_product(A.md(), t, d, B.md(), CE.md(), CE.md());
          CHECK(same(CE, add(E, mul(full, B))));
          Dense<T> B2 = random<T>(m, n), C2(m, n), E2 = random<T>(m, n);
          la::triangular_matrix_product(B2.md(), A.md(), t, d, C2.md());
          CHECK(same(C2, mul(B2, full)));
          la::triangular_matrix_product(B2.md(), A.md(), t, d, E2.md(), C2.md());
          CHECK(same(C2, add(E2, mul(B2, full))));
          // in place
          Dense<T> L = B;
          la::triangular_matrix_left_product(A.md(), t, d, L.md());
          CHECK(same(L, mul(full, B)));
          Dense<T> R = B2;
          la::triangular_matrix_right_product(A.md(), t, d, R.md());
          CHECK(same(R, mul(B2, full)));
        }
      }
    });
  });
}

template <class T>
void rank_updates(bool hermitian) {
  const Kind sk = hermitian ? Herm : Sym;
  both_triangles([&](auto t, bool upper) {
    for (int n : {1, 3, 4}) {
      for (int k : {1, 2, 5}) {
        Dense<T> A = random<T>(n, k), B = random<T>(n, k);
        Dense<T> before = random<T>(n, n);
        Dense<T> At = trans(A, hermitian), Bt = trans(B, hermitian);
        // rank-k, overwriting
        Dense<T> C = before;
        T alpha = T(3);
        Dense<T> want(n, n);
        if constexpr (std::is_same_v<T, cd>) {
          if (hermitian) {
            // the scalar used is real-if-needed(alpha): the imaginary part is ignored
            la::hermitian_matrix_rank_k_update(cd(3, 7), A.md(), C.md(), t);
            want = scale(alpha, mul(A, At));
          }
        }
        if (!hermitian) {
          la::symmetric_matrix_rank_k_update(alpha, A.md(), C.md(), t);
          want = scale(alpha, mul(A, At));
        }
        CHECK(same_in_triangle(C, want, before, upper));
        // rank-k with E (E is read only in its t-triangle), and C aliasing E
        Dense<T> E = random<T>(n, n);
        Dense<T> Efull = interpret(E, upper, sk);
        poison_outside(E, upper, sk);
        C = before;
        if constexpr (std::is_same_v<T, cd>) {
          if (hermitian) la::hermitian_matrix_rank_k_update(cd(3, -1), A.md(), E.md(), C.md(), t);
        }
        if (!hermitian) la::symmetric_matrix_rank_k_update(alpha, A.md(), E.md(), C.md(), t);
        want = add(Efull, scale(alpha, mul(A, At)));
        CHECK(same_in_triangle(C, want, before, upper));
        Dense<T> CE = E;
        if constexpr (std::is_same_v<T, cd>) {
          if (hermitian) la::hermitian_matrix_rank_k_update(alpha, A.md(), CE.md(), CE.md(), t);
        }
        if (!hermitian) la::symmetric_matrix_rank_k_update(alpha, A.md(), CE.md(), CE.md(), t);
        CHECK(same_in_triangle(CE, want, E, upper));
        // rank-2k, overwriting and with E
        C = before;
        if constexpr (std::is_same_v<T, cd>) {
          if (hermitian) la::hermitian_matrix_rank_2k_update(A.md(), B.md(), C.md(), t);
        }
        if (!hermitian) la::symmetric_matrix_rank_2k_update(A.md(), B.md(), C.md(), t);
        want = add(mul(A, Bt), mul(B, At));
        CHECK(same_in_triangle(C, want, before, upper));
        C = before;
        if constexpr (std::is_same_v<T, cd>) {
          if (hermitian) la::hermitian_matrix_rank_2k_update(A.md(), B.md(), E.md(), C.md(), t);
        }
        if (!hermitian) la::symmetric_matrix_rank_2k_update(A.md(), B.md(), E.md(), C.md(), t);
        want = add(Efull, add(mul(A, Bt), mul(B, At)));
        CHECK(same_in_triangle(C, want, before, upper));
      }
    }
  });
}

template <class T>
void solves() {
  both_triangles([&](auto t, bool upper) {
    both_diagonals([&](auto d, Kind k) {
      for (int n : {1, 2, 3, 5}) {
        for (int m : {1, 4}) {
          Dense<T> A = random<T>(n, n);
          for (int i = 0; i < n; ++i) A(i, i) = T(i % 2 ? -2 : 4);  // nonsingular
          Dense<T> full = interpret(A, upper, k);
          poison_outside(A, upper, k);
          Dense<T> X = random<T>(n, m);
          Dense<T> B = mul(full, X);
          Dense<T> got(n, m);
          la::triangular_matrix_matrix_left_solve(A.md(), t, d, B.md(), got.md());
          CHECK(close(got, X));
          Dense<T> inplace = B;
          la::triangular_matrix_matrix_left_solve(A.md(), t, d, inplace.md());
          CHECK(close(inplace, X));
          Dense<T> X2 = random<T>(m, n);
          Dense<T> B2 = mul(X2, full);
          Dense<T> got2(m, n);
          la::triangular_matrix_matrix_right_solve(A.md(), t, d, B2.md(), got2.md());
          CHECK(close(got2, X2));
          Dense<T> inplace2 = B2;
          la::triangular_matrix_matrix_right_solve(A.md(), t, d, inplace2.md());
          CHECK(close(inplace2, X2));
          // with an explicit divide operation
          auto div = [](T a, T b) { return a / b; };
          la::triangular_matrix_matrix_left_solve(A.md(), t, d, B.md(), got.md(), div);
          CHECK(close(got, X));
          la::triangular_matrix_matrix_right_solve(A.md(), t, d, B2.md(), got2.md(), div);
          CHECK(close(got2, X2));
        }
      }
    });
  });
}

int main() {
  structured_products<double>(false);
  structured_products<cd>(false);
  structured_products<cd>(true);
  triangular_products<double>();
  triangular_products<cd>();
  rank_updates<double>(false);
  rank_updates<cd>(false);
  rank_updates<cd>(true);
  solves<double>();
  solves<cd>();
}
