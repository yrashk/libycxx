// libycxx core: <ratio> ([ratio]).
//
// The arithmetic is exact: intermediate values are reduced by common factors first and computed
// in 128 bits where the target has them, so a result is ill-formed only when its reduced
// numerator or denominator does not fit in intmax_t ([ratio.arithmetic]/2). Without a 128-bit
// type, an intermediate overflow is diagnosed as well (allowed by the same paragraph).
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/cstdint.hpp>
#include <ycxx/core/meta_base.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

consteval std::intmax_t __ratio_abs(std::intmax_t __x) { return __x < 0 ? -__x : __x; }
consteval std::intmax_t __ratio_gcd(std::intmax_t a, std::intmax_t b) {
  a = __ycxx::__detail::__ratio_abs(a);
  b = __ycxx::__detail::__ratio_abs(b);
  while (b != 0) {
    std::intmax_t t = a % b;
    a = b;
    b = t;
  }
  return a;
}

// A reduced fraction; `ok` is false when a value does not fit in intmax_t.
struct __ratio_value {
  std::intmax_t num;
  std::intmax_t den;
  bool ok;
};

// a/b + c/d with b, d > 0 and both fractions reduced.
template <class _Wide = __ycxx::__detail::__y_int128>
consteval __ratio_value __ratio_add_values(std::intmax_t a, std::intmax_t b, std::intmax_t c, std::intmax_t d) {
  const std::intmax_t __g = __ycxx::__detail::__ratio_gcd(b, d);
  const std::intmax_t __bg = b / __g, __dg = d / __g;
  // a/b + c/d = (a*dg + c*bg) / (bg*d); gcd(a*dg + c*bg, bg*dg) == 1, so only g can be shared.
  if constexpr (__cfg::__has_int128) {
    using __wide = _Wide;
    const __wide n = __wide(a) * __dg + __wide(c) * __bg;
    const __wide __g2 = n == 0 ? __wide(__g) : __wide(__ycxx::__detail::__ratio_gcd(static_cast<std::intmax_t>(n % __g), __g));
    const __wide num = n / __g2, den = __wide(__bg) * (d / static_cast<std::intmax_t>(__g2));
    constexpr __wide __hi = __wide(__INTMAX_MAX__);
    if (num > __hi || num < -__hi || den > __hi) return {0, 1, false};
    return {static_cast<std::intmax_t>(num), static_cast<std::intmax_t>(den), true};
  } else {
    std::intmax_t __x, y, n, den;
    if (__builtin_mul_overflow(a, __dg, &__x) || __builtin_mul_overflow(c, __bg, &y) || __builtin_add_overflow(__x, y, &n))
      return {0, 1, false};
    const std::intmax_t __g2 = n == 0 ? __g : __ycxx::__detail::__ratio_gcd(n % __g, __g);
    if (__builtin_mul_overflow(__bg, d / __g2, &den) || n / __g2 == -__INTMAX_MAX__ - 1) return {0, 1, false};
    return {n / __g2, den, true};
  }
}

consteval __ratio_value __ratio_mul_values(std::intmax_t a, std::intmax_t b, std::intmax_t c, std::intmax_t d) {
  // (a/b) * (c/d) with both reduced: cancel across first, then the product is reduced.
  const std::intmax_t __g1 = __ycxx::__detail::__ratio_gcd(a, d), __g2 = __ycxx::__detail::__ratio_gcd(c, b);
  if (a == 0 || c == 0) return {0, 1, true};
  std::intmax_t num, den;
  if (__builtin_mul_overflow(a / __g1, c / __g2, &num) || __builtin_mul_overflow(b / __g2, d / __g1, &den) ||
      num == -__INTMAX_MAX__ - 1)
    return {0, 1, false};
  return {num, den, true};
}

// sign(a/b - c/d) for b, d > 0, without overflow (continued-fraction comparison).
consteval int __ratio_compare(std::intmax_t a, std::intmax_t b, std::intmax_t c, std::intmax_t d) {
  if ((a < 0) != (c < 0)) return a < 0 ? -1 : 1;
  if (a < 0) return __ycxx::__detail::__ratio_compare(-c, d, -a, b);
  // Both non-negative: compare integer parts, then the reciprocals of the remainders.
  for (int flip = 1;; flip = -flip) {
    const std::intmax_t __qa = a / b, __qc = c / d;
    if (__qa != __qc) return __qa < __qc ? -flip : flip;
    const std::intmax_t __ra = a % b, __rc = c % d;
    if (__ra == 0 || __rc == 0) return __ra == __rc ? 0 : (__ra == 0 ? -flip : flip);
    // a/b - qa = ra/b; compare b/ra with d/rc in the opposite direction.
    a = b;
    b = __ra;
    c = d;
    d = __rc;
  }
}

template <class _Rp>
inline constexpr bool __is_ratio = false;

template <class _R1, class _R2>
struct __ratio_check {
  static_assert(__is_ratio<_R1> && __is_ratio<_R2>,
                "[ratio.general]/2: R1 and R2 must be specializations of std::ratio");
};

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <intmax_t _Np, intmax_t _Dp = 1>
class ratio {
  static_assert(_Dp != 0, "[ratio.ratio]/1: the denominator of std::ratio must not be zero");
  static_assert(_Np != -__INTMAX_MAX__ - 1 && _Dp != -__INTMAX_MAX__ - 1,
                "[ratio.ratio]/1: the absolute values of N and D must be representable by intmax_t");
  static constexpr intmax_t __g = _Dp == 0 ? 1 : __ycxx::__detail::__ratio_gcd(_Np, _Dp == 0 ? 1 : _Dp);

public:
  static constexpr intmax_t num = (_Dp < 0 ? -_Np : _Np) / __g;
  static constexpr intmax_t den = (_Dp < 0 ? -_Dp : _Dp) / __g;
  using type = ratio<num, den>;
};

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

