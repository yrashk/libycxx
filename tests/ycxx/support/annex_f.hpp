// Annex F (ISO/IEC 9899:2024 F.10) special-value checks for one floating-point type, shared by
// cmath/annex_f_all and cmath/annex_f_extended. annex_f<T>() is constexpr: in constant evaluation
// it skips the RT checks (calls that raise a floating-point exception other than FE_INEXACT are
// not constant expressions, [library.c]/3); at run time the arguments go through a volatile.
#pragma once
#include <climits>
#include <cmath>
#include <limits>
#include <numbers>
#include <type_traits>
#include "check.hpp"

template <class T>
constexpr T id(T x) {
  if consteval {
    return x;
  } else {
    volatile T v = x;
    return v;
  }
}

#define CK(...)                 \
  do {                          \
    if (!(__VA_ARGS__)) {       \
      if !consteval {           \
        CHECK(__VA_ARGS__);     \
      }                         \
      return false;             \
    }                           \
  } while (0)
#define RT(...)                 \
  do {                          \
    if !consteval {             \
      CHECK(__VA_ARGS__);       \
    }                           \
  } while (0)

template <class T>
constexpr bool pz(T x) { return x == 0 && !std::signbit(x); }
template <class T>
constexpr bool nz(T x) { return x == 0 && std::signbit(x); }
template <class T>
constexpr bool pinf(T x) { return x == std::numeric_limits<T>::infinity(); }
template <class T>
constexpr bool ninf(T x) { return x == -std::numeric_limits<T>::infinity(); }
template <class T>
constexpr bool isnan_(T x) { return x != x; }
template <class T>
constexpr bool close(T x, T y) {  // within a few ulps (the inexact results pi/2 etc.)
  T d = x > y ? x - y : y - x;
  T m = y < 0 ? -y : y;
  return d <= 4 * std::numeric_limits<T>::epsilon() * m;
}

