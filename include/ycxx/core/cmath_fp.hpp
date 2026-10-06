// libycxx core: building blocks of the constexpr ("soft") floating-point math behind <cmath>.
//
// - wide<N>: an N x 64-bit unsigned integer (little-endian limbs) with the few operations the
//   exact algorithms need.
// - fp_layout<T> / fp_decode / fp_encode: the bits of any binary floating-point format the
//   compilers offer (binary16/32/64/128, bfloat16, x87 extended), read and written with
//   __builtin_bit_cast, so they work during constant evaluation.
// - fp_round<T>: correct rounding (to nearest, ties to even) of an exact or sticky-tailed value
//   m * 2^e, with the IEEE exception flags it raises (overflow, underflow, inexact).
// - fp_raise: reports flags. It is deliberately not constexpr: reached during constant
//   evaluation, the call makes the evaluation non-constant ([library.c]/3: a C library call
//   that raises a floating-point exception other than FE_INEXACT is a non-constant library
//   call); at run time it raises the flags with volatile arithmetic.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/bit.hpp>
#include <ycxx/core/meta_base.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail::__fpm {

using __y_u64 = unsigned long long;

// ---- 64 x 64 -> 128-bit multiplication and 128 / 64 division ------------------------------
struct __u64pair {
  __y_u64 __hi, __lo;
};
template <class _Up = __ycxx::__detail::__uint128>
constexpr __u64pair __mul64(__y_u64 a, __y_u64 b) noexcept {
  if constexpr (__cfg::__has_int128) {
    const _Up p = _Up(a) * b;
    return {static_cast<__y_u64>(p >> 64), static_cast<__y_u64>(p)};
  } else {
    const __y_u64 __a0 = a & 0xffffffffu, __a1 = a >> 32, __b0 = b & 0xffffffffu, __b1 = b >> 32;
    const __y_u64 __p00 = __a0 * __b0, __p01 = __a0 * __b1, __p10 = __a1 * __b0, __p11 = __a1 * __b1;
    const __y_u64 __mid = (__p00 >> 32) + (__p01 & 0xffffffffu) + (__p10 & 0xffffffffu);
    return {__p11 + (__p01 >> 32) + (__p10 >> 32) + (__mid >> 32), (__mid << 32) | (__p00 & 0xffffffffu)};
  }
}
// (hi * 2^64 + lo) / d for hi < d; returns the quotient, stores the remainder.
template <class _Up = __ycxx::__detail::__uint128>
constexpr __y_u64 __div128(__y_u64 __hi, __y_u64 __lo, __y_u64 d, __y_u64& rem) noexcept {
  if constexpr (__cfg::__has_int128) {
    const _Up n = (_Up(__hi) << 64) | __lo;
    rem = static_cast<__y_u64>(n % d);
    return static_cast<__y_u64>(n / d);
  } else {
    __y_u64 __q = 0;
    for (int i = 63; i >= 0; --i) {
      const bool top = __hi >> 63;
      __hi = (__hi << 1) | ((__lo >> i) & 1);
      __q <<= 1;
      if (top || __hi >= d) {
        __hi -= d;
        __q |= 1;
      }
    }
    rem = __hi;
    return __q;
  }
}

constexpr int __clz64(__y_u64 __x) noexcept { return __x == 0 ? 64 : __builtin_clzll(__x); }
constexpr int __ctz64(__y_u64 __x) noexcept { return __x == 0 ? 64 : __builtin_ctzll(__x); }

// ---- wide<N> --------------------------------------------------------------------------------
template <int _Np>
struct __wide {
  __y_u64 __w[_Np] = {};
};

