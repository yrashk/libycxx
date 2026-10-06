// libycxx core: the <cmath> functions for one floating-point type T (__ycxx::__detail::__cm), used by
// <cmath>'s std:: overloads and by <complex>.
//
// Run time: the C library (libm) through the compiler builtins (cmath_builtins.hpp), for the
// "carrier" type with T's format: float32_t and float64_t use float and double, float128_t the
// *f128 functions, float16_t and bfloat16_t compute in double and round once more. The C23
// additions (nextup, fmaximum, ...) and the format-dependent operations of the 16-bit types
// (nextafter, fma) use the soft implementations, which are exact. Where the C library has no
// *f128 functions (cfg::c_math_float128), float128_t uses the soft implementations at run time
// as well (libm_carrier).
// Constant evaluation: exact and correctly rounded operations (sqrt, fma, fmod, ...) use the
// soft implementations in T itself (cmath_exact.hpp); the transcendental functions use the
// builtin where the compiler folds it (GCC, through MPFR) and the soft multiprecision code
// (cmath_mp.hpp) otherwise, in the carrier type.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/cmath_builtins.hpp>
#include <ycxx/core/cmath_exact.hpp>
#include <ycxx/core/cmath_mp.hpp>
#include <ycxx/core/cmath_promote.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__cm {

template <class _Tp>
consteval auto __carrier_of() {
  if constexpr (__fp_std_index<_Tp> >= 0)
    return std::type_identity<_Tp>{};
  else if constexpr (__fp_same_values<_Tp, float>)
    return std::type_identity<float>{};
  else if constexpr (__fp_same_values<_Tp, double>)
    return std::type_identity<double>{};
  else if constexpr (__fp_same_values<_Tp, long double>)
    return std::type_identity<long double>{};
  else if constexpr (__fp_format<_Tp>.digits > 64)
    return std::type_identity<_Tp>{}; // binary128: the *f128 builtins
  else
    return std::type_identity<double>{}; // binary16, bfloat16
}
template <class _Tp>
using __carrier_t = typename decltype(__ycxx::__detail::__cm::__carrier_of<_Tp>())::type;
template <class _Tp>
inline constexpr bool __exact_carrier = __fp_same_values<_Tp, __carrier_t<_Tp>>;
// Whether the C library computes T's carrier at run time: always for float, double and long
// double; binary128 only where libm has the *f128 functions. Otherwise the carrier is T itself
// and run time uses the same soft implementations as constant evaluation.
template <class _Tp>
inline constexpr bool __libm_carrier = __fp_std_index<__carrier_t<_Tp>> >= 0 || __cfg::__c_math_float128;

// The rounding direction of the current floating-point environment (fegetround), for rint,
// nearbyint and lrint of a type without libm_carrier. Defined in the hosted runtime
// (src/hosted/cmath.cpp).
__ycxx::__detail::__fpm::__fp_rint_mode __current_rounding() noexcept;

// A carrier value converted to T: exact for an exact carrier; otherwise rounded, with the
// exceptions of that rounding reported during constant evaluation.
template <class _Tp, class _Cp>
constexpr _Tp narrow(_Cp __v) noexcept {
  if constexpr (__exact_carrier<_Tp>) {
    return static_cast<_Tp>(__v);
  } else {
    if consteval {
      const __ycxx::__detail::__fpm::__fp_value d = __ycxx::__detail::__fpm::__fp_decode(__v);
      if (d.kind != __ycxx::__detail::__fpm::__fp_kind::__finite) return static_cast<_Tp>(__v);
      return __ycxx::__detail::__fpm::__fp_finish(__ycxx::__detail::__fpm::__fp_round<_Tp>(d.__neg, d.__sig, d.exp));
    } else {
      return static_cast<_Tp>(__v);
    }
  }
}

// ---- transcendental functions ---------------------------------------------------------------------
enum class op : unsigned char {
  acos, asin, atan, cos, sin, tan, acosh, asinh, atanh, cosh, sinh, tanh,
  exp, exp2, expm1, log, log10, log1p, log2, erf, erfc, lgamma, tgamma, atan2, pow
};

