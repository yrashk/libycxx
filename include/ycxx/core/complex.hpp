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

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

// [complex.members]/3: complex<T>(const complex<X>&) is implicit iff the floating-point
// conversion rank of T is at least that of X (subranks do not matter).
template <class _Tp, class _Xp>
consteval bool __complex_rank_ge() {
  if constexpr (__is_floating_v<_Tp> && __is_floating_v<_Xp>) {
    constexpr int __it = __ycxx::__detail::__fp_rank_index<_Tp>(), __ix = __ycxx::__detail::__fp_rank_index<_Xp>();
    if constexpr (__it >= 0 && __ix >= 0)
      return __it >= __ix;
    else
      return __fp_values_subset<_Xp, _Tp>;
  } else {
    return true;
  }
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Tp>
class complex {
public:
  using value_type = _Tp;

  constexpr complex(const _Tp& __re = _Tp(), const _Tp& __im = _Tp()) : __re_(__re), __im_(__im) {}
  constexpr complex(const complex&) = default;
  template <class _Xp>
  constexpr explicit(!__ycxx::__detail::__complex_rank_ge<_Tp, _Xp>()) complex(const complex<_Xp>& other)
      : __re_(static_cast<_Tp>(other.real())), __im_(static_cast<_Tp>(other.imag())) {}

  constexpr _Tp real() const { return __re_; }
  constexpr void real(_Tp __val) { __re_ = __val; }
  constexpr _Tp imag() const { return __im_; }
  constexpr void imag(_Tp __val) { __im_ = __val; }

  constexpr complex& operator=(const _Tp& __rhs) {
    __re_ = __rhs;
    __im_ = _Tp();
    return *this;
  }
  constexpr complex& operator+=(const _Tp& __rhs) {
    __re_ += __rhs;
    return *this;
  }
  constexpr complex& operator-=(const _Tp& __rhs) {
    __re_ -= __rhs;
    return *this;
  }
  constexpr complex& operator*=(const _Tp& __rhs) {
    __re_ *= __rhs;
    __im_ *= __rhs;
    return *this;
  }
  constexpr complex& operator/=(const _Tp& __rhs) {
    __re_ /= __rhs;
    __im_ /= __rhs;
    return *this;
  }

  constexpr complex& operator=(const complex&) = default;
  template <class _Xp>
  constexpr complex& operator=(const complex<_Xp>& __rhs) {
    __re_ = static_cast<_Tp>(__rhs.real());
    __im_ = static_cast<_Tp>(__rhs.imag());
    return *this;
  }
  template <class _Xp>
  constexpr complex& operator+=(const complex<_Xp>& __rhs) {
    __re_ += static_cast<_Tp>(__rhs.real());
    __im_ += static_cast<_Tp>(__rhs.imag());
    return *this;
  }
  template <class _Xp>
  constexpr complex& operator-=(const complex<_Xp>& __rhs) {
    __re_ -= static_cast<_Tp>(__rhs.real());
    __im_ -= static_cast<_Tp>(__rhs.imag());
    return *this;
  }
  template <class _Xp>
  constexpr complex& operator*=(const complex<_Xp>& __rhs);
  template <class _Xp>
  constexpr complex& operator/=(const complex<_Xp>& __rhs);

private:
  template <class _Up>
  friend constexpr complex<_Up> operator*(const complex<_Up>&, const complex<_Up>&);
  template <class _Up>
  friend constexpr complex<_Up> operator/(const complex<_Up>&, const complex<_Up>&);
  // The comparisons read the members directly: constant evaluation counts every call.
  template <class _Up>
  friend constexpr bool operator==(const complex<_Up>&, const complex<_Up>&);
  template <class _Up>
  friend constexpr bool operator==(const complex<_Up>&, const _Up&);
  template <size_t _Ip, class _Up>
  friend constexpr _Up& get(complex<_Up>&) noexcept;
  template <size_t _Ip, class _Up>
  friend constexpr _Up&& get(complex<_Up>&&) noexcept;
  template <size_t _Ip, class _Up>
  friend constexpr const _Up& get(const complex<_Up>&) noexcept;
  template <size_t _Ip, class _Up>
  friend constexpr const _Up&& get(const complex<_Up>&&) noexcept;

  _Tp __re_;
  _Tp __im_;
};

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail::__cx {

template <class _Tp>
constexpr bool isnan(_Tp __x) noexcept {
  return __builtin_isnan(__x);
}
template <class _Tp>
constexpr bool isinf(_Tp __x) noexcept {
  return __builtin_isinf(__x);
}
template <class _Tp>
constexpr bool isfinite(_Tp __x) noexcept {
  return __builtin_isfinite(__x);
}
template <class _Tp>
constexpr bool signbit(_Tp __x) noexcept {
  return __builtin_signbit(__x);
}
// The helpers below are single expressions without nested calls: constant evaluation counts
// every statement and every call, and complex arithmetic on special values calls them often.
template <class _Tp>
constexpr _Tp copysign(_Tp __x, _Tp y) noexcept {
  return __builtin_signbit(__x) != __builtin_signbit(y) ? -__x : __x;
}
template <class _Tp>
constexpr _Tp fabs(_Tp __x) noexcept {
  return __builtin_signbit(__x) ? -__x : __x;
}
template <class _Tp>
constexpr _Tp __inf() noexcept {
  return __ycxx::__detail::__fpm::__fp_inf_v<_Tp>[0];
}
template <class _Tp>
constexpr _Tp nan() noexcept {
  return __ycxx::__detail::__fpm::__fp_qnan_v<_Tp>[0];
}
template <class _Tp>
inline constexpr _Tp pi = __ycxx::__detail::__math_constant_value<_Tp>(__math_constant::pi);
template <class _Tp>
inline constexpr _Tp __half_pi = __ycxx::__detail::__math_constant_value<_Tp>(__math_constant::pi) / 2;
template <class _Tp>
inline constexpr _Tp __quarter_pi = __ycxx::__detail::__math_constant_value<_Tp>(__math_constant::pi) / 4;
template <class _Tp>
inline constexpr _Tp __three_quarter_pi = __ycxx::__detail::__fpm::__mp_round<_Tp>(__ycxx::__detail::__fpm::__mp_ldexp(
    __ycxx::__detail::__fpm::__mp_mul_u64(__ycxx::__detail::__fpm::__mp_const<3>(__math_constant::pi), 3), -2)).value;
template <class _Tp>
inline constexpr _Tp ln2 = __ycxx::__detail::__math_constant_value<_Tp>(__math_constant::ln2);
template <class _Tp>
inline constexpr _Tp ln10 = __ycxx::__detail::__math_constant_value<_Tp>(__math_constant::ln10);

template <class _Tp>
constexpr _Tp sqrt(_Tp __x) noexcept {
  return __ycxx::__detail::__cm::sqrt<_Tp>(__x);
}
template <class _Tp>
constexpr _Tp hypot(_Tp __x, _Tp y) noexcept {
  return __ycxx::__detail::__cm::hypot<_Tp>(__x, y);
}
// During constant evaluation the bitwise scalbln/logb of cmath_impl.hpp cost hundreds of
// evaluation steps each. For operands of everyday magnitude, exact multiplications by powers of
// two give the same results for a fraction of that (types whose exponent range exceeds 2^64).
template <class _Tp>
inline constexpr bool __fast_scale = __fp_format<_Tp>.__max_exp > 64 && __fp_format<_Tp>.__min_exp < -64;
template <class _Tp>
constexpr _Tp scalbn(_Tp __x, long n) noexcept {
  if consteval {
    if (n == 0 || __x == _Tp(0) || __builtin_isinf(__x) || (__builtin_isnan(__x) && !__builtin_issignaling(__x)))
      return __x;
    if constexpr (__fast_scale<_Tp>) {
      // |x| in [2^-32, 2^32) and |n| <= 32: x and the result are normal, so the result is exact.
      const _Tp __ax = __x < _Tp(0) ? -__x : __x;
      if (n >= -32 && n <= 32 && __ax >= _Tp(0x1p-32) && __ax < _Tp(0x1p32))
        return n > 0 ? __x * _Tp(1ull << n) : __x / _Tp(1ull << -n);
    }
  }
  return __ycxx::__detail::__cm::scalbln<_Tp>(__x, n);
}
template <class _Tp>
constexpr _Tp logb(_Tp __x) noexcept {
  if consteval {
    if constexpr (__fast_scale<_Tp>) {
      _Tp y = __x < _Tp(0) ? -__x : __x;
      if (y >= _Tp(0x1p-63) && y < _Tp(0x1p64)) { // the exponent by binary search
        int e = 0;
        if (y >= _Tp(1)) {
          for (int k = 32; k != 0; k /= 2)
            if (y >= _Tp(1ull << k)) {
              y /= _Tp(1ull << k);
              e += k;
            }
        } else {
          for (int k = 32; k != 0; k /= 2)
            if (y * _Tp(1ull << k) < _Tp(2)) {
              y *= _Tp(1ull << k);
              e -= k;
            }
        }
        return _Tp(e);
      }
    }
  }
  return __ycxx::__detail::__cm::logb<_Tp>(__x);
}
template <__ycxx::__detail::__cm::op _Fp, class _Tp>
constexpr _Tp __f(_Tp __x, _Tp y = _Tp()) noexcept {
  return __ycxx::__detail::__cm::__transcendental<_Fp, _Tp>(__x, y);
}
template <class _Tp>
constexpr _Tp exp(_Tp __x) noexcept {
  return __ycxx::__detail::__cx::__f<__ycxx::__detail::__cm::op::exp>(__x);
}
template <class _Tp>
constexpr _Tp log(_Tp __x) noexcept {
  return __ycxx::__detail::__cx::__f<__ycxx::__detail::__cm::op::log>(__x);
}
template <class _Tp>
constexpr _Tp log1p(_Tp __x) noexcept {
  return __ycxx::__detail::__cx::__f<__ycxx::__detail::__cm::op::log1p>(__x);
}
template <class _Tp>
constexpr _Tp sin(_Tp __x) noexcept {
  return __ycxx::__detail::__cx::__f<__ycxx::__detail::__cm::op::sin>(__x);
}
template <class _Tp>
constexpr _Tp cos(_Tp __x) noexcept {
  return __ycxx::__detail::__cx::__f<__ycxx::__detail::__cm::op::cos>(__x);
}
template <class _Tp>
constexpr _Tp tan(_Tp __x) noexcept {
  return __ycxx::__detail::__cx::__f<__ycxx::__detail::__cm::op::tan>(__x);
}
template <class _Tp>
constexpr _Tp sinh(_Tp __x) noexcept {
  return __ycxx::__detail::__cx::__f<__ycxx::__detail::__cm::op::sinh>(__x);
}
template <class _Tp>
constexpr _Tp cosh(_Tp __x) noexcept {
  return __ycxx::__detail::__cx::__f<__ycxx::__detail::__cm::op::cosh>(__x);
}
template <class _Tp>
constexpr _Tp asinh(_Tp __x) noexcept {
  return __ycxx::__detail::__cx::__f<__ycxx::__detail::__cm::op::asinh>(__x);
}
template <class _Tp>
constexpr _Tp atan2(_Tp y, _Tp __x) noexcept {
  return __ycxx::__detail::__cx::__f<__ycxx::__detail::__cm::op::atan2>(y, __x);
}

template <class _Tp>
struct __cpair {
  _Tp __re, __im;
};

// IEEE arithmetic on special values. With S, the operations that would raise "invalid" or
// "divide-by-zero" (inf - inf, 0 * inf, x / 0, ...) produce their IEEE result without being
// evaluated: GCC rejects them during constant evaluation, and Annex G's recovery rules need
// their NaN and infinite results.
template <bool _Sp, class _Tp>
constexpr _Tp __s_mul(_Tp p, _Tp __q) noexcept {
  return _Sp && (__builtin_isnan(p) || __builtin_isnan(__q) || (__builtin_isinf(p) && __q == _Tp(0)) ||
               (p == _Tp(0) && __builtin_isinf(__q)))
             ? __ycxx::__detail::__fpm::__fp_qnan_v<_Tp>[0]
             : p * __q;
}
template <bool _Sp, class _Tp>
constexpr _Tp __s_add(_Tp p, _Tp __q) noexcept {
  return _Sp && (__builtin_isnan(p) || __builtin_isnan(__q) ||
               (__builtin_isinf(p) && __builtin_isinf(__q) && __builtin_signbit(p) != __builtin_signbit(__q)))
             ? __ycxx::__detail::__fpm::__fp_qnan_v<_Tp>[0]
             : p + __q;
}
template <bool _Sp, class _Tp>
constexpr _Tp __s_sub(_Tp p, _Tp __q) noexcept {
  return _Sp && (__builtin_isnan(p) || __builtin_isnan(__q) ||
               (__builtin_isinf(p) && __builtin_isinf(__q) && __builtin_signbit(p) == __builtin_signbit(__q)))
             ? __ycxx::__detail::__fpm::__fp_qnan_v<_Tp>[0]
             : p - __q;
}
template <bool _Sp, class _Tp>
constexpr _Tp __s_div(_Tp p, _Tp __q) noexcept {
  return !_Sp ? p / __q
         : __builtin_isnan(p) || __builtin_isnan(__q) || (__builtin_isinf(p) && __builtin_isinf(__q)) ||
                 (p == _Tp(0) && __q == _Tp(0))
             ? __ycxx::__detail::__fpm::__fp_qnan_v<_Tp>[0]
         : __q == _Tp(0) ? __ycxx::__detail::__fpm::__fp_inf_v<_Tp>[__builtin_signbit(p) != __builtin_signbit(__q)]
                     : p / __q;
}

// G.5.1, multiplication: (a + ib)(c + id), recovering infinities from a NaN result.
template <bool _Sp, class _Tp>
constexpr __cpair<_Tp> __mul_special(_Tp a, _Tp b, _Tp c, _Tp d) noexcept {
  const _Tp __ac = __ycxx::__detail::__cx::__s_mul<_Sp>(a, c), __bd = __ycxx::__detail::__cx::__s_mul<_Sp>(b, d);
  const _Tp __ad = __ycxx::__detail::__cx::__s_mul<_Sp>(a, d), __bc = __ycxx::__detail::__cx::__s_mul<_Sp>(b, c);
  _Tp __x = __ycxx::__detail::__cx::__s_sub<_Sp>(__ac, __bd), y = __ycxx::__detail::__cx::__s_add<_Sp>(__ad, __bc);
  if (!(__builtin_isnan(__x) && __builtin_isnan(y))) return {__x, y};
  bool __recalc = false;
  const _Tp zero = _Tp(0), __one = _Tp(1);
  if (__builtin_isinf(a) || __builtin_isinf(b)) {
    a = __ycxx::__detail::__cx::copysign(__builtin_isinf(a) ? __one : zero, a);
    b = __ycxx::__detail::__cx::copysign(__builtin_isinf(b) ? __one : zero, b);
    if (__builtin_isnan(c)) c = __ycxx::__detail::__cx::copysign(zero, c);
    if (__builtin_isnan(d)) d = __ycxx::__detail::__cx::copysign(zero, d);
    __recalc = true;
  }
  if (__builtin_isinf(c) || __builtin_isinf(d)) {
    c = __ycxx::__detail::__cx::copysign(__builtin_isinf(c) ? __one : zero, c);
    d = __ycxx::__detail::__cx::copysign(__builtin_isinf(d) ? __one : zero, d);
    if (__builtin_isnan(a)) a = __ycxx::__detail::__cx::copysign(zero, a);
    if (__builtin_isnan(b)) b = __ycxx::__detail::__cx::copysign(zero, b);
    __recalc = true;
  }
  if (!__recalc && (__builtin_isinf(__ac) || __builtin_isinf(__bd) || __builtin_isinf(__ad) || __builtin_isinf(__bc))) {
    if (__builtin_isnan(a)) a = __ycxx::__detail::__cx::copysign(zero, a);
    if (__builtin_isnan(b)) b = __ycxx::__detail::__cx::copysign(zero, b);
    if (__builtin_isnan(c)) c = __ycxx::__detail::__cx::copysign(zero, c);
    if (__builtin_isnan(d)) d = __ycxx::__detail::__cx::copysign(zero, d);
    __recalc = true;
  }
  if (__recalc) {
    const _Tp i = __ycxx::__detail::__cx::__inf<_Tp>();
    __x = __ycxx::__detail::__cx::__s_mul<_Sp>(i, __ycxx::__detail::__cx::__s_sub<_Sp>(__ycxx::__detail::__cx::__s_mul<_Sp>(a, c), __ycxx::__detail::__cx::__s_mul<_Sp>(b, d)));
    y = __ycxx::__detail::__cx::__s_mul<_Sp>(i, __ycxx::__detail::__cx::__s_add<_Sp>(__ycxx::__detail::__cx::__s_mul<_Sp>(a, d), __ycxx::__detail::__cx::__s_mul<_Sp>(b, c)));
  }
  return {__x, y};
}
template <class _Tp>
constexpr __cpair<_Tp> __mul(_Tp a, _Tp b, _Tp c, _Tp d) noexcept {
  if consteval { // the compilers do not evaluate operations that produce a NaN
    if (!(__builtin_isfinite(a) && __builtin_isfinite(b) && __builtin_isfinite(c) && __builtin_isfinite(d)))
      return __ycxx::__detail::__cx::__mul_special<true>(a, b, c, d);
  }
  const _Tp __x = a * c - b * d, y = a * d + b * c;
  if (__builtin_isnan(__x) && __builtin_isnan(y)) [[unlikely]]
    return __ycxx::__detail::__cx::__mul_special<false>(a, b, c, d);
  return {__x, y};
}

// G.5.1, division: (a + ib) / (c + id), scaled by a power of two to avoid spurious overflow and
// underflow, recovering infinities and zeros from a NaN result.
template <class _Tp>
inline constexpr _Tp __div_lo = __ycxx::__detail::__fpm::__fp_scale(_Tp(1), -(__fp_format<_Tp>.__max_exp / 4));
template <class _Tp>
inline constexpr _Tp __div_hi = __ycxx::__detail::__fpm::__fp_scale(_Tp(1), __fp_format<_Tp>.__max_exp / 4);
// Constant evaluation counts every statement and call, so the special cases below are written
// with few of them (Annex G's operations on special values come up often in tests).
template <bool _Sp, class _Tp>
constexpr __cpair<_Tp> __div_scaled(_Tp a, _Tp b, _Tp c, _Tp d) noexcept {
  const _Tp zero = _Tp(0), __one = _Tp(1), __inf = __ycxx::__detail::__fpm::__fp_inf_v<_Tp>[0],
          __ac = __builtin_signbit(c) ? -c : c, __ad = __builtin_signbit(d) ? -d : d,
          m = __ac > __ad || __builtin_isnan(d) ? __ac : __ad;
  // huge: logb(max(|c|, |d|)) is +inf. unscaled: in constant evaluation (S), the scaling is
  // skipped where it cannot change the result, div's common case with an infinite or NaN
  // numerator part (the finite parts cannot overflow or become subnormal).
  const bool __huge = __builtin_isinf(m),
             __unscaled = _Sp && m >= __div_lo<_Tp> && m <= __div_hi<_Tp> &&
                        !(__builtin_isfinite(a) && (a < zero ? -a : a) > __div_hi<_Tp>) &&
                        !(__builtin_isfinite(b) && (b < zero ? -b : b) > __div_hi<_Tp>);
  long __ilogbw = 0;
  if (m != zero && !__huge && !__builtin_isnan(m) && !__unscaled) {
    __ilogbw = static_cast<long>(__ycxx::__detail::__cx::logb(m));
    c = __ycxx::__detail::__cx::scalbn(c, -__ilogbw);
    d = __ycxx::__detail::__cx::scalbn(d, -__ilogbw);
  }
  const _Tp __denom = __ycxx::__detail::__cx::__s_add<_Sp>(__ycxx::__detail::__cx::__s_mul<_Sp>(c, c), __ycxx::__detail::__cx::__s_mul<_Sp>(d, d));
  _Tp __x = __ycxx::__detail::__cx::__s_div<_Sp>(
        __ycxx::__detail::__cx::__s_add<_Sp>(__ycxx::__detail::__cx::__s_mul<_Sp>(a, c), __ycxx::__detail::__cx::__s_mul<_Sp>(b, d)), __denom),
    y = __ycxx::__detail::__cx::__s_div<_Sp>(
        __ycxx::__detail::__cx::__s_sub<_Sp>(__ycxx::__detail::__cx::__s_mul<_Sp>(b, c), __ycxx::__detail::__cx::__s_mul<_Sp>(a, d)), __denom);
  if (__ilogbw != 0) {
    __x = __ycxx::__detail::__cx::scalbn(__x, -__ilogbw);
    y = __ycxx::__detail::__cx::scalbn(y, -__ilogbw);
  }
  if (__builtin_isnan(__x) && __builtin_isnan(y)) {
    if (__denom == zero && (!__builtin_isnan(a) || !__builtin_isnan(b))) {
      const _Tp i = __builtin_signbit(c) ? -__inf : __inf;
      __x = __ycxx::__detail::__cx::__s_mul<_Sp>(i, a);
      y = __ycxx::__detail::__cx::__s_mul<_Sp>(i, b);
    } else if ((__builtin_isinf(a) || __builtin_isinf(b)) && __builtin_isfinite(c) && __builtin_isfinite(d)) {
      // a and b become +-1 or +-0 and c, d are finite: the inner operations cannot give a NaN.
      const _Tp __a1 = __builtin_isinf(a) ? __one : zero, __b1 = __builtin_isinf(b) ? __one : zero;
      a = __builtin_signbit(a) ? -__a1 : __a1;
      b = __builtin_signbit(b) ? -__b1 : __b1;
      __x = __ycxx::__detail::__cx::__s_mul<_Sp>(__inf, a * c + b * d);
      y = __ycxx::__detail::__cx::__s_mul<_Sp>(__inf, b * c - a * d);
    } else if (__huge && __builtin_isfinite(a) && __builtin_isfinite(b)) {
      c = __ycxx::__detail::__cx::copysign(__builtin_isinf(c) ? __one : zero, c);
      d = __ycxx::__detail::__cx::copysign(__builtin_isinf(d) ? __one : zero, d);
      __x = zero * (a * c + b * d);
      y = zero * (b * c - a * d);
    }
  }
  return {__x, y};
}
template <class _Tp>
constexpr __cpair<_Tp> div(_Tp a, _Tp b, _Tp c, _Tp d) noexcept {
  // Common case: no intermediate result can overflow or become subnormal, so the scaling would
  // be exact and is skipped. Every comparison is false for a NaN, so no operand is a NaN here
  // (constant evaluation rejects an operation that produces one).
  const _Tp __ca = c < _Tp(0) ? -c : c, __da = d < _Tp(0) ? -d : d, __aa = a < _Tp(0) ? -a : a, __ba = b < _Tp(0) ? -b : b;
  if ((__ca >= __div_lo<_Tp> || __da >= __div_lo<_Tp>) && __ca <= __div_hi<_Tp> && __da <= __div_hi<_Tp> && __aa <= __div_hi<_Tp> &&
      __ba <= __div_hi<_Tp>) {
    const _Tp __denom = c * c + d * d;
    return {(a * c + b * d) / __denom, (b * c - a * d) / __denom};
  }
  if consteval {
    return __ycxx::__detail::__cx::__div_scaled<true>(a, b, c, d);
  } else {
    return __ycxx::__detail::__cx::__div_scaled<false>(a, b, c, d);
  }
}

}} // namespace __ycxx::__detail::__cx

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Tp>
template <class _Xp>
constexpr complex<_Tp>& complex<_Tp>::operator*=(const complex<_Xp>& __rhs) {
  const __ycxx::__detail::__cx::__cpair<_Tp> r =
      __ycxx::__detail::__cx::__mul(__re_, __im_, static_cast<_Tp>(__rhs.real()), static_cast<_Tp>(__rhs.imag()));
  __re_ = r.__re;
  __im_ = r.__im;
  return *this;
}
template <class _Tp>
template <class _Xp>
constexpr complex<_Tp>& complex<_Tp>::operator/=(const complex<_Xp>& __rhs) {
  const __ycxx::__detail::__cx::__cpair<_Tp> r =
      __ycxx::__detail::__cx::div(__re_, __im_, static_cast<_Tp>(__rhs.real()), static_cast<_Tp>(__rhs.imag()));
  __re_ = r.__re;
  __im_ = r.__im;
  return *this;
}