template <std::intmax_t _Np, std::intmax_t _Dp>
inline constexpr bool __is_ratio<std::ratio<_Np, _Dp>> = true;

template <__ratio_value _Vp>
struct __ratio_result {
  static_assert(_Vp.ok, "[ratio.arithmetic]/2: the result of the std::ratio arithmetic is not representable by intmax_t");
  using type = std::ratio<_Vp.ok ? _Vp.num : 0, _Vp.ok ? _Vp.den : 1>;
};

template <class _R1, class _R2, bool _Negate>
consteval __ratio_value __ratio_add_of() {
  (void)__ratio_check<_R1, _R2>{};
  return __ycxx::__detail::__ratio_add_values(_R1::num, _R1::den, _Negate ? -_R2::num : _R2::num, _R2::den);
}
template <class _R1, class _R2>
consteval __ratio_value __ratio_divide_of() {
  (void)__ratio_check<_R1, _R2>{};
  static_assert(_R2::num != 0, "[ratio.arithmetic]: std::ratio_divide by zero");
  if constexpr (_R2::num == 0)
    return {0, 1, true};
  else
    return __ycxx::__detail::__ratio_mul_values(_R1::num, _R1::den, _R2::num < 0 ? -_R2::den : _R2::den,
                                          _R2::num < 0 ? -_R2::num : _R2::num);
}
template <class _R1, class _R2>
consteval __ratio_value __ratio_multiply_of() {
  (void)__ratio_check<_R1, _R2>{};
  return __ycxx::__detail::__ratio_mul_values(_R1::num, _R1::den, _R2::num, _R2::den);
}
template <class _R1, class _R2>
consteval int __ratio_compare_of() {
  (void)__ratio_check<_R1, _R2>{};
  return __ycxx::__detail::__ratio_compare(_R1::num, _R1::den, _R2::num, _R2::den);
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

// [ratio.arithmetic]
template <class _R1, class _R2>
using ratio_add = typename __ycxx::__detail::__ratio_result<__ycxx::__detail::__ratio_add_of<_R1, _R2, false>()>::type;
template <class _R1, class _R2>
using ratio_subtract = typename __ycxx::__detail::__ratio_result<__ycxx::__detail::__ratio_add_of<_R1, _R2, true>()>::type;
template <class _R1, class _R2>
using ratio_multiply = typename __ycxx::__detail::__ratio_result<__ycxx::__detail::__ratio_multiply_of<_R1, _R2>()>::type;
template <class _R1, class _R2>
using ratio_divide = typename __ycxx::__detail::__ratio_result<__ycxx::__detail::__ratio_divide_of<_R1, _R2>()>::type;

// [ratio.comparison]
template <class _R1, class _R2>
struct ratio_equal : bool_constant<__ycxx::__detail::__ratio_compare_of<_R1, _R2>() == 0> {};
template <class _R1, class _R2>
struct ratio_not_equal : bool_constant<__ycxx::__detail::__ratio_compare_of<_R1, _R2>() != 0> {};
template <class _R1, class _R2>
struct ratio_less : bool_constant<(__ycxx::__detail::__ratio_compare_of<_R1, _R2>() < 0)> {};
template <class _R1, class _R2>
struct ratio_less_equal : bool_constant<(__ycxx::__detail::__ratio_compare_of<_R1, _R2>() <= 0)> {};
template <class _R1, class _R2>
struct ratio_greater : bool_constant<(__ycxx::__detail::__ratio_compare_of<_R1, _R2>() > 0)> {};
template <class _R1, class _R2>
struct ratio_greater_equal : bool_constant<(__ycxx::__detail::__ratio_compare_of<_R1, _R2>() >= 0)> {};

template <class _R1, class _R2>
constexpr bool ratio_equal_v = ratio_equal<_R1, _R2>::value;
template <class _R1, class _R2>
constexpr bool ratio_not_equal_v = ratio_not_equal<_R1, _R2>::value;
template <class _R1, class _R2>
constexpr bool ratio_less_v = ratio_less<_R1, _R2>::value;
template <class _R1, class _R2>
constexpr bool ratio_less_equal_v = ratio_less_equal<_R1, _R2>::value;
template <class _R1, class _R2>
constexpr bool ratio_greater_v = ratio_greater<_R1, _R2>::value;
template <class _R1, class _R2>
constexpr bool ratio_greater_equal_v = ratio_greater_equal<_R1, _R2>::value;

// [ratio.si]: quecto ... zepto and zetta ... quetta need more than 64 bits, so they are declared
// only where intmax_t is wider (/1); there is no such target among those libycxx supports.
using atto = ratio<1, 1'000'000'000'000'000'000>;
using femto = ratio<1, 1'000'000'000'000'000>;
using pico = ratio<1, 1'000'000'000'000>;
using nano = ratio<1, 1'000'000'000>;
using micro = ratio<1, 1'000'000>;
using milli = ratio<1, 1'000>;
using centi = ratio<1, 100>;
using deci = ratio<1, 10>;
using deca = ratio<10, 1>;
using hecto = ratio<100, 1>;
using kilo = ratio<1'000, 1>;
using mega = ratio<1'000'000, 1>;
using giga = ratio<1'000'000'000, 1>;
using tera = ratio<1'000'000'000'000, 1>;
using peta = ratio<1'000'000'000'000'000, 1>;
using exa = ratio<1'000'000'000'000'000'000, 1>;

} // namespace std
