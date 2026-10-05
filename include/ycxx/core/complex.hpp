// libycxx core: <complex> ([complex.numbers]).
//
// Everything is constexpr (C++26). Multiplication and division follow ISO/IEC 9899:2024
// Annex G (G.5.1: the recovery of infinities from NaN results, division scaled by a power of
// two); a real operand of +, -, * is not turned into a complex number first (G.5.1), so
// x * (a + bi) is (xa, xb). The transcendental functions give the special values of Annex G
// (G.6) and use formulas that stay accurate near their branch points (Kahan's for the inverse
// trigonometric and hyperbolic functions, log1p-based log near |z| = 1). Real functions come
// from cmath_impl.hpp, so constant evaluation has the semantics of [library.c]/3.
// No stream operators: libycxx has no <istream>/<ostream> yet.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/cmath.hpp> // the real functions; also <cmath>, which complex arithmetic usually goes with
#include <ycxx/core/cmath_promote.hpp>
#include <ycxx/core/error.hpp>
#include <ycxx/core/limits.hpp> // also numeric_limits, which code using complex commonly needs
#include <ycxx/core/math_constants.hpp>
#include <ycxx/core/tuple_like.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

// [complex.members]/3: complex<T>(const complex<X>&) is implicit iff the floating-point
// conversion rank of T is at least that of X (subranks do not matter).
template <class T, class X>
consteval bool complex_rank_ge() {
  if constexpr (is_floating_v<T> && is_floating_v<X>) {
    constexpr int it = ycxx::detail::fp_rank_index<T>(), ix = ycxx::detail::fp_rank_index<X>();
    if constexpr (it >= 0 && ix >= 0)
      return it >= ix;
    else
      return fp_values_subset<X, T>;
  } else {
    return true;
  }
}

}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std {

template <class T>
class complex {
public:
  using value_type = T;

  constexpr complex(const T& re = T(), const T& im = T()) : re_(re), im_(im) {}
  constexpr complex(const complex&) = default;
  template <class X>
  constexpr explicit(!ycxx::detail::complex_rank_ge<T, X>()) complex(const complex<X>& other)
      : re_(static_cast<T>(other.real())), im_(static_cast<T>(other.imag())) {}

  constexpr T real() const { return re_; }
  constexpr void real(T val) { re_ = val; }
  constexpr T imag() const { return im_; }
  constexpr void imag(T val) { im_ = val; }

  constexpr complex& operator=(const T& rhs) {
    re_ = rhs;
    im_ = T();
    return *this;
  }
  constexpr complex& operator+=(const T& rhs) {
    re_ += rhs;
    return *this;
  }
  constexpr complex& operator-=(const T& rhs) {
    re_ -= rhs;
    return *this;
  }
  constexpr complex& operator*=(const T& rhs) {
    re_ *= rhs;
    im_ *= rhs;
    return *this;
  }
  constexpr complex& operator/=(const T& rhs) {
    re_ /= rhs;
    im_ /= rhs;
    return *this;
  }

  constexpr complex& operator=(const complex&) = default;
  template <class X>
  constexpr complex& operator=(const complex<X>& rhs) {
    re_ = static_cast<T>(rhs.real());
    im_ = static_cast<T>(rhs.imag());
    return *this;
  }
  template <class X>
  constexpr complex& operator+=(const complex<X>& rhs) {
    re_ += static_cast<T>(rhs.real());
    im_ += static_cast<T>(rhs.imag());
    return *this;
  }
  template <class X>
  constexpr complex& operator-=(const complex<X>& rhs) {
    re_ -= static_cast<T>(rhs.real());
    im_ -= static_cast<T>(rhs.imag());
    return *this;
  }
  template <class X>
  constexpr complex& operator*=(const complex<X>& rhs);
  template <class X>
  constexpr complex& operator/=(const complex<X>& rhs);

private:
  template <class U>
  friend constexpr complex<U> operator*(const complex<U>&, const complex<U>&);
  template <class U>
  friend constexpr complex<U> operator/(const complex<U>&, const complex<U>&);
  // The comparisons read the members directly: constant evaluation counts every call.
  template <class U>
  friend constexpr bool operator==(const complex<U>&, const complex<U>&);
  template <class U>
  friend constexpr bool operator==(const complex<U>&, const U&);
  template <size_t I, class U>
  friend constexpr U& get(complex<U>&) noexcept;
  template <size_t I, class U>
  friend constexpr U&& get(complex<U>&&) noexcept;
  template <size_t I, class U>
  friend constexpr const U& get(const complex<U>&) noexcept;
  template <size_t I, class U>
  friend constexpr const U&& get(const complex<U>&&) noexcept;

  T re_;
  T im_;
};

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::cx {

template <class T>
constexpr bool isnan(T x) noexcept {
  return __builtin_isnan(x);
}
template <class T>
constexpr bool isinf(T x) noexcept {
  return __builtin_isinf(x);
}
template <class T>
constexpr bool isfinite(T x) noexcept {
  return __builtin_isfinite(x);
}
template <class T>
constexpr bool signbit(T x) noexcept {
  return __builtin_signbit(x);
}
// The helpers below are single expressions without nested calls: constant evaluation counts
// every statement and every call, and complex arithmetic on special values calls them often.
template <class T>
constexpr T copysign(T x, T y) noexcept {
  return __builtin_signbit(x) != __builtin_signbit(y) ? -x : x;
}
template <class T>
constexpr T fabs(T x) noexcept {
  return __builtin_signbit(x) ? -x : x;
}
template <class T>
constexpr T inf() noexcept {
  return ycxx::detail::fpm::fp_inf_v<T>[0];
}
template <class T>
constexpr T nan() noexcept {
  return ycxx::detail::fpm::fp_qnan_v<T>[0];
}
template <class T>
inline constexpr T pi = ycxx::detail::math_constant_value<T>(math_constant::pi);
template <class T>
inline constexpr T half_pi = ycxx::detail::math_constant_value<T>(math_constant::pi) / 2;
template <class T>
inline constexpr T quarter_pi = ycxx::detail::math_constant_value<T>(math_constant::pi) / 4;
template <class T>
inline constexpr T three_quarter_pi = ycxx::detail::fpm::mp_round<T>(ycxx::detail::fpm::mp_ldexp(
    ycxx::detail::fpm::mp_mul_u64(ycxx::detail::fpm::mp_const<3>(math_constant::pi), 3), -2)).value;
template <class T>
inline constexpr T ln2 = ycxx::detail::math_constant_value<T>(math_constant::ln2);
template <class T>
inline constexpr T ln10 = ycxx::detail::math_constant_value<T>(math_constant::ln10);

template <class T>
constexpr T sqrt(T x) noexcept {
  return ycxx::detail::cm::sqrt<T>(x);
}
template <class T>
constexpr T hypot(T x, T y) noexcept {
  return ycxx::detail::cm::hypot<T>(x, y);
}
// During constant evaluation the bitwise scalbln/logb of cmath_impl.hpp cost hundreds of
// evaluation steps each. For operands of everyday magnitude, exact multiplications by powers of
// two give the same results for a fraction of that (types whose exponent range exceeds 2^64).
template <class T>
inline constexpr bool fast_scale = fp_format<T>.max_exp > 64 && fp_format<T>.min_exp < -64;
template <class T>
constexpr T scalbn(T x, long n) noexcept {
  if consteval {
    if (n == 0 || x == T(0) || __builtin_isinf(x) || (__builtin_isnan(x) && !__builtin_issignaling(x)))
      return x;
    if constexpr (fast_scale<T>) {
      // |x| in [2^-32, 2^32) and |n| <= 32: x and the result are normal, so the result is exact.
      const T ax = x < T(0) ? -x : x;
      if (n >= -32 && n <= 32 && ax >= T(0x1p-32) && ax < T(0x1p32))
        return n > 0 ? x * T(1ull << n) : x / T(1ull << -n);
    }
  }
  return ycxx::detail::cm::scalbln<T>(x, n);
}
template <class T>
constexpr T logb(T x) noexcept {
  if consteval {
    if constexpr (fast_scale<T>) {
      T y = x < T(0) ? -x : x;
      if (y >= T(0x1p-63) && y < T(0x1p64)) { // the exponent by binary search
        int e = 0;
        if (y >= T(1)) {
          for (int k = 32; k != 0; k /= 2)
            if (y >= T(1ull << k)) {
              y /= T(1ull << k);
              e += k;
            }
        } else {
          for (int k = 32; k != 0; k /= 2)
            if (y * T(1ull << k) < T(2)) {
              y *= T(1ull << k);
              e -= k;
            }
        }
        return T(e);
      }
    }
  }
  return ycxx::detail::cm::logb<T>(x);
}
template <ycxx::detail::cm::op F, class T>
constexpr T f(T x, T y = T()) noexcept {
  return ycxx::detail::cm::transcendental<F, T>(x, y);
}
template <class T>
constexpr T exp(T x) noexcept {
  return ycxx::detail::cx::f<ycxx::detail::cm::op::exp>(x);
}
template <class T>
constexpr T log(T x) noexcept {
  return ycxx::detail::cx::f<ycxx::detail::cm::op::log>(x);
}
template <class T>
constexpr T log1p(T x) noexcept {
  return ycxx::detail::cx::f<ycxx::detail::cm::op::log1p>(x);
}
template <class T>
constexpr T sin(T x) noexcept {
  return ycxx::detail::cx::f<ycxx::detail::cm::op::sin>(x);
}
template <class T>
constexpr T cos(T x) noexcept {
  return ycxx::detail::cx::f<ycxx::detail::cm::op::cos>(x);
}
template <class T>
constexpr T tan(T x) noexcept {
  return ycxx::detail::cx::f<ycxx::detail::cm::op::tan>(x);
}
template <class T>
constexpr T sinh(T x) noexcept {
  return ycxx::detail::cx::f<ycxx::detail::cm::op::sinh>(x);
}
template <class T>
constexpr T cosh(T x) noexcept {
  return ycxx::detail::cx::f<ycxx::detail::cm::op::cosh>(x);
}
template <class T>
constexpr T asinh(T x) noexcept {
  return ycxx::detail::cx::f<ycxx::detail::cm::op::asinh>(x);
}
template <class T>
constexpr T atan2(T y, T x) noexcept {
  return ycxx::detail::cx::f<ycxx::detail::cm::op::atan2>(y, x);
}

template <class T>
struct cpair {
  T re, im;
};

// IEEE arithmetic on special values. With S, the operations that would raise "invalid" or
// "divide-by-zero" (inf - inf, 0 * inf, x / 0, ...) produce their IEEE result without being
// evaluated: GCC rejects them during constant evaluation, and Annex G's recovery rules need
// their NaN and infinite results.
template <bool S, class T>
constexpr T s_mul(T p, T q) noexcept {
  return S && (__builtin_isnan(p) || __builtin_isnan(q) || (__builtin_isinf(p) && q == T(0)) ||
               (p == T(0) && __builtin_isinf(q)))
             ? ycxx::detail::fpm::fp_qnan_v<T>[0]
             : p * q;
}
template <bool S, class T>
constexpr T s_add(T p, T q) noexcept {
  return S && (__builtin_isnan(p) || __builtin_isnan(q) ||
               (__builtin_isinf(p) && __builtin_isinf(q) && __builtin_signbit(p) != __builtin_signbit(q)))
             ? ycxx::detail::fpm::fp_qnan_v<T>[0]
             : p + q;
}
template <bool S, class T>
constexpr T s_sub(T p, T q) noexcept {
  return S && (__builtin_isnan(p) || __builtin_isnan(q) ||
               (__builtin_isinf(p) && __builtin_isinf(q) && __builtin_signbit(p) == __builtin_signbit(q)))
             ? ycxx::detail::fpm::fp_qnan_v<T>[0]
             : p - q;
}
template <bool S, class T>
constexpr T s_div(T p, T q) noexcept {
  return !S ? p / q
         : __builtin_isnan(p) || __builtin_isnan(q) || (__builtin_isinf(p) && __builtin_isinf(q)) ||
                 (p == T(0) && q == T(0))
             ? ycxx::detail::fpm::fp_qnan_v<T>[0]
         : q == T(0) ? ycxx::detail::fpm::fp_inf_v<T>[__builtin_signbit(p) != __builtin_signbit(q)]
                     : p / q;
}

// G.5.1, multiplication: (a + ib)(c + id), recovering infinities from a NaN result.
template <bool S, class T>
constexpr cpair<T> mul_special(T a, T b, T c, T d) noexcept {
  const T ac = ycxx::detail::cx::s_mul<S>(a, c), bd = ycxx::detail::cx::s_mul<S>(b, d);
  const T ad = ycxx::detail::cx::s_mul<S>(a, d), bc = ycxx::detail::cx::s_mul<S>(b, c);
  T x = ycxx::detail::cx::s_sub<S>(ac, bd), y = ycxx::detail::cx::s_add<S>(ad, bc);
  if (!(__builtin_isnan(x) && __builtin_isnan(y))) return {x, y};
  bool recalc = false;
  const T zero = T(0), one = T(1);
  if (__builtin_isinf(a) || __builtin_isinf(b)) {
    a = ycxx::detail::cx::copysign(__builtin_isinf(a) ? one : zero, a);
    b = ycxx::detail::cx::copysign(__builtin_isinf(b) ? one : zero, b);
    if (__builtin_isnan(c)) c = ycxx::detail::cx::copysign(zero, c);
    if (__builtin_isnan(d)) d = ycxx::detail::cx::copysign(zero, d);
    recalc = true;
  }
  if (__builtin_isinf(c) || __builtin_isinf(d)) {
    c = ycxx::detail::cx::copysign(__builtin_isinf(c) ? one : zero, c);
    d = ycxx::detail::cx::copysign(__builtin_isinf(d) ? one : zero, d);
    if (__builtin_isnan(a)) a = ycxx::detail::cx::copysign(zero, a);
    if (__builtin_isnan(b)) b = ycxx::detail::cx::copysign(zero, b);
    recalc = true;
  }
  if (!recalc && (__builtin_isinf(ac) || __builtin_isinf(bd) || __builtin_isinf(ad) || __builtin_isinf(bc))) {
    if (__builtin_isnan(a)) a = ycxx::detail::cx::copysign(zero, a);
    if (__builtin_isnan(b)) b = ycxx::detail::cx::copysign(zero, b);
    if (__builtin_isnan(c)) c = ycxx::detail::cx::copysign(zero, c);
    if (__builtin_isnan(d)) d = ycxx::detail::cx::copysign(zero, d);
    recalc = true;
  }
  if (recalc) {
    const T i = ycxx::detail::cx::inf<T>();
    x = ycxx::detail::cx::s_mul<S>(i, ycxx::detail::cx::s_sub<S>(ycxx::detail::cx::s_mul<S>(a, c), ycxx::detail::cx::s_mul<S>(b, d)));
    y = ycxx::detail::cx::s_mul<S>(i, ycxx::detail::cx::s_add<S>(ycxx::detail::cx::s_mul<S>(a, d), ycxx::detail::cx::s_mul<S>(b, c)));
  }
  return {x, y};
}
template <class T>
constexpr cpair<T> mul(T a, T b, T c, T d) noexcept {
  if consteval { // the compilers do not evaluate operations that produce a NaN
    if (!(__builtin_isfinite(a) && __builtin_isfinite(b) && __builtin_isfinite(c) && __builtin_isfinite(d)))
      return ycxx::detail::cx::mul_special<true>(a, b, c, d);
  }
  const T x = a * c - b * d, y = a * d + b * c;
  if (__builtin_isnan(x) && __builtin_isnan(y)) [[unlikely]]
    return ycxx::detail::cx::mul_special<false>(a, b, c, d);
  return {x, y};
}

// G.5.1, division: (a + ib) / (c + id), scaled by a power of two to avoid spurious overflow and
// underflow, recovering infinities and zeros from a NaN result.
template <class T>
inline constexpr T div_lo = ycxx::detail::fpm::fp_scale(T(1), -(fp_format<T>.max_exp / 4));
template <class T>
inline constexpr T div_hi = ycxx::detail::fpm::fp_scale(T(1), fp_format<T>.max_exp / 4);
// Constant evaluation counts every statement and call, so the special cases below are written
// with few of them (Annex G's operations on special values come up often in tests).
template <bool S, class T>
constexpr cpair<T> div_scaled(T a, T b, T c, T d) noexcept {
  const T zero = T(0), one = T(1), inf = ycxx::detail::fpm::fp_inf_v<T>[0],
          ac = __builtin_signbit(c) ? -c : c, ad = __builtin_signbit(d) ? -d : d,
          m = ac > ad || __builtin_isnan(d) ? ac : ad;
  // huge: logb(max(|c|, |d|)) is +inf. unscaled: in constant evaluation (S), the scaling is
  // skipped where it cannot change the result, div's common case with an infinite or NaN
  // numerator part (the finite parts cannot overflow or become subnormal).
  const bool huge = __builtin_isinf(m),
             unscaled = S && m >= div_lo<T> && m <= div_hi<T> &&
                        !(__builtin_isfinite(a) && (a < zero ? -a : a) > div_hi<T>) &&
                        !(__builtin_isfinite(b) && (b < zero ? -b : b) > div_hi<T>);
  long ilogbw = 0;
  if (m != zero && !huge && !__builtin_isnan(m) && !unscaled) {
    ilogbw = static_cast<long>(ycxx::detail::cx::logb(m));
    c = ycxx::detail::cx::scalbn(c, -ilogbw);
    d = ycxx::detail::cx::scalbn(d, -ilogbw);
  }
  const T denom = ycxx::detail::cx::s_add<S>(ycxx::detail::cx::s_mul<S>(c, c), ycxx::detail::cx::s_mul<S>(d, d));
  T x = ycxx::detail::cx::s_div<S>(
        ycxx::detail::cx::s_add<S>(ycxx::detail::cx::s_mul<S>(a, c), ycxx::detail::cx::s_mul<S>(b, d)), denom),
    y = ycxx::detail::cx::s_div<S>(
        ycxx::detail::cx::s_sub<S>(ycxx::detail::cx::s_mul<S>(b, c), ycxx::detail::cx::s_mul<S>(a, d)), denom);
  if (ilogbw != 0) {
    x = ycxx::detail::cx::scalbn(x, -ilogbw);
    y = ycxx::detail::cx::scalbn(y, -ilogbw);
  }
  if (__builtin_isnan(x) && __builtin_isnan(y)) {
    if (denom == zero && (!__builtin_isnan(a) || !__builtin_isnan(b))) {
      const T i = __builtin_signbit(c) ? -inf : inf;
      x = ycxx::detail::cx::s_mul<S>(i, a);
      y = ycxx::detail::cx::s_mul<S>(i, b);
    } else if ((__builtin_isinf(a) || __builtin_isinf(b)) && __builtin_isfinite(c) && __builtin_isfinite(d)) {
      // a and b become +-1 or +-0 and c, d are finite: the inner operations cannot give a NaN.
      const T a1 = __builtin_isinf(a) ? one : zero, b1 = __builtin_isinf(b) ? one : zero;
      a = __builtin_signbit(a) ? -a1 : a1;
      b = __builtin_signbit(b) ? -b1 : b1;
      x = ycxx::detail::cx::s_mul<S>(inf, a * c + b * d);
      y = ycxx::detail::cx::s_mul<S>(inf, b * c - a * d);
    } else if (huge && __builtin_isfinite(a) && __builtin_isfinite(b)) {
      c = ycxx::detail::cx::copysign(__builtin_isinf(c) ? one : zero, c);
      d = ycxx::detail::cx::copysign(__builtin_isinf(d) ? one : zero, d);
      x = zero * (a * c + b * d);
      y = zero * (b * c - a * d);
    }
  }
  return {x, y};
}
template <class T>
constexpr cpair<T> div(T a, T b, T c, T d) noexcept {
  // Common case: no intermediate result can overflow or become subnormal, so the scaling would
  // be exact and is skipped. Every comparison is false for a NaN, so no operand is a NaN here
  // (constant evaluation rejects an operation that produces one).
  const T ca = c < T(0) ? -c : c, da = d < T(0) ? -d : d, aa = a < T(0) ? -a : a, ba = b < T(0) ? -b : b;
  if ((ca >= div_lo<T> || da >= div_lo<T>) && ca <= div_hi<T> && da <= div_hi<T> && aa <= div_hi<T> &&
      ba <= div_hi<T>) {
    const T denom = c * c + d * d;
    return {(a * c + b * d) / denom, (b * c - a * d) / denom};
  }
  if consteval {
    return ycxx::detail::cx::div_scaled<true>(a, b, c, d);
  } else {
    return ycxx::detail::cx::div_scaled<false>(a, b, c, d);
  }
}

}} // namespace ycxx::detail::cx