template <op _Fp, class _Cp>
consteval bool __folds() {
  namespace __bi = __ycxx::__detail::__fpm::__bi;
  if constexpr (_Fp == op::acos) return __bi::__folds_acos<_Cp>;
  else if constexpr (_Fp == op::asin) return __bi::__folds_asin<_Cp>;
  else if constexpr (_Fp == op::atan) return __bi::__folds_atan<_Cp>;
  else if constexpr (_Fp == op::cos) return __bi::__folds_cos<_Cp>;
  else if constexpr (_Fp == op::sin) return __bi::__folds_sin<_Cp>;
  else if constexpr (_Fp == op::tan) return __bi::__folds_tan<_Cp>;
  else if constexpr (_Fp == op::acosh) return __bi::__folds_acosh<_Cp>;
  else if constexpr (_Fp == op::asinh) return __bi::__folds_asinh<_Cp>;
  else if constexpr (_Fp == op::atanh) return __bi::__folds_atanh<_Cp>;
  else if constexpr (_Fp == op::cosh) return __bi::__folds_cosh<_Cp>;
  else if constexpr (_Fp == op::sinh) return __bi::__folds_sinh<_Cp>;
  else if constexpr (_Fp == op::tanh) return __bi::__folds_tanh<_Cp>;
  else if constexpr (_Fp == op::exp) return __bi::__folds_exp<_Cp>;
  else if constexpr (_Fp == op::exp2) return __bi::__folds_exp2<_Cp>;
  else if constexpr (_Fp == op::expm1) return __bi::__folds_expm1<_Cp>;
  else if constexpr (_Fp == op::log) return __bi::__folds_log<_Cp>;
  else if constexpr (_Fp == op::log10) return __bi::__folds_log10<_Cp>;
  else if constexpr (_Fp == op::log1p) return __bi::__folds_log1p<_Cp>;
  else if constexpr (_Fp == op::log2) return __bi::__folds_log2<_Cp>;
  else if constexpr (_Fp == op::erf) return __bi::__folds_erf<_Cp>;
  else if constexpr (_Fp == op::erfc) return __bi::__folds_erfc<_Cp>;
  else if constexpr (_Fp == op::lgamma) return __bi::__folds_lgamma<_Cp>;
  else if constexpr (_Fp == op::tgamma) return __bi::__folds_tgamma<_Cp>;
  else if constexpr (_Fp == op::atan2) return __bi::__folds_atan2<_Cp>;
  else return __bi::__folds_pow<_Cp>;
}

template <op _Fp, class _Cp>
constexpr _Cp __y_builtin(_Cp __x, _Cp y = _Cp()) noexcept {
  namespace __bi = __ycxx::__detail::__fpm::__bi;
  if constexpr (_Fp == op::acos) return __bi::acos<_Cp>(__x);
  else if constexpr (_Fp == op::asin) return __bi::asin<_Cp>(__x);
  else if constexpr (_Fp == op::atan) return __bi::atan<_Cp>(__x);
  else if constexpr (_Fp == op::cos) return __bi::cos<_Cp>(__x);
  else if constexpr (_Fp == op::sin) return __bi::sin<_Cp>(__x);
  else if constexpr (_Fp == op::tan) return __bi::tan<_Cp>(__x);
  else if constexpr (_Fp == op::acosh) return __bi::acosh<_Cp>(__x);
  else if constexpr (_Fp == op::asinh) return __bi::asinh<_Cp>(__x);
  else if constexpr (_Fp == op::atanh) return __bi::atanh<_Cp>(__x);
  else if constexpr (_Fp == op::cosh) return __bi::cosh<_Cp>(__x);
  else if constexpr (_Fp == op::sinh) return __bi::sinh<_Cp>(__x);
  else if constexpr (_Fp == op::tanh) return __bi::tanh<_Cp>(__x);
  else if constexpr (_Fp == op::exp) return __bi::exp<_Cp>(__x);
  else if constexpr (_Fp == op::exp2) return __bi::exp2<_Cp>(__x);
  else if constexpr (_Fp == op::expm1) return __bi::expm1<_Cp>(__x);
  else if constexpr (_Fp == op::log) return __bi::log<_Cp>(__x);
  else if constexpr (_Fp == op::log10) return __bi::log10<_Cp>(__x);
  else if constexpr (_Fp == op::log1p) return __bi::log1p<_Cp>(__x);
  else if constexpr (_Fp == op::log2) return __bi::log2<_Cp>(__x);
  else if constexpr (_Fp == op::erf) return __bi::erf<_Cp>(__x);
  else if constexpr (_Fp == op::erfc) return __bi::erfc<_Cp>(__x);
  else if constexpr (_Fp == op::lgamma) return __bi::lgamma<_Cp>(__x);
  else if constexpr (_Fp == op::tgamma) return __bi::tgamma<_Cp>(__x);
  else if constexpr (_Fp == op::atan2) return __bi::atan2<_Cp>(__x, y);
  else return __bi::pow<_Cp>(__x, y);
}