// [complex.ops]
template <class _Tp>
constexpr complex<_Tp> operator+(const complex<_Tp>& __lhs) {
  return complex<_Tp>(__lhs);
}
template <class _Tp>
constexpr complex<_Tp> operator-(const complex<_Tp>& __lhs) {
  return complex<_Tp>(-__lhs.real(), -__lhs.imag());
}
template <class _Tp>
constexpr complex<_Tp> operator+(const complex<_Tp>& __lhs, const complex<_Tp>& __rhs) {
  return complex<_Tp>(__lhs) += __rhs;
}
template <class _Tp>
constexpr complex<_Tp> operator+(const complex<_Tp>& __lhs, const _Tp& __rhs) {
  return complex<_Tp>(__lhs) += __rhs;
}
template <class _Tp>
constexpr complex<_Tp> operator+(const _Tp& __lhs, const complex<_Tp>& __rhs) {
  return complex<_Tp>(__lhs + __rhs.real(), __rhs.imag());
}
template <class _Tp>
constexpr complex<_Tp> operator-(const complex<_Tp>& __lhs, const complex<_Tp>& __rhs) {
  return complex<_Tp>(__lhs) -= __rhs;
}
template <class _Tp>
constexpr complex<_Tp> operator-(const complex<_Tp>& __lhs, const _Tp& __rhs) {
  return complex<_Tp>(__lhs) -= __rhs;
}
template <class _Tp>
constexpr complex<_Tp> operator-(const _Tp& __lhs, const complex<_Tp>& __rhs) {
  return complex<_Tp>(__lhs - __rhs.real(), -__rhs.imag());
}
template <class _Tp>
constexpr complex<_Tp> operator*(const complex<_Tp>& __lhs, const complex<_Tp>& __rhs) {
  const __ycxx::__detail::__cx::__cpair<_Tp> r = __ycxx::__detail::__cx::__mul(__lhs.__re_, __lhs.__im_, __rhs.__re_, __rhs.__im_);
  return complex<_Tp>(r.__re, r.__im);
}
template <class _Tp>
constexpr complex<_Tp> operator*(const complex<_Tp>& __lhs, const _Tp& __rhs) {
  return complex<_Tp>(__lhs) *= __rhs;
}
template <class _Tp>
constexpr complex<_Tp> operator*(const _Tp& __lhs, const complex<_Tp>& __rhs) {
  return complex<_Tp>(__lhs * __rhs.real(), __lhs * __rhs.imag());
}
template <class _Tp>
constexpr complex<_Tp> operator/(const complex<_Tp>& __lhs, const complex<_Tp>& __rhs) {
  const __ycxx::__detail::__cx::__cpair<_Tp> r = __ycxx::__detail::__cx::div(__lhs.__re_, __lhs.__im_, __rhs.__re_, __rhs.__im_);
  return complex<_Tp>(r.__re, r.__im);
}
template <class _Tp>
constexpr complex<_Tp> operator/(const complex<_Tp>& __lhs, const _Tp& __rhs) {
  return complex<_Tp>(__lhs) /= __rhs;
}
template <class _Tp>
constexpr complex<_Tp> operator/(const _Tp& __lhs, const complex<_Tp>& __rhs) {
  return complex<_Tp>(__lhs) /= __rhs;
}
template <class _Tp>
constexpr bool operator==(const complex<_Tp>& __lhs, const complex<_Tp>& __rhs) {
  return __lhs.__re_ == __rhs.__re_ && __lhs.__im_ == __rhs.__im_;
}
template <class _Tp>
constexpr bool operator==(const complex<_Tp>& __lhs, const _Tp& __rhs) {
  return __lhs.__re_ == __rhs && __lhs.__im_ == _Tp();
}