template <int _Np>
constexpr __wide<_Np> __wide_from(__y_u64 __x) noexcept {
  __wide<_Np> r;
  r.__w[0] = __x;
  return r;
}
template <int _Mp, int _Np>
constexpr __wide<_Mp> __wide_resize(const __wide<_Np>& __x) noexcept { // zero-extends or truncates
  __wide<_Mp> r;
  for (int i = 0; i < _Mp && i < _Np; ++i) r.__w[i] = __x.__w[i];
  return r;
}
template <int _Np>
constexpr bool __wide_is_zero(const __wide<_Np>& __x) noexcept {
  for (int i = 0; i < _Np; ++i)
    if (__x.__w[i] != 0) return false;
  return true;
}
template <int _Np>
constexpr int __wide_bitlen(const __wide<_Np>& __x) noexcept {
  for (int i = _Np - 1; i >= 0; --i)
    if (__x.__w[i] != 0) return 64 * i + 64 - __ycxx::__detail::__fpm::__clz64(__x.__w[i]);
  return 0;
}
template <int _Np>
constexpr int __wide_ctz(const __wide<_Np>& __x) noexcept {
  for (int i = 0; i < _Np; ++i)
    if (__x.__w[i] != 0) return 64 * i + __ycxx::__detail::__fpm::__ctz64(__x.__w[i]);
  return 64 * _Np;
}
template <int _Np>
constexpr bool __wide_bit(const __wide<_Np>& __x, int i) noexcept {
  return i >= 0 && i < 64 * _Np && ((__x.__w[i / 64] >> (i % 64)) & 1);
}
template <int _Np>
constexpr void __wide_set_bit(__wide<_Np>& __x, int i) noexcept {
  __x.__w[i / 64] |= __y_u64(1) << (i % 64);
}
// Keeps the low `n` bits.
template <int _Np>
constexpr __wide<_Np> __wide_low_bits(__wide<_Np> __x, int n) noexcept {
  for (int i = 0; i < _Np; ++i) {
    const int __lo = 64 * i;
    if (n <= __lo)
      __x.__w[i] = 0;
    else if (n < __lo + 64)
      __x.__w[i] &= (__y_u64(1) << (n - __lo)) - 1;
  }
  return __x;
}
template <int _Np>
constexpr int __wide_cmp(const __wide<_Np>& a, const __wide<_Np>& b) noexcept {
  for (int i = _Np - 1; i >= 0; --i)
    if (a.__w[i] != b.__w[i]) return a.__w[i] < b.__w[i] ? -1 : 1;
  return 0;
}
template <int _Np>
constexpr __wide<_Np> __wide_shl(const __wide<_Np>& __x, int s) noexcept {
  __wide<_Np> r;
  if (s >= 64 * _Np) return r;
  const int __q = s / 64, b = s % 64;
  for (int i = _Np - 1; i >= __q; --i) {
    r.__w[i] = __x.__w[i - __q] << b;
    if (b != 0 && i - __q - 1 >= 0) r.__w[i] |= __x.__w[i - __q - 1] >> (64 - b);
  }
  return r;
}
// Shift right; `__sticky` is or-ed with "a nonzero bit was shifted out".
template <int _Np>
constexpr __wide<_Np> __wide_shr(const __wide<_Np>& __x, int s, bool& __sticky) noexcept {
  __wide<_Np> r;
  if (s <= 0) return s == 0 ? __x : __ycxx::__detail::__fpm::__wide_shl(__x, -s);
  if (s >= 64 * _Np) {
    __sticky = __sticky || !__ycxx::__detail::__fpm::__wide_is_zero(__x);
    return r;
  }
  const int __q = s / 64, b = s % 64;
  for (int i = 0; i < __q; ++i) __sticky = __sticky || __x.__w[i] != 0;
  if (b != 0) __sticky = __sticky || (__x.__w[__q] << (64 - b)) != 0;
  for (int i = 0; i + __q < _Np; ++i) {
    r.__w[i] = __x.__w[i + __q] >> b;
    if (b != 0 && i + __q + 1 < _Np) r.__w[i] |= __x.__w[i + __q + 1] << (64 - b);
  }
  return r;
}
template <int _Np>
constexpr __wide<_Np> __wide_shr(const __wide<_Np>& __x, int s) noexcept {
  bool __ignored = false;
  return __ycxx::__detail::__fpm::__wide_shr(__x, s, __ignored);
}
template <int _Np>
constexpr bool __wide_add(__wide<_Np>& a, const __wide<_Np>& b) noexcept { // a += b; returns the carry
  bool __carry = false;
  for (int i = 0; i < _Np; ++i) {
    const __y_u64 s = a.__w[i] + b.__w[i];
    const bool __c1 = s < a.__w[i];
    a.__w[i] = s + __carry;
    __carry = __c1 || (__carry && a.__w[i] == 0);
  }
  return __carry;
}
template <int _Np>
constexpr bool __wide_sub(__wide<_Np>& a, const __wide<_Np>& b) noexcept { // a -= b; returns the borrow
  bool __borrow = false;
  for (int i = 0; i < _Np; ++i) {
    const __y_u64 d = a.__w[i] - b.__w[i];
    const bool __b1 = a.__w[i] < b.__w[i];
    a.__w[i] = d - __borrow;
    __borrow = __b1 || (__borrow && d == 0);
  }
  return __borrow;
}
template <int _Np>
constexpr void __wide_add_small(__wide<_Np>& a, __y_u64 __v) noexcept {
  for (int i = 0; i < _Np && __v != 0; ++i) {
    a.__w[i] += __v;
    __v = a.__w[i] < __v ? 1 : 0;
  }
}
template <int _Np, int _Mp>
constexpr __wide<_Np + _Mp> __wide_mul(const __wide<_Np>& a, const __wide<_Mp>& b) noexcept {
  __wide<_Np + _Mp> r;
  for (int i = 0; i < _Np; ++i) {
    if (a.__w[i] == 0) continue;
    __y_u64 __carry = 0;
    for (int __j = 0; __j < _Mp; ++__j) {
      const __u64pair p = __ycxx::__detail::__fpm::__mul64(a.__w[i], b.__w[__j]);
      __y_u64 __lo = p.__lo + __carry;
      __y_u64 __hi = p.__hi + (__lo < __carry);
      const __y_u64 cur = r.__w[i + __j];
      __lo += cur;
      __hi += __lo < cur;
      r.__w[i + __j] = __lo;
      __carry = __hi;
    }
    r.__w[i + _Mp] = __carry;
  }
  return r;
}
template <int _Np>
constexpr __y_u64 __wide_mul_small(__wide<_Np>& a, __y_u64 m) noexcept { // a *= m; returns the overflow limb
  __y_u64 __carry = 0;
  for (int i = 0; i < _Np; ++i) {
    const __u64pair p = __ycxx::__detail::__fpm::__mul64(a.__w[i], m);
    const __y_u64 __lo = p.__lo + __carry;
    __carry = p.__hi + (__lo < __carry);
    a.__w[i] = __lo;
  }
  return __carry;
}
template <int _Np>
constexpr __y_u64 __wide_div_small(__wide<_Np>& a, __y_u64 d) noexcept { // a /= d; returns the remainder
  __y_u64 rem = 0;
  for (int i = _Np - 1; i >= 0; --i) a.__w[i] = __ycxx::__detail::__fpm::__div128(rem, a.__w[i], d, rem);
  return rem;
}