template <op _Fp, class _Cp>
constexpr _Cp __soft(_Cp __x, _Cp y = _Cp()) noexcept {
  namespace __fpm = __ycxx::__detail::__fpm;
  if constexpr (_Fp == op::acos) return __fpm::__fp_asin_acos(__x, true);
  else if constexpr (_Fp == op::asin) return __fpm::__fp_asin_acos(__x, false);
  else if constexpr (_Fp == op::atan) return __fpm::__fp_atan(__x);
  else if constexpr (_Fp == op::cos) return __fpm::__fp_trig(__x, 1);
  else if constexpr (_Fp == op::sin) return __fpm::__fp_trig(__x, 0);
  else if constexpr (_Fp == op::tan) return __fpm::__fp_trig(__x, 2);
  else if constexpr (_Fp == op::acosh) return __fpm::__fp_inverse_hyperbolic(__x, 1);
  else if constexpr (_Fp == op::asinh) return __fpm::__fp_inverse_hyperbolic(__x, 0);
  else if constexpr (_Fp == op::atanh) return __fpm::__fp_inverse_hyperbolic(__x, 2);
  else if constexpr (_Fp == op::cosh) return __fpm::__fp_hyperbolic(__x, 1);
  else if constexpr (_Fp == op::sinh) return __fpm::__fp_hyperbolic(__x, 0);
  else if constexpr (_Fp == op::tanh) return __fpm::__fp_hyperbolic(__x, 2);
  else if constexpr (_Fp == op::exp) return __fpm::__fp_exp(__x);
  else if constexpr (_Fp == op::exp2) return __fpm::__fp_exp2(__x);
  else if constexpr (_Fp == op::expm1) return __fpm::__fp_expm1(__x);
  else if constexpr (_Fp == op::log) return __fpm::__fp_log_base(__x, 0);
  else if constexpr (_Fp == op::log10) return __fpm::__fp_log_base(__x, 10);
  else if constexpr (_Fp == op::log1p) return __fpm::__fp_log1p(__x);
  else if constexpr (_Fp == op::log2) return __fpm::__fp_log_base(__x, 2);
  else if constexpr (_Fp == op::erf) return __fpm::__fp_erf(__x, false);
  else if constexpr (_Fp == op::erfc) return __fpm::__fp_erf(__x, true);
  else if constexpr (_Fp == op::lgamma) return __fpm::__fp_gamma(__x, true);
  else if constexpr (_Fp == op::tgamma) return __fpm::__fp_gamma(__x, false);
  else if constexpr (_Fp == op::atan2) return __fpm::__fp_atan2(__x, y);
  else return __fpm::__fp_pow(__x, y);
}

