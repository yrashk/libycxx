// [linalg.algs.blas2.hemv] hermitian_matrix_vector_product and [linalg.algs.blas2.trmv]
// triangular_matrix_vector_product, all overloads (overwriting, updating with z aliasing y,
// in place; with and without an execution policy), checked against a dense oracle and with an
// accessor that records every element the algorithm reads.
//   [linalg.general]/4: "F will only access the triangle of m specified by t. For accesses of
//     diagonal elements m[i, i], F will use the value real-if-needed(m[i, i]) if the name of F
//     starts with hermitian. For accesses m[i, j] outside the triangle specified by t, F will
//     use the value conj-if-needed(m[j, i]) if the name of F starts with hermitian, ..." and
//     for triangular functions the elements outside the triangle are zero.
//   [linalg.general]/5: with implicit_unit_diagonal "F will not access the diagonal of m" and
//     interprets it as a unit diagonal; with explicit_diagonal it accesses it.
//   hemv /7: y = A x; /9: z = y + A x; /10 "z may alias y". trmv /6: y = A x; /9: in place
//     y' = A y assigned to y; /12: z = y + A x; /14: z may alias y.
//   [linalg.layout.packed]: the same products through layout_blas_packed (only the triangle
//     is stored), column_major and row_major.
// The elements outside the triangle (and the diagonal for implicit_unit_diagonal, and the
// diagonal's imaginary part for the Hermitian functions) hold NaN, so reading and using them
// would also show in the results; the values are small integers, so every result is exact.
#include <linalg>
#include <cmath>
#include <complex>
#include <cstddef>
#include <execution>
#include <mdspan>
#include <vector>
#include "check.hpp"

namespace la = std::linalg;
using C = std::complex<double>;
constexpr int n = 4;

template <class T>
struct rec_acc {
  using offset_policy = rec_acc;
  using element_type = const T;
  using reference = const T&;
  using data_handle_type = const T*;
  bool* touched = nullptr;
  constexpr rec_acc() noexcept = default;
  constexpr explicit rec_acc(bool* t) noexcept : touched(t) {}
  constexpr reference access(data_handle_type p, std::size_t i) const noexcept {
    if (touched) touched[i] = true;
    return p[i];
  }
  constexpr data_handle_type offset(data_handle_type p, std::size_t i) const noexcept { return p + i; }
};

template <class T>
using RecMat = std::mdspan<const T, std::dextents<int, 2>, std::layout_right, rec_acc<T>>;

static double nan() { return std::nan(""); }
template <class T>
T poison() {
  if constexpr (std::is_same_v<T, C>)
    return C(nan(), nan());
  else
    return nan();
}
template <class T>
T val(int i, int j) {
  if constexpr (std::is_same_v<T, C>)
    return C(i + 2 * j + 1, i - j);
  else
    return T(i + 2 * j + 1);
}
template <class T>
T conj_if(T v) {
  if constexpr (std::is_same_v<T, C>)
    return std::conj(v);
  else
    return v;
}
template <class T>
T real_if(T v) {
  if constexpr (std::is_same_v<T, C>)
    return C(v.real(), 0);
  else
    return v;
}
static bool in_tri(bool upper, int i, int j) { return upper ? i <= j : i >= j; }

enum class Kind { herm, tri_explicit, tri_implicit };