// [complex.value.ops]
template <class _Tp>
constexpr _Tp real(const complex<_Tp>& __x) {
  return __x.real();
}
template <class _Tp>
constexpr _Tp imag(const complex<_Tp>& __x) {
  return __x.imag();
}
template <class _Tp>
constexpr _Tp abs(const complex<_Tp>& __x) {
  return __ycxx::__detail::__cx::hypot(__x.real(), __x.imag());
}
template <class _Tp>
constexpr _Tp arg(const complex<_Tp>& __x) {
  return __ycxx::__detail::__cx::atan2(__x.imag(), __x.real());
}
template <class _Tp>
constexpr _Tp norm(const complex<_Tp>& __x) {
  if (__ycxx::__detail::__cx::isinf(__x.real())) return __ycxx::__detail::__cx::fabs(__x.real());
  if (__ycxx::__detail::__cx::isinf(__x.imag())) return __ycxx::__detail::__cx::fabs(__x.imag());
  return __x.real() * __x.real() + __x.imag() * __x.imag();
}
template <class _Tp>
constexpr complex<_Tp> conj(const complex<_Tp>& __x) {
  return complex<_Tp>(__x.real(), -__x.imag());
}
template <class _Tp>
constexpr complex<_Tp> proj(const complex<_Tp>& __x) {
  if (__ycxx::__detail::__cx::isinf(__x.real()) || __ycxx::__detail::__cx::isinf(__x.imag()))
    return complex<_Tp>(__ycxx::__detail::__cx::__inf<_Tp>(), __ycxx::__detail::__cx::copysign(_Tp(0), __x.imag()));
  return __x;
}
template <class _Tp>
constexpr complex<_Tp> polar(const _Tp& __rho, const _Tp& __theta = _Tp()) {
  __ycxx::__detail::__precondition(!__ycxx::__detail::__cx::isnan(__rho) && !(__rho < _Tp(0)) && __ycxx::__detail::__cx::isfinite(__theta),
                             "std::polar: rho must be non-negative and theta finite");
  if (__ycxx::__detail::__cx::isnan(__rho) || __rho < _Tp(0) || !__ycxx::__detail::__cx::isfinite(__theta)) { // violated precondition
    if (__ycxx::__detail::__cx::isinf(__rho) && __rho > _Tp(0)) return complex<_Tp>(__rho, __ycxx::__detail::__cx::nan<_Tp>());
    return complex<_Tp>(__ycxx::__detail::__cx::nan<_Tp>(), __ycxx::__detail::__cx::nan<_Tp>());
  }
  if (__theta == _Tp(0)) return complex<_Tp>(__rho, __theta);
  return complex<_Tp>(__rho * __ycxx::__detail::__cx::cos(__theta), __rho * __ycxx::__detail::__cx::sin(__theta));
}