namespace [[gnu::visibility("hidden")]] std {

template <class T>
template <class X>
constexpr complex<T>& complex<T>::operator*=(const complex<X>& rhs) {
  const ycxx::detail::cx::cpair<T> r =
      ycxx::detail::cx::mul(re_, im_, static_cast<T>(rhs.real()), static_cast<T>(rhs.imag()));
  re_ = r.re;
  im_ = r.im;
  return *this;
}
template <class T>
template <class X>
constexpr complex<T>& complex<T>::operator/=(const complex<X>& rhs) {
  const ycxx::detail::cx::cpair<T> r =
      ycxx::detail::cx::div(re_, im_, static_cast<T>(rhs.real()), static_cast<T>(rhs.imag()));
  re_ = r.re;
  im_ = r.im;
  return *this;
}

// [complex.ops]
template <class T>
constexpr complex<T> operator+(const complex<T>& lhs) {
  return complex<T>(lhs);
}
template <class T>
constexpr complex<T> operator-(const complex<T>& lhs) {
  return complex<T>(-lhs.real(), -lhs.imag());
}
template <class T>
constexpr complex<T> operator+(const complex<T>& lhs, const complex<T>& rhs) {
  return complex<T>(lhs) += rhs;
}
template <class T>
constexpr complex<T> operator+(const complex<T>& lhs, const T& rhs) {
  return complex<T>(lhs) += rhs;
}
template <class T>
constexpr complex<T> operator+(const T& lhs, const complex<T>& rhs) {
  return complex<T>(lhs + rhs.real(), rhs.imag());
}
template <class T>
constexpr complex<T> operator-(const complex<T>& lhs, const complex<T>& rhs) {
  return complex<T>(lhs) -= rhs;
}
template <class T>
constexpr complex<T> operator-(const complex<T>& lhs, const T& rhs) {
  return complex<T>(lhs) -= rhs;
}
template <class T>
constexpr complex<T> operator-(const T& lhs, const complex<T>& rhs) {
  return complex<T>(lhs - rhs.real(), -rhs.imag());
}
template <class T>
constexpr complex<T> operator*(const complex<T>& lhs, const complex<T>& rhs) {
  const ycxx::detail::cx::cpair<T> r = ycxx::detail::cx::mul(lhs.re_, lhs.im_, rhs.re_, rhs.im_);
  return complex<T>(r.re, r.im);
}
template <class T>
constexpr complex<T> operator*(const complex<T>& lhs, const T& rhs) {
  return complex<T>(lhs) *= rhs;
}
template <class T>
constexpr complex<T> operator*(const T& lhs, const complex<T>& rhs) {
  return complex<T>(lhs * rhs.real(), lhs * rhs.imag());
}
template <class T>
constexpr complex<T> operator/(const complex<T>& lhs, const complex<T>& rhs) {
  const ycxx::detail::cx::cpair<T> r = ycxx::detail::cx::div(lhs.re_, lhs.im_, rhs.re_, rhs.im_);
  return complex<T>(r.re, r.im);
}
template <class T>
constexpr complex<T> operator/(const complex<T>& lhs, const T& rhs) {
  return complex<T>(lhs) /= rhs;
}
template <class T>
constexpr complex<T> operator/(const T& lhs, const complex<T>& rhs) {
  return complex<T>(lhs) /= rhs;
}
template <class T>
constexpr bool operator==(const complex<T>& lhs, const complex<T>& rhs) {
  return lhs.re_ == rhs.re_ && lhs.im_ == rhs.im_;
}
template <class T>
constexpr bool operator==(const complex<T>& lhs, const T& rhs) {
  return lhs.re_ == rhs && lhs.im_ == T();
}

// [complex.value.ops]
template <class T>
constexpr T real(const complex<T>& x) {
  return x.real();
}
template <class T>
constexpr T imag(const complex<T>& x) {
  return x.imag();
}
template <class T>
constexpr T abs(const complex<T>& x) {
  return ycxx::detail::cx::hypot(x.real(), x.imag());
}
template <class T>
constexpr T arg(const complex<T>& x) {
  return ycxx::detail::cx::atan2(x.imag(), x.real());
}
template <class T>
constexpr T norm(const complex<T>& x) {
  if (ycxx::detail::cx::isinf(x.real())) return ycxx::detail::cx::fabs(x.real());
  if (ycxx::detail::cx::isinf(x.imag())) return ycxx::detail::cx::fabs(x.imag());
  return x.real() * x.real() + x.imag() * x.imag();
}
template <class T>
constexpr complex<T> conj(const complex<T>& x) {
  return complex<T>(x.real(), -x.imag());
}
template <class T>
constexpr complex<T> proj(const complex<T>& x) {
  if (ycxx::detail::cx::isinf(x.real()) || ycxx::detail::cx::isinf(x.imag()))
    return complex<T>(ycxx::detail::cx::inf<T>(), ycxx::detail::cx::copysign(T(0), x.imag()));
  return x;
}
template <class T>
constexpr complex<T> polar(const T& rho, const T& theta = T()) {
  ycxx::detail::precondition(!ycxx::detail::cx::isnan(rho) && !(rho < T(0)) && ycxx::detail::cx::isfinite(theta),
                             "std::polar: rho must be non-negative and theta finite");
  if (ycxx::detail::cx::isnan(rho) || rho < T(0) || !ycxx::detail::cx::isfinite(theta)) { // violated precondition
    if (ycxx::detail::cx::isinf(rho) && rho > T(0)) return complex<T>(rho, ycxx::detail::cx::nan<T>());
    return complex<T>(ycxx::detail::cx::nan<T>(), ycxx::detail::cx::nan<T>());
  }
  if (theta == T(0)) return complex<T>(rho, theta);
  return complex<T>(rho * ycxx::detail::cx::cos(theta), rho * ycxx::detail::cx::sin(theta));
}

// [cmplx.over]/1-2: a floating-point argument is effectively cast to complex<T>, an integer one
// to complex<double>.
template <class A>
  requires ycxx::detail::is_arithmetic_v<A>
constexpr ycxx::detail::cmath_promote_t<A> real(A x) {
  return static_cast<ycxx::detail::cmath_promote_t<A>>(x);
}
template <class A>
  requires ycxx::detail::is_arithmetic_v<A>
constexpr ycxx::detail::cmath_promote_t<A> imag(A) {
  return ycxx::detail::cmath_promote_t<A>();
}
template <class A>
  requires ycxx::detail::is_arithmetic_v<A>
constexpr ycxx::detail::cmath_promote_t<A> arg(A x) {
  using T = ycxx::detail::cmath_promote_t<A>;
  return ycxx::detail::cx::atan2(T(0), static_cast<T>(x));
}
template <class A>
  requires ycxx::detail::is_arithmetic_v<A>
constexpr ycxx::detail::cmath_promote_t<A> norm(A x) {
  using T = ycxx::detail::cmath_promote_t<A>;
  return std::norm(complex<T>(static_cast<T>(x)));
}
template <class A>
  requires ycxx::detail::is_arithmetic_v<A>
constexpr complex<ycxx::detail::cmath_promote_t<A>> conj(A x) {
  using T = ycxx::detail::cmath_promote_t<A>;
  return std::conj(complex<T>(static_cast<T>(x)));
}
template <class A>
  requires ycxx::detail::is_arithmetic_v<A>
constexpr complex<ycxx::detail::cmath_promote_t<A>> proj(A x) {
  using T = ycxx::detail::cmath_promote_t<A>;
  return std::proj(complex<T>(static_cast<T>(x)));
}

} // namespace std

