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

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <class _Mp>
constexpr std::size_t __la_n(const _Mp& m, std::size_t r) noexcept {
  return static_cast<std::size_t>(m.extent(r));
}
// Element access with size_t indices.
template <class _Mp>
constexpr decltype(auto) __la_v(const _Mp& __v, std::size_t i) {
  return __v[static_cast<typename _Mp::index_type>(i)];
}
template <class _Mp>
constexpr decltype(auto) __la_m(const _Mp& a, std::size_t i, std::size_t __j) {
  using _Ip = typename _Mp::index_type;
  return a[static_cast<_Ip>(i), static_cast<_Ip>(__j)];
}

template <class _Triangle>
constexpr bool __la_in_triangle(std::size_t i, std::size_t __j) noexcept {
  if constexpr (std::is_same_v<_Triangle, std::linalg::upper_triangle_t>)
    return i <= __j;
  else
    return i >= __j;
}

// The value of a symmetric or Hermitian matrix at (i, j), reading only the triangle t
// ([linalg.general]/4).
enum class __la_structure { __symmetric, __hermitian };
template <__la_structure _Sp, class _Triangle, class _Mp>
constexpr typename _Mp::value_type __la_sym(const _Mp& a, std::size_t i, std::size_t __j) {
  using _Vp = typename _Mp::value_type;
  if constexpr (_Sp == __la_structure::__hermitian) {
    if (i == __j)
      return _Vp(__la_adl::__real_if_needed(__la_m(a, i, i)));
    if (__la_in_triangle<_Triangle>(i, __j))
      return _Vp(__la_m(a, i, __j));
    return _Vp(__la_adl::__conj_if_needed(__la_m(a, __j, i)));
  } else {
    if (__la_in_triangle<_Triangle>(i, __j))
      return _Vp(__la_m(a, i, __j));
    return _Vp(__la_m(a, __j, i));
  }
}

template <class _Dp>
inline constexpr bool __la_unit = std::is_same_v<_Dp, std::linalg::implicit_unit_diagonal_t>;

// The absent E of the overwriting overloads (an E parameter is passed by address).
struct __la_none {};
inline constexpr const __la_none* __la_no_e = nullptr;
// "addable(A, E, A) is true for those overloads with an E parameter".
template <class _Ep, class _Ap>
constexpr bool __la_e_addable(const _Ep* e, const _Ap& a) {
  if constexpr (std::is_same_v<_Ep, __la_none>)
    return true;
  else
    return __la_addable(a, *e, a);
}

// A matrix argument that is neither a function object nor an mdspan: the divide operation.
template <class _Tp>
concept __la_divide_op = !__md_is_mdspan<_Tp>;

// ---------------------------------------------------------------------------------------------
// Scaled sum of squares: the state of LAPACK's xLASSQ (scale * scale * ssq is the sum).
// ---------------------------------------------------------------------------------------------
template <class _Rp>
struct __la_ssq {
  _Rp scale{};
  _Rp __ssq{};
  bool nan = false, __inf = false;

  constexpr void add(_Rp a) {
    if (a != a) {
      nan = true;
    } else if (a > std::numeric_limits<_Rp>::max()) {
      __inf = true;
    } else if (a != _Rp(0)) {
      if (scale < a) {
        _Rp __q = scale / a;
        __ssq = _Rp(1) + __ssq * __q * __q;
        scale = a;
      } else {
        _Rp __q = a / scale;
        __ssq += __q * __q;
      }
    }
  }
  constexpr _Rp __root() const {
    if (nan)
      return std::numeric_limits<_Rp>::quiet_NaN();
    if (__inf)
      return std::numeric_limits<_Rp>::infinity();
    return scale * std::sqrt(__ssq);
  }
};

// The 2-norm of init and the elements of a vector or matrix ([linalg.algs.blas1.nrm2],
// [linalg.algs.blas1.matfrobnorm]).
template <class _Scalar, class _Mp>
_Scalar __la_two_norm(const _Mp& m, _Scalar init) {
  using _Vp = typename _Mp::value_type;
  using _Ap = decltype(__la_adl::__abs_if_needed(std::declval<_Vp>()));
  using _IA = decltype(__la_adl::__abs_if_needed(init));
  using _Rp = decltype(std::declval<_IA>() * std::declval<_IA>() + std::declval<_Ap>() * std::declval<_Ap>());
  static_assert(std::is_convertible_v<_Rp, _Scalar>, "std::linalg: the norm is not convertible to Scalar");
  __la_ssq<_Rp> s;
  s.add(static_cast<_Rp>(__la_adl::__abs_if_needed(init)));
  if constexpr (_Mp::rank() == 1) {
    for (std::size_t i = 0; i < __la_n(m, 0); ++i)
      s.add(static_cast<_Rp>(__la_adl::__abs_if_needed(_Vp(__la_v(m, i)))));
  } else {
    for (std::size_t i = 0; i < __la_n(m, 0); ++i)
      for (std::size_t __j = 0; __j < __la_n(m, 1); ++__j)
        s.add(static_cast<_Rp>(__la_adl::__abs_if_needed(_Vp(__la_m(m, i, __j)))));
  }
  return static_cast<_Scalar>(s.__root());
}

template <class _Tp>
inline constexpr bool __la_is_complex = false;
template <class _Tp>
inline constexpr bool __la_is_complex<std::complex<_Tp>> = true;
template <class _Tp>
concept __la_fp_or_complex = std::is_floating_point_v<_Tp> || __la_is_complex<_Tp>;

