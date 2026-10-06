// libycxx core: constexpr software implementations of the <cmath> functions whose results are
// exact or need only correct rounding of an exactly computable value (fma, sqrt, cbrt, hypot,
// fmod, remainder, nextafter, ...), for every binary floating-point format (cmath_fp.hpp).
//
// They are what constant evaluation uses (the compilers fold few of these builtins), and what
// the extended types without a C library counterpart use at run time. Results are correctly
// rounded; the floating-point exceptions of ISO/IEC 9899:2024 Annex F other than "inexact"
// go through fp_report, which makes a constant evaluation non-constant ([library.c]/3).
// Arguments are finite unless a function says otherwise; NaN and infinity handling that the
// C standard specifies is done here too.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/cmath_fp.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__fpm {

// ---- NaN handling ----------------------------------------------------------------------------
template <class _Tp>
constexpr bool __fp_isnan(_Tp __x) noexcept {
  return __builtin_isnan(__x);
}
template <class _Tp>
constexpr bool __fp_signbit(_Tp __x) noexcept {
  return __builtin_signbit(__x);
}
template <class _Tp>
constexpr bool __fp_issignaling(_Tp __x) noexcept {
  return __builtin_issignaling(__x);
}
// A NaN operand: a signaling NaN raises "invalid" and becomes quiet.
template <class _Tp>
constexpr _Tp __fp_nan_operand(_Tp __x) noexcept {
  if (__ycxx::__detail::__fpm::__fp_issignaling(__x)) {
    __ycxx::__detail::__fpm::__fp_report(__fe_invalid);
    return __ycxx::__detail::__fpm::__fp_quiet_nan<_Tp>(__ycxx::__detail::__fpm::__fp_signbit(__x));
  }
  return __x;
}
template <class _Tp>
constexpr _Tp __fp_nan_operands(_Tp __x, _Tp y) noexcept {
  if (__ycxx::__detail::__fpm::__fp_issignaling(__x) || __ycxx::__detail::__fpm::__fp_issignaling(y))
    __ycxx::__detail::__fpm::__fp_report(__fe_invalid);
  return __ycxx::__detail::__fpm::__fp_isnan(__x) ? (__ycxx::__detail::__fpm::__fp_issignaling(__x) ? __ycxx::__detail::__fpm::__fp_quiet_nan<_Tp>() : __x)
                                        : (__ycxx::__detail::__fpm::__fp_issignaling(y) ? __ycxx::__detail::__fpm::__fp_quiet_nan<_Tp>() : y);
}
template <class _Tp>
constexpr _Tp __fp_invalid() noexcept { // a new NaN with "invalid"
  __ycxx::__detail::__fpm::__fp_report(__fe_invalid);
  return __ycxx::__detail::__fpm::__fp_quiet_nan<_Tp>();
}
template <class _Tp>
constexpr _Tp __fp_finish(__fp_result<_Tp> r) noexcept {
  __ycxx::__detail::__fpm::__fp_report(r.flags);
  return r.value;
}

template <class _Tp>
constexpr _Tp __fp_abs(_Tp __x) noexcept {
  return __ycxx::__detail::__fpm::__fp_signbit(__x) ? -__x : __x;
}
template <class _Tp>
constexpr _Tp __fp_copysign(_Tp __x, _Tp y) noexcept {
  return __ycxx::__detail::__fpm::__fp_signbit(__x) != __ycxx::__detail::__fpm::__fp_signbit(y) ? -__x : __x;
}

// ---- rounding to integers ---------------------------------------------------------------------
enum class __fp_rint_mode : unsigned char { trunc, floor, ceil, __half_away, __half_even };

// The integer part of a finite decoded value, rounded by `__mode`. `__inexact` tells whether a
// fraction was dropped.
template <class _Tp>
constexpr __wide<2> __fp_integer_part(const __fp_value& __v, __fp_rint_mode __mode, bool& __inexact) noexcept {
  if (__v.exp >= 0) {
    __inexact = false;
    return __ycxx::__detail::__fpm::__wide_shl(__v.__sig, __v.exp > 127 ? 128 : __v.exp); // callers check the range
  }
  const int s = -__v.exp;
  const bool __half = __ycxx::__detail::__fpm::__wide_bit(__v.__sig, s - 1);
  bool __rest = false;
  __wide<2> __ip = __ycxx::__detail::__fpm::__wide_shr(__ycxx::__detail::__fpm::__wide_shr(__v.__sig, s - 1, __rest), 1);
  __inexact = __half || __rest;
  bool __up = false;
  switch (__mode) {
  case __fp_rint_mode::trunc:
    break;
  case __fp_rint_mode::floor:
    __up = __v.__neg && __inexact;
    break;
  case __fp_rint_mode::ceil:
    __up = !__v.__neg && __inexact;
    break;
  case __fp_rint_mode::__half_away:
    __up = __half;
    break;
  case __fp_rint_mode::__half_even:
    __up = __half && (__rest || (__ip.__w[0] & 1));
    break;
  }
  if (__up) __ycxx::__detail::__fpm::__wide_add_small(__ip, 1);
  return __ip;
}