// [cmplx.over]/1-2: a floating-point argument is effectively cast to complex<T>, an integer one
// to complex<double>.
template <class _Ap>
  requires __ycxx::__detail::is_arithmetic_v<_Ap>
constexpr __ycxx::__detail::__cmath_promote_t<_Ap> real(_Ap __x) {
  return static_cast<__ycxx::__detail::__cmath_promote_t<_Ap>>(__x);
}
template <class _Ap>
  requires __ycxx::__detail::is_arithmetic_v<_Ap>
constexpr __ycxx::__detail::__cmath_promote_t<_Ap> imag(_Ap) {
  return __ycxx::__detail::__cmath_promote_t<_Ap>();
}
template <class _Ap>
  requires __ycxx::__detail::is_arithmetic_v<_Ap>
constexpr __ycxx::__detail::__cmath_promote_t<_Ap> arg(_Ap __x) {
  using _Tp = __ycxx::__detail::__cmath_promote_t<_Ap>;
  return __ycxx::__detail::__cx::atan2(_Tp(0), static_cast<_Tp>(__x));
}
template <class _Ap>
  requires __ycxx::__detail::is_arithmetic_v<_Ap>
constexpr __ycxx::__detail::__cmath_promote_t<_Ap> norm(_Ap __x) {
  using _Tp = __ycxx::__detail::__cmath_promote_t<_Ap>;
  return std::norm(complex<_Tp>(static_cast<_Tp>(__x)));
}
template <class _Ap>
  requires __ycxx::__detail::is_arithmetic_v<_Ap>