// The stored matrix: the triangle's values; NaN elsewhere (and where the algorithm must not look).
template <class T>
std::vector<T> stored(Kind k, bool upper) {
  std::vector<T> a(n * n);
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) {
      T v = val<T>(i, j);
      if (!in_tri(upper, i, j)) v = poison<T>();
      if (i == j && k == Kind::tri_implicit) v = poison<T>();
      if (i == j && k == Kind::herm) {
        if constexpr (std::is_same_v<T, C>) v = C(v.real(), nan());
      }
      a[i * n + j] = v;
    }
  return a;
}
// The matrix the algorithm must use.
template <class T>
T effective(Kind k, bool upper, int i, int j) {
  if (k == Kind::herm) {
    if (i == j) return real_if(val<T>(i, i));
    return in_tri(upper, i, j) ? val<T>(i, j) : conj_if(val<T>(j, i));
  }
  if (i == j) return k == Kind::tri_implicit ? T(1) : val<T>(i, i);
  return in_tri(upper, i, j) ? val<T>(i, j) : T(0);
}
template <class T>
std::vector<T> oracle(Kind k, bool upper, const std::vector<T>& x, const std::vector<T>* y) {
  std::vector<T> r(n);
  for (int i = 0; i < n; ++i) {
    T s = y ? (*y)[i] : T(0);
    for (int j = 0; j < n; ++j) s += effective<T>(k, upper, i, j) * x[j];
    r[i] = s;
  }
  return r;
}
template <class T>
bool same(const std::vector<T>& a, const std::vector<T>& b) {
  for (int i = 0; i < n; ++i)
    if (!(a[i] == b[i])) return false;
  return true;
}
template <class T>
bool access_ok(Kind k, bool upper, const bool* touched) {
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j)
      if (touched[i * n + j] && (!in_tri(upper, i, j) || (i == j && k == Kind::tri_implicit))) {
        dprintf(2, "accessed A[%d,%d] (kind %d, upper %d)\n", i, j, int(k), int(upper));
        return false;
      }
  return true;
}

template <class T, Kind k, class Tri, class Policy>
void run(Tri t, Policy pol) {
  const bool upper = std::is_same_v<Tri, la::upper_triangle_t>;
  std::vector<T> a = stored<T>(k, upper);
  std::vector<T> x(n), y0(n);
  for (int i = 0; i < n; ++i) {
    x[i] = val<T>(i, 3 - i) - T(3);
    y0[i] = val<T>(2 * i, 1);
  }
  bool touched[n * n] = {};
  RecMat<T> A(a.data(), std::layout_right::mapping<std::dextents<int, 2>>(std::dextents<int, 2>(n, n)), rec_acc<T>(touched));
  using V = std::mdspan<T, std::dextents<int, 1>>;
  using CV = std::mdspan<const T, std::dextents<int, 1>>;
  auto call = [&](auto&&... args) {
    if constexpr (std::is_same_v<Policy, std::nullptr_t>) {
      if constexpr (k == Kind::herm)
        la::hermitian_matrix_vector_product(args...);
      else if constexpr (k == Kind::tri_explicit)
        la::triangular_matrix_vector_product(A, t, la::explicit_diagonal, args...);
      else
        la::triangular_matrix_vector_product(A, t, la::implicit_unit_diagonal, args...);
    } else {
      if constexpr (k == Kind::herm)
        la::hermitian_matrix_vector_product(pol, args...);
      else if constexpr (k == Kind::tri_explicit)
        la::triangular_matrix_vector_product(pol, A, t, la::explicit_diagonal, args...);
      else
        la::triangular_matrix_vector_product(pol, A, t, la::implicit_unit_diagonal, args...);
    }
  };
  // The Hermitian calls take (A, t, ...) themselves; the triangular ones get them from call.
  auto overwrite = [&](CV xv, V yv) {
    if constexpr (k == Kind::herm)
      call(A, t, xv, yv);
    else
      call(xv, yv);
  };
  auto update = [&](CV xv, CV yv, V zv) {
    if constexpr (k == Kind::herm)
      call(A, t, xv, yv, zv);
    else
      call(xv, yv, zv);
  };

  std::vector<T> y(n, poison<T>());
  overwrite(CV(x.data(), n), V(y.data(), n));
  CHECK(same(y, oracle<T>(k, upper, x, nullptr)));
  CHECK(access_ok<T>(k, upper, touched));
  CHECK(touched[upper ? 1 : n] && (k == Kind::tri_implicit || touched[0]));  // the recording works

  std::vector<T> z(n, poison<T>());
  update(CV(x.data(), n), CV(y0.data(), n), V(z.data(), n));
  CHECK(same(z, oracle<T>(k, upper, x, &y0)));
  CHECK(access_ok<T>(k, upper, touched));

  std::vector<T> yz = y0;  // z aliases y
  update(CV(x.data(), n), CV(yz.data(), n), V(yz.data(), n));
  CHECK(same(yz, oracle<T>(k, upper, x, &y0)));

  if constexpr (k != Kind::herm) {
    std::vector<T> in = x;
    const auto d = la::explicit_diagonal;
    const auto u = la::implicit_unit_diagonal;
    if constexpr (std::is_same_v<Policy, std::nullptr_t>) {
      if constexpr (k == Kind::tri_explicit)
        la::triangular_matrix_vector_product(A, t, d, V(in.data(), n));
      else
        la::triangular_matrix_vector_product(A, t, u, V(in.data(), n));
    } else {
      if constexpr (k == Kind::tri_explicit)
        la::triangular_matrix_vector_product(pol, A, t, d, V(in.data(), n));
      else
        la::triangular_matrix_vector_product(pol, A, t, u, V(in.data(), n));
    }
    CHECK(same(in, oracle<T>(k, upper, x, nullptr)));
    CHECK(access_ok<T>(k, upper, touched));
  }
}

