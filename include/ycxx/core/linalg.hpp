// libycxx core: <linalg> part 2, the BLAS 1, 2 and 3 algorithms ([linalg.algs]).
//
// Straightforward loops over mdspan element access (no blocking, no vendor BLAS). The
// ExecutionPolicy overloads run sequentially (DECISIONS §3, as for <algorithm>) and are
// noexcept: an exception escaping a parallel algorithm calls terminate. Sums are accumulated in
// the output's value_type (or Scalar), products keep the operand order of the specification
// (multiplication need not be commutative). The 2-norms scale their sum of squares (LAPACK's
// xLASSQ) so that they neither overflow nor underflow for representable results.
#pragma once

#include <ycxx/core/functional_base.hpp>
#include <ycxx/core/linalg_base.hpp>

namespace ycxx::detail {

template <class M>
constexpr std::size_t la_n(const M& m, std::size_t r) noexcept {
  return static_cast<std::size_t>(m.extent(r));
}
// Element access with size_t indices.
template <class M>
constexpr decltype(auto) la_v(const M& v, std::size_t i) {
  return v[static_cast<typename M::index_type>(i)];
}
template <class M>
constexpr decltype(auto) la_m(const M& a, std::size_t i, std::size_t j) {
  using I = typename M::index_type;
  return a[static_cast<I>(i), static_cast<I>(j)];
}

template <class Triangle>
constexpr bool la_in_triangle(std::size_t i, std::size_t j) noexcept {
  if constexpr (std::is_same_v<Triangle, std::linalg::upper_triangle_t>)
    return i <= j;
  else
    return i >= j;
}

// The value of a symmetric or Hermitian matrix at (i, j), reading only the triangle t
// ([linalg.general]/4).
enum class la_structure { symmetric, hermitian };
template <la_structure S, class Triangle, class M>
constexpr typename M::value_type la_sym(const M& a, std::size_t i, std::size_t j) {
  using V = typename M::value_type;
  if constexpr (S == la_structure::hermitian) {
    if (i == j)
      return V(la_adl::real_if_needed(la_m(a, i, i)));
    if (la_in_triangle<Triangle>(i, j))
      return V(la_m(a, i, j));
    return V(la_adl::conj_if_needed(la_m(a, j, i)));
  } else {
    if (la_in_triangle<Triangle>(i, j))
      return V(la_m(a, i, j));
    return V(la_m(a, j, i));
  }
}

template <class D>
inline constexpr bool la_unit = std::is_same_v<D, std::linalg::implicit_unit_diagonal_t>;

// The absent E of the overwriting overloads (an E parameter is passed by address).
struct la_none {};
inline constexpr const la_none* la_no_e = nullptr;
// "addable(A, E, A) is true for those overloads with an E parameter".
template <class E, class A>
constexpr bool la_e_addable(const E* e, const A& a) {
  if constexpr (std::is_same_v<E, la_none>)
    return true;
  else
    return la_addable(a, *e, a);
}

// A matrix argument that is neither a function object nor an mdspan: the divide operation.
template <class T>
concept la_divide_op = !md_is_mdspan<T>;

// ---------------------------------------------------------------------------------------------
// Scaled sum of squares: the state of LAPACK's xLASSQ (scale * scale * ssq is the sum).
// ---------------------------------------------------------------------------------------------
template <class R>
struct la_ssq {
  R scale{};
  R ssq{};
  bool nan = false, inf = false;

  constexpr void add(R a) {
    if (a != a) {
      nan = true;
    } else if (a > std::numeric_limits<R>::max()) {
      inf = true;
    } else if (a != R(0)) {
      if (scale < a) {
        R q = scale / a;
        ssq = R(1) + ssq * q * q;
        scale = a;
      } else {
        R q = a / scale;
        ssq += q * q;
      }
    }
  }
  constexpr R root() const {
    if (nan)
      return std::numeric_limits<R>::quiet_NaN();
    if (inf)
      return std::numeric_limits<R>::infinity();
    return scale * std::sqrt(ssq);
  }
};

// The 2-norm of init and the elements of a vector or matrix ([linalg.algs.blas1.nrm2],
// [linalg.algs.blas1.matfrobnorm]).
template <class Scalar, class M>
Scalar la_two_norm(const M& m, Scalar init) {
  using V = typename M::value_type;
  using A = decltype(la_adl::abs_if_needed(std::declval<V>()));
  using IA = decltype(la_adl::abs_if_needed(init));
  using R = decltype(std::declval<IA>() * std::declval<IA>() + std::declval<A>() * std::declval<A>());
  static_assert(std::is_convertible_v<R, Scalar>, "std::linalg: the norm is not convertible to Scalar");
  la_ssq<R> s;
  s.add(static_cast<R>(la_adl::abs_if_needed(init)));
  if constexpr (M::rank() == 1) {
    for (std::size_t i = 0; i < la_n(m, 0); ++i)
      s.add(static_cast<R>(la_adl::abs_if_needed(V(la_v(m, i)))));
  } else {
    for (std::size_t i = 0; i < la_n(m, 0); ++i)
      for (std::size_t j = 0; j < la_n(m, 1); ++j)
        s.add(static_cast<R>(la_adl::abs_if_needed(V(la_m(m, i, j)))));
  }
  return static_cast<Scalar>(s.root());
}

template <class T>
inline constexpr bool la_is_complex = false;
template <class T>
inline constexpr bool la_is_complex<std::complex<T>> = true;
template <class T>
concept la_fp_or_complex = std::is_floating_point_v<T> || la_is_complex<T>;

// |re| + |im| (or |x| for an arithmetic value): the magnitude of vector_abs_sum and
// vector_idx_abs_max.
template <class V>
constexpr auto la_abs_parts(const V& v) {
  if constexpr (std::is_arithmetic_v<V>)
    return la_adl::abs_if_needed(v);
  else
    return la_adl::abs_if_needed(la_adl::real_if_needed(v)) + la_adl::abs_if_needed(la_adl::imag_if_needed(v));
}

} // namespace ycxx::detail

namespace std::linalg {

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas1.givens]
// ---------------------------------------------------------------------------------------------
template <class Real>
struct setup_givens_rotation_result {
  Real c;
  Real s;
  Real r;
};
template <class Real>
struct setup_givens_rotation_result<complex<Real>> {
  Real c;
  complex<Real> s;
  complex<Real> r;
};

// c = a / r, s = b / r with r = hypot(a, b) >= 0, the Euclidean norm.
template <ycxx::detail::la_real Real>
setup_givens_rotation_result<Real> setup_givens_rotation(Real a, Real b) noexcept {
  Real r = std::hypot(a, b);
  if (r == Real(0))
    return {Real(1), Real(0), Real(0)};
  return {a / r, b / r, r};
}
// With c real, r has the phase of a and the magnitude of the Euclidean norm (as LAPACK's xLARTG):
// c = |a| / n, s = (a / |a|) conj(b) / n, r = (a / |a|) n.
template <ycxx::detail::la_real Real>
setup_givens_rotation_result<complex<Real>> setup_givens_rotation(complex<Real> a, complex<Real> b) noexcept {
  Real abs_a = std::abs(a), abs_b = std::abs(b);
  if (abs_b == Real(0))
    return {Real(1), complex<Real>(0), a};
  if (abs_a == Real(0))
    return {Real(0), std::conj(b) / abs_b, complex<Real>(abs_b)};
  Real n = std::hypot(abs_a, abs_b);
  complex<Real> phase = a / abs_a;
  return {abs_a / n, phase * (std::conj(b) / n), phase * n};
}

template <ycxx::detail::la_inout_vector InOutVec1, ycxx::detail::la_inout_vector InOutVec2, ycxx::detail::la_real Real>
void apply_givens_rotation(InOutVec1 x, InOutVec2 y, Real c, Real s) {
  static_assert(ycxx::detail::la_compatible<InOutVec1, InOutVec2>(0, 0), "apply_givens_rotation: incompatible extents");
  ycxx::detail::precondition(cmp_equal(x.extent(0), y.extent(0)), "apply_givens_rotation: extents differ");
  for (size_t i = 0; i < ycxx::detail::la_n(x, 0); ++i) {
    typename InOutVec1::value_type xi = ycxx::detail::la_v(x, i);
    typename InOutVec2::value_type yi = ycxx::detail::la_v(y, i);
    ycxx::detail::la_v(x, i) = c * xi + s * yi;
    ycxx::detail::la_v(y, i) = -s * xi + c * yi;
  }
}
template <class ExecutionPolicy, ycxx::detail::la_inout_vector InOutVec1, ycxx::detail::la_inout_vector InOutVec2,
          ycxx::detail::la_real Real>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void apply_givens_rotation(ExecutionPolicy&&, InOutVec1 x, InOutVec2 y, Real c, Real s) noexcept {
  std::linalg::apply_givens_rotation(x, y, c, s);
}
template <ycxx::detail::la_inout_vector InOutVec1, ycxx::detail::la_inout_vector InOutVec2, ycxx::detail::la_real Real>
void apply_givens_rotation(InOutVec1 x, InOutVec2 y, Real c, complex<Real> s) {
  static_assert(ycxx::detail::la_compatible<InOutVec1, InOutVec2>(0, 0), "apply_givens_rotation: incompatible extents");
  ycxx::detail::precondition(cmp_equal(x.extent(0), y.extent(0)), "apply_givens_rotation: extents differ");
  for (size_t i = 0; i < ycxx::detail::la_n(x, 0); ++i) {
    typename InOutVec1::value_type xi = ycxx::detail::la_v(x, i);
    typename InOutVec2::value_type yi = ycxx::detail::la_v(y, i);
    ycxx::detail::la_v(x, i) = c * xi + s * yi;
    ycxx::detail::la_v(y, i) = -std::conj(s) * xi + c * yi;
  }
}
template <class ExecutionPolicy, ycxx::detail::la_inout_vector InOutVec1, ycxx::detail::la_inout_vector InOutVec2,
          ycxx::detail::la_real Real>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void apply_givens_rotation(ExecutionPolicy&&, InOutVec1 x, InOutVec2 y, Real c, complex<Real> s) noexcept {
  std::linalg::apply_givens_rotation(x, y, c, s);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas1.swap], [linalg.algs.blas1.scal], [linalg.algs.blas1.copy],
// [linalg.algs.blas1.add]
// ---------------------------------------------------------------------------------------------
template <ycxx::detail::la_inout_object InOutObj1, ycxx::detail::la_inout_object InOutObj2>
  requires(InOutObj1::rank() == InOutObj2::rank())
void swap_elements(InOutObj1 x, InOutObj2 y) {
  static_assert(ycxx::detail::la_compatible<InOutObj1, InOutObj2>(0, 0) &&
                    (InOutObj1::rank() == 1 || ycxx::detail::la_compatible<InOutObj1, InOutObj2>(1, 1)),
                "swap_elements: incompatible extents");
  ycxx::detail::precondition(x.extents() == y.extents(), "swap_elements: extents differ");
  ycxx::detail::md_for_each_index(x.extents(), [&](auto... i) {
    typename InOutObj1::value_type t = x[i...];
    x[i...] = y[i...];
    y[i...] = std::move(t);
  });
}
template <class ExecutionPolicy, ycxx::detail::la_inout_object InOutObj1, ycxx::detail::la_inout_object InOutObj2>
  requires(ycxx::detail::execution_policy<ExecutionPolicy> && InOutObj1::rank() == InOutObj2::rank())