template <class _Tp>
constexpr _Tp __fp_rint(_Tp __x, __fp_rint_mode __mode) noexcept {
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  if (__v.kind == __fp_kind::nan) return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  if (__v.kind != __fp_kind::__finite || __v.exp >= 0) return __x;
  bool __inexact = false;
  const __wide<2> __ip = __ycxx::__detail::__fpm::__fp_integer_part<_Tp>(__v, __mode, __inexact);
  return __ycxx::__detail::__fpm::__fp_round<_Tp>(__v.__neg, __ip, 0).value; // exact; zero keeps the sign
}

// lround/llround (I = long or long long), and the exactly-rounded integer for lrint at
// constant evaluation (not used: lrint is not constexpr).
template <class _Ip, class _Tp>
constexpr _Ip __fp_to_integer(_Tp __x, __fp_rint_mode __mode) noexcept {
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  if (__v.kind == __fp_kind::zero) return 0;
  if (__v.kind != __fp_kind::__finite) {
    __ycxx::__detail::__fpm::__fp_report(__fe_invalid);
    return static_cast<_Ip>(-__LONG_LONG_MAX__ - 1);
  }
  constexpr int bits = 8 * int(sizeof(_Ip)) - 1;
  bool __inexact = false;
  if (__v.exp + __ycxx::__detail::__fpm::__wide_bitlen(__v.__sig) > bits + 1) { // |x| >= 2^(bits + 1)
    __ycxx::__detail::__fpm::__fp_report(__fe_invalid);
    return static_cast<_Ip>(-__LONG_LONG_MAX__ - 1);
  }
  const __wide<2> __ip = __ycxx::__detail::__fpm::__fp_integer_part<_Tp>(__v, __mode, __inexact);
  const __y_u64 __limit = (__y_u64(1) << bits) - (__v.__neg ? 0 : 1); // magnitude limit
  if (__ip.__w[1] != 0 || __ip.__w[0] > __limit) {
    __ycxx::__detail::__fpm::__fp_report(__fe_invalid);
    return static_cast<_Ip>(-__LONG_LONG_MAX__ - 1);
  }
  return __v.__neg ? static_cast<_Ip>(-static_cast<_Ip>(__ip.__w[0] - 1) - 1) : static_cast<_Ip>(__ip.__w[0]);
}

template <class _Tp>
constexpr _Tp __fp_modf(_Tp __x, _Tp* __iptr) noexcept {
  if (__ycxx::__detail::__fpm::__fp_isnan(__x)) {
    __x = __ycxx::__detail::__fpm::__fp_nan_operand(__x);
    *__iptr = __x;
    return __x;
  }
  if (__builtin_isinf(__x)) {
    *__iptr = __x;
    return __ycxx::__detail::__fpm::__fp_zero<_Tp>(__ycxx::__detail::__fpm::__fp_signbit(__x));
  }
  const _Tp __ip = __ycxx::__detail::__fpm::__fp_rint(__x, __fp_rint_mode::trunc);
  *__iptr = __ip;
  return __ycxx::__detail::__fpm::__fp_copysign(_Tp(__x - __ip), __x); // exact
}

// ---- exponent manipulation ---------------------------------------------------------------------
template <class _Tp>
constexpr _Tp __fp_frexp(_Tp __x, int* e) noexcept {
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  *e = 0;
  if (__v.kind == __fp_kind::nan) return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  if (__v.kind != __fp_kind::__finite) return __x;
  const int __len = __ycxx::__detail::__fpm::__wide_bitlen(__v.__sig);
  *e = __v.exp + __len;
  return __ycxx::__detail::__fpm::__fp_round<_Tp>(__v.__neg, __v.__sig, -__len).value;
}

template <class _Tp>
constexpr _Tp __fp_scale(_Tp __x, long n) noexcept {
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  if (__v.kind == __fp_kind::nan) return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  if (__v.kind != __fp_kind::__finite) return __x;
  constexpr long __lim = 1L << 20; // far beyond every format's exponent range
  if (n > __lim) n = __lim;
  if (n < -__lim) n = -__lim;
  using _Lp = __fp_layout<_Tp>;
  const long e = long(__v.exp) + n;
  if (__v.exp > _Lp::__qmin && e > _Lp::__qmin && e <= _Lp::__emax - (_Lp::p - 1)) // normal in, normal out: exact
    return __ycxx::__detail::__fpm::__fp_encode_finite<_Tp>(__v.__neg, __v.__sig, static_cast<int>(e));
  return __ycxx::__detail::__fpm::__fp_finish(__ycxx::__detail::__fpm::__fp_round<_Tp>(__v.__neg, __v.__sig, static_cast<int>(e)));
}

// ilogb: the exponent; FP_ILOGB0 / INT_MAX / FP_ILOGBNAN (with "invalid", F.10.3.5) otherwise.
template <class _Tp>
constexpr int __fp_ilogb(_Tp __x, int __ilogb0, int __ilogbnan) noexcept {
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  if (__v.kind != __fp_kind::__finite) {
    __ycxx::__detail::__fpm::__fp_report(__fe_invalid);
    return __v.kind == __fp_kind::zero ? __ilogb0 : __v.kind == __fp_kind::__inf ? __INT_MAX__ : __ilogbnan;
  }
  return __v.exp + __ycxx::__detail::__fpm::__wide_bitlen(__v.__sig) - 1;
}
template <class _Tp>
constexpr _Tp __fp_logb(_Tp __x) noexcept {
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  switch (__v.kind) {
  case __fp_kind::nan:
    return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  case __fp_kind::__inf:
    return __ycxx::__detail::__fpm::__fp_infinity<_Tp>(false);
  case __fp_kind::zero:
    __ycxx::__detail::__fpm::__fp_report(__fe_divbyzero);
    return __ycxx::__detail::__fpm::__fp_infinity<_Tp>(true);
  default:
    return _Tp(__v.exp + __ycxx::__detail::__fpm::__wide_bitlen(__v.__sig) - 1);
  }
}