// ---- transcendental functions ([complex.transcendentals]) ------------------------------------------
namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::cx {

template <class T>
using C = std::complex<T>;

template <class T>
constexpr C<T> csqrt(T x, T y) noexcept {
  // G.6.4.2
  if (ycxx::detail::cx::isinf(y)) return {ycxx::detail::cx::inf<T>(), y};
  if (ycxx::detail::cx::isnan(x)) return {x, x};
  if (ycxx::detail::cx::isinf(x)) {
    if (x > T(0)) return {x, ycxx::detail::cx::isnan(y) ? y : ycxx::detail::cx::copysign(T(0), y)};
    return {ycxx::detail::cx::isnan(y) ? y : T(0), ycxx::detail::cx::copysign(ycxx::detail::cx::inf<T>(), y)};
  }
  if (ycxx::detail::cx::isnan(y)) return {y, y};
  if (x == T(0) && y == T(0)) return {T(0), y};
  // Scale tiny and huge values by an even power of two.
  long scale = 0;
  const T ax = ycxx::detail::cx::fabs(x), ay = ycxx::detail::cx::fabs(y);
  const T big = ax > ay ? ax : ay;
  constexpr int p = fp_format<T>.digits;
  if (big < ycxx::detail::cx::scalbn(T(1), fp_format<T>.min_exp + p)) {
    scale = -p;
  } else if (big > ycxx::detail::cx::scalbn(T(1), fp_format<T>.max_exp - 3)) {
    scale = 2;
  }
  const T sx = ycxx::detail::cx::scalbn(ax, -2 * scale), sy = ycxx::detail::cx::scalbn(ay, -2 * scale);
  const T h = ycxx::detail::cx::hypot(sx, sy);
  const T t = ycxx::detail::cx::sqrt(sx / 2 + h / 2);
  const T u = ycxx::detail::cx::scalbn(t, scale);
  const T v = ycxx::detail::cx::scalbn(sy / (2 * t), scale);
  if (x >= T(0)) return {u, ycxx::detail::cx::copysign(v, y)};
  return {v, ycxx::detail::cx::copysign(u, y)};
}

// log|z| + i arg z, with log1p near |z| = 1.
template <class T>
constexpr C<T> clog(T x, T y) noexcept {
  const T ax = ycxx::detail::cx::fabs(x), ay = ycxx::detail::cx::fabs(y);
  const T mx = ax > ay ? ax : ay, mn = ax > ay ? ay : ax;
  T re;
  if (mx >= T(0.5) && mx <= T(2) && ycxx::detail::cx::isfinite(mn))
    re = ycxx::detail::cx::log1p((mx - T(1)) * (mx + T(1)) + mn * mn) / 2;
  else
    re = ycxx::detail::cx::log(ycxx::detail::cx::hypot(x, y));
  return {re, ycxx::detail::cx::atan2(y, x)};
}
// log(1 + w) for small w (the inverse functions near their branch points).
template <class T>
constexpr C<T> clog1p(T x, T y) noexcept {
  const T re = ycxx::detail::cx::log1p(x * (2 + x) + y * y) / 2;
  return {re, ycxx::detail::cx::atan2(y, T(1) + x)};
}

template <class T>
constexpr C<T> cexp(T x, T y) noexcept {
  // G.6.3.1
  if (ycxx::detail::cx::isinf(x)) {
    if (x > T(0)) {
      if (y == T(0)) return {x, y};
      if (!ycxx::detail::cx::isfinite(y)) return {x, ycxx::detail::cx::nan<T>()};
      return {x * ycxx::detail::cx::cos(y), x * ycxx::detail::cx::sin(y)};
    }
    if (!ycxx::detail::cx::isfinite(y)) return {T(0), T(0)};
    return {ycxx::detail::cx::copysign(T(0), ycxx::detail::cx::cos(y)), ycxx::detail::cx::copysign(T(0), ycxx::detail::cx::sin(y))};
  }
  if (ycxx::detail::cx::isnan(x)) {
    if (y == T(0)) return {x, y};
    return {x, x};
  }
  if (!ycxx::detail::cx::isfinite(y)) return {y - y, y - y}; // NaN, "invalid" for an infinite y
  const T e = ycxx::detail::cx::exp(x);
  if (y == T(0)) return {e, y};
  return {e * ycxx::detail::cx::cos(y), e * ycxx::detail::cx::sin(y)};
}

// G.6.2.5 csinh, G.6.2.4 ccosh (x + iy).
template <class T>
constexpr C<T> csinh(T x, T y) noexcept {
  if (ycxx::detail::cx::isfinite(x) && ycxx::detail::cx::isfinite(y)) {
    if (y == T(0)) return {ycxx::detail::cx::sinh(x), y};
    return {ycxx::detail::cx::sinh(x) * ycxx::detail::cx::cos(y), ycxx::detail::cx::cosh(x) * ycxx::detail::cx::sin(y)};
  }
  if (x == T(0)) return {x, y - y}; // +-0 + i NaN
  if (ycxx::detail::cx::isinf(x)) {
    if (y == T(0)) return {x, y};
    if (ycxx::detail::cx::isfinite(y)) return {x * ycxx::detail::cx::cos(y), ycxx::detail::cx::inf<T>() * ycxx::detail::cx::sin(y)};
    return {x, y - y}; // +-inf + i NaN
  }
  if (ycxx::detail::cx::isnan(x) && y == T(0)) return {x, y};
  return {ycxx::detail::cx::nan<T>(), y - y + x};
}
template <class T>
constexpr C<T> ccosh(T x, T y) noexcept {
  if (ycxx::detail::cx::isfinite(x) && ycxx::detail::cx::isfinite(y)) {
    if (y == T(0)) return {ycxx::detail::cx::cosh(x), x == T(0) ? y : ycxx::detail::cx::sinh(x) * y};
    return {ycxx::detail::cx::cosh(x) * ycxx::detail::cx::cos(y), ycxx::detail::cx::sinh(x) * ycxx::detail::cx::sin(y)};
  }
  if (x == T(0)) return {y - y, T(0)}; // NaN +- i0
  if (ycxx::detail::cx::isinf(x)) {
    if (y == T(0)) return {ycxx::detail::cx::inf<T>(), y};
    if (ycxx::detail::cx::isfinite(y))
      return {ycxx::detail::cx::inf<T>() * ycxx::detail::cx::cos(y), x * ycxx::detail::cx::sin(y)};
    return {ycxx::detail::cx::inf<T>(), y - y}; // +inf + i NaN
  }
  if (ycxx::detail::cx::isnan(x) && y == T(0)) return {x, y};
  return {ycxx::detail::cx::nan<T>(), y - y + x};
}
// G.6.2.6 ctanh
template <class T>
constexpr C<T> ctanh(T x, T y) noexcept {
  if (ycxx::detail::cx::isinf(x)) {
    const T one = ycxx::detail::cx::copysign(T(1), x);
    if (ycxx::detail::cx::isfinite(y) && y != T(0))
      return {one, ycxx::detail::cx::copysign(T(0), ycxx::detail::cx::sin(2 * y))};
    return {one, ycxx::detail::cx::copysign(T(0), y)};
  }
  if (ycxx::detail::cx::isnan(x)) {
    if (y == T(0)) return {x, y};
    return {x, x};
  }
  if (!ycxx::detail::cx::isfinite(y)) return {y - y, y - y}; // NaN + i NaN ("invalid" for an infinite y)
  if (y == T(0)) return {ycxx::detail::cx::f<ycxx::detail::cm::op::tanh>(x), y};
  // Large |x|: tanh -> +-1, the imaginary part 4 sin y cos y e^(-2|x|).
  const T big = T(fp_format<T>.digits + 2) * T(0.35);
  if (ycxx::detail::cx::fabs(x) > big) {
    const T e = ycxx::detail::cx::exp(-2 * ycxx::detail::cx::fabs(x));
    return {ycxx::detail::cx::copysign(T(1), x), 4 * ycxx::detail::cx::sin(y) * ycxx::detail::cx::cos(y) * e};
  }
  // Kahan: t = tan y, b = 1 + t^2, s = sinh x, r = sqrt(1 + s^2); (b r s + i t) / (1 + b s^2)
  const T t = ycxx::detail::cx::tan(y), b = 1 + t * t, s = ycxx::detail::cx::sinh(x), r = ycxx::detail::cx::sqrt(1 + s * s);
  const T den = 1 + b * s * s;
  return {b * r * s / den, t / den};
}

// G.6.1.1 cacos (x + iy)
template <class T>
constexpr C<T> cacos(T x, T y) noexcept {
  const T hp = ycxx::detail::cx::half_pi<T>;
  if (ycxx::detail::cx::isinf(y)) {
    const T ni = -y; // -i inf * sign
    if (ycxx::detail::cx::isnan(x)) return {x, ni};
    if (ycxx::detail::cx::isinf(x)) return {x < T(0) ? ycxx::detail::cx::three_quarter_pi<T> : ycxx::detail::cx::quarter_pi<T>, ni};
    return {hp, ni};
  }
  if (ycxx::detail::cx::isinf(x)) {
    if (ycxx::detail::cx::isnan(y)) return {y, x}; // NaN +- i inf
    const T im = ycxx::detail::cx::signbit(y) ? ycxx::detail::cx::inf<T>() : -ycxx::detail::cx::inf<T>();
    return {x < T(0) ? ycxx::detail::cx::pi<T> : T(0), im};
  }
  if (ycxx::detail::cx::isnan(x)) return {x, x};
  if (ycxx::detail::cx::isnan(y)) {
    if (x == T(0)) return {hp, y};
    return {y, y};
  }
  if (x == T(0) && y == T(0)) return {hp, -y};
  // Kahan: 2 atan2(re sqrt(1 - z), re sqrt(1 + z)) + i asinh(im(conj(sqrt(1 + z)) sqrt(1 - z)))
  const C<T> s1 = ycxx::detail::cx::csqrt(T(1) - x, -y), s2 = ycxx::detail::cx::csqrt(T(1) + x, y);
  return {2 * ycxx::detail::cx::atan2(s1.real(), s2.real()),
          ycxx::detail::cx::asinh(s2.real() * s1.imag() - s2.imag() * s1.real())};
}
// casin via Kahan's formula (special values from casinh by casin(z) = -i casinh(iz))
template <class T>
constexpr C<T> casin_finite(T x, T y) noexcept {
  const C<T> s1 = ycxx::detail::cx::csqrt(T(1) - x, -y), s2 = ycxx::detail::cx::csqrt(T(1) + x, y);
  return {ycxx::detail::cx::atan2(x, s1.real() * s2.real() - s1.imag() * s2.imag()),
          ycxx::detail::cx::asinh(s1.real() * s2.imag() - s1.imag() * s2.real())};
}
// G.6.2.2 casinh
template <class T>
constexpr C<T> casinh(T x, T y) noexcept {
  if (ycxx::detail::cx::isinf(x)) {
    if (ycxx::detail::cx::isnan(y)) return {x, y};
    if (ycxx::detail::cx::isinf(y)) return {x, ycxx::detail::cx::copysign(ycxx::detail::cx::quarter_pi<T>, y)};
    return {x, ycxx::detail::cx::copysign(T(0), y)};
  }
  if (ycxx::detail::cx::isinf(y)) {
    if (ycxx::detail::cx::isnan(x)) return {y, x}; // +-inf + i NaN
    return {ycxx::detail::cx::copysign(ycxx::detail::cx::inf<T>(), x), ycxx::detail::cx::copysign(ycxx::detail::cx::half_pi<T>, y)};
  }
  if (ycxx::detail::cx::isnan(x)) {
    if (y == T(0)) return {x, y};
    return {x, x};
  }
  if (ycxx::detail::cx::isnan(y)) return {y, y};
  if (x == T(0) && y == T(0)) return {x, y};
  // asinh(z) = -i asin(iz), iz = -y + ix
  const C<T> a = ycxx::detail::cx::casin_finite(-y, x);
  return {a.imag(), -a.real()};
}
// G.6.2.1 cacosh
template <class T>
constexpr C<T> cacosh(T x, T y) noexcept {
  if (ycxx::detail::cx::isinf(x)) {
    if (ycxx::detail::cx::isnan(y)) return {ycxx::detail::cx::inf<T>(), y};
    if (ycxx::detail::cx::isinf(y))
      return {ycxx::detail::cx::inf<T>(), ycxx::detail::cx::copysign(x < T(0) ? ycxx::detail::cx::three_quarter_pi<T>
                                                                               : ycxx::detail::cx::quarter_pi<T>, y)};
    return {ycxx::detail::cx::inf<T>(), ycxx::detail::cx::copysign(x < T(0) ? ycxx::detail::cx::pi<T> : T(0), y)};
  }
  if (ycxx::detail::cx::isinf(y)) {
    if (ycxx::detail::cx::isnan(x)) return {ycxx::detail::cx::inf<T>(), x};
    return {ycxx::detail::cx::inf<T>(), ycxx::detail::cx::copysign(ycxx::detail::cx::half_pi<T>, y)};
  }
  if (ycxx::detail::cx::isnan(x) || ycxx::detail::cx::isnan(y)) return {x + y, x + y};
  if (x == T(0) && y == T(0)) return {T(0), ycxx::detail::cx::copysign(ycxx::detail::cx::half_pi<T>, y)};
  // Kahan: asinh(re(conj(sqrt(z - 1)) sqrt(z + 1))) + 2i atan2(im sqrt(z - 1), re sqrt(z + 1))
  const C<T> t1 = ycxx::detail::cx::csqrt(x - T(1), y), t2 = ycxx::detail::cx::csqrt(x + T(1), y);
  return {ycxx::detail::cx::asinh(t1.real() * t2.real() + t1.imag() * t2.imag()),
          2 * ycxx::detail::cx::atan2(t1.imag(), t2.real())};
}
// G.6.2.3 catanh
template <class T>
constexpr C<T> catanh(T x, T y) noexcept {
  const T hp = ycxx::detail::cx::half_pi<T>;
  if (ycxx::detail::cx::isinf(y)) return {ycxx::detail::cx::copysign(T(0), x), ycxx::detail::cx::copysign(hp, y)};
  if (ycxx::detail::cx::isinf(x)) {
    if (ycxx::detail::cx::isnan(y)) return {ycxx::detail::cx::copysign(T(0), x), y};
    return {ycxx::detail::cx::copysign(T(0), x), ycxx::detail::cx::copysign(hp, y)};
  }
  if (ycxx::detail::cx::isnan(x)) return {x, x};
  if (ycxx::detail::cx::isnan(y)) {
    if (x == T(0)) return {x, y};
    return {y, y};
  }
  if (x == T(0) && y == T(0)) return {x, y};
  const T ax = ycxx::detail::cx::fabs(x), ay = ycxx::detail::cx::fabs(y);
  if (ax == T(1) && y == T(0)) { // a pole: +-inf + i0, "divide-by-zero"
    const T one = T(1), zero = T(0);
    return {ycxx::detail::cx::copysign(one / zero, x), y};
  }
  const T big = ycxx::detail::cx::scalbn(T(1), fp_format<T>.max_exp / 2 - 2);
  if (ax > big || ay > big) {
    const T h = ycxx::detail::cx::hypot(x, y);
    return {(x / h) / h, ycxx::detail::cx::copysign(hp, y)};
  }
  const T one_m = T(1) - ax;
  const T re = ycxx::detail::cx::log1p(4 * ax / (one_m * one_m + ay * ay)) / 4;
  const T im = ycxx::detail::cx::atan2(2 * y, one_m * (T(1) + ax) - ay * ay) / 2;
  return {ycxx::detail::cx::copysign(re, x), im};
}

}} // namespace ycxx::detail::cx

