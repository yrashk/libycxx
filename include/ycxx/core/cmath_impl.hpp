// libycxx core: the <cmath> functions for one floating-point type T (ycxx::detail::cm), used by
// <cmath>'s std:: overloads and by <complex>.
//
// Run time: the C library (libm) through the compiler builtins (cmath_builtins.hpp), for the
// "carrier" type with T's format: float32_t and float64_t use float and double, float128_t the
// *f128 functions, float16_t and bfloat16_t compute in double and round once more. The C23
// additions (nextup, fmaximum, ...) and the format-dependent operations of the 16-bit types
// (nextafter, fma) use the soft implementations, which are exact.
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

namespace ycxx::detail::cm {

template <class T>
consteval auto carrier_of() {
  if constexpr (fp_std_index<T> >= 0)
    return std::type_identity<T>{};
  else if constexpr (fp_same_values<T, float>)
    return std::type_identity<float>{};
  else if constexpr (fp_same_values<T, double>)
    return std::type_identity<double>{};
  else if constexpr (fp_same_values<T, long double>)
    return std::type_identity<long double>{};
  else if constexpr (fp_format<T>.digits > 64)
    return std::type_identity<T>{}; // binary128: the *f128 builtins
  else
    return std::type_identity<double>{}; // binary16, bfloat16
}
template <class T>
using carrier_t = typename decltype(ycxx::detail::cm::carrier_of<T>())::type;
template <class T>
inline constexpr bool exact_carrier = fp_same_values<T, carrier_t<T>>;

// A carrier value converted to T: exact for an exact carrier; otherwise rounded, with the
// exceptions of that rounding reported during constant evaluation.
template <class T, class C>
constexpr T narrow(C v) noexcept {
  if constexpr (exact_carrier<T>) {
    return static_cast<T>(v);
  } else {
    if consteval {
      const ycxx::detail::fpm::fp_value d = ycxx::detail::fpm::fp_decode(v);
      if (d.kind != ycxx::detail::fpm::fp_kind::finite) return static_cast<T>(v);
      return ycxx::detail::fpm::fp_finish(ycxx::detail::fpm::fp_round<T>(d.neg, d.sig, d.exp));
    } else {
      return static_cast<T>(v);
    }
  }
}

// ---- transcendental functions ---------------------------------------------------------------------
enum class op : unsigned char {
  acos, asin, atan, cos, sin, tan, acosh, asinh, atanh, cosh, sinh, tanh,
  exp, exp2, expm1, log, log10, log1p, log2, erf, erfc, lgamma, tgamma, atan2, pow
};

template <op F, class C>
consteval bool folds() {
  namespace bi = ycxx::detail::fpm::bi;
  if constexpr (F == op::acos) return bi::folds_acos<C>;
  else if constexpr (F == op::asin) return bi::folds_asin<C>;
  else if constexpr (F == op::atan) return bi::folds_atan<C>;
  else if constexpr (F == op::cos) return bi::folds_cos<C>;
  else if constexpr (F == op::sin) return bi::folds_sin<C>;
  else if constexpr (F == op::tan) return bi::folds_tan<C>;
  else if constexpr (F == op::acosh) return bi::folds_acosh<C>;
  else if constexpr (F == op::asinh) return bi::folds_asinh<C>;
  else if constexpr (F == op::atanh) return bi::folds_atanh<C>;
  else if constexpr (F == op::cosh) return bi::folds_cosh<C>;
  else if constexpr (F == op::sinh) return bi::folds_sinh<C>;
  else if constexpr (F == op::tanh) return bi::folds_tanh<C>;
  else if constexpr (F == op::exp) return bi::folds_exp<C>;
  else if constexpr (F == op::exp2) return bi::folds_exp2<C>;
  else if constexpr (F == op::expm1) return bi::folds_expm1<C>;
  else if constexpr (F == op::log) return bi::folds_log<C>;
  else if constexpr (F == op::log10) return bi::folds_log10<C>;
  else if constexpr (F == op::log1p) return bi::folds_log1p<C>;
  else if constexpr (F == op::log2) return bi::folds_log2<C>;
  else if constexpr (F == op::erf) return bi::folds_erf<C>;
  else if constexpr (F == op::erfc) return bi::folds_erfc<C>;
  else if constexpr (F == op::lgamma) return bi::folds_lgamma<C>;
  else if constexpr (F == op::tgamma) return bi::folds_tgamma<C>;
  else if constexpr (F == op::atan2) return bi::folds_atan2<C>;
  else return bi::folds_pow<C>;
}

template <op F, class C>
constexpr C builtin(C x, C y = C()) noexcept {
  namespace bi = ycxx::detail::fpm::bi;
  if constexpr (F == op::acos) return bi::acos<C>(x);
  else if constexpr (F == op::asin) return bi::asin<C>(x);
  else if constexpr (F == op::atan) return bi::atan<C>(x);
  else if constexpr (F == op::cos) return bi::cos<C>(x);
  else if constexpr (F == op::sin) return bi::sin<C>(x);
  else if constexpr (F == op::tan) return bi::tan<C>(x);
  else if constexpr (F == op::acosh) return bi::acosh<C>(x);
  else if constexpr (F == op::asinh) return bi::asinh<C>(x);
  else if constexpr (F == op::atanh) return bi::atanh<C>(x);
  else if constexpr (F == op::cosh) return bi::cosh<C>(x);
  else if constexpr (F == op::sinh) return bi::sinh<C>(x);
  else if constexpr (F == op::tanh) return bi::tanh<C>(x);
  else if constexpr (F == op::exp) return bi::exp<C>(x);
  else if constexpr (F == op::exp2) return bi::exp2<C>(x);
  else if constexpr (F == op::expm1) return bi::expm1<C>(x);
  else if constexpr (F == op::log) return bi::log<C>(x);
  else if constexpr (F == op::log10) return bi::log10<C>(x);
  else if constexpr (F == op::log1p) return bi::log1p<C>(x);
  else if constexpr (F == op::log2) return bi::log2<C>(x);
  else if constexpr (F == op::erf) return bi::erf<C>(x);
  else if constexpr (F == op::erfc) return bi::erfc<C>(x);
  else if constexpr (F == op::lgamma) return bi::lgamma<C>(x);
  else if constexpr (F == op::tgamma) return bi::tgamma<C>(x);
  else if constexpr (F == op::atan2) return bi::atan2<C>(x, y);
  else return bi::pow<C>(x, y);
}

template <op F, class C>
constexpr C soft(C x, C y = C()) noexcept {
  namespace fpm = ycxx::detail::fpm;
  if constexpr (F == op::acos) return fpm::fp_asin_acos(x, true);
  else if constexpr (F == op::asin) return fpm::fp_asin_acos(x, false);
  else if constexpr (F == op::atan) return fpm::fp_atan(x);
  else if constexpr (F == op::cos) return fpm::fp_trig(x, 1);
  else if constexpr (F == op::sin) return fpm::fp_trig(x, 0);
  else if constexpr (F == op::tan) return fpm::fp_trig(x, 2);
  else if constexpr (F == op::acosh) return fpm::fp_inverse_hyperbolic(x, 1);
  else if constexpr (F == op::asinh) return fpm::fp_inverse_hyperbolic(x, 0);
  else if constexpr (F == op::atanh) return fpm::fp_inverse_hyperbolic(x, 2);
  else if constexpr (F == op::cosh) return fpm::fp_hyperbolic(x, 1);
  else if constexpr (F == op::sinh) return fpm::fp_hyperbolic(x, 0);
  else if constexpr (F == op::tanh) return fpm::fp_hyperbolic(x, 2);
  else if constexpr (F == op::exp) return fpm::fp_exp(x);
  else if constexpr (F == op::exp2) return fpm::fp_exp2(x);
  else if constexpr (F == op::expm1) return fpm::fp_expm1(x);
  else if constexpr (F == op::log) return fpm::fp_log_base(x, 0);
  else if constexpr (F == op::log10) return fpm::fp_log_base(x, 10);
  else if constexpr (F == op::log1p) return fpm::fp_log1p(x);
  else if constexpr (F == op::log2) return fpm::fp_log_base(x, 2);
  else if constexpr (F == op::erf) return fpm::fp_erf(x, false);
  else if constexpr (F == op::erfc) return fpm::fp_erf(x, true);
  else if constexpr (F == op::lgamma) return fpm::fp_gamma(x, true);
  else if constexpr (F == op::tgamma) return fpm::fp_gamma(x, false);
  else if constexpr (F == op::atan2) return fpm::fp_atan2(x, y);
  else return fpm::fp_pow(x, y);
}

template <op F, class T>
constexpr T transcendental(T x, T y = T()) noexcept {
  using C = carrier_t<T>;
  if consteval {
    // The builtins fold only for finite operands (and not for every finite one: GCC refuses
    // results that raise exceptions, which are non-constant anyway).
    if constexpr (ycxx::detail::cm::folds<F, C>()) {
      if (__builtin_isfinite(x) && __builtin_isfinite(y))
        return ycxx::detail::cm::narrow<T>(ycxx::detail::cm::builtin<F, C>(static_cast<C>(x), static_cast<C>(y)));
    }
    return ycxx::detail::cm::narrow<T>(ycxx::detail::cm::soft<F, C>(static_cast<C>(x), static_cast<C>(y)));
  } else {
    return ycxx::detail::cm::narrow<T>(ycxx::detail::cm::builtin<F, C>(static_cast<C>(x), static_cast<C>(y)));
  }
}

// ---- exact and correctly rounded functions -----------------------------------------------------------
// The pattern: soft in T during constant evaluation, the carrier's builtin at run time.
template <class T>
constexpr T fabs(T x) noexcept {
  return ycxx::detail::fpm::fp_abs(x);
}
template <class T>
constexpr T copysign(T x, T y) noexcept {
  return ycxx::detail::fpm::fp_copysign(x, y);
}
template <class T>
constexpr T sqrt(T x) noexcept {
  using C = carrier_t<T>;
  if consteval {
    return ycxx::detail::fpm::fp_sqrt(x);
  } else {
    return static_cast<T>(ycxx::detail::fpm::bi::sqrt<C>(static_cast<C>(x))); // double rounding innocuous
  }
}
template <class T>
constexpr T cbrt(T x) noexcept {
  using C = carrier_t<T>;
  if consteval {
    return ycxx::detail::fpm::fp_cbrt(x);
  } else {
    // glibc's cbrt is not exact even for perfect cubes (cbrt(-27.0) != -3): use a wider type
    // when there is one (x87 long double), which also makes the result correctly rounded
    // except in rare double-rounding cases.
    if constexpr (fp_format<long double>.digits >= fp_format<C>.digits + 11)
      return static_cast<T>(ycxx::detail::fpm::bi::cbrt<long double>(static_cast<long double>(x)));
    else
      return static_cast<T>(ycxx::detail::fpm::bi::cbrt<C>(static_cast<C>(x)));
  }
}
template <class T>
constexpr T hypot(T x, T y) noexcept {
  using C = carrier_t<T>;
  if consteval {
    return ycxx::detail::fpm::fp_hypot(x, y);
  } else {
    return static_cast<T>(ycxx::detail::fpm::bi::hypot<C>(static_cast<C>(x), static_cast<C>(y)));
  }
}
// [c.math.hypot3]: no C counterpart. At run time float and double compute in a wider type
// where one has the range for the squares; otherwise (and always during constant evaluation)
// the exact algorithm.
template <class T>
constexpr T hypot3(T x, T y, T z) noexcept {
  if !consteval {
    if constexpr (fp_format<T>.digits <= 24 && fp_format<T>.max_exp <= 128) {
      const double a = x, b = y, c = z; // exact squares; a sum of three rounds once or twice
      return static_cast<T>(__builtin_sqrt(a * a + b * b + c * c));
    } else if constexpr (fp_format<T>.digits <= 53 && fp_format<T>.max_exp <= 1024 && fp_format<long double>.digits > 53 &&
                         fp_format<long double>.max_exp >= 2048) {
      if (!__builtin_isinf(x) && !__builtin_isinf(y) && !__builtin_isinf(z)) {
        const long double a = x, b = y, c = z;
        return static_cast<T>(__builtin_sqrtl(a * a + b * b + c * c));
      }
    }
  }
  return ycxx::detail::fpm::fp_hypot3(x, y, z);
}

enum class rint_op : unsigned char { ceil, floor, trunc, round };
template <rint_op F, class T>
constexpr T round_integral(T x) noexcept {
  using C = carrier_t<T>;
  namespace fpm = ycxx::detail::fpm;
  if consteval {
    return fpm::fp_rint(x, F == rint_op::ceil ? fpm::fp_rint_mode::ceil : F == rint_op::floor ? fpm::fp_rint_mode::floor
                                             : F == rint_op::trunc ? fpm::fp_rint_mode::trunc : fpm::fp_rint_mode::half_away);
  } else {
    const C c = static_cast<C>(x);
    if constexpr (F == rint_op::ceil) return static_cast<T>(fpm::bi::ceil<C>(c));
    else if constexpr (F == rint_op::floor) return static_cast<T>(fpm::bi::floor<C>(c));
    else if constexpr (F == rint_op::trunc) return static_cast<T>(fpm::bi::trunc<C>(c));
    else return static_cast<T>(fpm::bi::round<C>(c));
  }
}
template <class I, class T>
constexpr I lround(T x) noexcept {
  using C = carrier_t<T>;
  if consteval {
    return ycxx::detail::fpm::fp_to_integer<I>(x, ycxx::detail::fpm::fp_rint_mode::half_away);
  } else {
    if constexpr (sizeof(I) == sizeof(long) && __is_same(I, long))
      return ycxx::detail::fpm::bi::lround<C>(static_cast<C>(x));
    else
      return ycxx::detail::fpm::bi::llround<C>(static_cast<C>(x));
  }
}
template <class T>
constexpr T fmod(T x, T y) noexcept {
  using C = carrier_t<T>;
  if consteval {
    return ycxx::detail::fpm::fp_fmod(x, y);
  } else {
    return static_cast<T>(ycxx::detail::fpm::bi::fmod<C>(static_cast<C>(x), static_cast<C>(y)));
  }
}
template <class T>
constexpr T remainder(T x, T y) noexcept {
  using C = carrier_t<T>;
  if consteval {
    return ycxx::detail::fpm::fp_remainder(x, y);
  } else {
    return static_cast<T>(ycxx::detail::fpm::bi::remainder<C>(static_cast<C>(x), static_cast<C>(y)));
  }
}
template <class T>
constexpr T remquo(T x, T y, int* q) noexcept {
  using C = carrier_t<T>;
  if consteval {
    return ycxx::detail::fpm::fp_remquo(x, y, q);
  } else {
    return static_cast<T>(ycxx::detail::fpm::bi::remquo<C>(static_cast<C>(x), static_cast<C>(y), q));
  }
}
template <class T>
constexpr T frexp(T x, int* e) noexcept {
  using C = carrier_t<T>;
  if consteval {
    return ycxx::detail::fpm::fp_frexp(x, e);
  } else {
    return static_cast<T>(ycxx::detail::fpm::bi::frexp<C>(static_cast<C>(x), e));
  }
}
template <class T>
constexpr T scalbln(T x, long n) noexcept {
  using C = carrier_t<T>;
  if consteval {
    return ycxx::detail::fpm::fp_scale(x, n);
  } else {
    return ycxx::detail::cm::narrow<T>(ycxx::detail::fpm::bi::scalbln<C>(static_cast<C>(x), n));
  }
}
template <class T>
constexpr T modf(T x, T* ip) noexcept {
  using C = carrier_t<T>;
  if consteval {
    return ycxx::detail::fpm::fp_modf(x, ip);
  } else {
    C ci{};
    const C r = ycxx::detail::fpm::bi::modf<C>(static_cast<C>(x), __builtin_addressof(ci));
    *ip = static_cast<T>(ci);
    return static_cast<T>(r);
  }
}
template <class T>
constexpr int ilogb(T x, int ilogb0, int ilogbnan) noexcept {
  using C = carrier_t<T>;
  if consteval {
    return ycxx::detail::fpm::fp_ilogb(x, ilogb0, ilogbnan);
  } else {
    return ycxx::detail::fpm::bi::ilogb<C>(static_cast<C>(x));
  }
}
template <class T>
constexpr T logb(T x) noexcept {
  using C = carrier_t<T>;
  if consteval {
    return ycxx::detail::fpm::fp_logb(x);
  } else {
    return static_cast<T>(ycxx::detail::fpm::bi::logb<C>(static_cast<C>(x)));
  }
}
template <class T>
constexpr T nextafter(T x, T y) noexcept {
  using C = carrier_t<T>;
  if constexpr (exact_carrier<T>) {
    if !consteval {
      return static_cast<T>(ycxx::detail::fpm::bi::nextafter<C>(static_cast<C>(x), static_cast<C>(y)));
    }
  }
  return ycxx::detail::fpm::fp_nextafter(x, y);
}
template <class T>
constexpr T nexttoward(T x, long double y) noexcept {
  if !consteval {
    return ycxx::detail::fpm::bi::nexttoward<T>(x, y);
  }
  return ycxx::detail::fpm::fp_nexttoward(x, y);
}
template <class T>
constexpr T fdim(T x, T y) noexcept {
  return ycxx::detail::fpm::fp_fdim(x, y);
}
template <class T>
constexpr T fmax(T x, T y) noexcept {
  return ycxx::detail::fpm::fp_minmax(x, y, true, true);
}
template <class T>
constexpr T fmin(T x, T y) noexcept {
  return ycxx::detail::fpm::fp_minmax(x, y, false, true);
}
template <class T>
constexpr T fma(T x, T y, T z) noexcept {
  using C = carrier_t<T>;
  if constexpr (exact_carrier<T>) {
    if !consteval {
      return static_cast<T>(ycxx::detail::fpm::bi::fma<C>(static_cast<C>(x), static_cast<C>(y), static_cast<C>(z)));
    }
  }
  return ycxx::detail::fpm::fp_fma(x, y, z);
}

// Not constexpr in the draft (they depend on the rounding mode).
template <class T>
T rint(T x) noexcept {
  using C = carrier_t<T>;
  return static_cast<T>(ycxx::detail::fpm::bi::rint<C>(static_cast<C>(x)));
}
template <class T>
T nearbyint(T x) noexcept {
  using C = carrier_t<T>;
  return static_cast<T>(ycxx::detail::fpm::bi::nearbyint<C>(static_cast<C>(x)));
}
template <class I, class T>
I lrint(T x) noexcept {
  using C = carrier_t<T>;
  if constexpr (__is_same(I, long))
    return ycxx::detail::fpm::bi::lrint<C>(static_cast<C>(x));
  else
    return ycxx::detail::fpm::bi::llrint<C>(static_cast<C>(x));
}

} // namespace ycxx::detail::cm