constexpr complex<__ycxx::__detail::__cmath_promote_t<_Ap>> conj(_Ap __x) {
  using _Tp = __ycxx::__detail::__cmath_promote_t<_Ap>;
  return std::conj(complex<_Tp>(static_cast<_Tp>(__x)));
}
template <class _Ap>
  requires __ycxx::__detail::is_arithmetic_v<_Ap>
constexpr complex<__ycxx::__detail::__cmath_promote_t<_Ap>> proj(_Ap __x) {
  using _Tp = __ycxx::__detail::__cmath_promote_t<_Ap>;
  return std::proj(complex<_Tp>(static_cast<_Tp>(__x)));
}

}} // namespace std

// ---- transcendental functions ([complex.transcendentals]) ------------------------------------------
namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail::__cx {

template <class _Tp>
using _Cp = std::complex<_Tp>;

template <class _Tp>
constexpr _Cp<_Tp> __csqrt(_Tp __x, _Tp y) noexcept {
  // G.6.4.2
  if (__ycxx::__detail::__cx::isinf(y)) return {__ycxx::__detail::__cx::__inf<_Tp>(), y};
  if (__ycxx::__detail::__cx::isnan(__x)) return {__x, __x};
  if (__ycxx::__detail::__cx::isinf(__x)) {
    if (__x > _Tp(0)) return {__x, __ycxx::__detail::__cx::isnan(y) ? y : __ycxx::__detail::__cx::copysign(_Tp(0), y)};
    return {__ycxx::__detail::__cx::isnan(y) ? y : _Tp(0), __ycxx::__detail::__cx::copysign(__ycxx::__detail::__cx::__inf<_Tp>(), y)};
  }
  if (__ycxx::__detail::__cx::isnan(y)) return {y, y};
  if (__x == _Tp(0) && y == _Tp(0)) return {_Tp(0), y};
  // Scale tiny and huge values by an even power of two.
  long scale = 0;
  const _Tp __ax = __ycxx::__detail::__cx::fabs(__x), __ay = __ycxx::__detail::__cx::fabs(y);
  const _Tp big = __ax > __ay ? __ax : __ay;
  constexpr int p = __fp_format<_Tp>.digits;
  if (big < __ycxx::__detail::__cx::scalbn(_Tp(1), __fp_format<_Tp>.__min_exp + p)) {
    scale = -p;
  } else if (big > __ycxx::__detail::__cx::scalbn(_Tp(1), __fp_format<_Tp>.__max_exp - 3)) {
    scale = 2;
  }
  const _Tp __sx = __ycxx::__detail::__cx::scalbn(__ax, -2 * scale), __sy = __ycxx::__detail::__cx::scalbn(__ay, -2 * scale);
  const _Tp h = __ycxx::__detail::__cx::hypot(__sx, __sy);
  const _Tp t = __ycxx::__detail::__cx::sqrt(__sx / 2 + h / 2);
  const _Tp __u = __ycxx::__detail::__cx::scalbn(t, scale);
  const _Tp __v = __ycxx::__detail::__cx::scalbn(__sy / (2 * t), scale);
  if (__x >= _Tp(0)) return {__u, __ycxx::__detail::__cx::copysign(__v, y)};
  return {__v, __ycxx::__detail::__cx::copysign(__u, y)};
}

// log|z| + i arg z, with log1p near |z| = 1.
template <class _Tp>
constexpr _Cp<_Tp> clog(_Tp __x, _Tp y) noexcept {
  const _Tp __ax = __ycxx::__detail::__cx::fabs(__x), __ay = __ycxx::__detail::__cx::fabs(y);
  const _Tp __mx = __ax > __ay ? __ax : __ay, __mn = __ax > __ay ? __ay : __ax;
  _Tp __re;
  if (__mx >= _Tp(0.5) && __mx <= _Tp(2) && __ycxx::__detail::__cx::isfinite(__mn))
    __re = __ycxx::__detail::__cx::log1p((__mx - _Tp(1)) * (__mx + _Tp(1)) + __mn * __mn) / 2;
  else
    __re = __ycxx::__detail::__cx::log(__ycxx::__detail::__cx::hypot(__x, y));
  return {__re, __ycxx::__detail::__cx::atan2(y, __x)};
}
// log(1 + w) for small w (the inverse functions near their branch points).
template <class _Tp>
constexpr _Cp<_Tp> __clog1p(_Tp __x, _Tp y) noexcept {
  const _Tp __re = __ycxx::__detail::__cx::log1p(__x * (2 + __x) + y * y) / 2;
  return {__re, __ycxx::__detail::__cx::atan2(y, _Tp(1) + __x)};
}

template <class _Tp>
constexpr _Cp<_Tp> __cexp(_Tp __x, _Tp y) noexcept {
  // G.6.3.1
  if (__ycxx::__detail::__cx::isinf(__x)) {
    if (__x > _Tp(0)) {
      if (y == _Tp(0)) return {__x, y};
      if (!__ycxx::__detail::__cx::isfinite(y)) return {__x, __ycxx::__detail::__cx::nan<_Tp>()};
      return {__x * __ycxx::__detail::__cx::cos(y), __x * __ycxx::__detail::__cx::sin(y)};
    }
    if (!__ycxx::__detail::__cx::isfinite(y)) return {_Tp(0), _Tp(0)};
    return {__ycxx::__detail::__cx::copysign(_Tp(0), __ycxx::__detail::__cx::cos(y)), __ycxx::__detail::__cx::copysign(_Tp(0), __ycxx::__detail::__cx::sin(y))};
  }
  if (__ycxx::__detail::__cx::isnan(__x)) {
    if (y == _Tp(0)) return {__x, y};
    return {__x, __x};
  }
  if (!__ycxx::__detail::__cx::isfinite(y)) return {y - y, y - y}; // NaN, "invalid" for an infinite y
  const _Tp e = __ycxx::__detail::__cx::exp(__x);
  if (y == _Tp(0)) return {e, y};
  return {e * __ycxx::__detail::__cx::cos(y), e * __ycxx::__detail::__cx::sin(y)};
}

// G.6.2.5 csinh, G.6.2.4 ccosh (x + iy).
template <class _Tp>
constexpr _Cp<_Tp> __csinh(_Tp __x, _Tp y) noexcept {
  if (__ycxx::__detail::__cx::isfinite(__x) && __ycxx::__detail::__cx::isfinite(y)) {
    if (y == _Tp(0)) return {__ycxx::__detail::__cx::sinh(__x), y};
    return {__ycxx::__detail::__cx::sinh(__x) * __ycxx::__detail::__cx::cos(y), __ycxx::__detail::__cx::cosh(__x) * __ycxx::__detail::__cx::sin(y)};
  }
  if (__x == _Tp(0)) return {__x, y - y}; // +-0 + i NaN
  if (__ycxx::__detail::__cx::isinf(__x)) {
    if (y == _Tp(0)) return {__x, y};
    if (__ycxx::__detail::__cx::isfinite(y)) return {__x * __ycxx::__detail::__cx::cos(y), __ycxx::__detail::__cx::__inf<_Tp>() * __ycxx::__detail::__cx::sin(y)};
    return {__x, y - y}; // +-inf + i NaN
  }
  if (__ycxx::__detail::__cx::isnan(__x) && y == _Tp(0)) return {__x, y};
  return {__ycxx::__detail::__cx::nan<_Tp>(), y - y + __x};
}
template <class _Tp>
constexpr _Cp<_Tp> __ccosh(_Tp __x, _Tp y) noexcept {
  if (__ycxx::__detail::__cx::isfinite(__x) && __ycxx::__detail::__cx::isfinite(y)) {
    if (y == _Tp(0)) return {__ycxx::__detail::__cx::cosh(__x), __x == _Tp(0) ? y : __ycxx::__detail::__cx::sinh(__x) * y};
    return {__ycxx::__detail::__cx::cosh(__x) * __ycxx::__detail::__cx::cos(y), __ycxx::__detail::__cx::sinh(__x) * __ycxx::__detail::__cx::sin(y)};
  }
  if (__x == _Tp(0)) return {y - y, _Tp(0)}; // NaN +- i0
  if (__ycxx::__detail::__cx::isinf(__x)) {
    if (y == _Tp(0)) return {__ycxx::__detail::__cx::__inf<_Tp>(), y};
    if (__ycxx::__detail::__cx::isfinite(y))
      return {__ycxx::__detail::__cx::__inf<_Tp>() * __ycxx::__detail::__cx::cos(y), __x * __ycxx::__detail::__cx::sin(y)};
    return {__ycxx::__detail::__cx::__inf<_Tp>(), y - y}; // +inf + i NaN
  }
  if (__ycxx::__detail::__cx::isnan(__x) && y == _Tp(0)) return {__x, y};
  return {__ycxx::__detail::__cx::nan<_Tp>(), y - y + __x};
}
// G.6.2.6 ctanh
template <class _Tp>
constexpr _Cp<_Tp> __ctanh(_Tp __x, _Tp y) noexcept {
  if (__ycxx::__detail::__cx::isinf(__x)) {
    const _Tp __one = __ycxx::__detail::__cx::copysign(_Tp(1), __x);
    if (__ycxx::__detail::__cx::isfinite(y) && y != _Tp(0))
      return {__one, __ycxx::__detail::__cx::copysign(_Tp(0), __ycxx::__detail::__cx::sin(2 * y))};
    return {__one, __ycxx::__detail::__cx::copysign(_Tp(0), y)};
  }
  if (__ycxx::__detail::__cx::isnan(__x)) {
    if (y == _Tp(0)) return {__x, y};
    return {__x, __x};
  }
  if (!__ycxx::__detail::__cx::isfinite(y)) return {y - y, y - y}; // NaN + i NaN ("invalid" for an infinite y)
  if (y == _Tp(0)) return {__ycxx::__detail::__cx::__f<__ycxx::__detail::__cm::op::tanh>(__x), y};
  // Large |x|: tanh -> +-1, the imaginary part 4 sin y cos y e^(-2|x|).
  const _Tp big = _Tp(__fp_format<_Tp>.digits + 2) * _Tp(0.35);
  if (__ycxx::__detail::__cx::fabs(__x) > big) {
    const _Tp e = __ycxx::__detail::__cx::exp(-2 * __ycxx::__detail::__cx::fabs(__x));
    return {__ycxx::__detail::__cx::copysign(_Tp(1), __x), 4 * __ycxx::__detail::__cx::sin(y) * __ycxx::__detail::__cx::cos(y) * e};
  }
  // Kahan: t = tan y, b = 1 + t^2, s = sinh x, r = sqrt(1 + s^2); (b r s + i t) / (1 + b s^2)
  const _Tp t = __ycxx::__detail::__cx::tan(y), b = 1 + t * t, s = __ycxx::__detail::__cx::sinh(__x), r = __ycxx::__detail::__cx::sqrt(1 + s * s);
  const _Tp den = 1 + b * s * s;
  return {b * r * s / den, t / den};
}

// G.6.1.1 cacos (x + iy)
template <class _Tp>
constexpr _Cp<_Tp> cacos(_Tp __x, _Tp y) noexcept {
  const _Tp __hp = __ycxx::__detail::__cx::__half_pi<_Tp>;
  if (__ycxx::__detail::__cx::isinf(y)) {
    const _Tp __ni = -y; // -i inf * sign
    if (__ycxx::__detail::__cx::isnan(__x)) return {__x, __ni};
    if (__ycxx::__detail::__cx::isinf(__x)) return {__x < _Tp(0) ? __ycxx::__detail::__cx::__three_quarter_pi<_Tp> : __ycxx::__detail::__cx::__quarter_pi<_Tp>, __ni};
    return {__hp, __ni};
  }
  if (__ycxx::__detail::__cx::isinf(__x)) {
    if (__ycxx::__detail::__cx::isnan(y)) return {y, __x}; // NaN +- i inf
    const _Tp __im = __ycxx::__detail::__cx::signbit(y) ? __ycxx::__detail::__cx::__inf<_Tp>() : -__ycxx::__detail::__cx::__inf<_Tp>();
    return {__x < _Tp(0) ? __ycxx::__detail::__cx::pi<_Tp> : _Tp(0), __im};
  }
  if (__ycxx::__detail::__cx::isnan(__x)) return {__x, __x};
  if (__ycxx::__detail::__cx::isnan(y)) {
    if (__x == _Tp(0)) return {__hp, y};
    return {y, y};
  }
  if (__x == _Tp(0) && y == _Tp(0)) return {__hp, -y};
  // Kahan: 2 atan2(re sqrt(1 - z), re sqrt(1 + z)) + i asinh(im(conj(sqrt(1 + z)) sqrt(1 - z)))
  const _Cp<_Tp> __s1 = __ycxx::__detail::__cx::__csqrt(_Tp(1) - __x, -y), __s2 = __ycxx::__detail::__cx::__csqrt(_Tp(1) + __x, y);
  return {2 * __ycxx::__detail::__cx::atan2(__s1.real(), __s2.real()),
          __ycxx::__detail::__cx::asinh(__s2.real() * __s1.imag() - __s2.imag() * __s1.real())};
}
// casin via Kahan's formula (special values from casinh by casin(z) = -i casinh(iz))
template <class _Tp>
constexpr _Cp<_Tp> __casin_finite(_Tp __x, _Tp y) noexcept {
  const _Cp<_Tp> __s1 = __ycxx::__detail::__cx::__csqrt(_Tp(1) - __x, -y), __s2 = __ycxx::__detail::__cx::__csqrt(_Tp(1) + __x, y);
  return {__ycxx::__detail::__cx::atan2(__x, __s1.real() * __s2.real() - __s1.imag() * __s2.imag()),
          __ycxx::__detail::__cx::asinh(__s1.real() * __s2.imag() - __s1.imag() * __s2.real())};
}
// G.6.2.2 casinh
template <class _Tp>
constexpr _Cp<_Tp> casinh(_Tp __x, _Tp y) noexcept {
  if (__ycxx::__detail::__cx::isinf(__x)) {
    if (__ycxx::__detail::__cx::isnan(y)) return {__x, y};
    if (__ycxx::__detail::__cx::isinf(y)) return {__x, __ycxx::__detail::__cx::copysign(__ycxx::__detail::__cx::__quarter_pi<_Tp>, y)};
    return {__x, __ycxx::__detail::__cx::copysign(_Tp(0), y)};
  }
  if (__ycxx::__detail::__cx::isinf(y)) {
    if (__ycxx::__detail::__cx::isnan(__x)) return {y, __x}; // +-inf + i NaN
    return {__ycxx::__detail::__cx::copysign(__ycxx::__detail::__cx::__inf<_Tp>(), __x), __ycxx::__detail::__cx::copysign(__ycxx::__detail::__cx::__half_pi<_Tp>, y)};
  }
  if (__ycxx::__detail::__cx::isnan(__x)) {
    if (y == _Tp(0)) return {__x, y};
    return {__x, __x};
  }
  if (__ycxx::__detail::__cx::isnan(y)) return {y, y};
  if (__x == _Tp(0) && y == _Tp(0)) return {__x, y};
  // asinh(z) = -i asin(iz), iz = -y + ix
  const _Cp<_Tp> a = __ycxx::__detail::__cx::__casin_finite(-y, __x);
  return {a.imag(), -a.real()};
}
// G.6.2.1 cacosh
template <class _Tp>
constexpr _Cp<_Tp> cacosh(_Tp __x, _Tp y) noexcept {
  if (__ycxx::__detail::__cx::isinf(__x)) {
    if (__ycxx::__detail::__cx::isnan(y)) return {__ycxx::__detail::__cx::__inf<_Tp>(), y};
    if (__ycxx::__detail::__cx::isinf(y))
      return {__ycxx::__detail::__cx::__inf<_Tp>(), __ycxx::__detail::__cx::copysign(__x < _Tp(0) ? __ycxx::__detail::__cx::__three_quarter_pi<_Tp>
                                                                               : __ycxx::__detail::__cx::__quarter_pi<_Tp>, y)};
    return {__ycxx::__detail::__cx::__inf<_Tp>(), __ycxx::__detail::__cx::copysign(__x < _Tp(0) ? __ycxx::__detail::__cx::pi<_Tp> : _Tp(0), y)};
  }
  if (__ycxx::__detail::__cx::isinf(y)) {
    if (__ycxx::__detail::__cx::isnan(__x)) return {__ycxx::__detail::__cx::__inf<_Tp>(), __x};
    return {__ycxx::__detail::__cx::__inf<_Tp>(), __ycxx::__detail::__cx::copysign(__ycxx::__detail::__cx::__half_pi<_Tp>, y)};
  }
  if (__ycxx::__detail::__cx::isnan(__x) || __ycxx::__detail::__cx::isnan(y)) return {__x + y, __x + y};
  if (__x == _Tp(0) && y == _Tp(0)) return {_Tp(0), __ycxx::__detail::__cx::copysign(__ycxx::__detail::__cx::__half_pi<_Tp>, y)};
  // Kahan: asinh(re(conj(sqrt(z - 1)) sqrt(z + 1))) + 2i atan2(im sqrt(z - 1), re sqrt(z + 1))
  const _Cp<_Tp> __t1 = __ycxx::__detail::__cx::__csqrt(__x - _Tp(1), y), __t2 = __ycxx::__detail::__cx::__csqrt(__x + _Tp(1), y);
  return {__ycxx::__detail::__cx::asinh(__t1.real() * __t2.real() + __t1.imag() * __t2.imag()),
          2 * __ycxx::__detail::__cx::atan2(__t1.imag(), __t2.real())};
}
// G.6.2.3 catanh
template <class _Tp>
constexpr _Cp<_Tp> catanh(_Tp __x, _Tp y) noexcept {
  const _Tp __hp = __ycxx::__detail::__cx::__half_pi<_Tp>;
  if (__ycxx::__detail::__cx::isinf(y)) return {__ycxx::__detail::__cx::copysign(_Tp(0), __x), __ycxx::__detail::__cx::copysign(__hp, y)};
  if (__ycxx::__detail::__cx::isinf(__x)) {
    if (__ycxx::__detail::__cx::isnan(y)) return {__ycxx::__detail::__cx::copysign(_Tp(0), __x), y};
    return {__ycxx::__detail::__cx::copysign(_Tp(0), __x), __ycxx::__detail::__cx::copysign(__hp, y)};
  }
  if (__ycxx::__detail::__cx::isnan(__x)) return {__x, __x};
  if (__ycxx::__detail::__cx::isnan(y)) {
    if (__x == _Tp(0)) return {__x, y};
    return {y, y};
  }
  if (__x == _Tp(0) && y == _Tp(0)) return {__x, y};
  const _Tp __ax = __ycxx::__detail::__cx::fabs(__x), __ay = __ycxx::__detail::__cx::fabs(y);
  if (__ax == _Tp(1) && y == _Tp(0)) { // a pole: +-inf + i0, "divide-by-zero"
    const _Tp __one = _Tp(1), zero = _Tp(0);
    return {__ycxx::__detail::__cx::copysign(__one / zero, __x), y};
  }
  const _Tp big = __ycxx::__detail::__cx::scalbn(_Tp(1), __fp_format<_Tp>.__max_exp / 2 - 2);
  if (__ax > big || __ay > big) {
    const _Tp h = __ycxx::__detail::__cx::hypot(__x, y);
    return {(__x / h) / h, __ycxx::__detail::__cx::copysign(__hp, y)};
  }
  const _Tp __one_m = _Tp(1) - __ax;
  const _Tp __re = __ycxx::__detail::__cx::log1p(4 * __ax / (__one_m * __one_m + __ay * __ay)) / 4;
  const _Tp __im = __ycxx::__detail::__cx::atan2(2 * y, __one_m * (_Tp(1) + __ax) - __ay * __ay) / 2;
  return {__ycxx::__detail::__cx::copysign(__re, __x), __im};
}

}} // namespace __ycxx::__detail::__cx

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class _Tp>
constexpr complex<_Tp> acos(const complex<_Tp>& __x) {
  return __ycxx::__detail::__cx::cacos(__x.real(), __x.imag());
}
template <class _Tp>
constexpr complex<_Tp> asin(const complex<_Tp>& __x) {
  // asin(z) = -i asinh(iz)
  const complex<_Tp> a = __ycxx::__detail::__cx::casinh(-__x.imag(), __x.real());
  return complex<_Tp>(a.imag(), -a.real());
}
template <class _Tp>
constexpr complex<_Tp> atan(const complex<_Tp>& __x) {
  // atan(z) = -i atanh(iz)
  const complex<_Tp> a = __ycxx::__detail::__cx::catanh(-__x.imag(), __x.real());
  return complex<_Tp>(a.imag(), -a.real());
}
template <class _Tp>
constexpr complex<_Tp> acosh(const complex<_Tp>& __x) {
  return __ycxx::__detail::__cx::cacosh(__x.real(), __x.imag());
}
template <class _Tp>
constexpr complex<_Tp> asinh(const complex<_Tp>& __x) {
  return __ycxx::__detail::__cx::casinh(__x.real(), __x.imag());
}
template <class _Tp>
constexpr complex<_Tp> atanh(const complex<_Tp>& __x) {
  return __ycxx::__detail::__cx::catanh(__x.real(), __x.imag());
}
template <class _Tp>
constexpr complex<_Tp> cos(const complex<_Tp>& __x) {
  // cos(z) = cosh(iz)
  return __ycxx::__detail::__cx::__ccosh(-__x.imag(), __x.real());
}
template <class _Tp>
constexpr complex<_Tp> cosh(const complex<_Tp>& __x) {
  return __ycxx::__detail::__cx::__ccosh(__x.real(), __x.imag());
}
template <class _Tp>
constexpr complex<_Tp> exp(const complex<_Tp>& __x) {
  return __ycxx::__detail::__cx::__cexp(__x.real(), __x.imag());
}
template <class _Tp>
constexpr complex<_Tp> log(const complex<_Tp>& __x) {
  return __ycxx::__detail::__cx::clog(__x.real(), __x.imag());
}
template <class _Tp>
constexpr complex<_Tp> log10(const complex<_Tp>& __x) {
  // [complex.transcendentals]/18: log(x) / log(10)
  return __ycxx::__detail::__cx::clog(__x.real(), __x.imag()) / __ycxx::__detail::__cx::ln10<_Tp>;
}
template <class _Tp>
constexpr complex<_Tp> sin(const complex<_Tp>& __x) {
  // sin(z) = -i sinh(iz)
  const complex<_Tp> s = __ycxx::__detail::__cx::__csinh(-__x.imag(), __x.real());
  return complex<_Tp>(s.imag(), -s.real());
}
template <class _Tp>
constexpr complex<_Tp> sinh(const complex<_Tp>& __x) {
  return __ycxx::__detail::__cx::__csinh(__x.real(), __x.imag());
}
template <class _Tp>
constexpr complex<_Tp> sqrt(const complex<_Tp>& __x) {
  return __ycxx::__detail::__cx::__csqrt(__x.real(), __x.imag());
}
template <class _Tp>
constexpr complex<_Tp> tan(const complex<_Tp>& __x) {
  // tan(z) = -i tanh(iz)
  const complex<_Tp> t = __ycxx::__detail::__cx::__ctanh(-__x.imag(), __x.real());
  return complex<_Tp>(t.imag(), -t.real());
}
template <class _Tp>
constexpr complex<_Tp> tanh(const complex<_Tp>& __x) {
  return __ycxx::__detail::__cx::__ctanh(__x.real(), __x.imag());
}

}} // namespace std

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail::__cx {
// [complex.transcendentals]/20: exp(y * log(x)), literally, so pow(0, 0) (implementation-defined)
// is exp(0 * log(0)), a NaN.
template <class _Tp>
constexpr std::complex<_Tp> __cpow(const std::complex<_Tp>& __x, const std::complex<_Tp>& y) noexcept {
  return std::exp(y * std::log(__x));
}
// [cmplx.over]/3: complex<common_type_t<T1, T3>>, T3 = double for an integer T2.
template <class _T1, class _T2>
using __pow_common_t = std::common_type_t<_T1, std::conditional_t<is_integral_v<_T2>, double, _T2>>;
}} // namespace __ycxx::__detail::__cx

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