void swap_elements(ExecutionPolicy&&, InOutObj1 x, InOutObj2 y) noexcept {
  std::linalg::swap_elements(x, y);
}

template <ycxx::detail::la_scalar Scalar, ycxx::detail::la_inout_object InOutObj>
void scale(Scalar alpha, InOutObj x) {
  ycxx::detail::md_for_each_index(x.extents(), [&](auto... i) { x[i...] = alpha * x[i...]; });
}
template <class ExecutionPolicy, ycxx::detail::la_scalar Scalar, ycxx::detail::la_inout_object InOutObj>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void scale(ExecutionPolicy&&, Scalar alpha, InOutObj x) noexcept {
  std::linalg::scale(alpha, x);
}

template <ycxx::detail::la_in_object InObj, ycxx::detail::la_out_object OutObj>
  requires(InObj::rank() == OutObj::rank())
void copy(InObj x, OutObj y) {
  static_assert(ycxx::detail::la_compatible<InObj, OutObj>(0, 0) &&
                    (InObj::rank() == 1 || ycxx::detail::la_compatible<InObj, OutObj>(1, 1)),
                "std::linalg::copy: incompatible extents");
  ycxx::detail::precondition(x.extents() == y.extents(), "std::linalg::copy: extents differ");
  ycxx::detail::md_for_each_index(x.extents(), [&](auto... i) { y[i...] = x[i...]; });
}
template <class ExecutionPolicy, ycxx::detail::la_in_object InObj, ycxx::detail::la_out_object OutObj>
  requires(ycxx::detail::execution_policy<ExecutionPolicy> && InObj::rank() == OutObj::rank())
void copy(ExecutionPolicy&&, InObj x, OutObj y) noexcept {
  std::linalg::copy(x, y);
}

template <ycxx::detail::la_in_object InObj1, ycxx::detail::la_in_object InObj2, ycxx::detail::la_out_object OutObj>
  requires(InObj1::rank() == InObj2::rank() && InObj1::rank() == OutObj::rank())
void add(InObj1 x, InObj2 y, OutObj z) {
  static_assert(ycxx::detail::la_possibly_addable<InObj1, InObj2, OutObj>(), "std::linalg::add: incompatible extents");
  ycxx::detail::precondition(ycxx::detail::la_addable(x, y, z), "std::linalg::add: extents differ");
  ycxx::detail::md_for_each_index(z.extents(), [&](auto... i) { z[i...] = x[i...] + y[i...]; });
}
template <class ExecutionPolicy, ycxx::detail::la_in_object InObj1, ycxx::detail::la_in_object InObj2,
          ycxx::detail::la_out_object OutObj>
  requires(ycxx::detail::execution_policy<ExecutionPolicy> && InObj1::rank() == InObj2::rank() &&
           InObj1::rank() == OutObj::rank())
void add(ExecutionPolicy&&, InObj1 x, InObj2 y, OutObj z) noexcept {
  std::linalg::add(x, y, z);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas1.dot]
// ---------------------------------------------------------------------------------------------
template <ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2, ycxx::detail::la_scalar Scalar>
Scalar dot(InVec1 v1, InVec2 v2, Scalar init) {
  static_assert(ycxx::detail::la_compatible<InVec1, InVec2>(0, 0), "std::linalg::dot: incompatible extents");
  ycxx::detail::precondition(cmp_equal(v1.extent(0), v2.extent(0)), "std::linalg::dot: extents differ");
  Scalar sum = init;
  for (size_t i = 0; i < ycxx::detail::la_n(v1, 0); ++i)
    sum += ycxx::detail::la_v(v1, i) * ycxx::detail::la_v(v2, i);
  return sum;
}
template <class ExecutionPolicy, ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2,
          ycxx::detail::la_scalar Scalar>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
Scalar dot(ExecutionPolicy&&, InVec1 v1, InVec2 v2, Scalar init) noexcept {
  return std::linalg::dot(v1, v2, init);
}
template <ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2>
auto dot(InVec1 v1, InVec2 v2) {
  using T = decltype(declval<typename InVec1::value_type>() * declval<typename InVec2::value_type>());
  return std::linalg::dot(v1, v2, T{});
}
template <class ExecutionPolicy, ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
auto dot(ExecutionPolicy&& exec, InVec1 v1, InVec2 v2) noexcept {
  using T = decltype(declval<typename InVec1::value_type>() * declval<typename InVec2::value_type>());
  return std::linalg::dot(std::forward<ExecutionPolicy>(exec), v1, v2, T{});
}
template <ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2, ycxx::detail::la_scalar Scalar>
Scalar dotc(InVec1 v1, InVec2 v2, Scalar init) {
  return std::linalg::dot(std::linalg::conjugated(v1), v2, init);
}
template <class ExecutionPolicy, ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2,
          ycxx::detail::la_scalar Scalar>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
Scalar dotc(ExecutionPolicy&& exec, InVec1 v1, InVec2 v2, Scalar init) noexcept {
  return std::linalg::dot(std::forward<ExecutionPolicy>(exec), std::linalg::conjugated(v1), v2, init);
}
template <ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2>
auto dotc(InVec1 v1, InVec2 v2) {
  using T = decltype(ycxx::detail::la_adl::conj_if_needed(declval<typename InVec1::value_type>()) *
                     declval<typename InVec2::value_type>());
  return std::linalg::dotc(v1, v2, T{});
}
template <class ExecutionPolicy, ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
auto dotc(ExecutionPolicy&& exec, InVec1 v1, InVec2 v2) noexcept {
  using T = decltype(ycxx::detail::la_adl::conj_if_needed(declval<typename InVec1::value_type>()) *
                     declval<typename InVec2::value_type>());
  return std::linalg::dotc(std::forward<ExecutionPolicy>(exec), v1, v2, T{});
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas1.nrm2], [linalg.algs.blas1.asum], [linalg.algs.blas1.iamax]
// ---------------------------------------------------------------------------------------------
template <ycxx::detail::la_in_vector InVec, ycxx::detail::la_scalar Scalar>
Scalar vector_two_norm(InVec v, Scalar init) {
  static_assert(ycxx::detail::la_fp_or_complex<typename InVec::value_type> && ycxx::detail::la_fp_or_complex<Scalar>,
                "vector_two_norm: the value types must be floating-point or complex");
  return ycxx::detail::la_two_norm(v, init);
}
template <class ExecutionPolicy, ycxx::detail::la_in_vector InVec, ycxx::detail::la_scalar Scalar>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
Scalar vector_two_norm(ExecutionPolicy&&, InVec v, Scalar init) noexcept {
  return std::linalg::vector_two_norm(v, init);
}
template <ycxx::detail::la_in_vector InVec>
auto vector_two_norm(InVec v) {
  using A = decltype(ycxx::detail::la_adl::abs_if_needed(declval<typename InVec::value_type>()));
  return std::linalg::vector_two_norm(v, decltype(declval<A>() * declval<A>()){});
}
template <class ExecutionPolicy, ycxx::detail::la_in_vector InVec>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
auto vector_two_norm(ExecutionPolicy&&, InVec v) noexcept {
  return std::linalg::vector_two_norm(v);
}

template <ycxx::detail::la_in_vector InVec, ycxx::detail::la_scalar Scalar>
Scalar vector_abs_sum(InVec v, Scalar init) {
  using V = typename InVec::value_type;
  static_assert(is_convertible_v<decltype(init + ycxx::detail::la_abs_parts(declval<V>())), Scalar>,
                "vector_abs_sum: the sum is not convertible to Scalar");
  Scalar sum = init;
  for (size_t i = 0; i < ycxx::detail::la_n(v, 0); ++i)
    sum += ycxx::detail::la_abs_parts(V(ycxx::detail::la_v(v, i)));
  return sum;
}
template <class ExecutionPolicy, ycxx::detail::la_in_vector InVec, ycxx::detail::la_scalar Scalar>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
Scalar vector_abs_sum(ExecutionPolicy&&, InVec v, Scalar init) noexcept {
  return std::linalg::vector_abs_sum(v, init);
}
template <ycxx::detail::la_in_vector InVec>
auto vector_abs_sum(InVec v) {
  return std::linalg::vector_abs_sum(v, typename InVec::value_type{});
}
template <class ExecutionPolicy, ycxx::detail::la_in_vector InVec>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
auto vector_abs_sum(ExecutionPolicy&&, InVec v) noexcept {
  return std::linalg::vector_abs_sum(v);
}

template <ycxx::detail::la_in_vector InVec>
typename InVec::size_type vector_idx_abs_max(InVec v) {
  using V = typename InVec::value_type;
  using T = decltype(ycxx::detail::la_abs_parts(declval<V>()));
  static_assert(requires(T a, T b) { a < b; }, "vector_idx_abs_max: the magnitudes must be comparable with <");
  size_t n = ycxx::detail::la_n(v, 0);
  if (n == 0)
    return numeric_limits<typename InVec::size_type>::max();
  size_t best = 0;
  T best_value = ycxx::detail::la_abs_parts(V(ycxx::detail::la_v(v, 0)));
  for (size_t i = 1; i < n; ++i) {
    T x = ycxx::detail::la_abs_parts(V(ycxx::detail::la_v(v, i)));
    if (best_value < x) {
      best = i;
      best_value = x;
    }
  }
  return static_cast<typename InVec::size_type>(best);
}
template <class ExecutionPolicy, ycxx::detail::la_in_vector InVec>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
typename InVec::size_type vector_idx_abs_max(ExecutionPolicy&&, InVec v) noexcept {
  return std::linalg::vector_idx_abs_max(v);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas1.matfrobnorm], [linalg.algs.blas1.matonenorm], [linalg.algs.blas1.matinfnorm]