namespace [[gnu::visibility("hidden")]] std {

template <class T>
constexpr complex<T> acos(const complex<T>& x) {
  return ycxx::detail::cx::cacos(x.real(), x.imag());
}
template <class T>
constexpr complex<T> asin(const complex<T>& x) {
  // asin(z) = -i asinh(iz)
  const complex<T> a = ycxx::detail::cx::casinh(-x.imag(), x.real());
  return complex<T>(a.imag(), -a.real());
}
template <class T>
constexpr complex<T> atan(const complex<T>& x) {
  // atan(z) = -i atanh(iz)
  const complex<T> a = ycxx::detail::cx::catanh(-x.imag(), x.real());
  return complex<T>(a.imag(), -a.real());
}
template <class T>
constexpr complex<T> acosh(const complex<T>& x) {
  return ycxx::detail::cx::cacosh(x.real(), x.imag());
}
template <class T>
constexpr complex<T> asinh(const complex<T>& x) {
  return ycxx::detail::cx::casinh(x.real(), x.imag());
}
template <class T>
constexpr complex<T> atanh(const complex<T>& x) {
  return ycxx::detail::cx::catanh(x.real(), x.imag());
}
template <class T>
constexpr complex<T> cos(const complex<T>& x) {
  // cos(z) = cosh(iz)
  return ycxx::detail::cx::ccosh(-x.imag(), x.real());
}
template <class T>
constexpr complex<T> cosh(const complex<T>& x) {
  return ycxx::detail::cx::ccosh(x.real(), x.imag());
}
template <class T>
constexpr complex<T> exp(const complex<T>& x) {
  return ycxx::detail::cx::cexp(x.real(), x.imag());
}
template <class T>
constexpr complex<T> log(const complex<T>& x) {
  return ycxx::detail::cx::clog(x.real(), x.imag());
}
template <class T>
constexpr complex<T> log10(const complex<T>& x) {
  // [complex.transcendentals]/18: log(x) / log(10)
  return ycxx::detail::cx::clog(x.real(), x.imag()) / ycxx::detail::cx::ln10<T>;
}
template <class T>
constexpr complex<T> sin(const complex<T>& x) {
  // sin(z) = -i sinh(iz)
  const complex<T> s = ycxx::detail::cx::csinh(-x.imag(), x.real());
  return complex<T>(s.imag(), -s.real());
}
template <class T>
constexpr complex<T> sinh(const complex<T>& x) {
  return ycxx::detail::cx::csinh(x.real(), x.imag());
}
template <class T>
constexpr complex<T> sqrt(const complex<T>& x) {
  return ycxx::detail::cx::csqrt(x.real(), x.imag());
}
template <class T>
constexpr complex<T> tan(const complex<T>& x) {
  // tan(z) = -i tanh(iz)
  const complex<T> t = ycxx::detail::cx::ctanh(-x.imag(), x.real());
  return complex<T>(t.imag(), -t.real());
}
template <class T>
constexpr complex<T> tanh(const complex<T>& x) {
  return ycxx::detail::cx::ctanh(x.real(), x.imag());
}

} // namespace std

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::cx {
// [complex.transcendentals]/20: exp(y * log(x)), literally, so pow(0, 0) (implementation-defined)
// is exp(0 * log(0)), a NaN.
template <class T>
constexpr std::complex<T> cpow(const std::complex<T>& x, const std::complex<T>& y) noexcept {
  return std::exp(y * std::log(x));
}
// [cmplx.over]/3: complex<common_type_t<T1, T3>>, T3 = double for an integer T2.
template <class T1, class T2>
using pow_common_t = std::common_type_t<T1, std::conditional_t<is_integral_v<T2>, double, T2>>;
}} // namespace ycxx::detail::cx