template <op _Fp, class _Tp>
constexpr _Tp __transcendental(_Tp __x, _Tp y = _Tp()) noexcept {
  using _Cp = __carrier_t<_Tp>;
  if consteval {
    // The builtins fold only for finite operands (and not for every finite one: GCC refuses
    // results that raise exceptions, which are non-constant anyway).
    if constexpr (__ycxx::__detail::__cm::__folds<_Fp, _Cp>()) {
      if (__builtin_isfinite(__x) && __builtin_isfinite(y))
        return __ycxx::__detail::__cm::narrow<_Tp>(__ycxx::__detail::__cm::__y_builtin<_Fp, _Cp>(static_cast<_Cp>(__x), static_cast<_Cp>(y)));
    }
  } else {
    if constexpr (__ycxx::__detail::__cm::__libm_carrier<_Tp>)
      return __ycxx::__detail::__cm::narrow<_Tp>(__ycxx::__detail::__cm::__y_builtin<_Fp, _Cp>(static_cast<_Cp>(__x), static_cast<_Cp>(y)));
  }
  return __ycxx::__detail::__cm::narrow<_Tp>(__ycxx::__detail::__cm::__soft<_Fp, _Cp>(static_cast<_Cp>(__x), static_cast<_Cp>(y)));
}

// ---- exact and correctly rounded functions -----------------------------------------------------------
// The pattern: soft in T during constant evaluation, the carrier's builtin at run time.
template <class _Tp>
constexpr _Tp fabs(_Tp __x) noexcept {
  return __ycxx::__detail::__fpm::__fp_abs(__x);
}
template <class _Tp>
constexpr _Tp copysign(_Tp __x, _Tp y) noexcept {
  return __ycxx::__detail::__fpm::__fp_copysign(__x, y);
}
template <class _Tp>
constexpr _Tp sqrt(_Tp __x) noexcept {
  using _Cp = __carrier_t<_Tp>;
  if !consteval {
    if constexpr (__ycxx::__detail::__cm::__libm_carrier<_Tp>) {
      return static_cast<_Tp>(__ycxx::__detail::__fpm::__bi::sqrt<_Cp>(static_cast<_Cp>(__x))); // double rounding innocuous
    }
  }
  return __ycxx::__detail::__fpm::__fp_sqrt(__x);
}
template <class _Tp>
constexpr _Tp cbrt(_Tp __x) noexcept {
  using _Cp = __carrier_t<_Tp>;
  if !consteval {
    if constexpr (__ycxx::__detail::__cm::__libm_carrier<_Tp>) {
      // glibc's cbrt is not exact even for perfect cubes (cbrt(-27.0) != -3): use a wider type
      // when there is one (x87 long double), which also makes the result correctly rounded
      // except in rare double-rounding cases.
      if constexpr (__fp_format<long double>.digits >= __fp_format<_Cp>.digits + 11)
        return static_cast<_Tp>(__ycxx::__detail::__fpm::__bi::cbrt<long double>(static_cast<long double>(__x)));
      else
        return static_cast<_Tp>(__ycxx::__detail::__fpm::__bi::cbrt<_Cp>(static_cast<_Cp>(__x)));
    }
  }
  return __ycxx::__detail::__fpm::__fp_cbrt(__x);
}
template <class _Tp>
constexpr _Tp hypot(_Tp __x, _Tp y) noexcept {
  using _Cp = __carrier_t<_Tp>;
  if !consteval {
    if constexpr (__ycxx::__detail::__cm::__libm_carrier<_Tp>) {
      return static_cast<_Tp>(__ycxx::__detail::__fpm::__bi::hypot<_Cp>(static_cast<_Cp>(__x), static_cast<_Cp>(y)));
    }
  }
  return __ycxx::__detail::__fpm::__fp_hypot(__x, y);
}
// [c.math.hypot3]: no C counterpart. At run time float and double compute in a wider type
// where one has the range for the squares; otherwise (and always during constant evaluation)
// the exact algorithm.
template <class _Tp>
constexpr _Tp __hypot3(_Tp __x, _Tp y, _Tp __z) noexcept {
  if !consteval {
    if constexpr (__fp_format<_Tp>.digits <= 24 && __fp_format<_Tp>.__max_exp <= 128) {
      const double a = __x, b = y, c = __z; // exact squares; a sum of three rounds once or twice
      return static_cast<_Tp>(__builtin_sqrt(a * a + b * b + c * c));
    } else if constexpr (__fp_format<_Tp>.digits <= 53 && __fp_format<_Tp>.__max_exp <= 1024 && __fp_format<long double>.digits > 53 &&
                         __fp_format<long double>.__max_exp >= 2048) {
      if (!__builtin_isinf(__x) && !__builtin_isinf(y) && !__builtin_isinf(__z)) {
        const long double a = __x, b = y, c = __z;
        return static_cast<_Tp>(__builtin_sqrtl(a * a + b * b + c * c));
      }
    }
  }
  return __ycxx::__detail::__fpm::__fp_hypot3(__x, y, __z);
}