// ---- neighbours ---------------------------------------------------------------------------------
// The next representable value from finite or infinite x towards +inf (up) or -inf.
template <class _Tp>
constexpr _Tp __fp_step(_Tp __x, bool __up) noexcept {
  using _Lp = __fp_layout<_Tp>;
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  if (__v.kind == __fp_kind::__inf) {
    if (__v.__neg == __up) { // towards zero: the largest finite value
      __wide<2> m = __ycxx::__detail::__fpm::__wide_from<2>(1);
      m = __ycxx::__detail::__fpm::__wide_shl(m, _Lp::p);
      __wide<2> __one = __ycxx::__detail::__fpm::__wide_from<2>(1);
      __ycxx::__detail::__fpm::__wide_sub(m, __one);
      return __ycxx::__detail::__fpm::__fp_round<_Tp>(__v.__neg, m, _Lp::__emax - (_Lp::p - 1)).value;
    }
    return __x;
  }
  if (__v.kind == __fp_kind::zero) return __ycxx::__detail::__fpm::__fp_round<_Tp>(!__up, __ycxx::__detail::__fpm::__wide_from<2>(1), _Lp::__qmin).value;
  __wide<2> m = __v.__sig;
  int e = __v.exp;
  if (__up != __v.__neg) { // magnitude grows
    __ycxx::__detail::__fpm::__wide_add_small(m, 1);
  } else if (e > _Lp::__qmin && __ycxx::__detail::__fpm::__wide_bitlen(m) == _Lp::p && __ycxx::__detail::__fpm::__wide_ctz(m) == _Lp::p - 1) {
    // A power of two above the subnormal range: the next smaller magnitude has a finer ulp.
    m = __ycxx::__detail::__fpm::__wide_shl(m, 1);
    __wide<2> __one = __ycxx::__detail::__fpm::__wide_from<2>(1);
    __ycxx::__detail::__fpm::__wide_sub(m, __one);
    --e;
  } else {
    __wide<2> __one = __ycxx::__detail::__fpm::__wide_from<2>(1);
    __ycxx::__detail::__fpm::__wide_sub(m, __one);
  }
  return __ycxx::__detail::__fpm::__fp_round<_Tp>(__v.__neg, m, e).value; // exact, or the overflow to infinity
}

template <class _Tp>
constexpr _Tp __fp_nextup(_Tp __x) noexcept {
  if (__ycxx::__detail::__fpm::__fp_isnan(__x)) return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  return __ycxx::__detail::__fpm::__fp_step(__x, true);
}
template <class _Tp>
constexpr _Tp __fp_nextdown(_Tp __x) noexcept {
  if (__ycxx::__detail::__fpm::__fp_isnan(__x)) return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  return __ycxx::__detail::__fpm::__fp_step(__x, false);
}
// nextafter / nexttoward: `__dir` is the sign of (y - x) (0: equal), y already compared in the
// right type. F.10.8.3: overflow for a finite x with an infinite result, underflow for a
// subnormal or zero result.
template <class _Tp>
constexpr _Tp __fp_next_toward(_Tp __x, int __dir) noexcept {
  const _Tp r = __ycxx::__detail::__fpm::__fp_step(__x, __dir > 0);
  if (__builtin_isinf(r) && !__builtin_isinf(__x))
    __ycxx::__detail::__fpm::__fp_report(__fe_overflow | __fe_inexact);
  else if (!__builtin_isnormal(r))
    __ycxx::__detail::__fpm::__fp_report(__fe_underflow | __fe_inexact);
  return r;
}
template <class _Tp>
constexpr _Tp __fp_nextafter(_Tp __x, _Tp y) noexcept {
  if (__ycxx::__detail::__fpm::__fp_isnan(__x) || __ycxx::__detail::__fpm::__fp_isnan(y)) return __ycxx::__detail::__fpm::__fp_nan_operands(__x, y);
  if (__x == y) return y;
  return __ycxx::__detail::__fpm::__fp_next_toward(__x, y > __x ? 1 : -1);
}
template <class _Tp>
constexpr _Tp __fp_nexttoward(_Tp __x, long double y) noexcept {
  if (__ycxx::__detail::__fpm::__fp_isnan(__x)) return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  if (__ycxx::__detail::__fpm::__fp_isnan(y)) {
    if (__ycxx::__detail::__fpm::__fp_issignaling(y)) __ycxx::__detail::__fpm::__fp_report(__fe_invalid);
    return __ycxx::__detail::__fpm::__fp_quiet_nan<_Tp>();
  }
  const long double __lx = static_cast<long double>(__x); // exact: T is a standard type
  if (__lx == y) return static_cast<_Tp>(y);
  return __ycxx::__detail::__fpm::__fp_next_toward(__x, y > __lx ? 1 : -1);
}

