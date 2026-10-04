// libycxx core: the "sufficient additional overloads" of <cmath> ([cmath.syn]/3) and of
// <complex> ([cmplx.over]): the floating-point type with the greatest conversion rank and
// subrank ([conv.rank]/2) among a list of arithmetic types, integers counting as double.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/concepts.hpp>
#include <ycxx/core/meta_base.hpp>
#include <ycxx/core/prim_traits.hpp>

namespace ycxx::detail {

template <class T>
inline constexpr int fp_std_index = -1;
template <>
inline constexpr int fp_std_index<float> = 0;
template <>
inline constexpr int fp_std_index<double> = 1;
template <>
inline constexpr int fp_std_index<long double> = 2;

// The set of values of A is a subset of that of B (all formats here are binary).
template <class A, class B>
inline constexpr bool fp_values_subset =
    fp_format<A>.digits <= fp_format<B>.digits && fp_format<A>.max_exp <= fp_format<B>.max_exp &&
    fp_format<A>.min_exp >= fp_format<B>.min_exp &&
    fp_format<A>.min_exp - fp_format<A>.digits >= fp_format<B>.min_exp - fp_format<B>.digits;
template <class A, class B>
inline constexpr bool fp_same_values = fp_values_subset<A, B> && fp_values_subset<B, A>;

// [conv.rank]/2: an extended type with the values of exactly one standard type has that
// type's rank; with the values of several, the rank of double.
template <class T>
consteval int fp_rank_index() {
  if constexpr (fp_std_index<T> >= 0)
    return fp_std_index<T>;
  else if constexpr (fp_same_values<T, double>)
    return 1;
  else if constexpr (fp_same_values<T, float>)
    return 0;
  else if constexpr (fp_same_values<T, long double>)
    return 2;
  else
    return -1;
}

// Compares (rank, subrank): 1 if A is greater, -1 if B is, 0 if the same type, 2 if unordered.
template <class A, class B>
consteval int fp_rank_compare() {
  if constexpr (__is_same(A, B)) {
    return 0;
  } else {
    constexpr int ia = ycxx::detail::fp_rank_index<A>(), ib = ycxx::detail::fp_rank_index<B>();
    if constexpr (ia >= 0 && ib >= 0) {
      if constexpr (ia != ib)
        return ia > ib ? 1 : -1;
      else // equal rank: the extended type has the greater subrank
        return fp_std_index<A> >= 0 ? -1 : 1;
    } else if constexpr (fp_values_subset<A, B> && !fp_values_subset<B, A>) {
      return -1;
    } else if constexpr (fp_values_subset<B, A> && !fp_values_subset<A, B>) {
      return 1;
    } else {
      return 2;
    }
  }
}

template <class T>
using cmath_as_fp = std::conditional_t<is_integral_v<T>, double, std::remove_cv_t<T>>;

template <class C, class... Ts>
inline constexpr bool fp_is_greatest = ((__is_same(C, Ts) || ycxx::detail::fp_rank_compare<C, Ts>() == 1) && ...);

template <class... Ts>
struct fp_greatest_of {};
template <class... Ts>
  requires(fp_is_greatest<Ts, Ts...> || ...)
struct fp_greatest_of<Ts...> {
  // The first candidate that is greatest.
  template <class C, class... Rest>
  static consteval auto pick() {
    if constexpr (fp_is_greatest<C, Ts...>)
      return std::type_identity<C>{};
    else
      return pick<Rest...>();
  }
  using type = typename decltype(pick<Ts...>())::type;
};

// [cmath.syn]/3: every argument is arithmetic and a type of greatest rank and subrank exists.
template <class... As>
concept cmath_args = ((is_arithmetic_v<As> && ...)) && requires { typename fp_greatest_of<cmath_as_fp<As>...>::type; };

// The additional overloads with two or three parameters: arithmetic arguments that are not
// all of the same floating-point type (those use the per-type overloads).
template <class A, class... As>
concept cmath_mixed = cmath_args<A, As...> && !(is_floating_v<A> && (__is_same(A, As) && ...));

// The per-type overloads: T is the floating-point type F (false for an unavailable one).
template <class T, class F>
concept fp_is = __is_same(T, F) && is_floating_v<F>;

template <class... As>
using cmath_promote_t = typename fp_greatest_of<cmath_as_fp<As>...>::type;

} // namespace ycxx::detail