namespace [[gnu::visibility("hidden")]] std {

// [complex.transcendentals]/20 and [cmplx.over]/3 in one set of templates.
template <class T, class U>
constexpr complex<ycxx::detail::cx::pow_common_t<T, U>> pow(const complex<T>& x, const complex<U>& y) {
  using R = ycxx::detail::cx::pow_common_t<T, U>;
  return ycxx::detail::cx::cpow(complex<R>(x), complex<R>(y));
}
template <class T, class U>
  requires ycxx::detail::is_arithmetic_v<U>
constexpr complex<ycxx::detail::cx::pow_common_t<T, U>> pow(const complex<T>& x, const U& y) {
  using R = ycxx::detail::cx::pow_common_t<T, U>;
  return ycxx::detail::cx::cpow(complex<R>(x), complex<R>(static_cast<R>(y)));
}
template <class T, class U>
  requires ycxx::detail::is_arithmetic_v<T>
constexpr complex<ycxx::detail::cx::pow_common_t<U, T>> pow(const T& x, const complex<U>& y) {
  using R = ycxx::detail::cx::pow_common_t<U, T>;
  return ycxx::detail::cx::cpow(complex<R>(static_cast<R>(x)), complex<R>(y));
}

// [complex.tuple]
template <class T>
struct tuple_size<complex<T>> : integral_constant<size_t, 2> {};
template <size_t I, class T>
struct tuple_element<I, complex<T>> {
  static_assert(I < 2, "[complex.tuple]/1: tuple_element index out of range for std::complex");
  using type = T;
};
template <size_t I, class T>
constexpr T& get(complex<T>& z) noexcept {
  static_assert(I < 2, "[complex.tuple]/2: std::get index out of range for std::complex");
  if constexpr (I == 0)
    return z.re_;
  else
    return z.im_;
}
template <size_t I, class T>
constexpr T&& get(complex<T>&& z) noexcept {
  static_assert(I < 2, "[complex.tuple]/2: std::get index out of range for std::complex");
  if constexpr (I == 0)
    return static_cast<T&&>(z.re_);
  else
    return static_cast<T&&>(z.im_);
}
template <size_t I, class T>
constexpr const T& get(const complex<T>& z) noexcept {
  static_assert(I < 2, "[complex.tuple]/2: std::get index out of range for std::complex");
  if constexpr (I == 0)
    return z.re_;
  else
    return z.im_;
}
template <size_t I, class T>
constexpr const T&& get(const complex<T>&& z) noexcept {
  static_assert(I < 2, "[complex.tuple]/2: std::get index out of range for std::complex");
  if constexpr (I == 0)
    return static_cast<const T&&>(z.re_);
  else
    return static_cast<const T&&>(z.im_);
}

// [complex.literals]
inline namespace literals {
inline namespace complex_literals {
constexpr complex<long double> operator""il(long double d) { return complex<long double>{0.0L, d}; }
constexpr complex<long double> operator""il(unsigned long long d) {
  return complex<long double>{0.0L, static_cast<long double>(d)};
}
constexpr complex<double> operator""i(long double d) { return complex<double>{0.0, static_cast<double>(d)}; }
constexpr complex<double> operator""i(unsigned long long d) { return complex<double>{0.0, static_cast<double>(d)}; }
constexpr complex<float> operator""if(long double d) { return complex<float>{0.0f, static_cast<float>(d)}; }
constexpr complex<float> operator""if(unsigned long long d) { return complex<float>{0.0f, static_cast<float>(d)}; }
} // namespace complex_literals
} // namespace literals

// [complex.ops]: the stream operators, declared against declarations of the stream templates
// (ycxx/core/iosfwd.hpp has the same ones) and defined with the streams (ycxx/hosted/istream.hpp,
// ycxx/hosted/ostream.hpp), so <complex> includes no stream header.
template <class charT, class traits>
class basic_istream;
template <class charT, class traits>
class basic_ostream;
template <class T, class charT, class traits>
basic_istream<charT, traits>& operator>>(basic_istream<charT, traits>& is, complex<T>& x);
template <class T, class charT, class traits>
basic_ostream<charT, traits>& operator<<(basic_ostream<charT, traits>& o, const complex<T>& x);

} // namespace std