// ---- min / max -------------------------------------------------------------------------------
// num: C's fmax/fmin and fmaximum_num/fminimum_num (a NaN operand is ignored); otherwise
// fmaximum/fminimum (NaN wins). All order -0 below +0 (allowed for fmax/fmin).
template <class _Tp>
constexpr _Tp __fp_minmax(_Tp __x, _Tp y, bool max, bool num) noexcept {
  const bool __nx = __ycxx::__detail::__fpm::__fp_isnan(__x), __ny = __ycxx::__detail::__fpm::__fp_isnan(y);
  if (__nx || __ny) {
    if (__ycxx::__detail::__fpm::__fp_issignaling(__x) || __ycxx::__detail::__fpm::__fp_issignaling(y))
      __ycxx::__detail::__fpm::__fp_report(__fe_invalid);
    if (num && !(__nx && __ny)) return __nx ? y : __x;
    return __ycxx::__detail::__fpm::__fp_quiet_nan<_Tp>(); // the payload is not preserved
  }
  if (__x == y) { // equal values, or zeros of either sign
    const bool __sx = __ycxx::__detail::__fpm::__fp_signbit(__x);
    return (__sx == max) ? y : __x;
  }
  return (__x < y) == max ? y : __x;
}

template <class _Tp>
constexpr _Tp __fp_fdim(_Tp __x, _Tp y) noexcept {
  if (__ycxx::__detail::__fpm::__fp_isnan(__x) || __ycxx::__detail::__fpm::__fp_isnan(y)) return __ycxx::__detail::__fpm::__fp_nan_operands(__x, y);
  if (!(__x > y)) return _Tp(0);
  const _Tp r = __x - y;
  if (__builtin_isinf(r) && !__builtin_isinf(__x) && !__builtin_isinf(y)) __ycxx::__detail::__fpm::__fp_report(__fe_overflow | __fe_inexact);
  return r;
}

// ---- fmod, remainder, remquo ------------------------------------------------------------------
// |x| mod |y| for finite nonzero x, y, with the low three bits of the quotient.
struct __fp_mod_result {
  __wide<2> rem; // remainder * 2^exp
  int exp;
  unsigned __quo; // the low bits of the integral quotient
};
template <class _Tp>
constexpr __fp_mod_result __fp_mod(const __fp_value& __x, const __fp_value& y) noexcept {
  // |x| = mx * 2^ex, |y| = my * 2^ey. With d = ex - ey >= 0: rem = (mx * 2^d) mod my, at 2^ey.
  // With d < 0: scale my up instead (exact; |x| >= |y| was checked by the caller or not).
  __wide<2> __mx = __x.__sig, __my = y.__sig;
  int e = y.exp;
  int d = __x.exp - y.exp;
  if (d < 0) {
    // Bring both to the exponent of x: my * 2^-d at 2^ex.
    const int __len = __ycxx::__detail::__fpm::__wide_bitlen(__my);
    if (__len - d > 127) return {__mx, __x.exp, 0}; // |y| is far larger than |x|
    __my = __ycxx::__detail::__fpm::__wide_shl(__my, -d);
    e = __x.exp;
    d = 0;
  }
  unsigned __quo = 0;
  // r = mx mod my, then repeatedly r = (r * 2^k) mod my.
  __wide<2> r = __mx;
  {
    // Initial reduction: mx < 2^128, so do it bitwise from the top.
    unsigned __q = 0;
    if (__ycxx::__detail::__fpm::__wide_cmp(r, __my) >= 0) {
      const int shift = __ycxx::__detail::__fpm::__wide_bitlen(r) - __ycxx::__detail::__fpm::__wide_bitlen(__my);
      for (int s = shift; s >= 0; --s) {
        const __wide<2> t = __ycxx::__detail::__fpm::__wide_shl(__my, s);
        __q <<= 1;
        if (__ycxx::__detail::__fpm::__wide_cmp(r, t) >= 0) {
          __ycxx::__detail::__fpm::__wide_sub(r, t);
          __q |= 1;
        }
      }
    }
    __quo = __q;
  }
  if (__my.__w[1] == 0) {
    // 64-bit modulus: 63 bits at a time with a 128 / 64 division.
    const __y_u64 m = __my.__w[0];
    __y_u64 __rr = r.__w[0];
    while (d > 0) {
      const int k = d > 63 ? 63 : d;
      const __y_u64 __hi = __rr >> (64 - k), __lo = __rr << k;
      __y_u64 rem = 0;
      const __y_u64 __q = __ycxx::__detail::__fpm::__div128(__hi, __lo, m, rem);
      __quo = k >= 3 ? static_cast<unsigned>(__q & 7) : static_cast<unsigned>(((__quo << k) | __q) & 7);
      __rr = rem;
      d -= k;
    }
    r = __ycxx::__detail::__fpm::__wide_from<2>(__rr);
  } else {
    while (d > 0) { // one bit at a time
      const bool top = __ycxx::__detail::__fpm::__wide_bit(r, 127);
      r = __ycxx::__detail::__fpm::__wide_shl(r, 1);
      __quo <<= 1;
      if (top || __ycxx::__detail::__fpm::__wide_cmp(r, __my) >= 0) {
        __ycxx::__detail::__fpm::__wide_sub(r, __my);
        __quo |= 1;
      }
      --d;
    }
  }
  return {r, e, __quo & 7};
}