// ---- floating-point formats -----------------------------------------------------------------
enum : int { __fe_inexact = 1, __fe_underflow = 2, __fe_overflow = 4, __fe_divbyzero = 8, __fe_invalid = 16 };

// Not constexpr on purpose (see the file comment).
[[__gnu__::__noinline__, __gnu__::__cold__]] inline void __fp_raise(int flags) noexcept {
  volatile double big = 0x1p1000, __small = 0x1p-1000, zero = 0.0, __one = 1.0, __third = 3.0;
  if (flags & __fe_inexact) __third = __one / __third;
  if (flags & __fe_overflow) big = big * big;
  if (flags & __fe_underflow) __small = __small * __small;
  if (flags & __fe_divbyzero) __one = __one / zero;
  if (flags & __fe_invalid) zero = zero / zero;
  (void)big;
  (void)__small;
  (void)__one;
  (void)zero;
  (void)__third;
}
constexpr void __fp_report(int flags) noexcept {
  if ((flags & ~__fe_inexact) != 0) __ycxx::__detail::__fpm::__fp_raise(flags);
}

template <class _Tp>
struct __fp_layout {
  static constexpr int p = __ycxx::__detail::__fp_format<_Tp>.digits;       // significand bits
  static constexpr int __emax = __ycxx::__detail::__fp_format<_Tp>.__max_exp - 1; // largest normal: 2^emax
  static constexpr int __emin = __ycxx::__detail::__fp_format<_Tp>.__min_exp - 1; // smallest normal: 2^emin
  static constexpr bool __explicit_bit = p == 64 && __emax == 16383;    // x87 extended
  static constexpr int __ebits = __ycxx::__detail::__fpm::__ctz64(__y_u64(__emax) + 1) + 1;
  static constexpr int __fbits = __explicit_bit ? p : p - 1;            // fraction field width
  static constexpr int bits = 1 + __ebits + __fbits;
  static constexpr int __bias = __emax;
  static constexpr int __qmin = __emin - (p - 1); // exponent of the least significant bit of denorm_min
  static_assert(p > 1 && p <= 113 && bits <= 128 && bits <= 8 * int(sizeof(_Tp)), "libycxx: unsupported floating-point format");
};