enum class __rint_op : unsigned char { ceil, floor, trunc, round };
template <__rint_op _Fp, class _Tp>
constexpr _Tp __round_integral(_Tp __x) noexcept {
  using _Cp = __carrier_t<_Tp>;
  namespace __fpm = __ycxx::__detail::__fpm;
  if !consteval {
    if constexpr (__ycxx::__detail::__cm::__libm_carrier<_Tp>) {
      const _Cp c = static_cast<_Cp>(__x);
      if constexpr (_Fp == __rint_op::ceil) return static_cast<_Tp>(__fpm::__bi::ceil<_Cp>(c));
      else if constexpr (_Fp == __rint_op::floor) return static_cast<_Tp>(__fpm::__bi::floor<_Cp>(c));
      else if constexpr (_Fp == __rint_op::trunc) return static_cast<_Tp>(__fpm::__bi::trunc<_Cp>(c));
      else return static_cast<_Tp>(__fpm::__bi::round<_Cp>(c));
    }
  }
  return __fpm::__fp_rint(__x, _Fp == __rint_op::ceil ? __fpm::__fp_rint_mode::ceil : _Fp == __rint_op::floor ? __fpm::__fp_rint_mode::floor
                                           : _Fp == __rint_op::trunc ? __fpm::__fp_rint_mode::trunc : __fpm::__fp_rint_mode::__half_away);
}
template <class _Ip, class _Tp>
constexpr _Ip lround(_Tp __x) noexcept {
  using _Cp = __carrier_t<_Tp>;
  if !consteval {
    if constexpr (__ycxx::__detail::__cm::__libm_carrier<_Tp>) {
      if constexpr (sizeof(_Ip) == sizeof(long) && __is_same(_Ip, long))
        return __ycxx::__detail::__fpm::__bi::lround<_Cp>(static_cast<_Cp>(__x));
      else
        return __ycxx::__detail::__fpm::__bi::llround<_Cp>(static_cast<_Cp>(__x));
    }
  }
  return __ycxx::__detail::__fpm::__fp_to_integer<_Ip>(__x, __ycxx::__detail::__fpm::__fp_rint_mode::__half_away);
}
template <class _Tp>
constexpr _Tp fmod(_Tp __x, _Tp y) noexcept {
  using _Cp = __carrier_t<_Tp>;
  if !consteval {
    if constexpr (__ycxx::__detail::__cm::__libm_carrier<_Tp>) {
      return static_cast<_Tp>(__ycxx::__detail::__fpm::__bi::fmod<_Cp>(static_cast<_Cp>(__x), static_cast<_Cp>(y)));
    }
  }
  return __ycxx::__detail::__fpm::__fp_fmod(__x, y);
}
template <class _Tp>
constexpr _Tp remainder(_Tp __x, _Tp y) noexcept {
  using _Cp = __carrier_t<_Tp>;
  if !consteval {
    if constexpr (__ycxx::__detail::__cm::__libm_carrier<_Tp>) {
      return static_cast<_Tp>(__ycxx::__detail::__fpm::__bi::remainder<_Cp>(static_cast<_Cp>(__x), static_cast<_Cp>(y)));
    }
  }
  return __ycxx::__detail::__fpm::__fp_remainder(__x, y);
}
template <class _Tp>
constexpr _Tp remquo(_Tp __x, _Tp y, int* __q) noexcept {
  using _Cp = __carrier_t<_Tp>;
  if !consteval {
    if constexpr (__ycxx::__detail::__cm::__libm_carrier<_Tp>) {
      return static_cast<_Tp>(__ycxx::__detail::__fpm::__bi::remquo<_Cp>(static_cast<_Cp>(__x), static_cast<_Cp>(y), __q));
    }
  }
  return __ycxx::__detail::__fpm::__fp_remquo(__x, y, __q);
}
template <class _Tp>
constexpr _Tp frexp(_Tp __x, int* e) noexcept {
  using _Cp = __carrier_t<_Tp>;
  if !consteval {
    if constexpr (__ycxx::__detail::__cm::__libm_carrier<_Tp>) {
      return static_cast<_Tp>(__ycxx::__detail::__fpm::__bi::frexp<_Cp>(static_cast<_Cp>(__x), e));
    }
  }
  return __ycxx::__detail::__fpm::__fp_frexp(__x, e);
}
template <class _Tp>
constexpr _Tp scalbln(_Tp __x, long n) noexcept {
  using _Cp = __carrier_t<_Tp>;
  if !consteval {
    if constexpr (__ycxx::__detail::__cm::__libm_carrier<_Tp>) {
      return __ycxx::__detail::__cm::narrow<_Tp>(__ycxx::__detail::__fpm::__bi::scalbln<_Cp>(static_cast<_Cp>(__x), n));
    }
  }
  return __ycxx::__detail::__fpm::__fp_scale(__x, n);
}
template <class _Tp>
constexpr _Tp modf(_Tp __x, _Tp* __ip) noexcept {
  using _Cp = __carrier_t<_Tp>;
  if !consteval {
    if constexpr (__ycxx::__detail::__cm::__libm_carrier<_Tp>) {
      _Cp __ci{};
      const _Cp r = __ycxx::__detail::__fpm::__bi::modf<_Cp>(static_cast<_Cp>(__x), __builtin_addressof(__ci));
      *__ip = static_cast<_Tp>(__ci);
      return static_cast<_Tp>(r);
    }
  }
  return __ycxx::__detail::__fpm::__fp_modf(__x, __ip);
}
template <class _Tp>
constexpr int ilogb(_Tp __x, int __ilogb0, int __ilogbnan) noexcept {
  using _Cp = __carrier_t<_Tp>;
  if !consteval {
    if constexpr (__ycxx::__detail::__cm::__libm_carrier<_Tp>) {
      return __ycxx::__detail::__fpm::__bi::ilogb<_Cp>(static_cast<_Cp>(__x));
    }
  }
  return __ycxx::__detail::__fpm::__fp_ilogb(__x, __ilogb0, __ilogbnan);
}
template <class _Tp>
constexpr _Tp logb(_Tp __x) noexcept {
  using _Cp = __carrier_t<_Tp>;
  if !consteval {
    if constexpr (__ycxx::__detail::__cm::__libm_carrier<_Tp>) {
      return static_cast<_Tp>(__ycxx::__detail::__fpm::__bi::logb<_Cp>(static_cast<_Cp>(__x)));
    }
  }
  return __ycxx::__detail::__fpm::__fp_logb(__x);
}
template <class _Tp>
constexpr _Tp nextafter(_Tp __x, _Tp y) noexcept {
  using _Cp = __carrier_t<_Tp>;
  if constexpr (__exact_carrier<_Tp> && __libm_carrier<_Tp>) {
    if !consteval {
      return static_cast<_Tp>(__ycxx::__detail::__fpm::__bi::nextafter<_Cp>(static_cast<_Cp>(__x), static_cast<_Cp>(y)));
    }
  }
  return __ycxx::__detail::__fpm::__fp_nextafter(__x, y);
}
template <class _Tp>
constexpr _Tp nexttoward(_Tp __x, long double y) noexcept {
  if !consteval {
    return __ycxx::__detail::__fpm::__bi::nexttoward<_Tp>(__x, y);
  }
  return __ycxx::__detail::__fpm::__fp_nexttoward(__x, y);
}
template <class _Tp>
constexpr _Tp fdim(_Tp __x, _Tp y) noexcept {
  return __ycxx::__detail::__fpm::__fp_fdim(__x, y);
}
template <class _Tp>
constexpr _Tp fmax(_Tp __x, _Tp y) noexcept {
  return __ycxx::__detail::__fpm::__fp_minmax(__x, y, true, true);
}
template <class _Tp>
constexpr _Tp fmin(_Tp __x, _Tp y) noexcept {
  return __ycxx::__detail::__fpm::__fp_minmax(__x, y, false, true);
}
template <class _Tp>
constexpr _Tp fma(_Tp __x, _Tp y, _Tp __z) noexcept {
  using _Cp = __carrier_t<_Tp>;
  if constexpr (__exact_carrier<_Tp> && __libm_carrier<_Tp>) {
    if !consteval {
      return static_cast<_Tp>(__ycxx::__detail::__fpm::__bi::fma<_Cp>(static_cast<_Cp>(__x), static_cast<_Cp>(y), static_cast<_Cp>(__z)));
    }
  }
  return __ycxx::__detail::__fpm::__fp_fma(__x, y, __z);
}