template <class _Tp>
constexpr _Tp __fp_fmod(_Tp __x, _Tp y) noexcept {
  if (__ycxx::__detail::__fpm::__fp_isnan(__x) || __ycxx::__detail::__fpm::__fp_isnan(y)) return __ycxx::__detail::__fpm::__fp_nan_operands(__x, y);
  const __fp_value __vx = __ycxx::__detail::__fpm::__fp_decode(__x), __vy = __ycxx::__detail::__fpm::__fp_decode(y);
  if (__vx.kind == __fp_kind::__inf || __vy.kind == __fp_kind::zero) return __ycxx::__detail::__fpm::__fp_invalid<_Tp>();
  if (__vx.kind == __fp_kind::zero || __vy.kind == __fp_kind::__inf) return __x;
  if (__ycxx::__detail::__fpm::__fp_abs(__x) < __ycxx::__detail::__fpm::__fp_abs(y)) return __x;
  const __fp_mod_result r = __ycxx::__detail::__fpm::__fp_mod<_Tp>(__vx, __vy);
  return __ycxx::__detail::__fpm::__fp_round<_Tp>(__vx.__neg, r.rem, r.exp).value; // exact
}

template <class _Tp>
constexpr _Tp __fp_remquo(_Tp __x, _Tp y, int* __quo) noexcept {
  *__quo = 0;
  if (__ycxx::__detail::__fpm::__fp_isnan(__x) || __ycxx::__detail::__fpm::__fp_isnan(y)) return __ycxx::__detail::__fpm::__fp_nan_operands(__x, y);
  const __fp_value __vx = __ycxx::__detail::__fpm::__fp_decode(__x), __vy = __ycxx::__detail::__fpm::__fp_decode(y);
  if (__vx.kind == __fp_kind::__inf || __vy.kind == __fp_kind::zero) return __ycxx::__detail::__fpm::__fp_invalid<_Tp>();
  if (__vx.kind == __fp_kind::zero || __vy.kind == __fp_kind::__inf) return __x;
  __fp_mod_result r{__vx.__sig, __vx.exp, 0};
  const bool __small = __ycxx::__detail::__fpm::__fp_abs(__x) < __ycxx::__detail::__fpm::__fp_abs(y);
  if (!__small) r = __ycxx::__detail::__fpm::__fp_mod<_Tp>(__vx, __vy);
  // Round the quotient to nearest (ties to even): compare 2 * rem with |y|, at a common scale.
  // rem is at 2^r.exp and |y| = sig * 2^vy.exp with r.exp <= vy.exp (or rem = |x| < |y|).
  __wide<4> __two_rem = __ycxx::__detail::__fpm::__wide_resize<4>(r.rem);
  __wide<4> __ym = __ycxx::__detail::__fpm::__wide_resize<4>(__vy.__sig);
  int __er = r.exp + 1, __ey = __vy.exp; // 2 * rem = rem * 2^(exp + 1)
  if (__er > __ey)
    __two_rem = __ycxx::__detail::__fpm::__wide_shl(__two_rem, __er - __ey);
  else if (__ey - __er < 200)
    __ym = __ycxx::__detail::__fpm::__wide_shl(__ym, __ey - __er);
  else
    __ym = __ycxx::__detail::__fpm::__wide_shl(__ycxx::__detail::__fpm::__wide_from<4>(1), 255); // |y| >> 2 rem
  const int c = __ycxx::__detail::__fpm::__wide_cmp(__two_rem, __ym);
  unsigned __q = r.__quo;
  _Tp rem = __ycxx::__detail::__fpm::__fp_round<_Tp>(false, r.rem, r.exp).value; // exact, >= 0
  if (c > 0 || (c == 0 && (__q & 1))) {
    rem = rem - __ycxx::__detail::__fpm::__fp_abs(y); // exact (Sterbenz-like: |y|/2 <= rem < |y|)
    ++__q;
  }
  if (__vx.__neg) rem = -rem;
  const int __sq = static_cast<int>(__q & 7);
  *__quo = __vx.__neg != __vy.__neg ? -__sq : __sq;
  return rem == 0 ? __ycxx::__detail::__fpm::__fp_zero<_Tp>(__vx.__neg) : rem;
}
template <class _Tp>
constexpr _Tp __fp_remainder(_Tp __x, _Tp y) noexcept {
  int __q = 0;
  return __ycxx::__detail::__fpm::__fp_remquo(__x, y, &__q);
}

// ---- integer roots ----------------------------------------------------------------------------
// floor(sqrt(n)); `__exact` tells whether n is a perfect square.
template <int _Np>
constexpr __wide<_Np> __wide_isqrt(__wide<_Np> n, bool& __exact) noexcept {
  __wide<_Np> __res;
  int top = __ycxx::__detail::__fpm::__wide_bitlen(n);
  if (top == 0) {
    __exact = true;
    return __res;
  }
  int b = (top - 1) & ~1; // highest even bit position <= top bit
  for (; b >= 0; b -= 2) {
    __wide<_Np> t = __res;
    __wide<_Np> __bit;
    __ycxx::__detail::__fpm::__wide_set_bit(__bit, b);
    __ycxx::__detail::__fpm::__wide_add(t, __bit); // res + bit
    __res = __ycxx::__detail::__fpm::__wide_shr(__res, 1);
    if (__ycxx::__detail::__fpm::__wide_cmp(n, t) >= 0) {
      __ycxx::__detail::__fpm::__wide_sub(n, t);
      __ycxx::__detail::__fpm::__wide_add(__res, __bit);
    }
  }
  __exact = __ycxx::__detail::__fpm::__wide_is_zero(n);
  return __res;
}