template <class _Tp>
struct __fp_bytes {
  unsigned char b[sizeof(_Tp)];
};

template <class _Tp>
constexpr __wide<2> __fp_to_bits(_Tp __x) noexcept {
  const __fp_bytes<_Tp> __by = __builtin_bit_cast(__fp_bytes<_Tp>, __x);
  constexpr int n = (__fp_layout<_Tp>::bits + 7) / 8;
  constexpr bool __le = std::endian::native == std::endian::little;
  __wide<2> r;
  for (int i = 0; i < n; ++i) r.__w[i / 8] |= __y_u64(__by.b[__le ? i : int(sizeof(_Tp)) - 1 - i]) << (8 * (i % 8));
  return r;
}
template <class _Tp>
constexpr _Tp __fp_from_bits(const __wide<2>& __v) noexcept {
  __fp_bytes<_Tp> __by{};
  constexpr int n = (__fp_layout<_Tp>::bits + 7) / 8;
  constexpr bool __le = std::endian::native == std::endian::little;
  for (int i = 0; i < n; ++i)
    __by.b[__le ? i : int(sizeof(_Tp)) - 1 - i] = static_cast<unsigned char>(__v.__w[i / 8] >> (8 * (i % 8)));
  return __builtin_bit_cast(_Tp, __by);
}

enum class __fp_kind : unsigned char { zero, __finite, __inf, nan };

// A decoded value. For finite nonzero values: |x| = sig * 2^exp, 1 <= sig < 2^p (subnormals
// have fewer significant bits).
struct __fp_value {
  bool __neg = false;
  __fp_kind kind = __fp_kind::zero;
  bool __signaling = false;
  int exp = 0;
  __wide<2> __sig;
};