// |re| + |im| (or |x| for an arithmetic value): the magnitude of vector_abs_sum and
// vector_idx_abs_max.
template <class _Vp>
constexpr auto __la_abs_parts(const _Vp& __v) {
  if constexpr (std::is_arithmetic_v<_Vp>)
    return __la_adl::__abs_if_needed(__v);
  else
    return __la_adl::__abs_if_needed(__la_adl::__real_if_needed(__v)) + __la_adl::__abs_if_needed(__la_adl::__imag_if_needed(__v));
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace linalg {

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas1.givens]
// ---------------------------------------------------------------------------------------------
template <class _Real>
struct setup_givens_rotation_result {
  _Real c;
  _Real s;
  _Real r;
};
template <class _Real>
struct setup_givens_rotation_result<complex<_Real>> {
  _Real c;
  complex<_Real> s;
  complex<_Real> r;
};

// c = a / r, s = b / r with r = hypot(a, b) >= 0, the Euclidean norm.
template <__ycxx::__detail::__la_real _Real>
setup_givens_rotation_result<_Real> setup_givens_rotation(_Real a, _Real b) noexcept {
  _Real r = std::hypot(a, b);
  if (r == _Real(0))
    return {_Real(1), _Real(0), _Real(0)};
  return {a / r, b / r, r};
}
// With c real, r has the phase of a and the magnitude of the Euclidean norm (as LAPACK's xLARTG):
// c = |a| / n, s = (a / |a|) conj(b) / n, r = (a / |a|) n.
template <__ycxx::__detail::__la_real _Real>
setup_givens_rotation_result<complex<_Real>> setup_givens_rotation(complex<_Real> a, complex<_Real> b) noexcept {
  _Real __abs_a = std::abs(a), __abs_b = std::abs(b);
  if (__abs_b == _Real(0))
    return {_Real(1), complex<_Real>(0), a};
  if (__abs_a == _Real(0))
    return {_Real(0), std::conj(b) / __abs_b, complex<_Real>(__abs_b)};
  _Real n = std::hypot(__abs_a, __abs_b);
  complex<_Real> __phase = a / __abs_a;
  return {__abs_a / n, __phase * (std::conj(b) / n), __phase * n};
}

template <__ycxx::__detail::__la_inout_vector _InOutVec1, __ycxx::__detail::__la_inout_vector _InOutVec2, __ycxx::__detail::__la_real _Real>
void apply_givens_rotation(_InOutVec1 __x, _InOutVec2 y, _Real c, _Real s) {
  static_assert(__ycxx::__detail::__la_compatible<_InOutVec1, _InOutVec2>(0, 0), "apply_givens_rotation: incompatible extents");
  __ycxx::__detail::__precondition(cmp_equal(__x.extent(0), y.extent(0)), "apply_givens_rotation: extents differ");
  for (size_t i = 0; i < __ycxx::__detail::__la_n(__x, 0); ++i) {
    typename _InOutVec1::value_type __xi = __ycxx::__detail::__la_v(__x, i);
    typename _InOutVec2::value_type __yi = __ycxx::__detail::__la_v(y, i);
    __ycxx::__detail::__la_v(__x, i) = c * __xi + s * __yi;
    __ycxx::__detail::__la_v(y, i) = -s * __xi + c * __yi;
  }
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_inout_vector _InOutVec1, __ycxx::__detail::__la_inout_vector _InOutVec2,
          __ycxx::__detail::__la_real _Real>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void apply_givens_rotation(_ExecutionPolicy&&, _InOutVec1 __x, _InOutVec2 y, _Real c, _Real s) noexcept {
  std::linalg::apply_givens_rotation(__x, y, c, s);
}
template <__ycxx::__detail::__la_inout_vector _InOutVec1, __ycxx::__detail::__la_inout_vector _InOutVec2, __ycxx::__detail::__la_real _Real>
void apply_givens_rotation(_InOutVec1 __x, _InOutVec2 y, _Real c, complex<_Real> s) {
  static_assert(__ycxx::__detail::__la_compatible<_InOutVec1, _InOutVec2>(0, 0), "apply_givens_rotation: incompatible extents");
  __ycxx::__detail::__precondition(cmp_equal(__x.extent(0), y.extent(0)), "apply_givens_rotation: extents differ");
  for (size_t i = 0; i < __ycxx::__detail::__la_n(__x, 0); ++i) {
    typename _InOutVec1::value_type __xi = __ycxx::__detail::__la_v(__x, i);
    typename _InOutVec2::value_type __yi = __ycxx::__detail::__la_v(y, i);
    __ycxx::__detail::__la_v(__x, i) = c * __xi + s * __yi;
    __ycxx::__detail::__la_v(y, i) = -std::conj(s) * __xi + c * __yi;
  }
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_inout_vector _InOutVec1, __ycxx::__detail::__la_inout_vector _InOutVec2,
          __ycxx::__detail::__la_real _Real>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void apply_givens_rotation(_ExecutionPolicy&&, _InOutVec1 __x, _InOutVec2 y, _Real c, complex<_Real> s) noexcept {
  std::linalg::apply_givens_rotation(__x, y, c, s);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas1.swap], [linalg.algs.blas1.scal], [linalg.algs.blas1.copy],
// [linalg.algs.blas1.add]
// ---------------------------------------------------------------------------------------------
template <__ycxx::__detail::__la_inout_object _InOutObj1, __ycxx::__detail::__la_inout_object _InOutObj2>
  requires(_InOutObj1::rank() == _InOutObj2::rank())
void swap_elements(_InOutObj1 __x, _InOutObj2 y) {
  static_assert(__ycxx::__detail::__la_compatible<_InOutObj1, _InOutObj2>(0, 0) &&
                    (_InOutObj1::rank() == 1 || __ycxx::__detail::__la_compatible<_InOutObj1, _InOutObj2>(1, 1)),
                "swap_elements: incompatible extents");
  __ycxx::__detail::__precondition(__x.extents() == y.extents(), "swap_elements: extents differ");
  __ycxx::__detail::__md_for_each_index(__x.extents(), [&](auto... i) {
    typename _InOutObj1::value_type t = __x[i...];
    __x[i...] = y[i...];
    y[i...] = std::move(t);
  });
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_inout_object _InOutObj1, __ycxx::__detail::__la_inout_object _InOutObj2>
  requires(__ycxx::__detail::__execution_policy<_ExecutionPolicy> && _InOutObj1::rank() == _InOutObj2::rank())
void swap_elements(_ExecutionPolicy&&, _InOutObj1 __x, _InOutObj2 y) noexcept {
  std::linalg::swap_elements(__x, y);
}

template <__ycxx::__detail::__la_scalar _Scalar, __ycxx::__detail::__la_inout_object _InOutObj>
void scale(_Scalar alpha, _InOutObj __x) {
  __ycxx::__detail::__md_for_each_index(__x.extents(), [&](auto... i) { __x[i...] = alpha * __x[i...]; });
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_scalar _Scalar, __ycxx::__detail::__la_inout_object _InOutObj>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void scale(_ExecutionPolicy&&, _Scalar alpha, _InOutObj __x) noexcept {
  std::linalg::scale(alpha, __x);
}

template <__ycxx::__detail::__la_in_object _InObj, __ycxx::__detail::__la_out_object _OutObj>
  requires(_InObj::rank() == _OutObj::rank())
void copy(_InObj __x, _OutObj y) {
  static_assert(__ycxx::__detail::__la_compatible<_InObj, _OutObj>(0, 0) &&
                    (_InObj::rank() == 1 || __ycxx::__detail::__la_compatible<_InObj, _OutObj>(1, 1)),
                "std::linalg::copy: incompatible extents");
  __ycxx::__detail::__precondition(__x.extents() == y.extents(), "std::linalg::copy: extents differ");
  __ycxx::__detail::__md_for_each_index(__x.extents(), [&](auto... i) { y[i...] = __x[i...]; });
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_object _InObj, __ycxx::__detail::__la_out_object _OutObj>
  requires(__ycxx::__detail::__execution_policy<_ExecutionPolicy> && _InObj::rank() == _OutObj::rank())
void copy(_ExecutionPolicy&&, _InObj __x, _OutObj y) noexcept {
  std::linalg::copy(__x, y);
}

template <__ycxx::__detail::__la_in_object _InObj1, __ycxx::__detail::__la_in_object _InObj2, __ycxx::__detail::__la_out_object _OutObj>
  requires(_InObj1::rank() == _InObj2::rank() && _InObj1::rank() == _OutObj::rank())
void add(_InObj1 __x, _InObj2 y, _OutObj __z) {
  static_assert(__ycxx::__detail::__la_possibly_addable<_InObj1, _InObj2, _OutObj>(), "std::linalg::add: incompatible extents");
  __ycxx::__detail::__precondition(__ycxx::__detail::__la_addable(__x, y, __z), "std::linalg::add: extents differ");
  __ycxx::__detail::__md_for_each_index(__z.extents(), [&](auto... i) { __z[i...] = __x[i...] + y[i...]; });
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_object _InObj1, __ycxx::__detail::__la_in_object _InObj2,
          __ycxx::__detail::__la_out_object _OutObj>
  requires(__ycxx::__detail::__execution_policy<_ExecutionPolicy> && _InObj1::rank() == _InObj2::rank() &&
           _InObj1::rank() == _OutObj::rank())
void add(_ExecutionPolicy&&, _InObj1 __x, _InObj2 y, _OutObj __z) noexcept {
  std::linalg::add(__x, y, __z);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas1.dot]
// ---------------------------------------------------------------------------------------------
template <__ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2, __ycxx::__detail::__la_scalar _Scalar>
_Scalar dot(_InVec1 __v1, _InVec2 __v2, _Scalar init) {
  static_assert(__ycxx::__detail::__la_compatible<_InVec1, _InVec2>(0, 0), "std::linalg::dot: incompatible extents");
  __ycxx::__detail::__precondition(cmp_equal(__v1.extent(0), __v2.extent(0)), "std::linalg::dot: extents differ");
  _Scalar sum = init;
  for (size_t i = 0; i < __ycxx::__detail::__la_n(__v1, 0); ++i)
    sum += __ycxx::__detail::__la_v(__v1, i) * __ycxx::__detail::__la_v(__v2, i);
  return sum;
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2,
          __ycxx::__detail::__la_scalar _Scalar>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_Scalar dot(_ExecutionPolicy&&, _InVec1 __v1, _InVec2 __v2, _Scalar init) noexcept {
  return std::linalg::dot(__v1, __v2, init);
}
template <__ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2>
auto dot(_InVec1 __v1, _InVec2 __v2) {
  using _Tp = decltype(declval<typename _InVec1::value_type>() * declval<typename _InVec2::value_type>());
  return std::linalg::dot(__v1, __v2, _Tp{});
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
auto dot(_ExecutionPolicy&& __exec, _InVec1 __v1, _InVec2 __v2) noexcept {
  using _Tp = decltype(declval<typename _InVec1::value_type>() * declval<typename _InVec2::value_type>());
  return std::linalg::dot(std::forward<_ExecutionPolicy>(__exec), __v1, __v2, _Tp{});
}
template <__ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2, __ycxx::__detail::__la_scalar _Scalar>
_Scalar dotc(_InVec1 __v1, _InVec2 __v2, _Scalar init) {
  return std::linalg::dot(std::linalg::conjugated(__v1), __v2, init);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2,
          __ycxx::__detail::__la_scalar _Scalar>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_Scalar dotc(_ExecutionPolicy&& __exec, _InVec1 __v1, _InVec2 __v2, _Scalar init) noexcept {
  return std::linalg::dot(std::forward<_ExecutionPolicy>(__exec), std::linalg::conjugated(__v1), __v2, init);
}
template <__ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2>
auto dotc(_InVec1 __v1, _InVec2 __v2) {
  using _Tp = decltype(__ycxx::__detail::__la_adl::__conj_if_needed(declval<typename _InVec1::value_type>()) *
                     declval<typename _InVec2::value_type>());
  return std::linalg::dotc(__v1, __v2, _Tp{});
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
auto dotc(_ExecutionPolicy&& __exec, _InVec1 __v1, _InVec2 __v2) noexcept {
  using _Tp = decltype(__ycxx::__detail::__la_adl::__conj_if_needed(declval<typename _InVec1::value_type>()) *
                     declval<typename _InVec2::value_type>());
  return std::linalg::dotc(std::forward<_ExecutionPolicy>(__exec), __v1, __v2, _Tp{});
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas1.nrm2], [linalg.algs.blas1.asum], [linalg.algs.blas1.iamax]
// ---------------------------------------------------------------------------------------------
template <__ycxx::__detail::__la_in_vector _InVec, __ycxx::__detail::__la_scalar _Scalar>
_Scalar vector_two_norm(_InVec __v, _Scalar init) {
  static_assert(__ycxx::__detail::__la_fp_or_complex<typename _InVec::value_type> && __ycxx::__detail::__la_fp_or_complex<_Scalar>,
                "vector_two_norm: the value types must be floating-point or complex");
  return __ycxx::__detail::__la_two_norm(__v, init);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_vector _InVec, __ycxx::__detail::__la_scalar _Scalar>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_Scalar vector_two_norm(_ExecutionPolicy&&, _InVec __v, _Scalar init) noexcept {
  return std::linalg::vector_two_norm(__v, init);
}
template <__ycxx::__detail::__la_in_vector _InVec>
auto vector_two_norm(_InVec __v) {
  using _Ap = decltype(__ycxx::__detail::__la_adl::__abs_if_needed(declval<typename _InVec::value_type>()));
  return std::linalg::vector_two_norm(__v, decltype(declval<_Ap>() * declval<_Ap>()){});
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_vector _InVec>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
auto vector_two_norm(_ExecutionPolicy&&, _InVec __v) noexcept {
  return std::linalg::vector_two_norm(__v);
}

template <__ycxx::__detail::__la_in_vector _InVec, __ycxx::__detail::__la_scalar _Scalar>
_Scalar vector_abs_sum(_InVec __v, _Scalar init) {
  using _Vp = typename _InVec::value_type;
  static_assert(is_convertible_v<decltype(init + __ycxx::__detail::__la_abs_parts(declval<_Vp>())), _Scalar>,
                "vector_abs_sum: the sum is not convertible to Scalar");
  _Scalar sum = init;
  for (size_t i = 0; i < __ycxx::__detail::__la_n(__v, 0); ++i)
    sum += __ycxx::__detail::__la_abs_parts(_Vp(__ycxx::__detail::__la_v(__v, i)));
  return sum;
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_vector _InVec, __ycxx::__detail::__la_scalar _Scalar>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_Scalar vector_abs_sum(_ExecutionPolicy&&, _InVec __v, _Scalar init) noexcept {
  return std::linalg::vector_abs_sum(__v, init);
}
template <__ycxx::__detail::__la_in_vector _InVec>
auto vector_abs_sum(_InVec __v) {
  return std::linalg::vector_abs_sum(__v, typename _InVec::value_type{});
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_vector _InVec>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
auto vector_abs_sum(_ExecutionPolicy&&, _InVec __v) noexcept {
  return std::linalg::vector_abs_sum(__v);
}

template <__ycxx::__detail::__la_in_vector _InVec>
typename _InVec::size_type vector_idx_abs_max(_InVec __v) {
  using _Vp = typename _InVec::value_type;
  using _Tp = decltype(__ycxx::__detail::__la_abs_parts(declval<_Vp>()));
  static_assert(requires(_Tp a, _Tp b) { a < b; }, "vector_idx_abs_max: the magnitudes must be comparable with <");
  size_t n = __ycxx::__detail::__la_n(__v, 0);
  if (n == 0)
    return numeric_limits<typename _InVec::size_type>::max();
  size_t __best = 0;
  _Tp __best_value = __ycxx::__detail::__la_abs_parts(_Vp(__ycxx::__detail::__la_v(__v, 0)));
  for (size_t i = 1; i < n; ++i) {
    _Tp __x = __ycxx::__detail::__la_abs_parts(_Vp(__ycxx::__detail::__la_v(__v, i)));
    if (__best_value < __x) {
      __best = i;
      __best_value = __x;
    }
  }
  return static_cast<typename _InVec::size_type>(__best);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_vector _InVec>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
typename _InVec::size_type vector_idx_abs_max(_ExecutionPolicy&&, _InVec __v) noexcept {
  return std::linalg::vector_idx_abs_max(__v);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas1.matfrobnorm], [linalg.algs.blas1.matonenorm], [linalg.algs.blas1.matinfnorm]
// ---------------------------------------------------------------------------------------------
template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_scalar _Scalar>
_Scalar matrix_frob_norm(_InMat _Ap, _Scalar init) {
  static_assert(__ycxx::__detail::__la_fp_or_complex<typename _InMat::value_type> && __ycxx::__detail::__la_fp_or_complex<_Scalar>,
                "matrix_frob_norm: the value types must be floating-point or complex");
  return __ycxx::__detail::__la_two_norm(_Ap, init);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_scalar _Scalar>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_Scalar matrix_frob_norm(_ExecutionPolicy&&, _InMat _Ap, _Scalar init) noexcept {
  return std::linalg::matrix_frob_norm(_Ap, init);
}
template <__ycxx::__detail::__la_in_matrix _InMat>
auto matrix_frob_norm(_InMat _Ap) {
  using _Tp = decltype(__ycxx::__detail::__la_adl::__abs_if_needed(declval<typename _InMat::value_type>()));
  return std::linalg::matrix_frob_norm(_Ap, decltype(declval<_Tp>() * declval<_Tp>()){});
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
auto matrix_frob_norm(_ExecutionPolicy&&, _InMat _Ap) noexcept {
  return std::linalg::matrix_frob_norm(_Ap);
}

template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_scalar _Scalar>
_Scalar matrix_one_norm(_InMat _Ap, _Scalar init) {
  using _Vp = typename _InMat::value_type;
  static_assert(is_convertible_v<decltype(__ycxx::__detail::__la_adl::__abs_if_needed(declval<_Vp>())), _Scalar>,
                "matrix_one_norm: the magnitudes are not convertible to Scalar");
  size_t m = __ycxx::__detail::__la_n(_Ap, 0), n = __ycxx::__detail::__la_n(_Ap, 1);
  if (n == 0)
    return init;
  _Scalar __best{};
  for (size_t __j = 0; __j < n; ++__j) {
    _Scalar __col{};
    for (size_t i = 0; i < m; ++i)
      __col += __ycxx::__detail::__la_adl::__abs_if_needed(_Vp(__ycxx::__detail::__la_m(_Ap, i, __j)));
    if (__j == 0 || __best < __col)
      __best = __col;
  }
  return init + __best;
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_scalar _Scalar>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_Scalar matrix_one_norm(_ExecutionPolicy&&, _InMat _Ap, _Scalar init) noexcept {
  return std::linalg::matrix_one_norm(_Ap, init);
}
template <__ycxx::__detail::__la_in_matrix _InMat>
auto matrix_one_norm(_InMat _Ap) {
  using _Tp = decltype(__ycxx::__detail::__la_adl::__abs_if_needed(declval<typename _InMat::value_type>()));
  return std::linalg::matrix_one_norm(_Ap, _Tp{});
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
auto matrix_one_norm(_ExecutionPolicy&&, _InMat _Ap) noexcept {
  return std::linalg::matrix_one_norm(_Ap);
}

template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_scalar _Scalar>
_Scalar matrix_inf_norm(_InMat _Ap, _Scalar init) {
  using _Vp = typename _InMat::value_type;
  static_assert(is_convertible_v<decltype(__ycxx::__detail::__la_adl::__abs_if_needed(declval<_Vp>())), _Scalar>,
                "matrix_inf_norm: the magnitudes are not convertible to Scalar");
  size_t m = __ycxx::__detail::__la_n(_Ap, 0), n = __ycxx::__detail::__la_n(_Ap, 1);
  if (m == 0)
    return init;
  _Scalar __best{};
  for (size_t i = 0; i < m; ++i) {
    _Scalar __row{};
    for (size_t __j = 0; __j < n; ++__j)
      __row += __ycxx::__detail::__la_adl::__abs_if_needed(_Vp(__ycxx::__detail::__la_m(_Ap, i, __j)));
    if (i == 0 || __best < __row)
      __best = __row;
  }
  return init + __best;
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_scalar _Scalar>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
_Scalar matrix_inf_norm(_ExecutionPolicy&&, _InMat _Ap, _Scalar init) noexcept {
  return std::linalg::matrix_inf_norm(_Ap, init);
}
template <__ycxx::__detail::__la_in_matrix _InMat>
auto matrix_inf_norm(_InMat _Ap) {
  using _Tp = decltype(__ycxx::__detail::__la_adl::__abs_if_needed(declval<typename _InMat::value_type>()));
  return std::linalg::matrix_inf_norm(_Ap, _Tp{});
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
auto matrix_inf_norm(_ExecutionPolicy&&, _InMat _Ap) noexcept {
  return std::linalg::matrix_inf_norm(_Ap);
}

}} // namespace std::linalg

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// ---------------------------------------------------------------------------------------------
// Kernels shared by the BLAS 2 and 3 algorithms.
// ---------------------------------------------------------------------------------------------

// z[i] = (y[i] +) sum_j a(i, j) * x[j], where a(i, j, x_j, acc) adds the term for (i, j).
template <class _Out, class _Xp, class _Term, class Init>
void __la_mv(std::size_t m, std::size_t n, const _Xp& __x, const _Out& __z, _Term __term, Init init) {
  for (std::size_t i = 0; i < m; ++i) {
    typename _Out::value_type __acc = init(i);
    for (std::size_t __j = 0; __j < n; ++__j)
      __term(i, __j, __la_v(__x, __j), __acc);
    __la_v(__z, i) = __acc;
  }
}

// The checks common to the matrix-vector products with a z parameter.
template <class _Ap, class _Xp, class _Yp, class _Zp>
constexpr void __la_check_mv(const _Ap& a, const _Xp& __x, const _Yp& y, const _Zp& __z) {
  static_assert(__la_possibly_multipliable<_Ap, _Xp, _Yp>(), "std::linalg: incompatible extents");
  static_assert(__la_possibly_addable<_Yp, _Yp, _Zp>(), "std::linalg: incompatible extents");
  ::__ycxx::__detail::__precondition(__la_multipliable(a, __x, y) && __la_addable(y, y, __z), "std::linalg: extents differ");
}

// y = A x for a symmetric or Hermitian A (z = y + A x when Upd).
template <__la_structure _Sp, class _Triangle, class _Ap, class _Xp, class _Yp, class _Zp>
void __la_symv(const _Ap& a, const _Xp& __x, const _Yp& y, const _Zp& __z, bool __update) {
  using _Vp = typename _Zp::value_type;
  __la_mv(
      __la_n(a, 0), __la_n(a, 1), __x, __z,
      [&](std::size_t i, std::size_t __j, auto&& __xj, _Vp& __acc) { __acc += __la_sym<_Sp, _Triangle>(a, i, __j) * __xj; },
      [&](std::size_t i) { return __update ? _Vp(__la_v(y, i)) : _Vp{}; });
}

// One element of a triangular matrix-vector product: acc += A[i, j] * xj for (i, j) in the
// triangle, xj alone on an implicit unit diagonal.
template <class _Triangle, class _Diag, class _Ap, class _Xp, class _Vp>
void __la_tri_term(const _Ap& a, std::size_t i, std::size_t __j, const _Xp& __xj, _Vp& __acc) {
  if (!__la_in_triangle<_Triangle>(i, __j))
    return;
  if (i == __j && __la_unit<_Diag>)
    __acc += __xj;
  else
    __acc += __la_m(a, i, __j) * __xj;
}

// Solves A x = b for a triangular A; b(i) and x(i) access the elements (b and x may be the
// same vector, which is then overwritten in place).
template <class _Triangle, class _Diag, class _Vp, class _Ap, class _BAt, class _XAt, class _Divide>
void __la_trsv(const _Ap& a, _BAt b, _XAt __x, _Divide& __divide) {
  std::size_t n = __la_n(a, 0);
  auto __row = [&](std::size_t i) {
    _Vp t = b(i);
    for (std::size_t __j = 0; __j < n; ++__j)
      if (__j != i && __la_in_triangle<_Triangle>(i, __j))
        t -= __la_m(a, i, __j) * __x(__j);
    if constexpr (__la_unit<_Diag>)
      __x(i) = t;
    else
      __x(i) = __divide(t, __la_m(a, i, i));
  };
  if constexpr (std::is_same_v<_Triangle, std::linalg::lower_triangle_t>) {
    for (std::size_t i = 0; i < n; ++i)
      __row(i);
  } else {
    for (std::size_t i = n; i-- > 0;)
      __row(i);
  }
}

// y = A y in place for a triangular A; y(i) accesses the elements. y[i] depends on y[j] for j
// in the triangle of row i, so upper rows go first and lower rows last.
template <class _Triangle, class _Diag, class _Vp, class _Ap, class _YAt>
void __la_trmv_inplace(const _Ap& a, _YAt y) {
  std::size_t n = __la_n(a, 0);
  auto __row = [&](std::size_t i) {
    _Vp __acc{};
    for (std::size_t __j = 0; __j < n; ++__j)
      __la_tri_term<_Triangle, _Diag>(a, i, __j, y(__j), __acc);
    y(i) = __acc;
  };
  if constexpr (std::is_same_v<_Triangle, std::linalg::upper_triangle_t>) {
    for (std::size_t i = 0; i < n; ++i)
      __row(i);
  } else {
    for (std::size_t i = n; i-- > 0;)
      __row(i);
  }
}

// Solves x A = b for a triangular A, one row of a right solve (x and b may be the same).
template <class _Triangle, class _Diag, class _Ap, class _Bp, class _Xp, class _Divide>
void __la_trsv_right(const _Ap& a, const _Bp& b, const _Xp& __x, std::size_t r, _Divide& __divide) {
  using _Vp = typename _Xp::value_type;
  std::size_t n = __la_n(a, 0);
  auto __col = [&](std::size_t __j) {
    _Vp t = __la_m(b, r, __j);
    for (std::size_t k = 0; k < n; ++k)
      if (k != __j && __la_in_triangle<_Triangle>(k, __j))
        t -= __la_m(__x, r, k) * __la_m(a, k, __j);
    if constexpr (__la_unit<_Diag>)
      __la_m(__x, r, __j) = t;
    else
      __la_m(__x, r, __j) = __divide(t, __la_m(a, __j, __j));
  };
  if constexpr (std::is_same_v<_Triangle, std::linalg::upper_triangle_t>) {
    for (std::size_t __j = 0; __j < n; ++__j)
      __col(__j);
  } else {
    for (std::size_t __j = n; __j-- > 0;)
      __col(__j);
  }
}

// The checks of a symmetric/Hermitian/triangular A in the BLAS 2 and 3 algorithms.
template <class _Ap, class _Triangle>
constexpr void __la_check_square(const _Ap& a) {
  static_assert(__la_triangle_matches<_Ap, _Triangle>,
                "std::linalg: the packed layout's Triangle differs from the function's Triangle");
  static_assert(__la_compatible<_Ap, _Ap>(0, 1), "std::linalg: the matrix must be square");
  ::__ycxx::__detail::__precondition(a.extent(0) == a.extent(1), "std::linalg: the matrix must be square");
}

// C (triangle t) = E + alpha-weighted sum, the common body of the symmetric and Hermitian
// rank-1, rank-2, rank-k and rank-2k updates: value(i, j) is the added term.
template <__la_structure _Sp, class _Triangle, class _Ep, class _Cp, class _Value>
void __la_rank_update(const _Ep* e, const _Cp& c, _Value value) {
  using _Vp = typename _Cp::value_type;
  std::size_t n = __la_n(c, 0);
  for (std::size_t i = 0; i < n; ++i)
    for (std::size_t __j = 0; __j < n; ++__j) {
      if (!__la_in_triangle<_Triangle>(i, __j))
        continue;
      _Vp __acc{};
      if constexpr (!std::is_same_v<_Ep, __la_none>) {
        if constexpr (_Sp == __la_structure::__hermitian)
          __acc = i == __j ? _Vp(__la_adl::__real_if_needed(__la_m(*e, i, i))) : _Vp(__la_m(*e, i, __j));
        else
          __acc = _Vp(__la_m(*e, i, __j));
      }
      __acc += value(i, __j);
      __la_m(c, i, __j) = __acc;
    }
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace linalg {

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas2.gemv]
// ---------------------------------------------------------------------------------------------
template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_in_vector _InVec, __ycxx::__detail::__la_out_vector _OutVec>
void matrix_vector_product(_InMat _Ap, _InVec __x, _OutVec y) {
  static_assert(__ycxx::__detail::__la_possibly_multipliable<_InMat, _InVec, _OutVec>(),
                "matrix_vector_product: incompatible extents");
  __ycxx::__detail::__precondition(__ycxx::__detail::__la_multipliable(_Ap, __x, y), "matrix_vector_product: extents differ");
  using _Vp = typename _OutVec::value_type;
  __ycxx::__detail::__la_mv(
      __ycxx::__detail::__la_n(_Ap, 0), __ycxx::__detail::__la_n(_Ap, 1), __x, y,
      [&](size_t i, size_t __j, auto&& __xj, _Vp& __acc) { __acc += __ycxx::__detail::__la_m(_Ap, i, __j) * __xj; },
      [](size_t) { return _Vp{}; });
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_in_vector _InVec,
          __ycxx::__detail::__la_out_vector _OutVec>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void matrix_vector_product(_ExecutionPolicy&&, _InMat _Ap, _InVec __x, _OutVec y) noexcept {
  std::linalg::matrix_vector_product(_Ap, __x, y);
}
template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2,
          __ycxx::__detail::__la_out_vector _OutVec>
void matrix_vector_product(_InMat _Ap, _InVec1 __x, _InVec2 y, _OutVec __z) {
  __ycxx::__detail::__la_check_mv(_Ap, __x, y, __z);
  using _Vp = typename _OutVec::value_type;
  __ycxx::__detail::__la_mv(
      __ycxx::__detail::__la_n(_Ap, 0), __ycxx::__detail::__la_n(_Ap, 1), __x, __z,
      [&](size_t i, size_t __j, auto&& __xj, _Vp& __acc) { __acc += __ycxx::__detail::__la_m(_Ap, i, __j) * __xj; },
      [&](size_t i) { return _Vp(__ycxx::__detail::__la_v(y, i)); });
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_in_vector _InVec1,
          __ycxx::__detail::__la_in_vector _InVec2, __ycxx::__detail::__la_out_vector _OutVec>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void matrix_vector_product(_ExecutionPolicy&&, _InMat _Ap, _InVec1 __x, _InVec2 y, _OutVec __z) noexcept {
  std::linalg::matrix_vector_product(_Ap, __x, y, __z);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas2.symv], [linalg.algs.blas2.hemv]
// ---------------------------------------------------------------------------------------------
template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle, __ycxx::__detail::__la_in_vector _InVec,
          __ycxx::__detail::__la_out_vector _OutVec>
void symmetric_matrix_vector_product(_InMat _Ap, _Triangle, _InVec __x, _OutVec y) {
  __ycxx::__detail::__la_check_square<_InMat, _Triangle>(_Ap);
  static_assert(__ycxx::__detail::__la_possibly_multipliable<_InMat, _InVec, _OutVec>(),
                "symmetric_matrix_vector_product: incompatible extents");
  __ycxx::__detail::__precondition(__ycxx::__detail::__la_multipliable(_Ap, __x, y), "symmetric_matrix_vector_product: extents differ");
  __ycxx::__detail::__la_symv<__ycxx::__detail::__la_structure::__symmetric, _Triangle>(_Ap, __x, y, y, false);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_in_vector _InVec, __ycxx::__detail::__la_out_vector _OutVec>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void symmetric_matrix_vector_product(_ExecutionPolicy&&, _InMat _Ap, _Triangle t, _InVec __x, _OutVec y) noexcept {
  std::linalg::symmetric_matrix_vector_product(_Ap, t, __x, y);
}
template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle, __ycxx::__detail::__la_in_vector _InVec1,
          __ycxx::__detail::__la_in_vector _InVec2, __ycxx::__detail::__la_out_vector _OutVec>
void symmetric_matrix_vector_product(_InMat _Ap, _Triangle, _InVec1 __x, _InVec2 y, _OutVec __z) {
  __ycxx::__detail::__la_check_square<_InMat, _Triangle>(_Ap);
  __ycxx::__detail::__la_check_mv(_Ap, __x, y, __z);
  __ycxx::__detail::__la_symv<__ycxx::__detail::__la_structure::__symmetric, _Triangle>(_Ap, __x, y, __z, true);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2, __ycxx::__detail::__la_out_vector _OutVec>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void symmetric_matrix_vector_product(_ExecutionPolicy&&, _InMat _Ap, _Triangle t, _InVec1 __x, _InVec2 y, _OutVec __z) noexcept {
  std::linalg::symmetric_matrix_vector_product(_Ap, t, __x, y, __z);
}

template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle, __ycxx::__detail::__la_in_vector _InVec,
          __ycxx::__detail::__la_out_vector _OutVec>
void hermitian_matrix_vector_product(_InMat _Ap, _Triangle, _InVec __x, _OutVec y) {
  __ycxx::__detail::__la_check_square<_InMat, _Triangle>(_Ap);
  static_assert(__ycxx::__detail::__la_possibly_multipliable<_InMat, _InVec, _OutVec>(),
                "hermitian_matrix_vector_product: incompatible extents");
  __ycxx::__detail::__precondition(__ycxx::__detail::__la_multipliable(_Ap, __x, y), "hermitian_matrix_vector_product: extents differ");
  __ycxx::__detail::__la_symv<__ycxx::__detail::__la_structure::__hermitian, _Triangle>(_Ap, __x, y, y, false);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_in_vector _InVec, __ycxx::__detail::__la_out_vector _OutVec>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void hermitian_matrix_vector_product(_ExecutionPolicy&&, _InMat _Ap, _Triangle t, _InVec __x, _OutVec y) noexcept {
  std::linalg::hermitian_matrix_vector_product(_Ap, t, __x, y);
}
template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle, __ycxx::__detail::__la_in_vector _InVec1,
          __ycxx::__detail::__la_in_vector _InVec2, __ycxx::__detail::__la_out_vector _OutVec>
void hermitian_matrix_vector_product(_InMat _Ap, _Triangle, _InVec1 __x, _InVec2 y, _OutVec __z) {
  __ycxx::__detail::__la_check_square<_InMat, _Triangle>(_Ap);
  __ycxx::__detail::__la_check_mv(_Ap, __x, y, __z);
  __ycxx::__detail::__la_symv<__ycxx::__detail::__la_structure::__hermitian, _Triangle>(_Ap, __x, y, __z, true);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2, __ycxx::__detail::__la_out_vector _OutVec>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void hermitian_matrix_vector_product(_ExecutionPolicy&&, _InMat _Ap, _Triangle t, _InVec1 __x, _InVec2 y, _OutVec __z) noexcept {
  std::linalg::hermitian_matrix_vector_product(_Ap, t, __x, y, __z);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas2.trmv]
// ---------------------------------------------------------------------------------------------
template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_vector _InVec,
          __ycxx::__detail::__la_out_vector _OutVec>
void triangular_matrix_vector_product(_InMat _Ap, _Triangle, _DiagonalStorage, _InVec __x, _OutVec y) {
  __ycxx::__detail::__la_check_square<_InMat, _Triangle>(_Ap);
  static_assert(__ycxx::__detail::__la_compatible<_InMat, _OutVec>(0, 0) && __ycxx::__detail::__la_compatible<_InMat, _InVec>(0, 0),
                "triangular_matrix_vector_product: incompatible extents");
  __ycxx::__detail::__precondition(cmp_equal(_Ap.extent(0), y.extent(0)) && cmp_equal(_Ap.extent(0), __x.extent(0)),
                             "triangular_matrix_vector_product: extents differ");
  using _Vp = typename _OutVec::value_type;
  __ycxx::__detail::__la_mv(
      __ycxx::__detail::__la_n(_Ap, 0), __ycxx::__detail::__la_n(_Ap, 1), __x, y,
      [&](size_t i, size_t __j, auto&& __xj, _Vp& __acc) {
        __ycxx::__detail::__la_tri_term<_Triangle, _DiagonalStorage>(_Ap, i, __j, __xj, __acc);
      },
      [](size_t) { return _Vp{}; });
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_vector _InVec,
          __ycxx::__detail::__la_out_vector _OutVec>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_vector_product(_ExecutionPolicy&&, _InMat _Ap, _Triangle t, _DiagonalStorage d, _InVec __x,
                                      _OutVec y) noexcept {
  std::linalg::triangular_matrix_vector_product(_Ap, t, d, __x, y);
}
template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_inout_vector _InOutVec>
void triangular_matrix_vector_product(_InMat _Ap, _Triangle, _DiagonalStorage, _InOutVec y) {
  __ycxx::__detail::__la_check_square<_InMat, _Triangle>(_Ap);
  static_assert(__ycxx::__detail::__la_compatible<_InMat, _InOutVec>(0, 0),
                "triangular_matrix_vector_product: incompatible extents");
  __ycxx::__detail::__precondition(cmp_equal(_Ap.extent(0), y.extent(0)), "triangular_matrix_vector_product: extents differ");
  __ycxx::__detail::__la_trmv_inplace<_Triangle, _DiagonalStorage, typename _InOutVec::value_type>(
      _Ap, [&](size_t i) -> decltype(auto) { return __ycxx::__detail::__la_v(y, i); });
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_inout_vector _InOutVec>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_vector_product(_ExecutionPolicy&&, _InMat _Ap, _Triangle t, _DiagonalStorage d, _InOutVec y) noexcept {
  std::linalg::triangular_matrix_vector_product(_Ap, t, d, y);
}
template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_vector _InVec1,
          __ycxx::__detail::__la_in_vector _InVec2, __ycxx::__detail::__la_out_vector _OutVec>
void triangular_matrix_vector_product(_InMat _Ap, _Triangle, _DiagonalStorage, _InVec1 __x, _InVec2 y, _OutVec __z) {
  __ycxx::__detail::__la_check_square<_InMat, _Triangle>(_Ap);
  static_assert(__ycxx::__detail::__la_compatible<_InMat, _InVec2>(0, 0) && __ycxx::__detail::__la_compatible<_InMat, _InVec1>(0, 0) &&
                    __ycxx::__detail::__la_compatible<_InMat, _OutVec>(0, 0),
                "triangular_matrix_vector_product: incompatible extents");
  __ycxx::__detail::__precondition(cmp_equal(_Ap.extent(0), y.extent(0)) && cmp_equal(_Ap.extent(0), __x.extent(0)) &&
                                 cmp_equal(_Ap.extent(0), __z.extent(0)),
                             "triangular_matrix_vector_product: extents differ");
  using _Vp = typename _OutVec::value_type;
  __ycxx::__detail::__la_mv(
      __ycxx::__detail::__la_n(_Ap, 0), __ycxx::__detail::__la_n(_Ap, 1), __x, __z,
      [&](size_t i, size_t __j, auto&& __xj, _Vp& __acc) {
        __ycxx::__detail::__la_tri_term<_Triangle, _DiagonalStorage>(_Ap, i, __j, __xj, __acc);
      },
      [&](size_t i) { return _Vp(__ycxx::__detail::__la_v(y, i)); });
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_vector _InVec1,
          __ycxx::__detail::__la_in_vector _InVec2, __ycxx::__detail::__la_out_vector _OutVec>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_vector_product(_ExecutionPolicy&&, _InMat _Ap, _Triangle t, _DiagonalStorage d, _InVec1 __x, _InVec2 y,
                                      _OutVec __z) noexcept {
  std::linalg::triangular_matrix_vector_product(_Ap, t, d, __x, y, __z);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas2.trsv]
// ---------------------------------------------------------------------------------------------
template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_vector _InVec,
          __ycxx::__detail::__la_out_vector _OutVec, __ycxx::__detail::__la_divide_op _BinaryDivideOp>
void triangular_matrix_vector_solve(_InMat _Ap, _Triangle, _DiagonalStorage, _InVec b, _OutVec __x, _BinaryDivideOp __divide) {
  __ycxx::__detail::__la_check_square<_InMat, _Triangle>(_Ap);
  static_assert(__ycxx::__detail::__la_compatible<_InMat, _InVec>(0, 0) && __ycxx::__detail::__la_compatible<_InMat, _OutVec>(0, 0),
                "triangular_matrix_vector_solve: incompatible extents");
  __ycxx::__detail::__precondition(cmp_equal(_Ap.extent(0), b.extent(0)) && cmp_equal(_Ap.extent(0), __x.extent(0)),
                             "triangular_matrix_vector_solve: extents differ");
  __ycxx::__detail::__la_trsv<_Triangle, _DiagonalStorage, typename _OutVec::value_type>(
      _Ap, [&](size_t i) -> decltype(auto) { return __ycxx::__detail::__la_v(b, i); },
      [&](size_t i) -> decltype(auto) { return __ycxx::__detail::__la_v(__x, i); }, __divide);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_vector _InVec,
          __ycxx::__detail::__la_out_vector _OutVec, __ycxx::__detail::__la_divide_op _BinaryDivideOp>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_vector_solve(_ExecutionPolicy&&, _InMat _Ap, _Triangle t, _DiagonalStorage d, _InVec b, _OutVec __x,
                                    _BinaryDivideOp __divide) noexcept {
  std::linalg::triangular_matrix_vector_solve(_Ap, t, d, b, __x, __divide);
}
template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_vector _InVec,
          __ycxx::__detail::__la_out_vector _OutVec>
void triangular_matrix_vector_solve(_InMat _Ap, _Triangle t, _DiagonalStorage d, _InVec b, _OutVec __x) {
  std::linalg::triangular_matrix_vector_solve(_Ap, t, d, b, __x, divides<void>{});
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_vector _InVec,
          __ycxx::__detail::__la_out_vector _OutVec>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_vector_solve(_ExecutionPolicy&& __exec, _InMat _Ap, _Triangle t, _DiagonalStorage d, _InVec b,
                                    _OutVec __x) noexcept {
  std::linalg::triangular_matrix_vector_solve(std::forward<_ExecutionPolicy>(__exec), _Ap, t, d, b, __x, divides<void>{});
}
template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_inout_vector _InOutVec,
          __ycxx::__detail::__la_divide_op _BinaryDivideOp>
void triangular_matrix_vector_solve(_InMat _Ap, _Triangle, _DiagonalStorage, _InOutVec b, _BinaryDivideOp __divide) {
  __ycxx::__detail::__la_check_square<_InMat, _Triangle>(_Ap);
  static_assert(__ycxx::__detail::__la_compatible<_InMat, _InOutVec>(0, 0),
                "triangular_matrix_vector_solve: incompatible extents");
  __ycxx::__detail::__precondition(cmp_equal(_Ap.extent(0), b.extent(0)), "triangular_matrix_vector_solve: extents differ");
  auto at = [&](size_t i) -> decltype(auto) { return __ycxx::__detail::__la_v(b, i); };
  __ycxx::__detail::__la_trsv<_Triangle, _DiagonalStorage, typename _InOutVec::value_type>(_Ap, at, at, __divide);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_inout_vector _InOutVec,
          __ycxx::__detail::__la_divide_op _BinaryDivideOp>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_vector_solve(_ExecutionPolicy&&, _InMat _Ap, _Triangle t, _DiagonalStorage d, _InOutVec b,
                                    _BinaryDivideOp __divide) noexcept {
  std::linalg::triangular_matrix_vector_solve(_Ap, t, d, b, __divide);
}
template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_inout_vector _InOutVec>
void triangular_matrix_vector_solve(_InMat _Ap, _Triangle t, _DiagonalStorage d, _InOutVec b) {
  std::linalg::triangular_matrix_vector_solve(_Ap, t, d, b, divides<void>{});
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_inout_vector _InOutVec>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_vector_solve(_ExecutionPolicy&& __exec, _InMat _Ap, _Triangle t, _DiagonalStorage d,
                                    _InOutVec b) noexcept {
  std::linalg::triangular_matrix_vector_solve(std::forward<_ExecutionPolicy>(__exec), _Ap, t, d, b, divides<void>{});
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas2.rank1]
// ---------------------------------------------------------------------------------------------
template <__ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2, __ycxx::__detail::__la_out_matrix _OutMat>
void matrix_rank_1_update(_InVec1 __x, _InVec2 y, _OutMat _Ap) {
  static_assert(__ycxx::__detail::__la_possibly_multipliable<_OutMat, _InVec2, _InVec1>(),
                "matrix_rank_1_update: incompatible extents");
  __ycxx::__detail::__precondition(__ycxx::__detail::__la_multipliable(_Ap, y, __x), "matrix_rank_1_update: extents differ");
  for (size_t i = 0; i < __ycxx::__detail::__la_n(_Ap, 0); ++i)
    for (size_t __j = 0; __j < __ycxx::__detail::__la_n(_Ap, 1); ++__j)
      __ycxx::__detail::__la_m(_Ap, i, __j) = __ycxx::__detail::__la_v(__x, i) * __ycxx::__detail::__la_v(y, __j);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2,
          __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void matrix_rank_1_update(_ExecutionPolicy&&, _InVec1 __x, _InVec2 y, _OutMat _Ap) noexcept {
  std::linalg::matrix_rank_1_update(__x, y, _Ap);
}
template <__ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2, __ycxx::__detail::__la_in_matrix _InMat,
          __ycxx::__detail::__la_out_matrix _OutMat>
void matrix_rank_1_update(_InVec1 __x, _InVec2 y, _InMat _Ep, _OutMat _Ap) {
  static_assert(__ycxx::__detail::__la_possibly_multipliable<_OutMat, _InVec2, _InVec1>() &&
                    __ycxx::__detail::__la_possibly_addable<_OutMat, _InMat, _OutMat>(),
                "matrix_rank_1_update: incompatible extents");
  __ycxx::__detail::__precondition(__ycxx::__detail::__la_multipliable(_Ap, y, __x) && __ycxx::__detail::__la_addable(_Ap, _Ep, _Ap),
                             "matrix_rank_1_update: extents differ");
  using _Vp = typename _OutMat::value_type;
  for (size_t i = 0; i < __ycxx::__detail::__la_n(_Ap, 0); ++i)
    for (size_t __j = 0; __j < __ycxx::__detail::__la_n(_Ap, 1); ++__j) {
      _Vp __acc = __ycxx::__detail::__la_m(_Ep, i, __j);
      __acc += __ycxx::__detail::__la_v(__x, i) * __ycxx::__detail::__la_v(y, __j);
      __ycxx::__detail::__la_m(_Ap, i, __j) = __acc;
    }
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2,
          __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void matrix_rank_1_update(_ExecutionPolicy&&, _InVec1 __x, _InVec2 y, _InMat _Ep, _OutMat _Ap) noexcept {
  std::linalg::matrix_rank_1_update(__x, y, _Ep, _Ap);
}
template <__ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2, __ycxx::__detail::__la_out_matrix _OutMat>
void matrix_rank_1_update_c(_InVec1 __x, _InVec2 y, _OutMat _Ap) {
  std::linalg::matrix_rank_1_update(__x, std::linalg::conjugated(y), _Ap);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2,
          __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void matrix_rank_1_update_c(_ExecutionPolicy&& __exec, _InVec1 __x, _InVec2 y, _OutMat _Ap) noexcept {
  std::linalg::matrix_rank_1_update(std::forward<_ExecutionPolicy>(__exec), __x, std::linalg::conjugated(y), _Ap);
}
template <__ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2, __ycxx::__detail::__la_in_matrix _InMat,
          __ycxx::__detail::__la_out_matrix _OutMat>
void matrix_rank_1_update_c(_InVec1 __x, _InVec2 y, _InMat _Ep, _OutMat _Ap) {
  std::linalg::matrix_rank_1_update(__x, std::linalg::conjugated(y), _Ep, _Ap);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2,
          __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void matrix_rank_1_update_c(_ExecutionPolicy&& __exec, _InVec1 __x, _InVec2 y, _InMat _Ep, _OutMat _Ap) noexcept {
  std::linalg::matrix_rank_1_update(std::forward<_ExecutionPolicy>(__exec), __x, std::linalg::conjugated(y), _Ep, _Ap);
}

}} // namespace std::linalg

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// [linalg.algs.blas2.symherrank1]: A = (E +) alpha x x^T (or x x^H), triangle t of A.
template <__la_structure _Sp, class _Triangle, class _Scalar, class _Xp, class _Ep, class _Ap>
void __la_rank1(_Scalar alpha, const _Xp& __x, const _Ep* e, const _Ap& a) {
  static_assert(__la_triangle_matches<_Ap, _Triangle>,
                "std::linalg: the packed layout's Triangle differs from the function's Triangle");
  static_assert(__la_compatible<_Ap, _Ap>(0, 1) && __la_compatible<_Ap, _Xp>(0, 0), "std::linalg: incompatible extents");
  ::__ycxx::__detail::__precondition(a.extent(0) == a.extent(1) && std::cmp_equal(a.extent(0), __x.extent(0)) &&
                                   __la_e_addable(e, a),
                               "std::linalg: extents differ");
  if constexpr (_Sp == __la_structure::__hermitian) {
    auto __real_alpha = __la_adl::__real_if_needed(alpha);
    __la_rank_update<_Sp, _Triangle>(e, a, [&](std::size_t i, std::size_t __j) {
      return __real_alpha * __la_v(__x, i) * __la_adl::__conj_if_needed(__la_v(__x, __j));
    });
  } else {
    __la_rank_update<_Sp, _Triangle>(e, a, [&](std::size_t i, std::size_t __j) { return alpha * __la_v(__x, i) * __la_v(__x, __j); });
  }
}

// [linalg.algs.blas2.rank2]: A = (E +) x y^T + y x^T (or x y^H + y x^H), triangle t of A.
template <__la_structure _Sp, class _Triangle, class _Xp, class _Yp, class _Ep, class _Ap>
void __la_rank2(const _Xp& __x, const _Yp& y, const _Ep* e, const _Ap& a) {
  static_assert(__la_triangle_matches<_Ap, _Triangle>,
                "std::linalg: the packed layout's Triangle differs from the function's Triangle");
  if constexpr (!std::is_same_v<_Ep, __la_none>)
    static_assert(__la_triangle_matches<_Ep, _Triangle>,
                  "std::linalg: the packed layout's Triangle differs from the function's Triangle");
  static_assert(__la_compatible<_Ap, _Ap>(0, 1) && __la_possibly_multipliable<_Ap, _Xp, _Yp>(), "std::linalg: incompatible extents");
  ::__ycxx::__detail::__precondition(a.extent(0) == a.extent(1) && __la_multipliable(a, __x, y) && __la_e_addable(e, a),
                               "std::linalg: extents differ");
  if constexpr (_Sp == __la_structure::__hermitian)
    __la_rank_update<_Sp, _Triangle>(e, a, [&](std::size_t i, std::size_t __j) {
      return __la_v(__x, i) * __la_adl::__conj_if_needed(__la_v(y, __j)) + __la_v(y, i) * __la_adl::__conj_if_needed(__la_v(__x, __j));
    });
  else
    __la_rank_update<_Sp, _Triangle>(
        e, a, [&](std::size_t i, std::size_t __j) { return __la_v(__x, i) * __la_v(y, __j) + __la_v(y, i) * __la_v(__x, __j); });
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace linalg {

template <__ycxx::__detail::__la_scalar _Scalar, __ycxx::__detail::__la_in_vector _InVec,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
void symmetric_matrix_rank_1_update(_Scalar alpha, _InVec __x, _OutMat _Ap, _Triangle) {
  __ycxx::__detail::__la_rank1<__ycxx::__detail::__la_structure::__symmetric, _Triangle>(alpha, __x, __ycxx::__detail::__la_no_e, _Ap);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_scalar _Scalar, __ycxx::__detail::__la_in_vector _InVec,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void symmetric_matrix_rank_1_update(_ExecutionPolicy&&, _Scalar alpha, _InVec __x, _OutMat _Ap, _Triangle t) noexcept {
  std::linalg::symmetric_matrix_rank_1_update(alpha, __x, _Ap, t);
}
template <__ycxx::__detail::__la_scalar _Scalar, __ycxx::__detail::__la_in_vector _InVec,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
void hermitian_matrix_rank_1_update(_Scalar alpha, _InVec __x, _OutMat _Ap, _Triangle) {
  __ycxx::__detail::__la_rank1<__ycxx::__detail::__la_structure::__hermitian, _Triangle>(alpha, __x, __ycxx::__detail::__la_no_e, _Ap);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_scalar _Scalar, __ycxx::__detail::__la_in_vector _InVec,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void hermitian_matrix_rank_1_update(_ExecutionPolicy&&, _Scalar alpha, _InVec __x, _OutMat _Ap, _Triangle t) noexcept {
  std::linalg::hermitian_matrix_rank_1_update(alpha, __x, _Ap, t);
}
template <__ycxx::__detail::__la_scalar _Scalar, __ycxx::__detail::__la_in_vector _InVec, __ycxx::__detail::__la_in_matrix _InMat,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
void symmetric_matrix_rank_1_update(_Scalar alpha, _InVec __x, _InMat _Ep, _OutMat _Ap, _Triangle) {
  static_assert(__ycxx::__detail::__la_possibly_addable<_OutMat, _InMat, _OutMat>(),
                "symmetric_matrix_rank_1_update: incompatible extents");
  __ycxx::__detail::__la_rank1<__ycxx::__detail::__la_structure::__symmetric, _Triangle>(alpha, __x, &_Ep, _Ap);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_scalar _Scalar, __ycxx::__detail::__la_in_vector _InVec,
          __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat,
          __ycxx::__detail::__la_triangle _Triangle>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void symmetric_matrix_rank_1_update(_ExecutionPolicy&&, _Scalar alpha, _InVec __x, _InMat _Ep, _OutMat _Ap, _Triangle t) noexcept {
  std::linalg::symmetric_matrix_rank_1_update(alpha, __x, _Ep, _Ap, t);
}
template <__ycxx::__detail::__la_scalar _Scalar, __ycxx::__detail::__la_in_vector _InVec, __ycxx::__detail::__la_in_matrix _InMat,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
void hermitian_matrix_rank_1_update(_Scalar alpha, _InVec __x, _InMat _Ep, _OutMat _Ap, _Triangle) {
  static_assert(__ycxx::__detail::__la_possibly_addable<_OutMat, _InMat, _OutMat>(),
                "hermitian_matrix_rank_1_update: incompatible extents");
  __ycxx::__detail::__la_rank1<__ycxx::__detail::__la_structure::__hermitian, _Triangle>(alpha, __x, &_Ep, _Ap);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_scalar _Scalar, __ycxx::__detail::__la_in_vector _InVec,
          __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat,
          __ycxx::__detail::__la_triangle _Triangle>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void hermitian_matrix_rank_1_update(_ExecutionPolicy&&, _Scalar alpha, _InVec __x, _InMat _Ep, _OutMat _Ap, _Triangle t) noexcept {
  std::linalg::hermitian_matrix_rank_1_update(alpha, __x, _Ep, _Ap, t);
}

template <__ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
void symmetric_matrix_rank_2_update(_InVec1 __x, _InVec2 y, _OutMat _Ap, _Triangle) {
  __ycxx::__detail::__la_rank2<__ycxx::__detail::__la_structure::__symmetric, _Triangle>(__x, y, __ycxx::__detail::__la_no_e, _Ap);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void symmetric_matrix_rank_2_update(_ExecutionPolicy&&, _InVec1 __x, _InVec2 y, _OutMat _Ap, _Triangle t) noexcept {
  std::linalg::symmetric_matrix_rank_2_update(__x, y, _Ap, t);
}
template <__ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
void hermitian_matrix_rank_2_update(_InVec1 __x, _InVec2 y, _OutMat _Ap, _Triangle) {
  __ycxx::__detail::__la_rank2<__ycxx::__detail::__la_structure::__hermitian, _Triangle>(__x, y, __ycxx::__detail::__la_no_e, _Ap);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void hermitian_matrix_rank_2_update(_ExecutionPolicy&&, _InVec1 __x, _InVec2 y, _OutMat _Ap, _Triangle t) noexcept {
  std::linalg::hermitian_matrix_rank_2_update(__x, y, _Ap, t);
}
template <__ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2, __ycxx::__detail::__la_in_matrix _InMat,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
void symmetric_matrix_rank_2_update(_InVec1 __x, _InVec2 y, _InMat _Ep, _OutMat _Ap, _Triangle) {
  static_assert(__ycxx::__detail::__la_possibly_addable<_OutMat, _InMat, _OutMat>(),
                "symmetric_matrix_rank_2_update: incompatible extents");
  __ycxx::__detail::__la_rank2<__ycxx::__detail::__la_structure::__symmetric, _Triangle>(__x, y, &_Ep, _Ap);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2,
          __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat,
          __ycxx::__detail::__la_triangle _Triangle>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void symmetric_matrix_rank_2_update(_ExecutionPolicy&&, _InVec1 __x, _InVec2 y, _InMat _Ep, _OutMat _Ap, _Triangle t) noexcept {
  std::linalg::symmetric_matrix_rank_2_update(__x, y, _Ep, _Ap, t);
}
template <__ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2, __ycxx::__detail::__la_in_matrix _InMat,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
void hermitian_matrix_rank_2_update(_InVec1 __x, _InVec2 y, _InMat _Ep, _OutMat _Ap, _Triangle) {
  static_assert(__ycxx::__detail::__la_possibly_addable<_OutMat, _InMat, _OutMat>(),
                "hermitian_matrix_rank_2_update: incompatible extents");
  __ycxx::__detail::__la_rank2<__ycxx::__detail::__la_structure::__hermitian, _Triangle>(__x, y, &_Ep, _Ap);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_vector _InVec1, __ycxx::__detail::__la_in_vector _InVec2,
          __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat,
          __ycxx::__detail::__la_triangle _Triangle>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void hermitian_matrix_rank_2_update(_ExecutionPolicy&&, _InVec1 __x, _InVec2 y, _InMat _Ep, _OutMat _Ap, _Triangle t) noexcept {
  std::linalg::hermitian_matrix_rank_2_update(__x, y, _Ep, _Ap, t);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas3.gemm]
// ---------------------------------------------------------------------------------------------
}} // namespace std::linalg

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// C = (E +) A B where a(i, k, b_kj, acc) adds the term A[i, k] * B[k, j] (structured A) and
// b(i, k, j, acc) is used instead when B is the structured operand.
template <class _Out, class _Term, class Init>
void __la_mm(std::size_t m, std::size_t n, std::size_t p, const _Out& c, _Term __term, Init init) {
  for (std::size_t i = 0; i < m; ++i)
    for (std::size_t __j = 0; __j < n; ++__j) {
      typename _Out::value_type __acc = init(i, __j);
      for (std::size_t k = 0; k < p; ++k)
        __term(i, k, __j, __acc);
      __la_m(c, i, __j) = __acc;
    }
}

template <class _Ap, class _Bp, class _Cp>
constexpr void __la_check_mm(const _Ap& a, const _Bp& b, const _Cp& c) {
  static_assert(__la_possibly_multipliable<_Ap, _Bp, _Cp>(), "std::linalg: incompatible extents");
  ::__ycxx::__detail::__precondition(__la_multipliable(a, b, c), "std::linalg: extents differ");
}
template <class _Ep, class _Cp>
constexpr void __la_check_e(const _Ep& e, const _Cp& c) {
  static_assert(__la_possibly_addable<_Ep, _Ep, _Cp>(), "std::linalg: incompatible extents");
  ::__ycxx::__detail::__precondition(__la_addable(e, e, c), "std::linalg: extents differ");
}

// The element (i, k) of a structured matrix times v (left operand) or v times (k, j) (right):
// symmetric, Hermitian or triangular with diagonal tag D.
enum class __la_kind { __symmetric, __hermitian, __triangular };
template <__la_kind _Kp, class _Triangle, class _Dp, class _Mp, class _Vp, class _Acc>
void __la_left_term(const _Mp& m, std::size_t i, std::size_t k, const _Vp& __v, _Acc& __acc) {
  if constexpr (_Kp == __la_kind::__triangular)
    __la_tri_term<_Triangle, _Dp>(m, i, k, __v, __acc);
  else
    __acc += __la_sym < _Kp == __la_kind::__hermitian ? __la_structure::__hermitian : __la_structure::__symmetric,
        _Triangle > (m, i, k) * __v;
}
template <__la_kind _Kp, class _Triangle, class _Dp, class _Mp, class _Vp, class _Acc>
void __la_right_term(const _Vp& __v, const _Mp& m, std::size_t k, std::size_t __j, _Acc& __acc) {
  if constexpr (_Kp == __la_kind::__triangular) {
    if (!__la_in_triangle<_Triangle>(k, __j))
      return;
    if (k == __j && __la_unit<_Dp>)
      __acc += __v;
    else
      __acc += __v * __la_m(m, k, __j);
  } else {
    __acc += __v * __la_sym < _Kp == __la_kind::__hermitian ? __la_structure::__hermitian : __la_structure::__symmetric,
        _Triangle > (m, k, __j);
  }
}

// C = (E +) A B with A structured (Left) or B structured.
template <__la_kind _Kp, bool _Left, class _Triangle, class _Dp, class _Ap, class _Bp, class _Ep, class _Cp>
void __la_xxmm(const _Ap& a, const _Bp& b, const _Ep* e, const _Cp& c) {
  __la_check_mm(a, b, c);
  if constexpr (_Left)
    __la_check_square<_Ap, _Triangle>(a);
  else
    __la_check_square<_Bp, _Triangle>(b);
  if constexpr (!std::is_same_v<_Ep, __la_none>)
    __la_check_e(*e, c);
  using _Vp = typename _Cp::value_type;
  __la_mm(
      __la_n(c, 0), __la_n(c, 1), __la_n(a, 1), c,
      [&](std::size_t i, std::size_t k, std::size_t __j, _Vp& __acc) {
        if constexpr (_Left)
          __la_left_term<_Kp, _Triangle, _Dp>(a, i, k, __la_m(b, k, __j), __acc);
        else
          __la_right_term<_Kp, _Triangle, _Dp>(__la_m(a, i, k), b, k, __j, __acc);
      },
      [&](std::size_t i, std::size_t __j) {
        if constexpr (std::is_same_v<_Ep, __la_none>)
          return _Vp{};
        else
          return _Vp(__la_m(*e, i, __j));
      });
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace linalg {

template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2, __ycxx::__detail::__la_out_matrix _OutMat>
void matrix_product(_InMat1 _Ap, _InMat2 _Bp, _OutMat _Cp) {
  __ycxx::__detail::__la_check_mm(_Ap, _Bp, _Cp);
  using _Vp = typename _OutMat::value_type;
  __ycxx::__detail::__la_mm(
      __ycxx::__detail::__la_n(_Cp, 0), __ycxx::__detail::__la_n(_Cp, 1), __ycxx::__detail::__la_n(_Ap, 1), _Cp,
      [&](size_t i, size_t k, size_t __j, _Vp& __acc) { __acc += __ycxx::__detail::__la_m(_Ap, i, k) * __ycxx::__detail::__la_m(_Bp, k, __j); },
      [](size_t, size_t) { return _Vp{}; });
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void matrix_product(_ExecutionPolicy&&, _InMat1 _Ap, _InMat2 _Bp, _OutMat _Cp) noexcept {
  std::linalg::matrix_product(_Ap, _Bp, _Cp);
}
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2, __ycxx::__detail::__la_in_matrix _InMat3,
          __ycxx::__detail::__la_out_matrix _OutMat>
void matrix_product(_InMat1 _Ap, _InMat2 _Bp, _InMat3 _Ep, _OutMat _Cp) {
  __ycxx::__detail::__la_check_mm(_Ap, _Bp, _Cp);
  __ycxx::__detail::__la_check_e(_Ep, _Cp);
  using _Vp = typename _OutMat::value_type;
  __ycxx::__detail::__la_mm(
      __ycxx::__detail::__la_n(_Cp, 0), __ycxx::__detail::__la_n(_Cp, 1), __ycxx::__detail::__la_n(_Ap, 1), _Cp,
      [&](size_t i, size_t k, size_t __j, _Vp& __acc) { __acc += __ycxx::__detail::__la_m(_Ap, i, k) * __ycxx::__detail::__la_m(_Bp, k, __j); },
      [&](size_t i, size_t __j) { return _Vp(__ycxx::__detail::__la_m(_Ep, i, __j)); });
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_in_matrix _InMat3, __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void matrix_product(_ExecutionPolicy&&, _InMat1 _Ap, _InMat2 _Bp, _InMat3 _Ep, _OutMat _Cp) noexcept {
  std::linalg::matrix_product(_Ap, _Bp, _Ep, _Cp);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas3.xxmm]
// ---------------------------------------------------------------------------------------------
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_out_matrix _OutMat>
void symmetric_matrix_product(_InMat1 _Ap, _Triangle, _InMat2 _Bp, _OutMat _Cp) {
  __ycxx::__detail::__la_xxmm<__ycxx::__detail::__la_kind::__symmetric, true, _Triangle, void>(_Ap, _Bp, __ycxx::__detail::__la_no_e, _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_in_matrix _InMat2, __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void symmetric_matrix_product(_ExecutionPolicy&&, _InMat1 _Ap, _Triangle t, _InMat2 _Bp, _OutMat _Cp) noexcept {
  std::linalg::symmetric_matrix_product(_Ap, t, _Bp, _Cp);
}
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_out_matrix _OutMat>
void hermitian_matrix_product(_InMat1 _Ap, _Triangle, _InMat2 _Bp, _OutMat _Cp) {
  __ycxx::__detail::__la_xxmm<__ycxx::__detail::__la_kind::__hermitian, true, _Triangle, void>(_Ap, _Bp, __ycxx::__detail::__la_no_e, _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_in_matrix _InMat2, __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void hermitian_matrix_product(_ExecutionPolicy&&, _InMat1 _Ap, _Triangle t, _InMat2 _Bp, _OutMat _Cp) noexcept {
  std::linalg::hermitian_matrix_product(_Ap, t, _Bp, _Cp);
}
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_out_matrix _OutMat>
void triangular_matrix_product(_InMat1 _Ap, _Triangle, _DiagonalStorage, _InMat2 _Bp, _OutMat _Cp) {
  __ycxx::__detail::__la_xxmm<__ycxx::__detail::__la_kind::__triangular, true, _Triangle, _DiagonalStorage>(_Ap, _Bp, __ycxx::__detail::__la_no_e,
                                                                                            _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_product(_ExecutionPolicy&&, _InMat1 _Ap, _Triangle t, _DiagonalStorage d, _InMat2 _Bp,
                               _OutMat _Cp) noexcept {
  std::linalg::triangular_matrix_product(_Ap, t, d, _Bp, _Cp);
}
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_out_matrix _OutMat>
void symmetric_matrix_product(_InMat1 _Ap, _InMat2 _Bp, _Triangle, _OutMat _Cp) {
  __ycxx::__detail::__la_xxmm<__ycxx::__detail::__la_kind::__symmetric, false, _Triangle, void>(_Ap, _Bp, __ycxx::__detail::__la_no_e, _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_triangle _Triangle, __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void symmetric_matrix_product(_ExecutionPolicy&&, _InMat1 _Ap, _InMat2 _Bp, _Triangle t, _OutMat _Cp) noexcept {
  std::linalg::symmetric_matrix_product(_Ap, _Bp, t, _Cp);
}
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_out_matrix _OutMat>
void hermitian_matrix_product(_InMat1 _Ap, _InMat2 _Bp, _Triangle, _OutMat _Cp) {
  __ycxx::__detail::__la_xxmm<__ycxx::__detail::__la_kind::__hermitian, false, _Triangle, void>(_Ap, _Bp, __ycxx::__detail::__la_no_e, _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_triangle _Triangle, __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void hermitian_matrix_product(_ExecutionPolicy&&, _InMat1 _Ap, _InMat2 _Bp, _Triangle t, _OutMat _Cp) noexcept {
  std::linalg::hermitian_matrix_product(_Ap, _Bp, t, _Cp);
}
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_out_matrix _OutMat>
void triangular_matrix_product(_InMat1 _Ap, _InMat2 _Bp, _Triangle, _DiagonalStorage, _OutMat _Cp) {
  __ycxx::__detail::__la_xxmm<__ycxx::__detail::__la_kind::__triangular, false, _Triangle, _DiagonalStorage>(_Ap, _Bp,
                                                                                             __ycxx::__detail::__la_no_e, _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_triangle _Triangle, __ycxx::__detail::__la_diagonal _DiagonalStorage,
          __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_product(_ExecutionPolicy&&, _InMat1 _Ap, _InMat2 _Bp, _Triangle t, _DiagonalStorage d,
                               _OutMat _Cp) noexcept {
  std::linalg::triangular_matrix_product(_Ap, _Bp, t, d, _Cp);
}
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_in_matrix _InMat3, __ycxx::__detail::__la_out_matrix _OutMat>
void symmetric_matrix_product(_InMat1 _Ap, _Triangle, _InMat2 _Bp, _InMat3 _Ep, _OutMat _Cp) {
  __ycxx::__detail::__la_xxmm<__ycxx::__detail::__la_kind::__symmetric, true, _Triangle, void>(_Ap, _Bp, &_Ep, _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_in_matrix _InMat2, __ycxx::__detail::__la_in_matrix _InMat3, __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void symmetric_matrix_product(_ExecutionPolicy&&, _InMat1 _Ap, _Triangle t, _InMat2 _Bp, _InMat3 _Ep, _OutMat _Cp) noexcept {
  std::linalg::symmetric_matrix_product(_Ap, t, _Bp, _Ep, _Cp);
}
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_in_matrix _InMat3, __ycxx::__detail::__la_out_matrix _OutMat>
void hermitian_matrix_product(_InMat1 _Ap, _Triangle, _InMat2 _Bp, _InMat3 _Ep, _OutMat _Cp) {
  __ycxx::__detail::__la_xxmm<__ycxx::__detail::__la_kind::__hermitian, true, _Triangle, void>(_Ap, _Bp, &_Ep, _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_in_matrix _InMat2, __ycxx::__detail::__la_in_matrix _InMat3, __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void hermitian_matrix_product(_ExecutionPolicy&&, _InMat1 _Ap, _Triangle t, _InMat2 _Bp, _InMat3 _Ep, _OutMat _Cp) noexcept {
  std::linalg::hermitian_matrix_product(_Ap, t, _Bp, _Ep, _Cp);
}
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_in_matrix _InMat3, __ycxx::__detail::__la_out_matrix _OutMat>
void triangular_matrix_product(_InMat1 _Ap, _Triangle, _DiagonalStorage, _InMat2 _Bp, _InMat3 _Ep, _OutMat _Cp) {
  __ycxx::__detail::__la_xxmm<__ycxx::__detail::__la_kind::__triangular, true, _Triangle, _DiagonalStorage>(_Ap, _Bp, &_Ep, _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_in_matrix _InMat3, __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_product(_ExecutionPolicy&&, _InMat1 _Ap, _Triangle t, _DiagonalStorage d, _InMat2 _Bp, _InMat3 _Ep,
                               _OutMat _Cp) noexcept {
  std::linalg::triangular_matrix_product(_Ap, t, d, _Bp, _Ep, _Cp);
}
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_in_matrix _InMat3, __ycxx::__detail::__la_out_matrix _OutMat>
void symmetric_matrix_product(_InMat1 _Ap, _InMat2 _Bp, _Triangle, _InMat3 _Ep, _OutMat _Cp) {
  __ycxx::__detail::__la_xxmm<__ycxx::__detail::__la_kind::__symmetric, false, _Triangle, void>(_Ap, _Bp, &_Ep, _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_triangle _Triangle, __ycxx::__detail::__la_in_matrix _InMat3, __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void symmetric_matrix_product(_ExecutionPolicy&&, _InMat1 _Ap, _InMat2 _Bp, _Triangle t, _InMat3 _Ep, _OutMat _Cp) noexcept {
  std::linalg::symmetric_matrix_product(_Ap, _Bp, t, _Ep, _Cp);
}
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_in_matrix _InMat3, __ycxx::__detail::__la_out_matrix _OutMat>
void hermitian_matrix_product(_InMat1 _Ap, _InMat2 _Bp, _Triangle, _InMat3 _Ep, _OutMat _Cp) {
  __ycxx::__detail::__la_xxmm<__ycxx::__detail::__la_kind::__hermitian, false, _Triangle, void>(_Ap, _Bp, &_Ep, _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_triangle _Triangle, __ycxx::__detail::__la_in_matrix _InMat3, __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void hermitian_matrix_product(_ExecutionPolicy&&, _InMat1 _Ap, _InMat2 _Bp, _Triangle t, _InMat3 _Ep, _OutMat _Cp) noexcept {
  std::linalg::hermitian_matrix_product(_Ap, _Bp, t, _Ep, _Cp);
}
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_matrix _InMat3,
          __ycxx::__detail::__la_out_matrix _OutMat>
void triangular_matrix_product(_InMat1 _Ap, _InMat2 _Bp, _Triangle, _DiagonalStorage, _InMat3 _Ep, _OutMat _Cp) {
  __ycxx::__detail::__la_xxmm<__ycxx::__detail::__la_kind::__triangular, false, _Triangle, _DiagonalStorage>(_Ap, _Bp, &_Ep, _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_triangle _Triangle, __ycxx::__detail::__la_diagonal _DiagonalStorage,
          __ycxx::__detail::__la_in_matrix _InMat3, __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_product(_ExecutionPolicy&&, _InMat1 _Ap, _InMat2 _Bp, _Triangle t, _DiagonalStorage d, _InMat3 _Ep,
                               _OutMat _Cp) noexcept {
  std::linalg::triangular_matrix_product(_Ap, _Bp, t, d, _Ep, _Cp);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas3.trmm]
// ---------------------------------------------------------------------------------------------
template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_inout_matrix _InOutMat>
void triangular_matrix_left_product(_InMat _Ap, _Triangle, _DiagonalStorage, _InOutMat _Cp) {
  __ycxx::__detail::__la_check_square<_InMat, _Triangle>(_Ap);
  __ycxx::__detail::__la_check_mm(_Ap, _Cp, _Cp);
  // C' = A C, column by column (an in-place triangular matrix-vector product each).
  for (size_t __j = 0; __j < __ycxx::__detail::__la_n(_Cp, 1); ++__j)
    __ycxx::__detail::__la_trmv_inplace<_Triangle, _DiagonalStorage, typename _InOutMat::value_type>(
        _Ap, [&](size_t i) -> decltype(auto) { return __ycxx::__detail::__la_m(_Cp, i, __j); });
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_inout_matrix _InOutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_left_product(_ExecutionPolicy&&, _InMat _Ap, _Triangle t, _DiagonalStorage d, _InOutMat _Cp) noexcept {
  std::linalg::triangular_matrix_left_product(_Ap, t, d, _Cp);
}
template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_inout_matrix _InOutMat>
void triangular_matrix_right_product(_InMat _Ap, _Triangle, _DiagonalStorage, _InOutMat _Cp) {
  __ycxx::__detail::__la_check_square<_InMat, _Triangle>(_Ap);
  __ycxx::__detail::__la_check_mm(_Cp, _Ap, _Cp);
  using _Vp = typename _InOutMat::value_type;
  size_t n = __ycxx::__detail::__la_n(_Ap, 0);
  // C' = C A, row by row: C'[r, j] = sum_k C[r, k] A[k, j] reads C[r, k] for k in the triangle
  // of column j, so upper columns go last to first and lower columns first to last.
  for (size_t r = 0; r < __ycxx::__detail::__la_n(_Cp, 0); ++r) {
    auto __col = [&](size_t __j) {
      _Vp __acc{};
      for (size_t k = 0; k < n; ++k)
        __ycxx::__detail::__la_right_term<__ycxx::__detail::__la_kind::__triangular, _Triangle, _DiagonalStorage>(
            __ycxx::__detail::__la_m(_Cp, r, k), _Ap, k, __j, __acc);
      __ycxx::__detail::__la_m(_Cp, r, __j) = __acc;
    };
    if constexpr (is_same_v<_Triangle, upper_triangle_t>) {
      for (size_t __j = n; __j-- > 0;)
        __col(__j);
    } else {
      for (size_t __j = 0; __j < n; ++__j)
        __col(__j);
    }
  }
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_inout_matrix _InOutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_right_product(_ExecutionPolicy&&, _InMat _Ap, _Triangle t, _DiagonalStorage d, _InOutMat _Cp) noexcept {
  std::linalg::triangular_matrix_right_product(_Ap, t, d, _Cp);
}

}} // namespace std::linalg

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// [linalg.algs.blas3.rankk]: C = (E +) alpha A A^T (or A A^H), triangle t of C.
template <__la_structure _Sp, class _Triangle, class _Scalar, class _Ap, class _Ep, class _Cp>
void __la_rankk(_Scalar alpha, const _Ap& a, const _Ep* e, const _Cp& c) {
  static_assert(__la_triangle_matches<_Cp, _Triangle>,
                "std::linalg: the packed layout's Triangle differs from the function's Triangle");
  if constexpr (!std::is_same_v<_Ep, __la_none>)
    static_assert(__la_triangle_matches<_Ep, _Triangle> && __la_possibly_addable<_Cp, _Ep, _Cp>(), "std::linalg: incompatible E");
  static_assert(__la_compatible<_Cp, _Ap>(0, 0) && __la_compatible<_Cp, _Ap>(1, 0), "std::linalg: incompatible extents");
  ::__ycxx::__detail::__precondition(std::cmp_equal(c.extent(0), a.extent(0)) && std::cmp_equal(c.extent(1), a.extent(0)) &&
                                   __la_e_addable(e, c),
                               "std::linalg: extents differ");
  using _Vp = typename _Cp::value_type;
  std::size_t p = __la_n(a, 1);
  if constexpr (_Sp == __la_structure::__hermitian) {
    auto __real_alpha = __la_adl::__real_if_needed(alpha);
    __la_rank_update<_Sp, _Triangle>(e, c, [&](std::size_t i, std::size_t __j) {
      _Vp sum{};
      for (std::size_t k = 0; k < p; ++k)
        sum += __real_alpha * __la_m(a, i, k) * __la_adl::__conj_if_needed(__la_m(a, __j, k));
      return sum;
    });
  } else {
    __la_rank_update<_Sp, _Triangle>(e, c, [&](std::size_t i, std::size_t __j) {
      _Vp sum{};
      for (std::size_t k = 0; k < p; ++k)
        sum += alpha * __la_m(a, i, k) * __la_m(a, __j, k);
      return sum;
    });
  }
}

// [linalg.algs.blas3.rank2k]: C = (E +) A B^T + B A^T (or A B^H + B A^H), triangle t of C.
template <__la_structure _Sp, class _Triangle, class _Ap, class _Bp, class _Ep, class _Cp>
void __la_rank2k(const _Ap& a, const _Bp& b, const _Ep* e, const _Cp& c) {
  static_assert(__la_triangle_matches<_Cp, _Triangle>,
                "std::linalg: the packed layout's Triangle differs from the function's Triangle");
  if constexpr (!std::is_same_v<_Ep, __la_none>)
    static_assert(__la_triangle_matches<_Ep, _Triangle> && __la_possibly_addable<_Cp, _Ep, _Cp>(), "std::linalg: incompatible E");
  static_assert(__la_compatible<_Cp, _Ap>(0, 0) && __la_compatible<_Cp, _Bp>(1, 0) && __la_compatible<_Ap, _Bp>(1, 1) &&
                    __la_compatible<_Cp, _Bp>(0, 0) && __la_compatible<_Cp, _Ap>(1, 0),
                "std::linalg: incompatible extents");
  ::__ycxx::__detail::__precondition(std::cmp_equal(c.extent(0), a.extent(0)) && std::cmp_equal(c.extent(1), b.extent(0)) &&
                                   std::cmp_equal(a.extent(1), b.extent(1)) &&
                                   std::cmp_equal(c.extent(0), b.extent(0)) &&
                                   std::cmp_equal(c.extent(1), a.extent(0)) && __la_e_addable(e, c),
                               "std::linalg: extents differ");
  using _Vp = typename _Cp::value_type;
  std::size_t p = __la_n(a, 1);
  __la_rank_update<_Sp, _Triangle>(e, c, [&](std::size_t i, std::size_t __j) {
    _Vp sum{};
    for (std::size_t k = 0; k < p; ++k) {
      if constexpr (_Sp == __la_structure::__hermitian) {
        sum += __la_m(a, i, k) * __la_adl::__conj_if_needed(__la_m(b, __j, k));
        sum += __la_m(b, i, k) * __la_adl::__conj_if_needed(__la_m(a, __j, k));
      } else {
        sum += __la_m(a, i, k) * __la_m(b, __j, k);
        sum += __la_m(b, i, k) * __la_m(a, __j, k);
      }
    }
    return sum;
  });
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std { namespace linalg {

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas3.rankk]
// ---------------------------------------------------------------------------------------------
template <__ycxx::__detail::__la_scalar _Scalar, __ycxx::__detail::__la_in_matrix _InMat,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
void symmetric_matrix_rank_k_update(_Scalar alpha, _InMat _Ap, _OutMat _Cp, _Triangle) {
  __ycxx::__detail::__la_rankk<__ycxx::__detail::__la_structure::__symmetric, _Triangle>(alpha, _Ap, __ycxx::__detail::__la_no_e, _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_scalar _Scalar, __ycxx::__detail::__la_in_matrix _InMat,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void symmetric_matrix_rank_k_update(_ExecutionPolicy&&, _Scalar alpha, _InMat _Ap, _OutMat _Cp, _Triangle t) noexcept {
  std::linalg::symmetric_matrix_rank_k_update(alpha, _Ap, _Cp, t);
}
template <__ycxx::__detail::__la_scalar _Scalar, __ycxx::__detail::__la_in_matrix _InMat,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
void hermitian_matrix_rank_k_update(_Scalar alpha, _InMat _Ap, _OutMat _Cp, _Triangle) {
  __ycxx::__detail::__la_rankk<__ycxx::__detail::__la_structure::__hermitian, _Triangle>(alpha, _Ap, __ycxx::__detail::__la_no_e, _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_scalar _Scalar, __ycxx::__detail::__la_in_matrix _InMat,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void hermitian_matrix_rank_k_update(_ExecutionPolicy&&, _Scalar alpha, _InMat _Ap, _OutMat _Cp, _Triangle t) noexcept {
  std::linalg::hermitian_matrix_rank_k_update(alpha, _Ap, _Cp, t);
}
template <__ycxx::__detail::__la_scalar _Scalar, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
void symmetric_matrix_rank_k_update(_Scalar alpha, _InMat1 _Ap, _InMat2 _Ep, _OutMat _Cp, _Triangle) {
  __ycxx::__detail::__la_rankk<__ycxx::__detail::__la_structure::__symmetric, _Triangle>(alpha, _Ap, &_Ep, _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_scalar _Scalar, __ycxx::__detail::__la_in_matrix _InMat1,
          __ycxx::__detail::__la_in_matrix _InMat2, __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat,
          __ycxx::__detail::__la_triangle _Triangle>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void symmetric_matrix_rank_k_update(_ExecutionPolicy&&, _Scalar alpha, _InMat1 _Ap, _InMat2 _Ep, _OutMat _Cp,
                                    _Triangle t) noexcept {
  std::linalg::symmetric_matrix_rank_k_update(alpha, _Ap, _Ep, _Cp, t);
}
template <__ycxx::__detail::__la_scalar _Scalar, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
void hermitian_matrix_rank_k_update(_Scalar alpha, _InMat1 _Ap, _InMat2 _Ep, _OutMat _Cp, _Triangle) {
  __ycxx::__detail::__la_rankk<__ycxx::__detail::__la_structure::__hermitian, _Triangle>(alpha, _Ap, &_Ep, _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_scalar _Scalar, __ycxx::__detail::__la_in_matrix _InMat1,
          __ycxx::__detail::__la_in_matrix _InMat2, __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat,
          __ycxx::__detail::__la_triangle _Triangle>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void hermitian_matrix_rank_k_update(_ExecutionPolicy&&, _Scalar alpha, _InMat1 _Ap, _InMat2 _Ep, _OutMat _Cp,
                                    _Triangle t) noexcept {
  std::linalg::hermitian_matrix_rank_k_update(alpha, _Ap, _Ep, _Cp, t);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas3.rank2k]
// ---------------------------------------------------------------------------------------------
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
void symmetric_matrix_rank_2k_update(_InMat1 _Ap, _InMat2 _Bp, _OutMat _Cp, _Triangle) {
  __ycxx::__detail::__la_rank2k<__ycxx::__detail::__la_structure::__symmetric, _Triangle>(_Ap, _Bp, __ycxx::__detail::__la_no_e, _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void symmetric_matrix_rank_2k_update(_ExecutionPolicy&&, _InMat1 _Ap, _InMat2 _Bp, _OutMat _Cp, _Triangle t) noexcept {
  std::linalg::symmetric_matrix_rank_2k_update(_Ap, _Bp, _Cp, t);
}
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
void hermitian_matrix_rank_2k_update(_InMat1 _Ap, _InMat2 _Bp, _OutMat _Cp, _Triangle) {
  __ycxx::__detail::__la_rank2k<__ycxx::__detail::__la_structure::__hermitian, _Triangle>(_Ap, _Bp, __ycxx::__detail::__la_no_e, _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void hermitian_matrix_rank_2k_update(_ExecutionPolicy&&, _InMat1 _Ap, _InMat2 _Bp, _OutMat _Cp, _Triangle t) noexcept {
  std::linalg::hermitian_matrix_rank_2k_update(_Ap, _Bp, _Cp, t);
}
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2, __ycxx::__detail::__la_in_matrix _InMat3,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
void symmetric_matrix_rank_2k_update(_InMat1 _Ap, _InMat2 _Bp, _InMat3 _Ep, _OutMat _Cp, _Triangle) {
  __ycxx::__detail::__la_rank2k<__ycxx::__detail::__la_structure::__symmetric, _Triangle>(_Ap, _Bp, &_Ep, _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_in_matrix _InMat3, __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat,
          __ycxx::__detail::__la_triangle _Triangle>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void symmetric_matrix_rank_2k_update(_ExecutionPolicy&&, _InMat1 _Ap, _InMat2 _Bp, _InMat3 _Ep, _OutMat _Cp, _Triangle t) noexcept {
  std::linalg::symmetric_matrix_rank_2k_update(_Ap, _Bp, _Ep, _Cp, t);
}
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2, __ycxx::__detail::__la_in_matrix _InMat3,
          __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat, __ycxx::__detail::__la_triangle _Triangle>
void hermitian_matrix_rank_2k_update(_InMat1 _Ap, _InMat2 _Bp, _InMat3 _Ep, _OutMat _Cp, _Triangle) {
  __ycxx::__detail::__la_rank2k<__ycxx::__detail::__la_structure::__hermitian, _Triangle>(_Ap, _Bp, &_Ep, _Cp);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_in_matrix _InMat3, __ycxx::__detail::__la_possibly_packed_out_matrix _OutMat,
          __ycxx::__detail::__la_triangle _Triangle>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void hermitian_matrix_rank_2k_update(_ExecutionPolicy&&, _InMat1 _Ap, _InMat2 _Bp, _InMat3 _Ep, _OutMat _Cp, _Triangle t) noexcept {
  std::linalg::hermitian_matrix_rank_2k_update(_Ap, _Bp, _Ep, _Cp, t);
}

// ---------------------------------------------------------------------------------------------
// [linalg.algs.blas3.trsm], [linalg.algs.blas3.inplacetrsm]
// ---------------------------------------------------------------------------------------------
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_out_matrix _OutMat, __ycxx::__detail::__la_divide_op _BinaryDivideOp>
void triangular_matrix_matrix_left_solve(_InMat1 _Ap, _Triangle, _DiagonalStorage, _InMat2 _Bp, _OutMat _Xp,
                                         _BinaryDivideOp __divide) {
  __ycxx::__detail::__la_check_square<_InMat1, _Triangle>(_Ap);
  __ycxx::__detail::__la_check_mm(_Ap, _Xp, _Bp);
  for (size_t __j = 0; __j < __ycxx::__detail::__la_n(_Bp, 1); ++__j)
    __ycxx::__detail::__la_trsv<_Triangle, _DiagonalStorage, typename _OutMat::value_type>(
        _Ap, [&](size_t i) -> decltype(auto) { return __ycxx::__detail::__la_m(_Bp, i, __j); },
        [&](size_t i) -> decltype(auto) { return __ycxx::__detail::__la_m(_Xp, i, __j); }, __divide);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_out_matrix _OutMat, __ycxx::__detail::__la_divide_op _BinaryDivideOp>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_matrix_left_solve(_ExecutionPolicy&&, _InMat1 _Ap, _Triangle t, _DiagonalStorage d, _InMat2 _Bp, _OutMat _Xp,
                                         _BinaryDivideOp __divide) noexcept {
  std::linalg::triangular_matrix_matrix_left_solve(_Ap, t, d, _Bp, _Xp, __divide);
}
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_out_matrix _OutMat>
void triangular_matrix_matrix_left_solve(_InMat1 _Ap, _Triangle t, _DiagonalStorage d, _InMat2 _Bp, _OutMat _Xp) {
  std::linalg::triangular_matrix_matrix_left_solve(_Ap, t, d, _Bp, _Xp, divides<void>{});
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_matrix_left_solve(_ExecutionPolicy&& __exec, _InMat1 _Ap, _Triangle t, _DiagonalStorage d, _InMat2 _Bp,
                                         _OutMat _Xp) noexcept {
  std::linalg::triangular_matrix_matrix_left_solve(std::forward<_ExecutionPolicy>(__exec), _Ap, t, d, _Bp, _Xp, divides<void>{});
}
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_out_matrix _OutMat, __ycxx::__detail::__la_divide_op _BinaryDivideOp>
void triangular_matrix_matrix_right_solve(_InMat1 _Ap, _Triangle, _DiagonalStorage, _InMat2 _Bp, _OutMat _Xp,
                                          _BinaryDivideOp __divide) {
  __ycxx::__detail::__la_check_square<_InMat1, _Triangle>(_Ap);
  __ycxx::__detail::__la_check_mm(_Xp, _Ap, _Bp);
  for (size_t r = 0; r < __ycxx::__detail::__la_n(_Bp, 0); ++r)
    __ycxx::__detail::__la_trsv_right<_Triangle, _DiagonalStorage>(_Ap, _Bp, _Xp, r, __divide);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_out_matrix _OutMat, __ycxx::__detail::__la_divide_op _BinaryDivideOp>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_matrix_right_solve(_ExecutionPolicy&&, _InMat1 _Ap, _Triangle t, _DiagonalStorage d, _InMat2 _Bp,
                                          _OutMat _Xp, _BinaryDivideOp __divide) noexcept {
  std::linalg::triangular_matrix_matrix_right_solve(_Ap, t, d, _Bp, _Xp, __divide);
}
template <__ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_out_matrix _OutMat>
void triangular_matrix_matrix_right_solve(_InMat1 _Ap, _Triangle t, _DiagonalStorage d, _InMat2 _Bp, _OutMat _Xp) {
  std::linalg::triangular_matrix_matrix_right_solve(_Ap, t, d, _Bp, _Xp, divides<void>{});
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat1, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_in_matrix _InMat2,
          __ycxx::__detail::__la_out_matrix _OutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_matrix_right_solve(_ExecutionPolicy&& __exec, _InMat1 _Ap, _Triangle t, _DiagonalStorage d, _InMat2 _Bp,
                                          _OutMat _Xp) noexcept {
  std::linalg::triangular_matrix_matrix_right_solve(std::forward<_ExecutionPolicy>(__exec), _Ap, t, d, _Bp, _Xp,
                                                    divides<void>{});
}

template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_inout_matrix _InOutMat,
          __ycxx::__detail::__la_divide_op _BinaryDivideOp>
void triangular_matrix_matrix_left_solve(_InMat _Ap, _Triangle, _DiagonalStorage, _InOutMat _Bp, _BinaryDivideOp __divide) {
  __ycxx::__detail::__la_check_square<_InMat, _Triangle>(_Ap);
  __ycxx::__detail::__la_check_mm(_Ap, _Bp, _Bp);
  for (size_t __j = 0; __j < __ycxx::__detail::__la_n(_Bp, 1); ++__j) {
    auto at = [&](size_t i) -> decltype(auto) { return __ycxx::__detail::__la_m(_Bp, i, __j); };
    __ycxx::__detail::__la_trsv<_Triangle, _DiagonalStorage, typename _InOutMat::value_type>(_Ap, at, at, __divide);
  }
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_inout_matrix _InOutMat,
          __ycxx::__detail::__la_divide_op _BinaryDivideOp>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_matrix_left_solve(_ExecutionPolicy&&, _InMat _Ap, _Triangle t, _DiagonalStorage d, _InOutMat _Bp,
                                         _BinaryDivideOp __divide) noexcept {
  std::linalg::triangular_matrix_matrix_left_solve(_Ap, t, d, _Bp, __divide);
}
template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_inout_matrix _InOutMat>
void triangular_matrix_matrix_left_solve(_InMat _Ap, _Triangle t, _DiagonalStorage d, _InOutMat _Bp) {
  std::linalg::triangular_matrix_matrix_left_solve(_Ap, t, d, _Bp, divides<void>{});
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_inout_matrix _InOutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_matrix_left_solve(_ExecutionPolicy&& __exec, _InMat _Ap, _Triangle t, _DiagonalStorage d,
                                         _InOutMat _Bp) noexcept {
  std::linalg::triangular_matrix_matrix_left_solve(std::forward<_ExecutionPolicy>(__exec), _Ap, t, d, _Bp, divides<void>{});
}
template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_inout_matrix _InOutMat,
          __ycxx::__detail::__la_divide_op _BinaryDivideOp>
void triangular_matrix_matrix_right_solve(_InMat _Ap, _Triangle, _DiagonalStorage, _InOutMat _Bp, _BinaryDivideOp __divide) {
  __ycxx::__detail::__la_check_square<_InMat, _Triangle>(_Ap);
  __ycxx::__detail::__la_check_mm(_Bp, _Ap, _Bp);
  for (size_t r = 0; r < __ycxx::__detail::__la_n(_Bp, 0); ++r)
    __ycxx::__detail::__la_trsv_right<_Triangle, _DiagonalStorage>(_Ap, _Bp, _Bp, r, __divide);
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_inout_matrix _InOutMat,
          __ycxx::__detail::__la_divide_op _BinaryDivideOp>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_matrix_right_solve(_ExecutionPolicy&&, _InMat _Ap, _Triangle t, _DiagonalStorage d, _InOutMat _Bp,
                                          _BinaryDivideOp __divide) noexcept {
  std::linalg::triangular_matrix_matrix_right_solve(_Ap, t, d, _Bp, __divide);
}
template <__ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_inout_matrix _InOutMat>
void triangular_matrix_matrix_right_solve(_InMat _Ap, _Triangle t, _DiagonalStorage d, _InOutMat _Bp) {
  std::linalg::triangular_matrix_matrix_right_solve(_Ap, t, d, _Bp, divides<void>{});
}
template <class _ExecutionPolicy, __ycxx::__detail::__la_in_matrix _InMat, __ycxx::__detail::__la_triangle _Triangle,
          __ycxx::__detail::__la_diagonal _DiagonalStorage, __ycxx::__detail::__la_inout_matrix _InOutMat>
  requires __ycxx::__detail::__execution_policy<_ExecutionPolicy>
void triangular_matrix_matrix_right_solve(_ExecutionPolicy&& __exec, _InMat _Ap, _Triangle t, _DiagonalStorage d,
                                          _InOutMat _Bp) noexcept {
  std::linalg::triangular_matrix_matrix_right_solve(std::forward<_ExecutionPolicy>(__exec), _Ap, t, d, _Bp, divides<void>{});
}

}} // namespace std::linalg