// Rounds sqrt(S * 2^e + tail) for an exact integer S (tail nonzero iff `__sticky`, below 2^e).
template <class _Tp, int _Np>
constexpr __fp_result<_Tp> __fp_sqrt_round(__wide<_Np> s, int e, bool __sticky) noexcept {
  using _Lp = __fp_layout<_Tp>;
  // Make e even, then give S at least 2 (p + 2) bits (or drop extra low bits into sticky).
  if (e & 1) {
    s = __ycxx::__detail::__fpm::__wide_shl(s, 1);
    --e;
  }
  const int __want = 2 * (_Lp::p + 2);
  int __len = __ycxx::__detail::__fpm::__wide_bitlen(s);
  if (__len < __want) {
    const int k = (__want - __len + 1) / 2;
    s = __ycxx::__detail::__fpm::__wide_shl(s, 2 * k);
    e -= 2 * k;
  } else if (__len > __want + 1) {
    const int k = (__len - __want) / 2;
    s = __ycxx::__detail::__fpm::__wide_shr(s, 2 * k, __sticky);
    e += 2 * k;
  }
  bool __exact = true;
  const __wide<_Np> r = __ycxx::__detail::__fpm::__wide_isqrt(s, __exact);
  return __ycxx::__detail::__fpm::__fp_round<_Tp>(false, r, e / 2, __sticky || !__exact);
}

template <class _Tp>
constexpr _Tp __fp_sqrt(_Tp __x) noexcept {
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  if (__v.kind == __fp_kind::nan) return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  if (__v.kind == __fp_kind::zero) return __x;
  if (__v.__neg) return __ycxx::__detail::__fpm::__fp_invalid<_Tp>();
  if (__v.kind == __fp_kind::__inf) return __x;
  return __ycxx::__detail::__fpm::__fp_finish(__ycxx::__detail::__fpm::__fp_sqrt_round<_Tp>(__ycxx::__detail::__fpm::__wide_resize<4>(__v.__sig), __v.exp, false));
}

template <class _Tp>
constexpr _Tp __fp_cbrt(_Tp __x) noexcept {
  using _Lp = __fp_layout<_Tp>;
  const __fp_value __v = __ycxx::__detail::__fpm::__fp_decode(__x);
  if (__v.kind == __fp_kind::nan) return __ycxx::__detail::__fpm::__fp_nan_operand(__x);
  if (__v.kind != __fp_kind::__finite) return __x;
  // n = sig * 2^(3k) with at least 3 (p + 2) bits, exponent a multiple of 3.
  __wide<6> n = __ycxx::__detail::__fpm::__wide_resize<6>(__v.__sig);
  int e = __v.exp;
  int __len = __ycxx::__detail::__fpm::__wide_bitlen(n);
  int __y_sh = 3 * (_Lp::p + 2) - __len;
  if (__y_sh < 0) __y_sh = 0;
  __y_sh += ((e - __y_sh) % 3 + 3) % 3; // make e - sh a multiple of 3
  n = __ycxx::__detail::__fpm::__wide_shl(n, __y_sh);
  e -= __y_sh;
  __len = __ycxx::__detail::__fpm::__wide_bitlen(n);
  // Digit by digit: r, r^2, r^3 kept exactly; compare r^3 with the top 3i bits of n.
  __wide<6> r, __r2, __r3;
  const int digits = (__len + 2) / 3;
  for (int i = digits - 1; i >= 0; --i) {
    // Candidate (2r + 1): (2r+1)^3 = 8 r^3 + 12 r^2 + 6 r + 1, (2r+1)^2 = 4 r^2 + 4 r + 1.
    __wide<6> __c3 = __ycxx::__detail::__fpm::__wide_shl(__r3, 3);
    __wide<6> t = __r2;
    __ycxx::__detail::__fpm::__wide_mul_small(t, 12);
    __ycxx::__detail::__fpm::__wide_add(__c3, t);
    t = r;
    __ycxx::__detail::__fpm::__wide_mul_small(t, 6);
    __ycxx::__detail::__fpm::__wide_add(__c3, t);
    __ycxx::__detail::__fpm::__wide_add_small(__c3, 1);
    const __wide<6> top = __ycxx::__detail::__fpm::__wide_shr(n, 3 * i);
    __wide<6> __two_r = __ycxx::__detail::__fpm::__wide_shl(r, 1);
    if (__ycxx::__detail::__fpm::__wide_cmp(__c3, top) <= 0) {
      __wide<6> __c2 = __ycxx::__detail::__fpm::__wide_shl(__r2, 2);
      __wide<6> __fr = __ycxx::__detail::__fpm::__wide_shl(r, 2);
      __ycxx::__detail::__fpm::__wide_add(__c2, __fr);
      __ycxx::__detail::__fpm::__wide_add_small(__c2, 1);
      r = __two_r;
      __ycxx::__detail::__fpm::__wide_add_small(r, 1);
      __r2 = __c2;
      __r3 = __c3;
    } else {
      r = __two_r;
      __r2 = __ycxx::__detail::__fpm::__wide_shl(__r2, 2);
      __r3 = __ycxx::__detail::__fpm::__wide_shl(__r3, 3);
    }
  }
  const bool __exact = __ycxx::__detail::__fpm::__wide_cmp(__r3, n) == 0;
  return __ycxx::__detail::__fpm::__fp_round<_Tp>(__v.__neg, r, e / 3, !__exact).value; // never over/underflows
}