template <class _Tp>
constexpr __fp_value __fp_decode(_Tp __x) noexcept {
  using _Lp = __fp_layout<_Tp>;
  const __wide<2> bits = __ycxx::__detail::__fpm::__fp_to_bits(__x);
  __fp_value __v;
  __v.__neg = __ycxx::__detail::__fpm::__wide_bit(bits, _Lp::bits - 1);
  const int e = static_cast<int>(__ycxx::__detail::__fpm::__wide_shr(bits, _Lp::__fbits).__w[0] & ((__y_u64(1) << _Lp::__ebits) - 1));
  __wide<2> __frac = __ycxx::__detail::__fpm::__wide_low_bits(bits, _Lp::__fbits);
  if (e == (1 << _Lp::__ebits) - 1) {
    // Infinity or NaN. x87: the explicit bit is set in both; the fraction below it decides.
    const int top = _Lp::__explicit_bit ? _Lp::__fbits - 2 : _Lp::__fbits - 1; // the quiet bit
    __wide<2> __payload = __frac;
    if (_Lp::__explicit_bit) __payload.__w[0] &= ~(__y_u64(1) << 63);
    __v.kind = __ycxx::__detail::__fpm::__wide_is_zero(__payload) ? __fp_kind::__inf : __fp_kind::nan;
    __v.__signaling = __v.kind == __fp_kind::nan && !__ycxx::__detail::__fpm::__wide_bit(__frac, top);
    return __v;
  }
  if (e == 0 && __ycxx::__detail::__fpm::__wide_is_zero(__frac)) return __v;
  __v.kind = __fp_kind::__finite;
  __v.__sig = __frac;
  if (e == 0) {
    __v.exp = _Lp::__qmin;
  } else {
    if (!_Lp::__explicit_bit) __ycxx::__detail::__fpm::__wide_set_bit(__v.__sig, _Lp::p - 1);
    __v.exp = e - _Lp::__bias - (_Lp::p - 1);
  }
  return __v;
}

// Encodes a significand q < 2^p whose least significant bit has weight 2^lsb_exp, with
// lsb_exp >= qmin and (q < 2^(p-1) only if lsb_exp == qmin), or infinity / NaN.
template <class _Tp>
constexpr _Tp __fp_encode_finite(bool __neg, __wide<2> __q, int __lsb_exp) noexcept {
  using _Lp = __fp_layout<_Tp>;
  int e = 0;
  if (__ycxx::__detail::__fpm::__wide_bit(__q, _Lp::p - 1)) {
    e = __lsb_exp + (_Lp::p - 1) + _Lp::__bias;
    if (!_Lp::__explicit_bit) __q.__w[(_Lp::p - 1) / 64] &= ~(__y_u64(1) << ((_Lp::p - 1) % 64));
  }
  __wide<2> bits = __q;
  __wide<2> __ef = __ycxx::__detail::__fpm::__wide_shl(__ycxx::__detail::__fpm::__wide_from<2>(static_cast<__y_u64>(e)), _Lp::__fbits);
  __ycxx::__detail::__fpm::__wide_add(bits, __ef);
  if (__neg) __ycxx::__detail::__fpm::__wide_set_bit(bits, _Lp::bits - 1);
  return __ycxx::__detail::__fpm::__fp_from_bits<_Tp>(bits);
}
template <class _Tp>
constexpr _Tp __fp_make_special(bool nan, bool __neg) noexcept {
  using _Lp = __fp_layout<_Tp>;
  __wide<2> bits = __ycxx::__detail::__fpm::__wide_shl(__ycxx::__detail::__fpm::__wide_from<2>((__y_u64(1) << _Lp::__ebits) - 1), _Lp::__fbits);
  if (nan) __ycxx::__detail::__fpm::__wide_set_bit(bits, _Lp::__explicit_bit ? _Lp::__fbits - 2 : _Lp::__fbits - 1);
  if (_Lp::__explicit_bit) __ycxx::__detail::__fpm::__wide_set_bit(bits, 63);
  if (__neg) __ycxx::__detail::__fpm::__wide_set_bit(bits, _Lp::bits - 1);
  return __ycxx::__detail::__fpm::__fp_from_bits<_Tp>(bits);
}
// Built once per type (cheap during constant evaluation).
template <class _Tp>
inline constexpr _Tp __fp_inf_v[2] = {__ycxx::__detail::__fpm::__fp_make_special<_Tp>(false, false), __ycxx::__detail::__fpm::__fp_make_special<_Tp>(false, true)};
template <class _Tp>
inline constexpr _Tp __fp_qnan_v[2] = {__ycxx::__detail::__fpm::__fp_make_special<_Tp>(true, false), __ycxx::__detail::__fpm::__fp_make_special<_Tp>(true, true)};
template <class _Tp>
constexpr _Tp __fp_infinity(bool __neg) noexcept {
  return __ycxx::__detail::__fpm::__fp_inf_v<_Tp>[__neg];
}
template <class _Tp>
constexpr _Tp __fp_quiet_nan(bool __neg = false) noexcept {
  return __ycxx::__detail::__fpm::__fp_qnan_v<_Tp>[__neg];
}
template <class _Tp>
constexpr _Tp __fp_zero(bool __neg) noexcept {
  return __neg ? -_Tp(0) : _Tp(0);
}