// [complex.transcendentals]/20 and [cmplx.over]/3 in one set of templates.
template <class _Tp, class _Up>
constexpr complex<__ycxx::__detail::__cx::__pow_common_t<_Tp, _Up>> pow(const complex<_Tp>& __x, const complex<_Up>& y) {
  using _Rp = __ycxx::__detail::__cx::__pow_common_t<_Tp, _Up>;
  return __ycxx::__detail::__cx::__cpow(complex<_Rp>(__x), complex<_Rp>(y));
}
template <class _Tp, class _Up>
  requires __ycxx::__detail::is_arithmetic_v<_Up>
constexpr complex<__ycxx::__detail::__cx::__pow_common_t<_Tp, _Up>> pow(const complex<_Tp>& __x, const _Up& y) {
  using _Rp = __ycxx::__detail::__cx::__pow_common_t<_Tp, _Up>;
  return __ycxx::__detail::__cx::__cpow(complex<_Rp>(__x), complex<_Rp>(static_cast<_Rp>(y)));
}
template <class _Tp, class _Up>
  requires __ycxx::__detail::is_arithmetic_v<_Tp>
constexpr complex<__ycxx::__detail::__cx::__pow_common_t<_Up, _Tp>> pow(const _Tp& __x, const complex<_Up>& y) {
  using _Rp = __ycxx::__detail::__cx::__pow_common_t<_Up, _Tp>;
  return __ycxx::__detail::__cx::__cpow(complex<_Rp>(static_cast<_Rp>(__x)), complex<_Rp>(y));
}