template <class T>
constexpr bool annex_f() {
  using L = std::numeric_limits<T>;
  const T z = id(T(0)), mz = id(-T(0)), one = id(T(1)), two = id(T(2)), inf = id(L::infinity()),
          nan = id(L::quiet_NaN()), half = id(T(0.5));
  const T pi = std::numbers::pi_v<T>;
  // F.10.1 trigonometric
  CK(pz(std::acos(one)));
  RT(isnan_(std::acos(id(T(1.5)))));
  CK(pz(std::asin(z)) && nz(std::asin(mz)));
  RT(isnan_(std::asin(-two)));
  CK(pz(std::atan(z)) && nz(std::atan(mz)));
  CK(close(std::atan(inf), pi / 2) && close(std::atan(-inf), -pi / 2));
  CK(close(std::atan2(z, mz), pi) && close(std::atan2(mz, mz), -pi));
  CK(pz(std::atan2(z, z)) && nz(std::atan2(mz, z)));
  CK(close(std::atan2(z, -one), pi) && close(std::atan2(mz, -one), -pi));
  CK(pz(std::atan2(z, one)) && nz(std::atan2(mz, one)));
  CK(close(std::atan2(-one, z), -pi / 2) && close(std::atan2(-one, mz), -pi / 2));
  CK(close(std::atan2(one, z), pi / 2) && close(std::atan2(one, mz), pi / 2));
  CK(close(std::atan2(one, -inf), pi) && close(std::atan2(-one, -inf), -pi));
  CK(pz(std::atan2(one, inf)) && nz(std::atan2(-one, inf)));
  CK(close(std::atan2(inf, one), pi / 2) && close(std::atan2(-inf, one), -pi / 2));
  CK(close(std::atan2(inf, -inf), 3 * pi / 4) && close(std::atan2(-inf, -inf), -3 * pi / 4));
  CK(close(std::atan2(inf, inf), pi / 4) && close(std::atan2(-inf, inf), -pi / 4));
  CK(std::cos(z) == 1 && std::cos(mz) == 1);
  RT(isnan_(std::cos(inf)) && isnan_(std::cos(-inf)));
  CK(pz(std::sin(z)) && nz(std::sin(mz)));
  RT(isnan_(std::sin(inf)) && isnan_(std::sin(-inf)));
  CK(pz(std::tan(z)) && nz(std::tan(mz)));
  RT(isnan_(std::tan(inf)));
  // F.10.2 hyperbolic
  CK(pz(std::acosh(one)) && pinf(std::acosh(inf)));
  RT(isnan_(std::acosh(half)) && isnan_(std::acosh(-inf)));
  CK(pz(std::asinh(z)) && nz(std::asinh(mz)) && pinf(std::asinh(inf)) && ninf(std::asinh(-inf)));
  CK(pz(std::atanh(z)) && nz(std::atanh(mz)));
  RT(pinf(std::atanh(one)) && ninf(std::atanh(-one)));
  RT(isnan_(std::atanh(two)));
  CK(std::cosh(z) == 1 && std::cosh(mz) == 1 && pinf(std::cosh(inf)) && pinf(std::cosh(-inf)));
  CK(pz(std::sinh(z)) && nz(std::sinh(mz)) && pinf(std::sinh(inf)) && ninf(std::sinh(-inf)));
  CK(pz(std::tanh(z)) && nz(std::tanh(mz)) && std::tanh(inf) == 1 && std::tanh(-inf) == -1);
  // F.10.3 exponential and logarithmic
  CK(std::exp(z) == 1 && std::exp(mz) == 1 && pz(std::exp(-inf)) && pinf(std::exp(inf)));
  CK(std::exp2(z) == 1 && std::exp2(mz) == 1 && pz(std::exp2(-inf)) && pinf(std::exp2(inf)));
  CK(close(std::exp2(id(T(-3))), T(0.125)));  // values that are not special: within a few ulps
  CK(pz(std::expm1(z)) && nz(std::expm1(mz)) && std::expm1(-inf) == -1 && pinf(std::expm1(inf)));
  {
    int e = 99;
    CK(pz(std::frexp(z, &e)) && e == 0);
    e = 99;
    CK(nz(std::frexp(mz, &e)) && e == 0);
    CK(pinf(std::frexp(inf, &e)) && ninf(std::frexp(-inf, &e)));
    CK(isnan_(std::frexp(nan, &e)));
    CK(std::frexp(L::denorm_min(), &e) == half && e == L::min_exponent - L::digits + 1);
  }
  RT(std::ilogb(z) == FP_ILOGB0 && std::ilogb(mz) == FP_ILOGB0);
  RT(std::ilogb(inf) == INT_MAX && std::ilogb(-inf) == INT_MAX);
  RT(std::ilogb(nan) == FP_ILOGBNAN);
  CK(std::ilogb(L::denorm_min()) == L::min_exponent - L::digits);
  CK(pz(std::ldexp(z, 5)) && nz(std::ldexp(mz, 5)) && pinf(std::ldexp(inf, -5)) && ninf(std::ldexp(-inf, 5)));
  CK(std::ldexp(id(T(3)), 0) == 3);
  RT(ninf(std::log(z)) && ninf(std::log(mz)));
  CK(pz(std::log(one)) && pinf(std::log(inf)));
  RT(isnan_(std::log(-one)));
  RT(ninf(std::log10(z)) && ninf(std::log2(mz)));
  CK(pz(std::log10(one)) && pz(std::log2(one)) && pinf(std::log10(inf)) && pinf(std::log2(inf)));
  CK(close(std::log2(id(T(1024))), T(10)) && close(std::log10(id(T(100))), T(2)));
  RT(isnan_(std::log10(-one)) && isnan_(std::log2(-inf)));
  CK(pz(std::log1p(z)) && nz(std::log1p(mz)) && pinf(std::log1p(inf)));
  RT(ninf(std::log1p(-one)) && isnan_(std::log1p(-two)));
  RT(ninf(std::logb(z)) && ninf(std::logb(mz)));
  CK(pinf(std::logb(inf)) && pinf(std::logb(-inf)));
  CK(std::logb(L::denorm_min()) == L::min_exponent - L::digits);
  {
    T ip = 7;
    CK(pz(std::modf(inf, &ip)) && pinf(ip));
    CK(nz(std::modf(-inf, &ip)) && ninf(ip));
    CK(isnan_(std::modf(nan, &ip)) && isnan_(ip));
    CK(nz(std::modf(id(T(-3)), &ip)) && ip == -3);
    CK(std::modf(id(T(-0.5)), &ip) == T(-0.5) && nz(ip));
  }
  CK(pz(std::scalbn(z, 3)) && nz(std::scalbn(mz, 3)) && ninf(std::scalbn(-inf, -3)));
  CK(nz(std::scalbln(mz, 3L)) && pinf(std::scalbln(inf, 3L)));
  // F.10.4 power and absolute value
  CK(pz(std::cbrt(z)) && nz(std::cbrt(mz)) && pinf(std::cbrt(inf)) && ninf(std::cbrt(-inf)));
  CK(close(std::cbrt(id(T(-27))), T(-3)));
  CK(pz(std::fabs(mz)) && pinf(std::fabs(-inf)));
  CK(std::hypot(id(T(-3)), z) == 3 && std::hypot(mz, id(T(-3))) == 3);
  CK(pinf(std::hypot(inf, nan)) && pinf(std::hypot(nan, -inf)));
  CK(close(std::hypot(id(T(3)), id(T(-4))), T(5)));
  RT(ninf(std::pow(mz, id(T(-3)))) && pinf(std::pow(z, id(T(-3)))));
  RT(pinf(std::pow(z, -inf)) && pinf(std::pow(mz, -inf)));
  RT(pinf(std::pow(mz, -two)) && pinf(std::pow(mz, -half)));
  CK(nz(std::pow(mz, id(T(3)))) && pz(std::pow(z, id(T(3)))));
  CK(pz(std::pow(mz, two)) && pz(std::pow(mz, half)) && pz(std::pow(mz, inf)));
  CK(std::pow(-one, inf) == 1 && std::pow(-one, -inf) == 1);
  CK(std::pow(one, nan) == 1 && std::pow(one, -inf) == 1);
  CK(std::pow(nan, z) == 1 && std::pow(inf, mz) == 1 && std::pow(-two, z) == 1);
  RT(isnan_(std::pow(-two, half)));
  CK(pinf(std::pow(half, -inf)) && pz(std::pow(-two, -inf)));
  CK(pz(std::pow(-half, inf)) && pinf(std::pow(-two, inf)));
  CK(nz(std::pow(-inf, id(T(-3)))) && pz(std::pow(-inf, -two)) && pz(std::pow(-inf, -half)));
  CK(ninf(std::pow(-inf, id(T(3)))) && pinf(std::pow(-inf, two)) && pinf(std::pow(-inf, half)));
  CK(pz(std::pow(inf, -half)) && pinf(std::pow(inf, half)));
  CK(close(std::pow(id(T(-2)), id(T(3))), T(-8)) && close(std::pow(id(T(4)), half), T(2)));
  CK(pz(std::sqrt(z)) && nz(std::sqrt(mz)) && pinf(std::sqrt(inf)));
  RT(isnan_(std::sqrt(-one)) && isnan_(std::sqrt(-inf)));
  // F.10.5 error and gamma
  CK(pz(std::erf(z)) && nz(std::erf(mz)) && std::erf(inf) == 1 && std::erf(-inf) == -1);
  CK(std::erfc(-inf) == 2 && pz(std::erfc(inf)) && std::erfc(z) == 1);
  CK(pz(std::lgamma(one)) && pz(std::lgamma(two)) && pinf(std::lgamma(inf)) && pinf(std::lgamma(-inf)));
  RT(pinf(std::lgamma(z)) && pinf(std::lgamma(mz)) && pinf(std::lgamma(-two)));
  RT(pinf(std::tgamma(z)) && ninf(std::tgamma(mz)));
  RT(isnan_(std::tgamma(-one)) && isnan_(std::tgamma(-inf)));
  CK(pinf(std::tgamma(inf)) && close(std::tgamma(id(T(5))), T(24)));
  // F.10.6 nearest integer
  CK(pz(std::ceil(z)) && nz(std::ceil(mz)) && nz(std::ceil(id(T(-0.5)))) && ninf(std::ceil(-inf)));
  CK(nz(std::floor(mz)) && pz(std::floor(half)) && pinf(std::floor(inf)) && std::floor(-half) == -1);
  RT(nz(std::nearbyint(id(T(-0.5)))) && nz(std::nearbyint(mz)) && std::nearbyint(id(T(2.5))) == 2);
  RT(nz(std::rint(id(T(-0.25)))) && std::rint(id(T(3.5))) == 4 && pinf(std::rint(inf)));
  RT(std::lrint(id(T(2.5))) == 2 && std::llrint(id(T(-2.5))) == -2 && std::lrint(id(T(-3.5))) == -4);
  CK(nz(std::round(id(T(-0.25)))) && std::round(id(T(-2.5))) == -3 && std::round(id(T(2.5))) == 3);
  CK(nz(std::round(mz)) && ninf(std::round(-inf)) && isnan_(std::round(nan)));
  CK(std::lround(id(T(-2.5))) == -3 && std::llround(id(T(0.5))) == 1);
  CK(nz(std::trunc(id(T(-0.75)))) && pz(std::trunc(id(T(0.75)))) && pinf(std::trunc(inf)));
  // F.10.7 remainder
  CK(pz(std::fmod(z, two)) && nz(std::fmod(mz, two)));
  RT(isnan_(std::fmod(inf, two)) && isnan_(std::fmod(two, z)) && isnan_(std::fmod(-inf, mz)));
  CK(std::fmod(id(T(5)), inf) == 5 && std::fmod(id(T(-5)), -inf) == -5);
  CK(nz(std::fmod(id(T(-4)), two)) && std::fmod(id(T(-6)), id(T(4))) == -2);
  RT(isnan_(std::remainder(two, z)) && isnan_(std::remainder(inf, two)));
  CK(std::remainder(id(T(5)), inf) == 5 && nz(std::remainder(id(T(-4)), two)));
  CK(std::remainder(id(T(7)), two) == -1 && std::remainder(id(T(5)), two) == 1);
  {
    int q = 0;
    CK(std::remquo(id(T(7)), two, &q) == -1 && (q & 7) == 4);
    CK(std::remquo(id(T(-7)), two, &q) == 1 && q < 0 && ((-q) & 7) == 4);
    CK(nz(std::remquo(id(T(-4)), two, &q)) && q < 0 && ((-q) & 7) == 2);
    RT(isnan_(std::remquo(one, z, &q)));
  }
  // F.10.8 manipulation
  CK(std::copysign(one, mz) == -1 && std::copysign(-one, z) == 1 && std::signbit(std::copysign(nan, -one)));
  CK(nz(std::copysign(z, -inf)));
  CK(std::nextafter(one, one) == 1 && pz(std::nextafter(mz, z)) && nz(std::nextafter(z, mz)));
  CK(std::nextafter(inf, inf) == inf && std::nextafter(L::max(), z) < L::max());
  RT(std::nextafter(z, -one) == -L::denorm_min() && std::nextafter(L::denorm_min(), -one) == 0);
  RT(pinf(std::nextafter(L::max(), inf)) && std::nextafter(inf, z) == L::max());
  CK(isnan_(std::nextafter(nan, one)) && isnan_(std::nextafter(one, nan)));
  if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double> || std::is_same_v<T, long double>) {
    // [cmath.syn]/4: nexttoward is ill-formed for the extended types
    CK(std::nexttoward(one, 2.0L) == one + L::epsilon() && nz(std::nexttoward(mz, -0.0L)));
  }
  // F.10.9 maximum, minimum, positive difference
  CK(pz(std::fdim(two, id(T(3)))) && std::fdim(id(T(5)), id(T(3))) == 2 && pz(std::fdim(inf, inf)));
  CK(std::fmax(nan, one) == 1 && std::fmax(one, nan) == 1 && std::fmin(nan, -one) == -1);
  CK(std::fmin(-inf, one) == -inf && std::fmax(-inf, nan) == -inf);
  // F.10.10 fma
  RT(isnan_(std::fma(inf, z, one)) && isnan_(std::fma(z, -inf, nan)) && isnan_(std::fma(inf, one, -inf)));
  CK(std::fma(one + L::epsilon(), one - L::epsilon(), -one) == -L::epsilon() * L::epsilon());
  CK(pz(std::fma(two, z, z)) && nz(std::fma(two, mz, mz)) && pz(std::fma(two, mz, z)));
  RT(ninf(std::fma(L::max(), two, -inf)));  // the exact xy = 2 max is finite; + -inf is -inf
  return true;
}

