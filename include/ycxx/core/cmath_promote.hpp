// libycxx core: the "sufficient additional overloads" of <cmath> ([cmath.syn]/3) and of
// <complex> ([cmplx.over]): the floating-point type with the greatest conversion rank and
// subrank ([conv.rank]/2) among a list of arithmetic types, integers counting as double.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/concepts.hpp>
#include <ycxx/core/meta_base.hpp>
#include <ycxx/core/prim_traits.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

template <class _Tp>
inline constexpr int __fp_std_index = -1;
template <>
inline constexpr int __fp_std_index<float> = 0;
template <>
inline constexpr int __fp_std_index<double> = 1;
template <>
inline constexpr int __fp_std_index<long double> = 2;

// The set of values of A is a subset of that of B (all formats here are binary).
template <class _Ap, class _Bp>
inline constexpr bool __fp_values_subset =
    __fp_format<_Ap>.digits <= __fp_format<_Bp>.digits && __fp_format<_Ap>.__max_exp <= __fp_format<_Bp>.__max_exp &&
    __fp_format<_Ap>.__min_exp >= __fp_format<_Bp>.__min_exp &&
    __fp_format<_Ap>.__min_exp - __fp_format<_Ap>.digits >= __fp_format<_Bp>.__min_exp - __fp_format<_Bp>.digits;
template <class _Ap, class _Bp>
inline constexpr bool __fp_same_values = __fp_values_subset<_Ap, _Bp> && __fp_values_subset<_Bp, _Ap>;

// [conv.rank]/2: an extended type with the values of exactly one standard type has that
// type's rank; with the values of several, the rank of double.
template <class _Tp>
consteval int __fp_rank_index() {
  if constexpr (__fp_std_index<_Tp> >= 0)
    return __fp_std_index<_Tp>;
  else if constexpr (__fp_same_values<_Tp, double>)
    return 1;
  else if constexpr (__fp_same_values<_Tp, float>)
    return 0;
  else if constexpr (__fp_same_values<_Tp, long double>)
    return 2;
  else
    return -1;
}

// Compares (rank, subrank): 1 if A is greater, -1 if B is, 0 if the same type, 2 if unordered.
template <class _Ap, class _Bp>
consteval int __fp_rank_compare() {
  if constexpr (__is_same(_Ap, _Bp)) {
    return 0;
  } else {
    constexpr int __ia = __ycxx::__detail::__fp_rank_index<_Ap>(), __ib = __ycxx::__detail::__fp_rank_index<_Bp>();
    if constexpr (__ia >= 0 && __ib >= 0) {
      if constexpr (__ia != __ib)
        return __ia > __ib ? 1 : -1;
      else // equal rank: the extended type has the greater subrank
        return __fp_std_index<_Ap> >= 0 ? -1 : 1;
    } else if constexpr (__fp_values_subset<_Ap, _Bp> && !__fp_values_subset<_Bp, _Ap>) {
      return -1;
    } else if constexpr (__fp_values_subset<_Bp, _Ap> && !__fp_values_subset<_Ap, _Bp>) {
      return 1;
    } else {
      return 2;
    }
  }
}

template <class _Tp>
using __cmath_as_fp = std::conditional_t<is_integral_v<_Tp>, double, std::remove_cv_t<_Tp>>;

template <class _Cp, class... _Ts>
inline constexpr bool __fp_is_greatest = ((__is_same(_Cp, _Ts) || __ycxx::__detail::__fp_rank_compare<_Cp, _Ts>() == 1) && ...);

template <class... _Ts>
struct __fp_greatest_of {};
template <class... _Ts>
  requires(__fp_is_greatest<_Ts, _Ts...> || ...)
struct __fp_greatest_of<_Ts...> {
  // The first candidate that is greatest.
  template <class _Cp, class... _Rest>
  static consteval auto __pick() {
    if constexpr (__fp_is_greatest<_Cp, _Ts...>)
      return std::type_identity<_Cp>{};
    else
      return __pick<_Rest...>();
  }
  using type = typename decltype(__pick<_Ts...>())::type;
};

// [cmath.syn]/3: every argument is arithmetic and a type of greatest rank and subrank exists.
template <class... _As>
concept __cmath_args = ((is_arithmetic_v<_As> && ...)) && requires { typename __fp_greatest_of<__cmath_as_fp<_As>...>::type; };

// The additional overloads with two or three parameters: arithmetic arguments that are not
// all of the same floating-point type (those use the per-type overloads).
template <class _Ap, class... _As>
concept __cmath_mixed = __cmath_args<_Ap, _As...> && !(__is_floating_v<_Ap> && (__is_same(_Ap, _As) && ...));

// The per-type overloads: T is the floating-point type F (false for an unavailable one).
template <class _Tp, class _Fp>
concept __fp_is = __is_same(_Tp, _Fp) && __is_floating_v<_Fp>;

template <class... _As>
using __cmath_promote_t = typename __fp_greatest_of<__cmath_as_fp<_As>...>::type;

}} // namespace __ycxx::__detail