// ---------------------------------------------------------------------------------------------
template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_scalar Scalar>
Scalar matrix_frob_norm(InMat A, Scalar init) {
  static_assert(ycxx::detail::la_fp_or_complex<typename InMat::value_type> && ycxx::detail::la_fp_or_complex<Scalar>,
                "matrix_frob_norm: the value types must be floating-point or complex");
  return ycxx::detail::la_two_norm(A, init);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_scalar Scalar>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
Scalar matrix_frob_norm(ExecutionPolicy&&, InMat A, Scalar init) noexcept {
  return std::linalg::matrix_frob_norm(A, init);
}
template <ycxx::detail::la_in_matrix InMat>
auto matrix_frob_norm(InMat A) {
  using T = decltype(ycxx::detail::la_adl::abs_if_needed(declval<typename InMat::value_type>()));
  return std::linalg::matrix_frob_norm(A, decltype(declval<T>() * declval<T>()){});
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
auto matrix_frob_norm(ExecutionPolicy&&, InMat A) noexcept {
  return std::linalg::matrix_frob_norm(A);
}

template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_scalar Scalar>
Scalar matrix_one_norm(InMat A, Scalar init) {
  using V = typename InMat::value_type;
  static_assert(is_convertible_v<decltype(ycxx::detail::la_adl::abs_if_needed(declval<V>())), Scalar>,
                "matrix_one_norm: the magnitudes are not convertible to Scalar");
  size_t m = ycxx::detail::la_n(A, 0), n = ycxx::detail::la_n(A, 1);
  if (n == 0)
    return init;
  Scalar best{};
  for (size_t j = 0; j < n; ++j) {
    Scalar col{};
    for (size_t i = 0; i < m; ++i)
      col += ycxx::detail::la_adl::abs_if_needed(V(ycxx::detail::la_m(A, i, j)));
    if (j == 0 || best < col)
      best = col;
  }
  return init + best;
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_scalar Scalar>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
Scalar matrix_one_norm(ExecutionPolicy&&, InMat A, Scalar init) noexcept {
  return std::linalg::matrix_one_norm(A, init);
}
template <ycxx::detail::la_in_matrix InMat>
auto matrix_one_norm(InMat A) {
  using T = decltype(ycxx::detail::la_adl::abs_if_needed(declval<typename InMat::value_type>()));
  return std::linalg::matrix_one_norm(A, T{});
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
auto matrix_one_norm(ExecutionPolicy&&, InMat A) noexcept {
  return std::linalg::matrix_one_norm(A);
}

template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_scalar Scalar>
Scalar matrix_inf_norm(InMat A, Scalar init) {
  using V = typename InMat::value_type;
  static_assert(is_convertible_v<decltype(ycxx::detail::la_adl::abs_if_needed(declval<V>())), Scalar>,
                "matrix_inf_norm: the magnitudes are not convertible to Scalar");
  size_t m = ycxx::detail::la_n(A, 0), n = ycxx::detail::la_n(A, 1);
  if (m == 0)
    return init;
  Scalar best{};
  for (size_t i = 0; i < m; ++i) {
    Scalar row{};
    for (size_t j = 0; j < n; ++j)
      row += ycxx::detail::la_adl::abs_if_needed(V(ycxx::detail::la_m(A, i, j)));
    if (i == 0 || best < row)
      best = row;
  }
  return init + best;
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_scalar Scalar>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
Scalar matrix_inf_norm(ExecutionPolicy&&, InMat A, Scalar init) noexcept {
  return std::linalg::matrix_inf_norm(A, init);
}
template <ycxx::detail::la_in_matrix InMat>
auto matrix_inf_norm(InMat A) {
  using T = decltype(ycxx::detail::la_adl::abs_if_needed(declval<typename InMat::value_type>()));
  return std::linalg::matrix_inf_norm(A, T{});
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
auto matrix_inf_norm(ExecutionPolicy&&, InMat A) noexcept {
  return std::linalg::matrix_inf_norm(A);
}

} // namespace std::linalg

namespace ycxx::detail {

// ---------------------------------------------------------------------------------------------
// Kernels shared by the BLAS 2 and 3 algorithms.
// ---------------------------------------------------------------------------------------------

// z[i] = (y[i] +) sum_j a(i, j) * x[j], where a(i, j, x_j, acc) adds the term for (i, j).
template <class Out, class X, class Term, class Init>
void la_mv(std::size_t m, std::size_t n, const X& x, const Out& z, Term term, Init init) {
  for (std::size_t i = 0; i < m; ++i) {
    typename Out::value_type acc = init(i);
    for (std::size_t j = 0; j < n; ++j)
      term(i, j, la_v(x, j), acc);
    la_v(z, i) = acc;
  }
}

// The checks common to the matrix-vector products with a z parameter.
template <class A, class X, class Y, class Z>
constexpr void la_check_mv(const A& a, const X& x, const Y& y, const Z& z) {
  static_assert(la_possibly_multipliable<A, X, Y>(), "std::linalg: incompatible extents");
  static_assert(la_possibly_addable<Y, Y, Z>(), "std::linalg: incompatible extents");
  ::ycxx::detail::precondition(la_multipliable(a, x, y) && la_addable(y, y, z), "std::linalg: extents differ");
}

// y = A x for a symmetric or Hermitian A (z = y + A x when Upd).
template <la_structure S, class Triangle, class A, class X, class Y, class Z>
void la_symv(const A& a, const X& x, const Y& y, const Z& z, bool update) {
  using V = typename Z::value_type;
  la_mv(
      la_n(a, 0), la_n(a, 1), x, z,
      [&](std::size_t i, std::size_t j, auto&& xj, V& acc) { acc += la_sym<S, Triangle>(a, i, j) * xj; },
      [&](std::size_t i) { return update ? V(la_v(y, i)) : V{}; });
}

// One element of a triangular matrix-vector product: acc += A[i, j] * xj for (i, j) in the
// triangle, xj alone on an implicit unit diagonal.
template <class Triangle, class Diag, class A, class X, class V>
void la_tri_term(const A& a, std::size_t i, std::size_t j, const X& xj, V& acc) {
  if (!la_in_triangle<Triangle>(i, j))
    return;
  if (i == j && la_unit<Diag>)
    acc += xj;
  else
    acc += la_m(a, i, j) * xj;
}

// Solves A x = b for a triangular A; b(i) and x(i) access the elements (b and x may be the
// same vector, which is then overwritten in place).
template <class Triangle, class Diag, class V, class A, class BAt, class XAt, class Divide>
void la_trsv(const A& a, BAt b, XAt x, Divide& divide) {
  std::size_t n = la_n(a, 0);
  auto row = [&](std::size_t i) {
    V t = b(i);
    for (std::size_t j = 0; j < n; ++j)
      if (j != i && la_in_triangle<Triangle>(i, j))
        t -= la_m(a, i, j) * x(j);
    if constexpr (la_unit<Diag>)
      x(i) = t;
    else
      x(i) = divide(t, la_m(a, i, i));
  };
  if constexpr (std::is_same_v<Triangle, std::linalg::lower_triangle_t>) {
    for (std::size_t i = 0; i < n; ++i)
      row(i);
  } else {
    for (std::size_t i = n; i-- > 0;)
      row(i);
  }
}

// y = A y in place for a triangular A; y(i) accesses the elements. y[i] depends on y[j] for j
// in the triangle of row i, so upper rows go first and lower rows last.
template <class Triangle, class Diag, class V, class A, class YAt>
void la_trmv_inplace(const A& a, YAt y) {
  std::size_t n = la_n(a, 0);
  auto row = [&](std::size_t i) {
    V acc{};
    for (std::size_t j = 0; j < n; ++j)
      la_tri_term<Triangle, Diag>(a, i, j, y(j), acc);
    y(i) = acc;
  };
  if constexpr (std::is_same_v<Triangle, std::linalg::upper_triangle_t>) {
    for (std::size_t i = 0; i < n; ++i)
      row(i);
  } else {
    for (std::size_t i = n; i-- > 0;)
      row(i);
  }
}

// Solves x A = b for a triangular A, one row of a right solve (x and b may be the same).
template <class Triangle, class Diag, class A, class B, class X, class Divide>
void la_trsv_right(const A& a, const B& b, const X& x, std::size_t r, Divide& divide) {
  using V = typename X::value_type;
  std::size_t n = la_n(a, 0);
  auto col = [&](std::size_t j) {
    V t = la_m(b, r, j);
    for (std::size_t k = 0; k < n; ++k)
      if (k != j && la_in_triangle<Triangle>(k, j))
        t -= la_m(x, r, k) * la_m(a, k, j);
    if constexpr (la_unit<Diag>)
      la_m(x, r, j) = t;
    else
      la_m(x, r, j) = divide(t, la_m(a, j, j));
  };
  if constexpr (std::is_same_v<Triangle, std::linalg::upper_triangle_t>) {
    for (std::size_t j = 0; j < n; ++j)
      col(j);
  } else {
    for (std::size_t j = n; j-- > 0;)
      col(j);
  }
}

// The checks of a symmetric/Hermitian/triangular A in the BLAS 2 and 3 algorithms.
template <class A, class Triangle>
constexpr void la_check_square(const A& a) {
  static_assert(la_triangle_matches<A, Triangle>,
                "std::linalg: the packed layout's Triangle differs from the function's Triangle");
  static_assert(la_compatible<A, A>(0, 1), "std::linalg: the matrix must be square");
  ::ycxx::detail::precondition(a.extent(0) == a.extent(1), "std::linalg: the matrix must be square");
}

// C (triangle t) = E + alpha-weighted sum, the common body of the symmetric and Hermitian
// rank-1, rank-2, rank-k and rank-2k updates: value(i, j) is the added term.
template <la_structure S, class Triangle, class E, class C, class Value>
void la_rank_update(const E* e, const C& c, Value value) {
  using V = typename C::value_type;
  std::size_t n = la_n(c, 0);
  for (std::size_t i = 0; i < n; ++i)
    for (std::size_t j = 0; j < n; ++j) {
      if (!la_in_triangle<Triangle>(i, j))
        continue;
      V acc{};
      if constexpr (!std::is_same_v<E, la_none>) {
        if constexpr (S == la_structure::hermitian)
          acc = i == j ? V(la_adl::real_if_needed(la_m(*e, i, i))) : V(la_m(*e, i, j));
        else
          acc = V(la_m(*e, i, j));
      }
      acc += value(i, j);
      la_m(c, i, j) = acc;
    }
}

} // namespace ycxx::detail

namespace std::linalg {

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas2.gemv]
// ---------------------------------------------------------------------------------------------
template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_in_vector InVec, ycxx::detail::la_out_vector OutVec>
void matrix_vector_product(InMat A, InVec x, OutVec y) {
  static_assert(ycxx::detail::la_possibly_multipliable<InMat, InVec, OutVec>(),
                "matrix_vector_product: incompatible extents");
  ycxx::detail::precondition(ycxx::detail::la_multipliable(A, x, y), "matrix_vector_product: extents differ");
  using V = typename OutVec::value_type;
  ycxx::detail::la_mv(
      ycxx::detail::la_n(A, 0), ycxx::detail::la_n(A, 1), x, y,
      [&](size_t i, size_t j, auto&& xj, V& acc) { acc += ycxx::detail::la_m(A, i, j) * xj; },
      [](size_t) { return V{}; });
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_in_vector InVec,
          ycxx::detail::la_out_vector OutVec>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void matrix_vector_product(ExecutionPolicy&&, InMat A, InVec x, OutVec y) noexcept {
  std::linalg::matrix_vector_product(A, x, y);
}
template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2,
          ycxx::detail::la_out_vector OutVec>
void matrix_vector_product(InMat A, InVec1 x, InVec2 y, OutVec z) {
  ycxx::detail::la_check_mv(A, x, y, z);
  using V = typename OutVec::value_type;
  ycxx::detail::la_mv(
      ycxx::detail::la_n(A, 0), ycxx::detail::la_n(A, 1), x, z,
      [&](size_t i, size_t j, auto&& xj, V& acc) { acc += ycxx::detail::la_m(A, i, j) * xj; },
      [&](size_t i) { return V(ycxx::detail::la_v(y, i)); });
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_in_vector InVec1,
          ycxx::detail::la_in_vector InVec2, ycxx::detail::la_out_vector OutVec>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void matrix_vector_product(ExecutionPolicy&&, InMat A, InVec1 x, InVec2 y, OutVec z) noexcept {
  std::linalg::matrix_vector_product(A, x, y, z);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas2.symv], [linalg.algs.blas2.hemv]
// ---------------------------------------------------------------------------------------------
template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle, ycxx::detail::la_in_vector InVec,
          ycxx::detail::la_out_vector OutVec>
void symmetric_matrix_vector_product(InMat A, Triangle, InVec x, OutVec y) {
  ycxx::detail::la_check_square<InMat, Triangle>(A);
  static_assert(ycxx::detail::la_possibly_multipliable<InMat, InVec, OutVec>(),
                "symmetric_matrix_vector_product: incompatible extents");
  ycxx::detail::precondition(ycxx::detail::la_multipliable(A, x, y), "symmetric_matrix_vector_product: extents differ");
  ycxx::detail::la_symv<ycxx::detail::la_structure::symmetric, Triangle>(A, x, y, y, false);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_in_vector InVec, ycxx::detail::la_out_vector OutVec>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void symmetric_matrix_vector_product(ExecutionPolicy&&, InMat A, Triangle t, InVec x, OutVec y) noexcept {
  std::linalg::symmetric_matrix_vector_product(A, t, x, y);
}
template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle, ycxx::detail::la_in_vector InVec1,
          ycxx::detail::la_in_vector InVec2, ycxx::detail::la_out_vector OutVec>
void symmetric_matrix_vector_product(InMat A, Triangle, InVec1 x, InVec2 y, OutVec z) {
  ycxx::detail::la_check_square<InMat, Triangle>(A);
  ycxx::detail::la_check_mv(A, x, y, z);
  ycxx::detail::la_symv<ycxx::detail::la_structure::symmetric, Triangle>(A, x, y, z, true);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2, ycxx::detail::la_out_vector OutVec>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void symmetric_matrix_vector_product(ExecutionPolicy&&, InMat A, Triangle t, InVec1 x, InVec2 y, OutVec z) noexcept {
  std::linalg::symmetric_matrix_vector_product(A, t, x, y, z);
}

template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle, ycxx::detail::la_in_vector InVec,
          ycxx::detail::la_out_vector OutVec>
void hermitian_matrix_vector_product(InMat A, Triangle, InVec x, OutVec y) {
  ycxx::detail::la_check_square<InMat, Triangle>(A);
  static_assert(ycxx::detail::la_possibly_multipliable<InMat, InVec, OutVec>(),
                "hermitian_matrix_vector_product: incompatible extents");
  ycxx::detail::precondition(ycxx::detail::la_multipliable(A, x, y), "hermitian_matrix_vector_product: extents differ");
  ycxx::detail::la_symv<ycxx::detail::la_structure::hermitian, Triangle>(A, x, y, y, false);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_in_vector InVec, ycxx::detail::la_out_vector OutVec>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void hermitian_matrix_vector_product(ExecutionPolicy&&, InMat A, Triangle t, InVec x, OutVec y) noexcept {
  std::linalg::hermitian_matrix_vector_product(A, t, x, y);
}
template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle, ycxx::detail::la_in_vector InVec1,
          ycxx::detail::la_in_vector InVec2, ycxx::detail::la_out_vector OutVec>
void hermitian_matrix_vector_product(InMat A, Triangle, InVec1 x, InVec2 y, OutVec z) {
  ycxx::detail::la_check_square<InMat, Triangle>(A);
  ycxx::detail::la_check_mv(A, x, y, z);
  ycxx::detail::la_symv<ycxx::detail::la_structure::hermitian, Triangle>(A, x, y, z, true);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2, ycxx::detail::la_out_vector OutVec>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void hermitian_matrix_vector_product(ExecutionPolicy&&, InMat A, Triangle t, InVec1 x, InVec2 y, OutVec z) noexcept {
  std::linalg::hermitian_matrix_vector_product(A, t, x, y, z);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas2.trmv]
// ---------------------------------------------------------------------------------------------
template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_vector InVec,
          ycxx::detail::la_out_vector OutVec>
void triangular_matrix_vector_product(InMat A, Triangle, DiagonalStorage, InVec x, OutVec y) {
  ycxx::detail::la_check_square<InMat, Triangle>(A);
  static_assert(ycxx::detail::la_compatible<InMat, OutVec>(0, 0) && ycxx::detail::la_compatible<InMat, InVec>(0, 0),
                "triangular_matrix_vector_product: incompatible extents");
  ycxx::detail::precondition(cmp_equal(A.extent(0), y.extent(0)) && cmp_equal(A.extent(0), x.extent(0)),
                             "triangular_matrix_vector_product: extents differ");
  using V = typename OutVec::value_type;
  ycxx::detail::la_mv(
      ycxx::detail::la_n(A, 0), ycxx::detail::la_n(A, 1), x, y,
      [&](size_t i, size_t j, auto&& xj, V& acc) {
        ycxx::detail::la_tri_term<Triangle, DiagonalStorage>(A, i, j, xj, acc);
      },
      [](size_t) { return V{}; });
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_vector InVec,
          ycxx::detail::la_out_vector OutVec>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_vector_product(ExecutionPolicy&&, InMat A, Triangle t, DiagonalStorage d, InVec x,
                                      OutVec y) noexcept {
  std::linalg::triangular_matrix_vector_product(A, t, d, x, y);
}
template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_inout_vector InOutVec>
void triangular_matrix_vector_product(InMat A, Triangle, DiagonalStorage, InOutVec y) {
  ycxx::detail::la_check_square<InMat, Triangle>(A);
  static_assert(ycxx::detail::la_compatible<InMat, InOutVec>(0, 0),
                "triangular_matrix_vector_product: incompatible extents");
  ycxx::detail::precondition(cmp_equal(A.extent(0), y.extent(0)), "triangular_matrix_vector_product: extents differ");
  ycxx::detail::la_trmv_inplace<Triangle, DiagonalStorage, typename InOutVec::value_type>(
      A, [&](size_t i) -> decltype(auto) { return ycxx::detail::la_v(y, i); });
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_inout_vector InOutVec>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_vector_product(ExecutionPolicy&&, InMat A, Triangle t, DiagonalStorage d, InOutVec y) noexcept {
  std::linalg::triangular_matrix_vector_product(A, t, d, y);
}
template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_vector InVec1,
          ycxx::detail::la_in_vector InVec2, ycxx::detail::la_out_vector OutVec>
void triangular_matrix_vector_product(InMat A, Triangle, DiagonalStorage, InVec1 x, InVec2 y, OutVec z) {
  ycxx::detail::la_check_square<InMat, Triangle>(A);
  static_assert(ycxx::detail::la_compatible<InMat, InVec2>(0, 0) && ycxx::detail::la_compatible<InMat, InVec1>(0, 0) &&
                    ycxx::detail::la_compatible<InMat, OutVec>(0, 0),
                "triangular_matrix_vector_product: incompatible extents");
  ycxx::detail::precondition(cmp_equal(A.extent(0), y.extent(0)) && cmp_equal(A.extent(0), x.extent(0)) &&
                                 cmp_equal(A.extent(0), z.extent(0)),
                             "triangular_matrix_vector_product: extents differ");
  using V = typename OutVec::value_type;
  ycxx::detail::la_mv(
      ycxx::detail::la_n(A, 0), ycxx::detail::la_n(A, 1), x, z,
      [&](size_t i, size_t j, auto&& xj, V& acc) {
        ycxx::detail::la_tri_term<Triangle, DiagonalStorage>(A, i, j, xj, acc);
      },
      [&](size_t i) { return V(ycxx::detail::la_v(y, i)); });
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_vector InVec1,
          ycxx::detail::la_in_vector InVec2, ycxx::detail::la_out_vector OutVec>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_vector_product(ExecutionPolicy&&, InMat A, Triangle t, DiagonalStorage d, InVec1 x, InVec2 y,
                                      OutVec z) noexcept {
  std::linalg::triangular_matrix_vector_product(A, t, d, x, y, z);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas2.trsv]
// ---------------------------------------------------------------------------------------------
template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_vector InVec,
          ycxx::detail::la_out_vector OutVec, ycxx::detail::la_divide_op BinaryDivideOp>
void triangular_matrix_vector_solve(InMat A, Triangle, DiagonalStorage, InVec b, OutVec x, BinaryDivideOp divide) {
  ycxx::detail::la_check_square<InMat, Triangle>(A);
  static_assert(ycxx::detail::la_compatible<InMat, InVec>(0, 0) && ycxx::detail::la_compatible<InMat, OutVec>(0, 0),
                "triangular_matrix_vector_solve: incompatible extents");
  ycxx::detail::precondition(cmp_equal(A.extent(0), b.extent(0)) && cmp_equal(A.extent(0), x.extent(0)),
                             "triangular_matrix_vector_solve: extents differ");
  ycxx::detail::la_trsv<Triangle, DiagonalStorage, typename OutVec::value_type>(
      A, [&](size_t i) -> decltype(auto) { return ycxx::detail::la_v(b, i); },
      [&](size_t i) -> decltype(auto) { return ycxx::detail::la_v(x, i); }, divide);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_vector InVec,
          ycxx::detail::la_out_vector OutVec, ycxx::detail::la_divide_op BinaryDivideOp>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_vector_solve(ExecutionPolicy&&, InMat A, Triangle t, DiagonalStorage d, InVec b, OutVec x,
                                    BinaryDivideOp divide) noexcept {
  std::linalg::triangular_matrix_vector_solve(A, t, d, b, x, divide);
}
template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_vector InVec,
          ycxx::detail::la_out_vector OutVec>
void triangular_matrix_vector_solve(InMat A, Triangle t, DiagonalStorage d, InVec b, OutVec x) {
  std::linalg::triangular_matrix_vector_solve(A, t, d, b, x, divides<void>{});
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_vector InVec,
          ycxx::detail::la_out_vector OutVec>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_vector_solve(ExecutionPolicy&& exec, InMat A, Triangle t, DiagonalStorage d, InVec b,
                                    OutVec x) noexcept {
  std::linalg::triangular_matrix_vector_solve(std::forward<ExecutionPolicy>(exec), A, t, d, b, x, divides<void>{});
}
template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_inout_vector InOutVec,
          ycxx::detail::la_divide_op BinaryDivideOp>
void triangular_matrix_vector_solve(InMat A, Triangle, DiagonalStorage, InOutVec b, BinaryDivideOp divide) {
  ycxx::detail::la_check_square<InMat, Triangle>(A);
  static_assert(ycxx::detail::la_compatible<InMat, InOutVec>(0, 0),
                "triangular_matrix_vector_solve: incompatible extents");
  ycxx::detail::precondition(cmp_equal(A.extent(0), b.extent(0)), "triangular_matrix_vector_solve: extents differ");
  auto at = [&](size_t i) -> decltype(auto) { return ycxx::detail::la_v(b, i); };
  ycxx::detail::la_trsv<Triangle, DiagonalStorage, typename InOutVec::value_type>(A, at, at, divide);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_inout_vector InOutVec,
          ycxx::detail::la_divide_op BinaryDivideOp>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_vector_solve(ExecutionPolicy&&, InMat A, Triangle t, DiagonalStorage d, InOutVec b,
                                    BinaryDivideOp divide) noexcept {
  std::linalg::triangular_matrix_vector_solve(A, t, d, b, divide);
}
template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_inout_vector InOutVec>
void triangular_matrix_vector_solve(InMat A, Triangle t, DiagonalStorage d, InOutVec b) {
  std::linalg::triangular_matrix_vector_solve(A, t, d, b, divides<void>{});
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_inout_vector InOutVec>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_vector_solve(ExecutionPolicy&& exec, InMat A, Triangle t, DiagonalStorage d,
                                    InOutVec b) noexcept {
  std::linalg::triangular_matrix_vector_solve(std::forward<ExecutionPolicy>(exec), A, t, d, b, divides<void>{});
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas2.rank1]
// ---------------------------------------------------------------------------------------------
template <ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2, ycxx::detail::la_out_matrix OutMat>
void matrix_rank_1_update(InVec1 x, InVec2 y, OutMat A) {
  static_assert(ycxx::detail::la_possibly_multipliable<OutMat, InVec2, InVec1>(),
                "matrix_rank_1_update: incompatible extents");
  ycxx::detail::precondition(ycxx::detail::la_multipliable(A, y, x), "matrix_rank_1_update: extents differ");
  for (size_t i = 0; i < ycxx::detail::la_n(A, 0); ++i)
    for (size_t j = 0; j < ycxx::detail::la_n(A, 1); ++j)
      ycxx::detail::la_m(A, i, j) = ycxx::detail::la_v(x, i) * ycxx::detail::la_v(y, j);
}
template <class ExecutionPolicy, ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2,
          ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void matrix_rank_1_update(ExecutionPolicy&&, InVec1 x, InVec2 y, OutMat A) noexcept {
  std::linalg::matrix_rank_1_update(x, y, A);
}
template <ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2, ycxx::detail::la_in_matrix InMat,
          ycxx::detail::la_out_matrix OutMat>
void matrix_rank_1_update(InVec1 x, InVec2 y, InMat E, OutMat A) {
  static_assert(ycxx::detail::la_possibly_multipliable<OutMat, InVec2, InVec1>() &&
                    ycxx::detail::la_possibly_addable<OutMat, InMat, OutMat>(),
                "matrix_rank_1_update: incompatible extents");
  ycxx::detail::precondition(ycxx::detail::la_multipliable(A, y, x) && ycxx::detail::la_addable(A, E, A),
                             "matrix_rank_1_update: extents differ");
  using V = typename OutMat::value_type;
  for (size_t i = 0; i < ycxx::detail::la_n(A, 0); ++i)
    for (size_t j = 0; j < ycxx::detail::la_n(A, 1); ++j) {
      V acc = ycxx::detail::la_m(E, i, j);
      acc += ycxx::detail::la_v(x, i) * ycxx::detail::la_v(y, j);
      ycxx::detail::la_m(A, i, j) = acc;
    }
}
template <class ExecutionPolicy, ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2,
          ycxx::detail::la_in_matrix InMat, ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void matrix_rank_1_update(ExecutionPolicy&&, InVec1 x, InVec2 y, InMat E, OutMat A) noexcept {
  std::linalg::matrix_rank_1_update(x, y, E, A);
}
template <ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2, ycxx::detail::la_out_matrix OutMat>
void matrix_rank_1_update_c(InVec1 x, InVec2 y, OutMat A) {
  std::linalg::matrix_rank_1_update(x, std::linalg::conjugated(y), A);
}
template <class ExecutionPolicy, ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2,
          ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void matrix_rank_1_update_c(ExecutionPolicy&& exec, InVec1 x, InVec2 y, OutMat A) noexcept {
  std::linalg::matrix_rank_1_update(std::forward<ExecutionPolicy>(exec), x, std::linalg::conjugated(y), A);
}
template <ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2, ycxx::detail::la_in_matrix InMat,
          ycxx::detail::la_out_matrix OutMat>
void matrix_rank_1_update_c(InVec1 x, InVec2 y, InMat E, OutMat A) {
  std::linalg::matrix_rank_1_update(x, std::linalg::conjugated(y), E, A);
}
template <class ExecutionPolicy, ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2,
          ycxx::detail::la_in_matrix InMat, ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void matrix_rank_1_update_c(ExecutionPolicy&& exec, InVec1 x, InVec2 y, InMat E, OutMat A) noexcept {
  std::linalg::matrix_rank_1_update(std::forward<ExecutionPolicy>(exec), x, std::linalg::conjugated(y), E, A);
}

} // namespace std::linalg

namespace ycxx::detail {

// [linalg.algs.blas2.symherrank1]: A = (E +) alpha x x^T (or x x^H), triangle t of A.
template <la_structure S, class Triangle, class Scalar, class X, class E, class A>
void la_rank1(Scalar alpha, const X& x, const E* e, const A& a) {
  static_assert(la_triangle_matches<A, Triangle>,
                "std::linalg: the packed layout's Triangle differs from the function's Triangle");
  static_assert(la_compatible<A, A>(0, 1) && la_compatible<A, X>(0, 0), "std::linalg: incompatible extents");
  ::ycxx::detail::precondition(a.extent(0) == a.extent(1) && std::cmp_equal(a.extent(0), x.extent(0)) &&
                                   la_e_addable(e, a),
                               "std::linalg: extents differ");
  if constexpr (S == la_structure::hermitian) {
    auto real_alpha = la_adl::real_if_needed(alpha);
    la_rank_update<S, Triangle>(e, a, [&](std::size_t i, std::size_t j) {
      return real_alpha * la_v(x, i) * la_adl::conj_if_needed(la_v(x, j));
    });
  } else {
    la_rank_update<S, Triangle>(e, a, [&](std::size_t i, std::size_t j) { return alpha * la_v(x, i) * la_v(x, j); });
  }
}

// [linalg.algs.blas2.rank2]: A = (E +) x y^T + y x^T (or x y^H + y x^H), triangle t of A.
template <la_structure S, class Triangle, class X, class Y, class E, class A>
void la_rank2(const X& x, const Y& y, const E* e, const A& a) {
  static_assert(la_triangle_matches<A, Triangle>,
                "std::linalg: the packed layout's Triangle differs from the function's Triangle");
  if constexpr (!std::is_same_v<E, la_none>)
    static_assert(la_triangle_matches<E, Triangle>,
                  "std::linalg: the packed layout's Triangle differs from the function's Triangle");
  static_assert(la_compatible<A, A>(0, 1) && la_possibly_multipliable<A, X, Y>(), "std::linalg: incompatible extents");
  ::ycxx::detail::precondition(a.extent(0) == a.extent(1) && la_multipliable(a, x, y) && la_e_addable(e, a),
                               "std::linalg: extents differ");
  if constexpr (S == la_structure::hermitian)
    la_rank_update<S, Triangle>(e, a, [&](std::size_t i, std::size_t j) {
      return la_v(x, i) * la_adl::conj_if_needed(la_v(y, j)) + la_v(y, i) * la_adl::conj_if_needed(la_v(x, j));
    });
  else
    la_rank_update<S, Triangle>(
        e, a, [&](std::size_t i, std::size_t j) { return la_v(x, i) * la_v(y, j) + la_v(y, i) * la_v(x, j); });
}

} // namespace ycxx::detail

namespace std::linalg {

template <ycxx::detail::la_scalar Scalar, ycxx::detail::la_in_vector InVec,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
void symmetric_matrix_rank_1_update(Scalar alpha, InVec x, OutMat A, Triangle) {
  ycxx::detail::la_rank1<ycxx::detail::la_structure::symmetric, Triangle>(alpha, x, ycxx::detail::la_no_e, A);
}
template <class ExecutionPolicy, ycxx::detail::la_scalar Scalar, ycxx::detail::la_in_vector InVec,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void symmetric_matrix_rank_1_update(ExecutionPolicy&&, Scalar alpha, InVec x, OutMat A, Triangle t) noexcept {
  std::linalg::symmetric_matrix_rank_1_update(alpha, x, A, t);
}
template <ycxx::detail::la_scalar Scalar, ycxx::detail::la_in_vector InVec,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
void hermitian_matrix_rank_1_update(Scalar alpha, InVec x, OutMat A, Triangle) {
  ycxx::detail::la_rank1<ycxx::detail::la_structure::hermitian, Triangle>(alpha, x, ycxx::detail::la_no_e, A);
}
template <class ExecutionPolicy, ycxx::detail::la_scalar Scalar, ycxx::detail::la_in_vector InVec,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void hermitian_matrix_rank_1_update(ExecutionPolicy&&, Scalar alpha, InVec x, OutMat A, Triangle t) noexcept {
  std::linalg::hermitian_matrix_rank_1_update(alpha, x, A, t);
}
template <ycxx::detail::la_scalar Scalar, ycxx::detail::la_in_vector InVec, ycxx::detail::la_in_matrix InMat,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
void symmetric_matrix_rank_1_update(Scalar alpha, InVec x, InMat E, OutMat A, Triangle) {
  static_assert(ycxx::detail::la_possibly_addable<OutMat, InMat, OutMat>(),
                "symmetric_matrix_rank_1_update: incompatible extents");
  ycxx::detail::la_rank1<ycxx::detail::la_structure::symmetric, Triangle>(alpha, x, &E, A);
}
template <class ExecutionPolicy, ycxx::detail::la_scalar Scalar, ycxx::detail::la_in_vector InVec,
          ycxx::detail::la_in_matrix InMat, ycxx::detail::la_possibly_packed_out_matrix OutMat,
          ycxx::detail::la_triangle Triangle>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void symmetric_matrix_rank_1_update(ExecutionPolicy&&, Scalar alpha, InVec x, InMat E, OutMat A, Triangle t) noexcept {
  std::linalg::symmetric_matrix_rank_1_update(alpha, x, E, A, t);
}
template <ycxx::detail::la_scalar Scalar, ycxx::detail::la_in_vector InVec, ycxx::detail::la_in_matrix InMat,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
void hermitian_matrix_rank_1_update(Scalar alpha, InVec x, InMat E, OutMat A, Triangle) {
  static_assert(ycxx::detail::la_possibly_addable<OutMat, InMat, OutMat>(),
                "hermitian_matrix_rank_1_update: incompatible extents");
  ycxx::detail::la_rank1<ycxx::detail::la_structure::hermitian, Triangle>(alpha, x, &E, A);
}
template <class ExecutionPolicy, ycxx::detail::la_scalar Scalar, ycxx::detail::la_in_vector InVec,
          ycxx::detail::la_in_matrix InMat, ycxx::detail::la_possibly_packed_out_matrix OutMat,
          ycxx::detail::la_triangle Triangle>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void hermitian_matrix_rank_1_update(ExecutionPolicy&&, Scalar alpha, InVec x, InMat E, OutMat A, Triangle t) noexcept {
  std::linalg::hermitian_matrix_rank_1_update(alpha, x, E, A, t);
}

template <ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
void symmetric_matrix_rank_2_update(InVec1 x, InVec2 y, OutMat A, Triangle) {
  ycxx::detail::la_rank2<ycxx::detail::la_structure::symmetric, Triangle>(x, y, ycxx::detail::la_no_e, A);
}
template <class ExecutionPolicy, ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void symmetric_matrix_rank_2_update(ExecutionPolicy&&, InVec1 x, InVec2 y, OutMat A, Triangle t) noexcept {
  std::linalg::symmetric_matrix_rank_2_update(x, y, A, t);
}
template <ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
void hermitian_matrix_rank_2_update(InVec1 x, InVec2 y, OutMat A, Triangle) {
  ycxx::detail::la_rank2<ycxx::detail::la_structure::hermitian, Triangle>(x, y, ycxx::detail::la_no_e, A);
}
template <class ExecutionPolicy, ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void hermitian_matrix_rank_2_update(ExecutionPolicy&&, InVec1 x, InVec2 y, OutMat A, Triangle t) noexcept {
  std::linalg::hermitian_matrix_rank_2_update(x, y, A, t);
}
template <ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2, ycxx::detail::la_in_matrix InMat,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
void symmetric_matrix_rank_2_update(InVec1 x, InVec2 y, InMat E, OutMat A, Triangle) {
  static_assert(ycxx::detail::la_possibly_addable<OutMat, InMat, OutMat>(),
                "symmetric_matrix_rank_2_update: incompatible extents");
  ycxx::detail::la_rank2<ycxx::detail::la_structure::symmetric, Triangle>(x, y, &E, A);
}
template <class ExecutionPolicy, ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2,
          ycxx::detail::la_in_matrix InMat, ycxx::detail::la_possibly_packed_out_matrix OutMat,
          ycxx::detail::la_triangle Triangle>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void symmetric_matrix_rank_2_update(ExecutionPolicy&&, InVec1 x, InVec2 y, InMat E, OutMat A, Triangle t) noexcept {
  std::linalg::symmetric_matrix_rank_2_update(x, y, E, A, t);
}
template <ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2, ycxx::detail::la_in_matrix InMat,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
void hermitian_matrix_rank_2_update(InVec1 x, InVec2 y, InMat E, OutMat A, Triangle) {
  static_assert(ycxx::detail::la_possibly_addable<OutMat, InMat, OutMat>(),
                "hermitian_matrix_rank_2_update: incompatible extents");
  ycxx::detail::la_rank2<ycxx::detail::la_structure::hermitian, Triangle>(x, y, &E, A);
}
template <class ExecutionPolicy, ycxx::detail::la_in_vector InVec1, ycxx::detail::la_in_vector InVec2,
          ycxx::detail::la_in_matrix InMat, ycxx::detail::la_possibly_packed_out_matrix OutMat,
          ycxx::detail::la_triangle Triangle>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void hermitian_matrix_rank_2_update(ExecutionPolicy&&, InVec1 x, InVec2 y, InMat E, OutMat A, Triangle t) noexcept {
  std::linalg::hermitian_matrix_rank_2_update(x, y, E, A, t);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas3.gemm]
// ---------------------------------------------------------------------------------------------
} // namespace std::linalg

namespace ycxx::detail {

// C = (E +) A B where a(i, k, b_kj, acc) adds the term A[i, k] * B[k, j] (structured A) and
// b(i, k, j, acc) is used instead when B is the structured operand.
template <class Out, class Term, class Init>
void la_mm(std::size_t m, std::size_t n, std::size_t p, const Out& c, Term term, Init init) {
  for (std::size_t i = 0; i < m; ++i)
    for (std::size_t j = 0; j < n; ++j) {
      typename Out::value_type acc = init(i, j);
      for (std::size_t k = 0; k < p; ++k)
        term(i, k, j, acc);
      la_m(c, i, j) = acc;
    }
}

template <class A, class B, class C>
constexpr void la_check_mm(const A& a, const B& b, const C& c) {
  static_assert(la_possibly_multipliable<A, B, C>(), "std::linalg: incompatible extents");
  ::ycxx::detail::precondition(la_multipliable(a, b, c), "std::linalg: extents differ");
}
template <class E, class C>
constexpr void la_check_e(const E& e, const C& c) {
  static_assert(la_possibly_addable<E, E, C>(), "std::linalg: incompatible extents");
  ::ycxx::detail::precondition(la_addable(e, e, c), "std::linalg: extents differ");
}

// The element (i, k) of a structured matrix times v (left operand) or v times (k, j) (right):
// symmetric, Hermitian or triangular with diagonal tag D.
enum class la_kind { symmetric, hermitian, triangular };
template <la_kind K, class Triangle, class D, class M, class V, class Acc>
void la_left_term(const M& m, std::size_t i, std::size_t k, const V& v, Acc& acc) {
  if constexpr (K == la_kind::triangular)
    la_tri_term<Triangle, D>(m, i, k, v, acc);
  else
    acc += la_sym < K == la_kind::hermitian ? la_structure::hermitian : la_structure::symmetric,
        Triangle > (m, i, k) * v;
}
template <la_kind K, class Triangle, class D, class M, class V, class Acc>
void la_right_term(const V& v, const M& m, std::size_t k, std::size_t j, Acc& acc) {
  if constexpr (K == la_kind::triangular) {
    if (!la_in_triangle<Triangle>(k, j))
      return;
    if (k == j && la_unit<D>)
      acc += v;
    else
      acc += v * la_m(m, k, j);
  } else {
    acc += v * la_sym < K == la_kind::hermitian ? la_structure::hermitian : la_structure::symmetric,
        Triangle > (m, k, j);
  }
}

// C = (E +) A B with A structured (Left) or B structured.
template <la_kind K, bool Left, class Triangle, class D, class A, class B, class E, class C>
void la_xxmm(const A& a, const B& b, const E* e, const C& c) {
  la_check_mm(a, b, c);
  if constexpr (Left)
    la_check_square<A, Triangle>(a);
  else
    la_check_square<B, Triangle>(b);
  if constexpr (!std::is_same_v<E, la_none>)
    la_check_e(*e, c);
  using V = typename C::value_type;
  la_mm(
      la_n(c, 0), la_n(c, 1), la_n(a, 1), c,
      [&](std::size_t i, std::size_t k, std::size_t j, V& acc) {
        if constexpr (Left)
          la_left_term<K, Triangle, D>(a, i, k, la_m(b, k, j), acc);
        else
          la_right_term<K, Triangle, D>(la_m(a, i, k), b, k, j, acc);
      },
      [&](std::size_t i, std::size_t j) {
        if constexpr (std::is_same_v<E, la_none>)
          return V{};
        else
          return V(la_m(*e, i, j));
      });
}

} // namespace ycxx::detail

namespace std::linalg {

template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2, ycxx::detail::la_out_matrix OutMat>
void matrix_product(InMat1 A, InMat2 B, OutMat C) {
  ycxx::detail::la_check_mm(A, B, C);
  using V = typename OutMat::value_type;
  ycxx::detail::la_mm(
      ycxx::detail::la_n(C, 0), ycxx::detail::la_n(C, 1), ycxx::detail::la_n(A, 1), C,
      [&](size_t i, size_t k, size_t j, V& acc) { acc += ycxx::detail::la_m(A, i, k) * ycxx::detail::la_m(B, k, j); },
      [](size_t, size_t) { return V{}; });
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void matrix_product(ExecutionPolicy&&, InMat1 A, InMat2 B, OutMat C) noexcept {
  std::linalg::matrix_product(A, B, C);
}
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2, ycxx::detail::la_in_matrix InMat3,
          ycxx::detail::la_out_matrix OutMat>
void matrix_product(InMat1 A, InMat2 B, InMat3 E, OutMat C) {
  ycxx::detail::la_check_mm(A, B, C);
  ycxx::detail::la_check_e(E, C);
  using V = typename OutMat::value_type;
  ycxx::detail::la_mm(
      ycxx::detail::la_n(C, 0), ycxx::detail::la_n(C, 1), ycxx::detail::la_n(A, 1), C,
      [&](size_t i, size_t k, size_t j, V& acc) { acc += ycxx::detail::la_m(A, i, k) * ycxx::detail::la_m(B, k, j); },
      [&](size_t i, size_t j) { return V(ycxx::detail::la_m(E, i, j)); });
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_in_matrix InMat3, ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void matrix_product(ExecutionPolicy&&, InMat1 A, InMat2 B, InMat3 E, OutMat C) noexcept {
  std::linalg::matrix_product(A, B, E, C);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas3.xxmm]
// ---------------------------------------------------------------------------------------------
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_out_matrix OutMat>
void symmetric_matrix_product(InMat1 A, Triangle, InMat2 B, OutMat C) {
  ycxx::detail::la_xxmm<ycxx::detail::la_kind::symmetric, true, Triangle, void>(A, B, ycxx::detail::la_no_e, C);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_in_matrix InMat2, ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void symmetric_matrix_product(ExecutionPolicy&&, InMat1 A, Triangle t, InMat2 B, OutMat C) noexcept {
  std::linalg::symmetric_matrix_product(A, t, B, C);
}
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_out_matrix OutMat>
void hermitian_matrix_product(InMat1 A, Triangle, InMat2 B, OutMat C) {
  ycxx::detail::la_xxmm<ycxx::detail::la_kind::hermitian, true, Triangle, void>(A, B, ycxx::detail::la_no_e, C);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_in_matrix InMat2, ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void hermitian_matrix_product(ExecutionPolicy&&, InMat1 A, Triangle t, InMat2 B, OutMat C) noexcept {
  std::linalg::hermitian_matrix_product(A, t, B, C);
}
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_out_matrix OutMat>
void triangular_matrix_product(InMat1 A, Triangle, DiagonalStorage, InMat2 B, OutMat C) {
  ycxx::detail::la_xxmm<ycxx::detail::la_kind::triangular, true, Triangle, DiagonalStorage>(A, B, ycxx::detail::la_no_e,
                                                                                            C);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_product(ExecutionPolicy&&, InMat1 A, Triangle t, DiagonalStorage d, InMat2 B,
                               OutMat C) noexcept {
  std::linalg::triangular_matrix_product(A, t, d, B, C);
}
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_out_matrix OutMat>
void symmetric_matrix_product(InMat1 A, InMat2 B, Triangle, OutMat C) {
  ycxx::detail::la_xxmm<ycxx::detail::la_kind::symmetric, false, Triangle, void>(A, B, ycxx::detail::la_no_e, C);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_triangle Triangle, ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void symmetric_matrix_product(ExecutionPolicy&&, InMat1 A, InMat2 B, Triangle t, OutMat C) noexcept {
  std::linalg::symmetric_matrix_product(A, B, t, C);
}
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_out_matrix OutMat>
void hermitian_matrix_product(InMat1 A, InMat2 B, Triangle, OutMat C) {
  ycxx::detail::la_xxmm<ycxx::detail::la_kind::hermitian, false, Triangle, void>(A, B, ycxx::detail::la_no_e, C);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_triangle Triangle, ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void hermitian_matrix_product(ExecutionPolicy&&, InMat1 A, InMat2 B, Triangle t, OutMat C) noexcept {
  std::linalg::hermitian_matrix_product(A, B, t, C);
}
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_out_matrix OutMat>
void triangular_matrix_product(InMat1 A, InMat2 B, Triangle, DiagonalStorage, OutMat C) {
  ycxx::detail::la_xxmm<ycxx::detail::la_kind::triangular, false, Triangle, DiagonalStorage>(A, B,
                                                                                             ycxx::detail::la_no_e, C);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_triangle Triangle, ycxx::detail::la_diagonal DiagonalStorage,
          ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_product(ExecutionPolicy&&, InMat1 A, InMat2 B, Triangle t, DiagonalStorage d,
                               OutMat C) noexcept {
  std::linalg::triangular_matrix_product(A, B, t, d, C);
}
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_in_matrix InMat3, ycxx::detail::la_out_matrix OutMat>
void symmetric_matrix_product(InMat1 A, Triangle, InMat2 B, InMat3 E, OutMat C) {
  ycxx::detail::la_xxmm<ycxx::detail::la_kind::symmetric, true, Triangle, void>(A, B, &E, C);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_in_matrix InMat2, ycxx::detail::la_in_matrix InMat3, ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void symmetric_matrix_product(ExecutionPolicy&&, InMat1 A, Triangle t, InMat2 B, InMat3 E, OutMat C) noexcept {
  std::linalg::symmetric_matrix_product(A, t, B, E, C);
}
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_in_matrix InMat3, ycxx::detail::la_out_matrix OutMat>
void hermitian_matrix_product(InMat1 A, Triangle, InMat2 B, InMat3 E, OutMat C) {
  ycxx::detail::la_xxmm<ycxx::detail::la_kind::hermitian, true, Triangle, void>(A, B, &E, C);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_in_matrix InMat2, ycxx::detail::la_in_matrix InMat3, ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void hermitian_matrix_product(ExecutionPolicy&&, InMat1 A, Triangle t, InMat2 B, InMat3 E, OutMat C) noexcept {
  std::linalg::hermitian_matrix_product(A, t, B, E, C);
}
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_in_matrix InMat3, ycxx::detail::la_out_matrix OutMat>
void triangular_matrix_product(InMat1 A, Triangle, DiagonalStorage, InMat2 B, InMat3 E, OutMat C) {
  ycxx::detail::la_xxmm<ycxx::detail::la_kind::triangular, true, Triangle, DiagonalStorage>(A, B, &E, C);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_in_matrix InMat3, ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_product(ExecutionPolicy&&, InMat1 A, Triangle t, DiagonalStorage d, InMat2 B, InMat3 E,
                               OutMat C) noexcept {
  std::linalg::triangular_matrix_product(A, t, d, B, E, C);
}
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_in_matrix InMat3, ycxx::detail::la_out_matrix OutMat>
void symmetric_matrix_product(InMat1 A, InMat2 B, Triangle, InMat3 E, OutMat C) {
  ycxx::detail::la_xxmm<ycxx::detail::la_kind::symmetric, false, Triangle, void>(A, B, &E, C);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_triangle Triangle, ycxx::detail::la_in_matrix InMat3, ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void symmetric_matrix_product(ExecutionPolicy&&, InMat1 A, InMat2 B, Triangle t, InMat3 E, OutMat C) noexcept {
  std::linalg::symmetric_matrix_product(A, B, t, E, C);
}
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_in_matrix InMat3, ycxx::detail::la_out_matrix OutMat>
void hermitian_matrix_product(InMat1 A, InMat2 B, Triangle, InMat3 E, OutMat C) {
  ycxx::detail::la_xxmm<ycxx::detail::la_kind::hermitian, false, Triangle, void>(A, B, &E, C);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_triangle Triangle, ycxx::detail::la_in_matrix InMat3, ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void hermitian_matrix_product(ExecutionPolicy&&, InMat1 A, InMat2 B, Triangle t, InMat3 E, OutMat C) noexcept {
  std::linalg::hermitian_matrix_product(A, B, t, E, C);
}
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_matrix InMat3,
          ycxx::detail::la_out_matrix OutMat>
void triangular_matrix_product(InMat1 A, InMat2 B, Triangle, DiagonalStorage, InMat3 E, OutMat C) {
  ycxx::detail::la_xxmm<ycxx::detail::la_kind::triangular, false, Triangle, DiagonalStorage>(A, B, &E, C);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_triangle Triangle, ycxx::detail::la_diagonal DiagonalStorage,
          ycxx::detail::la_in_matrix InMat3, ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_product(ExecutionPolicy&&, InMat1 A, InMat2 B, Triangle t, DiagonalStorage d, InMat3 E,
                               OutMat C) noexcept {
  std::linalg::triangular_matrix_product(A, B, t, d, E, C);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas3.trmm]
// ---------------------------------------------------------------------------------------------
template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_inout_matrix InOutMat>
void triangular_matrix_left_product(InMat A, Triangle, DiagonalStorage, InOutMat C) {
  ycxx::detail::la_check_square<InMat, Triangle>(A);
  ycxx::detail::la_check_mm(A, C, C);
  // C' = A C, column by column (an in-place triangular matrix-vector product each).
  for (size_t j = 0; j < ycxx::detail::la_n(C, 1); ++j)
    ycxx::detail::la_trmv_inplace<Triangle, DiagonalStorage, typename InOutMat::value_type>(
        A, [&](size_t i) -> decltype(auto) { return ycxx::detail::la_m(C, i, j); });
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_inout_matrix InOutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_left_product(ExecutionPolicy&&, InMat A, Triangle t, DiagonalStorage d, InOutMat C) noexcept {
  std::linalg::triangular_matrix_left_product(A, t, d, C);
}
template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_inout_matrix InOutMat>
void triangular_matrix_right_product(InMat A, Triangle, DiagonalStorage, InOutMat C) {
  ycxx::detail::la_check_square<InMat, Triangle>(A);
  ycxx::detail::la_check_mm(C, A, C);
  using V = typename InOutMat::value_type;
  size_t n = ycxx::detail::la_n(A, 0);
  // C' = C A, row by row: C'[r, j] = sum_k C[r, k] A[k, j] reads C[r, k] for k in the triangle
  // of column j, so upper columns go last to first and lower columns first to last.
  for (size_t r = 0; r < ycxx::detail::la_n(C, 0); ++r) {
    auto col = [&](size_t j) {
      V acc{};
      for (size_t k = 0; k < n; ++k)
        ycxx::detail::la_right_term<ycxx::detail::la_kind::triangular, Triangle, DiagonalStorage>(
            ycxx::detail::la_m(C, r, k), A, k, j, acc);
      ycxx::detail::la_m(C, r, j) = acc;
    };
    if constexpr (is_same_v<Triangle, upper_triangle_t>) {
      for (size_t j = n; j-- > 0;)
        col(j);
    } else {
      for (size_t j = 0; j < n; ++j)
        col(j);
    }
  }
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_inout_matrix InOutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_right_product(ExecutionPolicy&&, InMat A, Triangle t, DiagonalStorage d, InOutMat C) noexcept {
  std::linalg::triangular_matrix_right_product(A, t, d, C);
}

} // namespace std::linalg

namespace ycxx::detail {

// [linalg.algs.blas3.rankk]: C = (E +) alpha A A^T (or A A^H), triangle t of C.
template <la_structure S, class Triangle, class Scalar, class A, class E, class C>
void la_rankk(Scalar alpha, const A& a, const E* e, const C& c) {
  static_assert(la_triangle_matches<C, Triangle>,
                "std::linalg: the packed layout's Triangle differs from the function's Triangle");
  if constexpr (!std::is_same_v<E, la_none>)
    static_assert(la_triangle_matches<E, Triangle> && la_possibly_addable<C, E, C>(), "std::linalg: incompatible E");
  static_assert(la_compatible<C, A>(0, 0) && la_compatible<C, A>(1, 0), "std::linalg: incompatible extents");
  ::ycxx::detail::precondition(std::cmp_equal(c.extent(0), a.extent(0)) && std::cmp_equal(c.extent(1), a.extent(0)) &&
                                   la_e_addable(e, c),
                               "std::linalg: extents differ");
  using V = typename C::value_type;
  std::size_t p = la_n(a, 1);
  if constexpr (S == la_structure::hermitian) {
    auto real_alpha = la_adl::real_if_needed(alpha);
    la_rank_update<S, Triangle>(e, c, [&](std::size_t i, std::size_t j) {
      V sum{};
      for (std::size_t k = 0; k < p; ++k)
        sum += real_alpha * la_m(a, i, k) * la_adl::conj_if_needed(la_m(a, j, k));
      return sum;
    });
  } else {
    la_rank_update<S, Triangle>(e, c, [&](std::size_t i, std::size_t j) {
      V sum{};
      for (std::size_t k = 0; k < p; ++k)
        sum += alpha * la_m(a, i, k) * la_m(a, j, k);
      return sum;
    });
  }
}

// [linalg.algs.blas3.rank2k]: C = (E +) A B^T + B A^T (or A B^H + B A^H), triangle t of C.
template <la_structure S, class Triangle, class A, class B, class E, class C>
void la_rank2k(const A& a, const B& b, const E* e, const C& c) {
  static_assert(la_triangle_matches<C, Triangle>,
                "std::linalg: the packed layout's Triangle differs from the function's Triangle");
  if constexpr (!std::is_same_v<E, la_none>)
    static_assert(la_triangle_matches<E, Triangle> && la_possibly_addable<C, E, C>(), "std::linalg: incompatible E");
  static_assert(la_compatible<C, A>(0, 0) && la_compatible<C, B>(1, 0) && la_compatible<A, B>(1, 1) &&
                    la_compatible<C, B>(0, 0) && la_compatible<C, A>(1, 0),
                "std::linalg: incompatible extents");
  ::ycxx::detail::precondition(std::cmp_equal(c.extent(0), a.extent(0)) && std::cmp_equal(c.extent(1), b.extent(0)) &&
                                   std::cmp_equal(a.extent(1), b.extent(1)) &&
                                   std::cmp_equal(c.extent(0), b.extent(0)) &&
                                   std::cmp_equal(c.extent(1), a.extent(0)) && la_e_addable(e, c),
                               "std::linalg: extents differ");
  using V = typename C::value_type;
  std::size_t p = la_n(a, 1);
  la_rank_update<S, Triangle>(e, c, [&](std::size_t i, std::size_t j) {
    V sum{};
    for (std::size_t k = 0; k < p; ++k) {
      if constexpr (S == la_structure::hermitian) {
        sum += la_m(a, i, k) * la_adl::conj_if_needed(la_m(b, j, k));
        sum += la_m(b, i, k) * la_adl::conj_if_needed(la_m(a, j, k));
      } else {
        sum += la_m(a, i, k) * la_m(b, j, k);
        sum += la_m(b, i, k) * la_m(a, j, k);
      }
    }
    return sum;
  });
}

} // namespace ycxx::detail

namespace std::linalg {

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas3.rankk]
// ---------------------------------------------------------------------------------------------
template <ycxx::detail::la_scalar Scalar, ycxx::detail::la_in_matrix InMat,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
void symmetric_matrix_rank_k_update(Scalar alpha, InMat A, OutMat C, Triangle) {
  ycxx::detail::la_rankk<ycxx::detail::la_structure::symmetric, Triangle>(alpha, A, ycxx::detail::la_no_e, C);
}
template <class ExecutionPolicy, ycxx::detail::la_scalar Scalar, ycxx::detail::la_in_matrix InMat,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void symmetric_matrix_rank_k_update(ExecutionPolicy&&, Scalar alpha, InMat A, OutMat C, Triangle t) noexcept {
  std::linalg::symmetric_matrix_rank_k_update(alpha, A, C, t);
}
template <ycxx::detail::la_scalar Scalar, ycxx::detail::la_in_matrix InMat,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
void hermitian_matrix_rank_k_update(Scalar alpha, InMat A, OutMat C, Triangle) {
  ycxx::detail::la_rankk<ycxx::detail::la_structure::hermitian, Triangle>(alpha, A, ycxx::detail::la_no_e, C);
}
template <class ExecutionPolicy, ycxx::detail::la_scalar Scalar, ycxx::detail::la_in_matrix InMat,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void hermitian_matrix_rank_k_update(ExecutionPolicy&&, Scalar alpha, InMat A, OutMat C, Triangle t) noexcept {
  std::linalg::hermitian_matrix_rank_k_update(alpha, A, C, t);
}
template <ycxx::detail::la_scalar Scalar, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
void symmetric_matrix_rank_k_update(Scalar alpha, InMat1 A, InMat2 E, OutMat C, Triangle) {
  ycxx::detail::la_rankk<ycxx::detail::la_structure::symmetric, Triangle>(alpha, A, &E, C);
}
template <class ExecutionPolicy, ycxx::detail::la_scalar Scalar, ycxx::detail::la_in_matrix InMat1,
          ycxx::detail::la_in_matrix InMat2, ycxx::detail::la_possibly_packed_out_matrix OutMat,
          ycxx::detail::la_triangle Triangle>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void symmetric_matrix_rank_k_update(ExecutionPolicy&&, Scalar alpha, InMat1 A, InMat2 E, OutMat C,
                                    Triangle t) noexcept {
  std::linalg::symmetric_matrix_rank_k_update(alpha, A, E, C, t);
}
template <ycxx::detail::la_scalar Scalar, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
void hermitian_matrix_rank_k_update(Scalar alpha, InMat1 A, InMat2 E, OutMat C, Triangle) {
  ycxx::detail::la_rankk<ycxx::detail::la_structure::hermitian, Triangle>(alpha, A, &E, C);
}
template <class ExecutionPolicy, ycxx::detail::la_scalar Scalar, ycxx::detail::la_in_matrix InMat1,
          ycxx::detail::la_in_matrix InMat2, ycxx::detail::la_possibly_packed_out_matrix OutMat,
          ycxx::detail::la_triangle Triangle>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void hermitian_matrix_rank_k_update(ExecutionPolicy&&, Scalar alpha, InMat1 A, InMat2 E, OutMat C,
                                    Triangle t) noexcept {
  std::linalg::hermitian_matrix_rank_k_update(alpha, A, E, C, t);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas3.rank2k]
// ---------------------------------------------------------------------------------------------
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
void symmetric_matrix_rank_2k_update(InMat1 A, InMat2 B, OutMat C, Triangle) {
  ycxx::detail::la_rank2k<ycxx::detail::la_structure::symmetric, Triangle>(A, B, ycxx::detail::la_no_e, C);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void symmetric_matrix_rank_2k_update(ExecutionPolicy&&, InMat1 A, InMat2 B, OutMat C, Triangle t) noexcept {
  std::linalg::symmetric_matrix_rank_2k_update(A, B, C, t);
}
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
void hermitian_matrix_rank_2k_update(InMat1 A, InMat2 B, OutMat C, Triangle) {
  ycxx::detail::la_rank2k<ycxx::detail::la_structure::hermitian, Triangle>(A, B, ycxx::detail::la_no_e, C);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void hermitian_matrix_rank_2k_update(ExecutionPolicy&&, InMat1 A, InMat2 B, OutMat C, Triangle t) noexcept {
  std::linalg::hermitian_matrix_rank_2k_update(A, B, C, t);
}
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2, ycxx::detail::la_in_matrix InMat3,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
void symmetric_matrix_rank_2k_update(InMat1 A, InMat2 B, InMat3 E, OutMat C, Triangle) {
  ycxx::detail::la_rank2k<ycxx::detail::la_structure::symmetric, Triangle>(A, B, &E, C);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_in_matrix InMat3, ycxx::detail::la_possibly_packed_out_matrix OutMat,
          ycxx::detail::la_triangle Triangle>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void symmetric_matrix_rank_2k_update(ExecutionPolicy&&, InMat1 A, InMat2 B, InMat3 E, OutMat C, Triangle t) noexcept {
  std::linalg::symmetric_matrix_rank_2k_update(A, B, E, C, t);
}
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2, ycxx::detail::la_in_matrix InMat3,
          ycxx::detail::la_possibly_packed_out_matrix OutMat, ycxx::detail::la_triangle Triangle>
void hermitian_matrix_rank_2k_update(InMat1 A, InMat2 B, InMat3 E, OutMat C, Triangle) {
  ycxx::detail::la_rank2k<ycxx::detail::la_structure::hermitian, Triangle>(A, B, &E, C);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_in_matrix InMat3, ycxx::detail::la_possibly_packed_out_matrix OutMat,
          ycxx::detail::la_triangle Triangle>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void hermitian_matrix_rank_2k_update(ExecutionPolicy&&, InMat1 A, InMat2 B, InMat3 E, OutMat C, Triangle t) noexcept {
  std::linalg::hermitian_matrix_rank_2k_update(A, B, E, C, t);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas3.trsm], [linalg.algs.blas3.inplacetrsm]
// ---------------------------------------------------------------------------------------------
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_out_matrix OutMat, ycxx::detail::la_divide_op BinaryDivideOp>
void triangular_matrix_matrix_left_solve(InMat1 A, Triangle, DiagonalStorage, InMat2 B, OutMat X,
                                         BinaryDivideOp divide) {
  ycxx::detail::la_check_square<InMat1, Triangle>(A);
  ycxx::detail::la_check_mm(A, X, B);
  for (size_t j = 0; j < ycxx::detail::la_n(B, 1); ++j)
    ycxx::detail::la_trsv<Triangle, DiagonalStorage, typename OutMat::value_type>(
        A, [&](size_t i) -> decltype(auto) { return ycxx::detail::la_m(B, i, j); },
        [&](size_t i) -> decltype(auto) { return ycxx::detail::la_m(X, i, j); }, divide);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_out_matrix OutMat, ycxx::detail::la_divide_op BinaryDivideOp>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_matrix_left_solve(ExecutionPolicy&&, InMat1 A, Triangle t, DiagonalStorage d, InMat2 B, OutMat X,
                                         BinaryDivideOp divide) noexcept {
  std::linalg::triangular_matrix_matrix_left_solve(A, t, d, B, X, divide);
}
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_out_matrix OutMat>
void triangular_matrix_matrix_left_solve(InMat1 A, Triangle t, DiagonalStorage d, InMat2 B, OutMat X) {
  std::linalg::triangular_matrix_matrix_left_solve(A, t, d, B, X, divides<void>{});
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_matrix_left_solve(ExecutionPolicy&& exec, InMat1 A, Triangle t, DiagonalStorage d, InMat2 B,
                                         OutMat X) noexcept {
  std::linalg::triangular_matrix_matrix_left_solve(std::forward<ExecutionPolicy>(exec), A, t, d, B, X, divides<void>{});
}
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_out_matrix OutMat, ycxx::detail::la_divide_op BinaryDivideOp>
void triangular_matrix_matrix_right_solve(InMat1 A, Triangle, DiagonalStorage, InMat2 B, OutMat X,
                                          BinaryDivideOp divide) {
  ycxx::detail::la_check_square<InMat1, Triangle>(A);
  ycxx::detail::la_check_mm(X, A, B);
  for (size_t r = 0; r < ycxx::detail::la_n(B, 0); ++r)
    ycxx::detail::la_trsv_right<Triangle, DiagonalStorage>(A, B, X, r, divide);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_out_matrix OutMat, ycxx::detail::la_divide_op BinaryDivideOp>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_matrix_right_solve(ExecutionPolicy&&, InMat1 A, Triangle t, DiagonalStorage d, InMat2 B,
                                          OutMat X, BinaryDivideOp divide) noexcept {
  std::linalg::triangular_matrix_matrix_right_solve(A, t, d, B, X, divide);
}
template <ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_out_matrix OutMat>
void triangular_matrix_matrix_right_solve(InMat1 A, Triangle t, DiagonalStorage d, InMat2 B, OutMat X) {
  std::linalg::triangular_matrix_matrix_right_solve(A, t, d, B, X, divides<void>{});
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat1, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_in_matrix InMat2,
          ycxx::detail::la_out_matrix OutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_matrix_right_solve(ExecutionPolicy&& exec, InMat1 A, Triangle t, DiagonalStorage d, InMat2 B,
                                          OutMat X) noexcept {
  std::linalg::triangular_matrix_matrix_right_solve(std::forward<ExecutionPolicy>(exec), A, t, d, B, X,
                                                    divides<void>{});
}

template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_inout_matrix InOutMat,
          ycxx::detail::la_divide_op BinaryDivideOp>
void triangular_matrix_matrix_left_solve(InMat A, Triangle, DiagonalStorage, InOutMat B, BinaryDivideOp divide) {
  ycxx::detail::la_check_square<InMat, Triangle>(A);
  ycxx::detail::la_check_mm(A, B, B);
  for (size_t j = 0; j < ycxx::detail::la_n(B, 1); ++j) {
    auto at = [&](size_t i) -> decltype(auto) { return ycxx::detail::la_m(B, i, j); };
    ycxx::detail::la_trsv<Triangle, DiagonalStorage, typename InOutMat::value_type>(A, at, at, divide);
  }
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_inout_matrix InOutMat,
          ycxx::detail::la_divide_op BinaryDivideOp>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_matrix_left_solve(ExecutionPolicy&&, InMat A, Triangle t, DiagonalStorage d, InOutMat B,
                                         BinaryDivideOp divide) noexcept {
  std::linalg::triangular_matrix_matrix_left_solve(A, t, d, B, divide);
}
template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_inout_matrix InOutMat>
void triangular_matrix_matrix_left_solve(InMat A, Triangle t, DiagonalStorage d, InOutMat B) {
  std::linalg::triangular_matrix_matrix_left_solve(A, t, d, B, divides<void>{});
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_inout_matrix InOutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_matrix_left_solve(ExecutionPolicy&& exec, InMat A, Triangle t, DiagonalStorage d,
                                         InOutMat B) noexcept {
  std::linalg::triangular_matrix_matrix_left_solve(std::forward<ExecutionPolicy>(exec), A, t, d, B, divides<void>{});
}
template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_inout_matrix InOutMat,
          ycxx::detail::la_divide_op BinaryDivideOp>
void triangular_matrix_matrix_right_solve(InMat A, Triangle, DiagonalStorage, InOutMat B, BinaryDivideOp divide) {
  ycxx::detail::la_check_square<InMat, Triangle>(A);
  ycxx::detail::la_check_mm(B, A, B);
  for (size_t r = 0; r < ycxx::detail::la_n(B, 0); ++r)
    ycxx::detail::la_trsv_right<Triangle, DiagonalStorage>(A, B, B, r, divide);
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_inout_matrix InOutMat,
          ycxx::detail::la_divide_op BinaryDivideOp>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_matrix_right_solve(ExecutionPolicy&&, InMat A, Triangle t, DiagonalStorage d, InOutMat B,
                                          BinaryDivideOp divide) noexcept {
  std::linalg::triangular_matrix_matrix_right_solve(A, t, d, B, divide);
}
template <ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_inout_matrix InOutMat>
void triangular_matrix_matrix_right_solve(InMat A, Triangle t, DiagonalStorage d, InOutMat B) {
  std::linalg::triangular_matrix_matrix_right_solve(A, t, d, B, divides<void>{});
}
template <class ExecutionPolicy, ycxx::detail::la_in_matrix InMat, ycxx::detail::la_triangle Triangle,
          ycxx::detail::la_diagonal DiagonalStorage, ycxx::detail::la_inout_matrix InOutMat>
  requires ycxx::detail::execution_policy<ExecutionPolicy>
void triangular_matrix_matrix_right_solve(ExecutionPolicy&& exec, InMat A, Triangle t, DiagonalStorage d,
                                          InOutMat B) noexcept {
  std::linalg::triangular_matrix_matrix_right_solve(std::forward<ExecutionPolicy>(exec), A, t, d, B, divides<void>{});
}

} // namespace std::linalg