// [complex.tuple]
template <class _Tp>
struct tuple_size<complex<_Tp>> : integral_constant<size_t, 2> {};
template <size_t _Ip, class _Tp>
struct tuple_element<_Ip, complex<_Tp>> {
  static_assert(_Ip < 2, "[complex.tuple]/1: tuple_element index out of range for std::complex");
  using type = _Tp;
};
template <size_t _Ip, class _Tp>
constexpr _Tp& get(complex<_Tp>& __z) noexcept {
  static_assert(_Ip < 2, "[complex.tuple]/2: std::get index out of range for std::complex");
  if constexpr (_Ip == 0)
    return __z.__re_;
  else
    return __z.__im_;
}
template <size_t _Ip, class _Tp>
constexpr _Tp&& get(complex<_Tp>&& __z) noexcept {
  static_assert(_Ip < 2, "[complex.tuple]/2: std::get index out of range for std::complex");
  if constexpr (_Ip == 0)
    return static_cast<_Tp&&>(__z.__re_);
  else
    return static_cast<_Tp&&>(__z.__im_);
}
template <size_t _Ip, class _Tp>
constexpr const _Tp& get(const complex<_Tp>& __z) noexcept {
  static_assert(_Ip < 2, "[complex.tuple]/2: std::get index out of range for std::complex");
  if constexpr (_Ip == 0)
    return __z.__re_;
  else
    return __z.__im_;
}
template <size_t _Ip, class _Tp>
constexpr const _Tp&& get(const complex<_Tp>&& __z) noexcept {
  static_assert(_Ip < 2, "[complex.tuple]/2: std::get index out of range for std::complex");
  if constexpr (_Ip == 0)
    return static_cast<const _Tp&&>(__z.__re_);
  else
    return static_cast<const _Tp&&>(__z.__im_);
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
template <class __charT, class __traits>
class basic_istream;
template <class __charT, class __traits>
class basic_ostream;
template <class _Tp, class __charT, class __traits>
basic_istream<__charT, __traits>& operator>>(basic_istream<__charT, __traits>& is, complex<_Tp>& __x);
template <class _Tp, class __charT, class __traits>
basic_ostream<__charT, __traits>& operator<<(basic_ostream<__charT, __traits>& __o, const complex<_Tp>& __x);

}} // namespace std