// ---- hypot ---------------------------------------------------------------------------------------
// sqrt(sum of squares) of finite values, correctly rounded.
template <class _Tp, int _Kp>
constexpr _Tp __fp_hypot_finite(const _Tp (&in)[_Kp]) noexcept {
  using _Lp = __fp_layout<_Tp>;
  __fp_value __v[_Kp];
  int big = -1;
  long __top_big = 0;
  for (int i = 0; i < _Kp; ++i) {
    __v[i] = __ycxx::__detail::__fpm::__fp_decode(in[i]);
    if (__v[i].kind != __fp_kind::__finite) continue;
    const long top = long(__v[i].exp) + __ycxx::__detail::__fpm::__wide_bitlen(__v[i].__sig);
    if (big < 0 || top > __top_big) {
      big = i;
      __top_big = top;
    }
  }
  if (big < 0) return _Tp(0);
  // Terms more than p + 3 binades below the largest only make the result inexact.
  bool __sticky = false;
  int __lsb = __v[big].exp;
  for (int i = 0; i < _Kp; ++i) {
    if (__v[i].kind != __fp_kind::__finite || i == big) continue;
    const long top = long(__v[i].exp) + __ycxx::__detail::__fpm::__wide_bitlen(__v[i].__sig);
    if (__top_big - top > _Lp::p + 3) {
      __v[i].kind = __fp_kind::zero;
      __sticky = true;
    } else if (__v[i].exp < __lsb) {
      __lsb = __v[i].exp;
    }
  }
  __wide<8> sum;
  for (int i = 0; i < _Kp; ++i) {
    if (__v[i].kind != __fp_kind::__finite) continue;
    const __wide<4> m = __ycxx::__detail::__fpm::__wide_shl(__ycxx::__detail::__fpm::__wide_resize<4>(__v[i].__sig), __v[i].exp - __lsb);
    __ycxx::__detail::__fpm::__wide_add(sum, __ycxx::__detail::__fpm::__wide_mul(m, m));
  }
  return __ycxx::__detail::__fpm::__fp_finish(__ycxx::__detail::__fpm::__fp_sqrt_round<_Tp>(sum, 2 * __lsb, __sticky));
}

template <class _Tp>
constexpr _Tp __fp_hypot(_Tp __x, _Tp y) noexcept {
  // F.10.4.3: hypot(+-inf, y) is +inf even for a NaN y.
  if (__builtin_isinf(__x) || __builtin_isinf(y)) {
    if (__ycxx::__detail::__fpm::__fp_issignaling(__x) || __ycxx::__detail::__fpm::__fp_issignaling(y)) __ycxx::__detail::__fpm::__fp_report(__fe_invalid);
    return __ycxx::__detail::__fpm::__fp_infinity<_Tp>(false);
  }
  if (__ycxx::__detail::__fpm::__fp_isnan(__x) || __ycxx::__detail::__fpm::__fp_isnan(y)) return __ycxx::__detail::__fpm::__fp_nan_operands(__x, y);
  const _Tp in[2] = {__x, y};
  return __ycxx::__detail::__fpm::__fp_hypot_finite(in);
}
template <class _Tp>
constexpr _Tp __fp_hypot3(_Tp __x, _Tp y, _Tp __z) noexcept {
  if (__builtin_isinf(__x) || __builtin_isinf(y) || __builtin_isinf(__z)) {
    if (__ycxx::__detail::__fpm::__fp_issignaling(__x) || __ycxx::__detail::__fpm::__fp_issignaling(y) || __ycxx::__detail::__fpm::__fp_issignaling(__z))
      __ycxx::__detail::__fpm::__fp_report(__fe_invalid);
    return __ycxx::__detail::__fpm::__fp_infinity<_Tp>(false);
  }
  if (__ycxx::__detail::__fpm::__fp_isnan(__x) || __ycxx::__detail::__fpm::__fp_isnan(y)) return __ycxx::__detail::__fpm::__fp_nan_operands(__x, y);
  if (__ycxx::__detail::__fpm::__fp_isnan(__z)) return __ycxx::__detail::__fpm::__fp_nan_operand(__z);
  const _Tp in[3] = {__x, y, __z};
  return __ycxx::__detail::__fpm::__fp_hypot_finite(in);
}