// layout_blas_packed: only the triangle is stored.
template <class T, class Tri, class Order>
void packed(Tri t) {
  const bool upper = std::is_same_v<Tri, la::upper_triangle_t>;
  using L = la::layout_blas_packed<Tri, Order>;
  using M = typename L::template mapping<std::dextents<int, 2>>;
  M map(std::dextents<int, 2>(n, n));
  std::vector<T> storage(map.required_span_size());
  std::mdspan<T, std::dextents<int, 2>, L> P(storage.data(), map);
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j)
      if (in_tri(upper, i, j)) P[i, j] = val<T>(i, j);
  std::vector<T> x(n), y(n), z(n), y0(n, T(1));
  for (int i = 0; i < n; ++i) x[i] = val<T>(i, 0);
  using V = std::mdspan<T, std::dextents<int, 1>>;
  using CV = std::mdspan<const T, std::dextents<int, 1>>;
  if constexpr (std::is_same_v<T, C>) {
    // The packed diagonal stores real-valued diagonal entries for a Hermitian matrix.
    for (int i = 0; i < n; ++i) P[i, i] = real_if(val<T>(i, i));
    la::hermitian_matrix_vector_product(P, t, CV(x.data(), n), V(y.data(), n));
    CHECK(same(y, oracle<T>(Kind::herm, upper, x, nullptr)));
    la::hermitian_matrix_vector_product(P, t, CV(x.data(), n), CV(y0.data(), n), V(z.data(), n));
    CHECK(same(z, oracle<T>(Kind::herm, upper, x, &y0)));
  } else {
    la::triangular_matrix_vector_product(P, t, la::explicit_diagonal, CV(x.data(), n), V(y.data(), n));
    CHECK(same(y, oracle<T>(Kind::tri_explicit, upper, x, nullptr)));
    std::vector<T> in = x;
    la::triangular_matrix_vector_product(P, t, la::implicit_unit_diagonal, V(in.data(), n));
    CHECK(same(in, oracle<T>(Kind::tri_implicit, upper, x, nullptr)));
    la::triangular_matrix_vector_product(P, t, la::explicit_diagonal, CV(x.data(), n), CV(y0.data(), n), V(z.data(), n));
    CHECK(same(z, oracle<T>(Kind::tri_explicit, upper, x, &y0)));
  }
}

template <class T, class Policy>
void all(Policy pol) {
  run<T, Kind::herm>(la::upper_triangle, pol);
  run<T, Kind::herm>(la::lower_triangle, pol);
  run<T, Kind::tri_explicit>(la::upper_triangle, pol);
  run<T, Kind::tri_explicit>(la::lower_triangle, pol);
  run<T, Kind::tri_implicit>(la::upper_triangle, pol);
  run<T, Kind::tri_implicit>(la::lower_triangle, pol);
}

int main() {
  all<double>(nullptr);
  all<C>(nullptr);
  all<double>(std::execution::par);
  all<C>(std::execution::seq);
  packed<double, la::upper_triangle_t, la::column_major_t>(la::upper_triangle);
  packed<double, la::lower_triangle_t, la::row_major_t>(la::lower_triangle);
  packed<C, la::upper_triangle_t, la::row_major_t>(la::upper_triangle);
  packed<C, la::lower_triangle_t, la::column_major_t>(la::lower_triangle);
  return 0;
}