template <class _Tp>
struct __fp_result {
  _Tp value;
  int flags;
};

// Rounds +-(mag * 2^exp + tail) to T, where the tail is in [0, 2^exp) and nonzero iff `__sticky`.
// With a tail, mag must have at least two bits more than the result keeps (callers pass p + 2
// or more significant bits), so that the tail only ever acts as a sticky bit.
template <class _Tp, int _Np>
constexpr __fp_result<_Tp> __fp_round(bool __neg, __wide<_Np> __mag, int exp, bool __sticky = false) noexcept {
  using _Lp = __fp_layout<_Tp>;
  const int __len = __ycxx::__detail::__fpm::__wide_bitlen(__mag);
  if (__len == 0) return {__ycxx::__detail::__fpm::__fp_zero<_Tp>(__neg), 0};
  const long top = long(exp) + __len - 1;
  if (top > _Lp::__emax) return {__ycxx::__detail::__fpm::__fp_infinity<_Tp>(__neg), __fe_overflow | __fe_inexact};
  long __lsb = top - (_Lp::p - 1);
  if (__lsb < _Lp::__qmin) __lsb = _Lp::__qmin;
  const long shift = __lsb - exp;
  __wide<2> __q;
  bool __inexact = __sticky;
  if (shift <= 0) {
    __q = __ycxx::__detail::__fpm::__wide_resize<2>(__ycxx::__detail::__fpm::__wide_shl(__mag, static_cast<int>(-shift)));
  } else {
    const bool round = __ycxx::__detail::__fpm::__wide_bit(__mag, static_cast<int>(shift - 1));
    bool __rest = __sticky;
    __wide<_Np> m = __ycxx::__detail::__fpm::__wide_shr(__mag, static_cast<int>(shift - 1), __rest);
    __q = __ycxx::__detail::__fpm::__wide_resize<2>(__ycxx::__detail::__fpm::__wide_shr(m, 1));
    __inexact = round || __rest;
    if (round && (__rest || (__q.__w[0] & 1))) {
      __ycxx::__detail::__fpm::__wide_add_small(__q, 1);
      if (__ycxx::__detail::__fpm::__wide_bit(__q, _Lp::p)) {
        __q = __ycxx::__detail::__fpm::__wide_shr(__q, 1);
        ++__lsb;
        if (__lsb + (_Lp::p - 1) > _Lp::__emax) return {__ycxx::__detail::__fpm::__fp_infinity<_Tp>(__neg), __fe_overflow | __fe_inexact};
      }
    }
  }
  int flags = __inexact ? __fe_inexact : 0;
  if (__inexact && !__ycxx::__detail::__fpm::__wide_bit(__q, _Lp::p - 1)) flags |= __fe_underflow;
  return {__ycxx::__detail::__fpm::__fp_encode_finite<_Tp>(__neg, __q, static_cast<int>(__lsb)), flags};
}

// The exact value of a finite T as sig * 2^exp, normalised so that the top bit of sig is bit
// p - 1 (subnormals get a smaller exponent).
template <class _Tp>
constexpr __fp_value __fp_normalize(__fp_value __v) noexcept {
  using _Lp = __fp_layout<_Tp>;
  if (__v.kind != __fp_kind::__finite) return __v;
  const int __len = __ycxx::__detail::__fpm::__wide_bitlen(__v.__sig);
  __v.__sig = __ycxx::__detail::__fpm::__wide_shl(__v.__sig, _Lp::p - __len);
  __v.exp -= _Lp::p - __len;
  return __v;
}

}} // namespace __ycxx::__detail::__fpm