// ---- fma -----------------------------------------------------------------------------------------
template <class _Tp>
constexpr _Tp __fp_fma(_Tp __x, _Tp y, _Tp __z) noexcept {
  using _Lp = __fp_layout<_Tp>;
  if (__ycxx::__detail::__fpm::__fp_isnan(__x) || __ycxx::__detail::__fpm::__fp_isnan(y) || __ycxx::__detail::__fpm::__fp_isnan(__z)) {
    if (__ycxx::__detail::__fpm::__fp_issignaling(__x) || __ycxx::__detail::__fpm::__fp_issignaling(y) || __ycxx::__detail::__fpm::__fp_issignaling(__z))
      __ycxx::__detail::__fpm::__fp_report(__fe_invalid);
    // F.10.10.1: fma(inf, 0, NaN) may raise invalid; libycxx does not.
    return __ycxx::__detail::__fpm::__fp_isnan(__x) ? __ycxx::__detail::__fpm::__fp_nan_operand(__x)
         : __ycxx::__detail::__fpm::__fp_isnan(y) ? __ycxx::__detail::__fpm::__fp_nan_operand(y)
                                          : __ycxx::__detail::__fpm::__fp_nan_operand(__z);
  }
  const __fp_value __vx = __ycxx::__detail::__fpm::__fp_decode(__x), __vy = __ycxx::__detail::__fpm::__fp_decode(y), __vz = __ycxx::__detail::__fpm::__fp_decode(__z);
  const bool __pneg = __vx.__neg != __vy.__neg;
  if (__vx.kind == __fp_kind::__inf || __vy.kind == __fp_kind::__inf) {
    if (__vx.kind == __fp_kind::zero || __vy.kind == __fp_kind::zero) return __ycxx::__detail::__fpm::__fp_invalid<_Tp>();
    if (__vz.kind == __fp_kind::__inf && __vz.__neg != __pneg) return __ycxx::__detail::__fpm::__fp_invalid<_Tp>();
    return __ycxx::__detail::__fpm::__fp_infinity<_Tp>(__pneg);
  }
  if (__vz.kind == __fp_kind::__inf) return __z;
  if (__vx.kind == __fp_kind::zero || __vy.kind == __fp_kind::zero) {
    if (__vz.kind == __fp_kind::zero) return __ycxx::__detail::__fpm::__fp_zero<_Tp>(__pneg && __vz.__neg);
    return __z;
  }
  // Exact product P = mx * my * 2^(ex + ey).
  __wide<4> p = __ycxx::__detail::__fpm::__wide_mul(__vx.__sig, __vy.__sig);
  int __ep = __vx.exp + __vy.exp;
  if (__vz.kind == __fp_kind::zero) return __ycxx::__detail::__fpm::__fp_finish(__ycxx::__detail::__fpm::__fp_round<_Tp>(__pneg, p, __ep));
  __wide<4> __zm = __ycxx::__detail::__fpm::__wide_resize<4>(__vz.__sig);
  int __ez = __vz.exp;
  const long __tp = long(__ep) + __ycxx::__detail::__fpm::__wide_bitlen(p), __tz = long(__ez) + __ycxx::__detail::__fpm::__wide_bitlen(__zm);
  // An operand far below the other only decides the direction of an inexact rounding: replace
  // it by a single bit well below the rounding position (same rounded result and flags).
  constexpr int __far = 2 * _Lp::p + 8;
  if (__tz - __tp > __far) {
    p = __ycxx::__detail::__fpm::__wide_from<4>(1);
    __ep = static_cast<int>(__tz - __far);
  } else if (__tp - __tz > __far) {
    __zm = __ycxx::__detail::__fpm::__wide_from<4>(1);
    __ez = static_cast<int>(__tp - __far);
  }
  const int base = __ep < __ez ? __ep : __ez;
  __wide<8> a = __ycxx::__detail::__fpm::__wide_shl(__ycxx::__detail::__fpm::__wide_resize<8>(p), __ep - base);
  __wide<8> b = __ycxx::__detail::__fpm::__wide_shl(__ycxx::__detail::__fpm::__wide_resize<8>(__zm), __ez - base);
  bool __neg = __pneg;
  if (__pneg == __vz.__neg) {
    __ycxx::__detail::__fpm::__wide_add(a, b);
  } else if (__ycxx::__detail::__fpm::__wide_cmp(a, b) >= 0) {
    __ycxx::__detail::__fpm::__wide_sub(a, b);
  } else {
    __ycxx::__detail::__fpm::__wide_sub(b, a);
    a = b;
    __neg = __vz.__neg;
  }
  if (__ycxx::__detail::__fpm::__wide_is_zero(a)) return _Tp(0); // exact cancellation: +0
  return __ycxx::__detail::__fpm::__fp_finish(__ycxx::__detail::__fpm::__fp_round<_Tp>(__neg, a, base));
}

// ---- lerp (P0811) ---------------------------------------------------------------------------------
template <class _Tp>
constexpr _Tp __fp_lerp(_Tp a, _Tp b, _Tp t) noexcept {
  if ((a <= _Tp(0) && b >= _Tp(0)) || (a >= _Tp(0) && b <= _Tp(0))) {
    // Exact at t == 0 and t == 1; an infinite t would meet 0 * inf here.
    if (__builtin_isinf(t)) return a + t * (b - a);
    return t * b + (_Tp(1) - t) * a;
  }
  if (t == _Tp(1)) return b;
  const _Tp __x = a + t * (b - a);
  // Monotonic and exact at t == 1: clamp to b on the far side of it.
  if ((t > _Tp(1)) == (b > a)) return b < __x ? __x : b;
  return __x < b ? __x : b;
}

}} // namespace __ycxx::__detail::__fpm