// Not constexpr in the draft (they depend on the rounding mode). Without libm_carrier: the soft
// rounding in the current direction; rint and lrint raise "inexact" when the value changes
// (ISO/IEC 9899:2024 F.10.6.4-5), nearbyint does not.
template <class _Tp>
_Tp rint(_Tp __x) noexcept {
  using _Cp = __carrier_t<_Tp>;
  if constexpr (__ycxx::__detail::__cm::__libm_carrier<_Tp>) {
    return static_cast<_Tp>(__ycxx::__detail::__fpm::__bi::rint<_Cp>(static_cast<_Cp>(__x)));
  } else {
    const _Tp r = __ycxx::__detail::__fpm::__fp_rint(__x, __ycxx::__detail::__cm::__current_rounding());
    if (r != __x && !__ycxx::__detail::__fpm::__fp_isnan(__x)) __ycxx::__detail::__fpm::__fp_raise(__ycxx::__detail::__fpm::__fe_inexact);
    return r;
  }
}
template <class _Tp>
_Tp nearbyint(_Tp __x) noexcept {
  using _Cp = __carrier_t<_Tp>;
  if constexpr (__ycxx::__detail::__cm::__libm_carrier<_Tp>)
    return static_cast<_Tp>(__ycxx::__detail::__fpm::__bi::nearbyint<_Cp>(static_cast<_Cp>(__x)));
  else
    return __ycxx::__detail::__fpm::__fp_rint(__x, __ycxx::__detail::__cm::__current_rounding());
}
template <class _Ip, class _Tp>
_Ip lrint(_Tp __x) noexcept {
  using _Cp = __carrier_t<_Tp>;
  if constexpr (!__ycxx::__detail::__cm::__libm_carrier<_Tp>)
    return __ycxx::__detail::__fpm::__fp_to_integer<_Ip>(__ycxx::__detail::__cm::rint<_Tp>(__x), __ycxx::__detail::__fpm::__fp_rint_mode::trunc);
  else if constexpr (__is_same(_Ip, long))
    return __ycxx::__detail::__fpm::__bi::lrint<_Cp>(static_cast<_Cp>(__x));
  else
    return __ycxx::__detail::__fpm::__bi::llrint<_Cp>(static_cast<_Cp>(__x));
}

}} // namespace __ycxx::__detail::__cm
