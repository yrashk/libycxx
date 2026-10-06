// libycxx core: <limits>
//
// Integer limits are computed from the type. Floating-point limits are computed from the
// three per-format facts in __ycxx::__detail::__fp_format (config.hpp): mantissa digits, min_exp and
// max_exp. Every derived value is a constant expression evaluated in the type's own arithmetic.
#pragma once

#include <ycxx/core/type_traits.hpp>

namespace [[__gnu__::__visibility__("hidden")]] std {

enum float_round_style {
  round_indeterminate = -1,
  round_toward_zero = 0,
  round_to_nearest = 1,
  round_toward_infinity = 2,
  round_toward_neg_infinity = 3
};

// [depr.numeric.limits.has.denorm] (Annex D)
enum [[deprecated("float_denorm_style is deprecated ([depr.numeric.limits.has.denorm])")]] float_denorm_style {
  denorm_indeterminate [[deprecated("denorm_indeterminate is deprecated ([depr.numeric.limits.has.denorm])")]] = -1,
  denorm_absent [[deprecated("denorm_absent is deprecated ([depr.numeric.limits.has.denorm])")]] = 0,
  denorm_present [[deprecated("denorm_present is deprecated ([depr.numeric.limits.has.denorm])")]] = 1
};

} // namespace std

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

// floor(e * log10(2)) for |e| < 2^31 (log10(2) is irrational, so no product is an integer).
consteval int __floor_log10_pow2(int e) {
  constexpr __y_int128 num = 301029995663981195LL; // log10(2) * 10^18
  constexpr __y_int128 den = 1000000000000000000LL;
  __y_int128 p = static_cast<__y_int128>(e) * num;
  return static_cast<int>(p >= 0 ? p / den : -((-p + den - 1) / den));
}

template <class _Tp>
consteval _Tp __pow2(int e) {
  _Tp r = 1;
  for (; e > 0; --e)
    r *= 2;
  for (; e < 0; ++e)
    r /= 2;
  return r;
}

// Defaults shared by every specialization ([numeric.limits.general]).
struct __limits_base {
  static constexpr bool is_specialized = false;
  static constexpr int digits = 0;
  static constexpr int digits10 = 0;
  static constexpr int max_digits10 = 0;
  static constexpr bool is_signed = false;
  static constexpr bool is_integer = false;
  static constexpr bool is_exact = false;
  static constexpr int radix = 0;
  static constexpr int min_exponent = 0;
  static constexpr int min_exponent10 = 0;
  static constexpr int max_exponent = 0;
  static constexpr int max_exponent10 = 0;
  static constexpr bool has_infinity = false;
  static constexpr bool has_quiet_NaN = false;
  static constexpr bool has_signaling_NaN = false;
  static constexpr bool is_iec559 = false;
  static constexpr bool is_bounded = false;
  static constexpr bool is_modulo = false;
  static constexpr bool traps = false;
  static constexpr bool tinyness_before = false;
  static constexpr std::float_round_style round_style = std::round_toward_zero;
  [[deprecated("has_denorm is deprecated ([depr.numeric.limits.has.denorm])")]]
  static constexpr std::float_denorm_style has_denorm = std::denorm_absent;
  [[deprecated("has_denorm_loss is deprecated ([depr.numeric.limits.has.denorm])")]]
  static constexpr bool has_denorm_loss = false;
};

template <class _Tp>
struct __int_limits : __limits_base {
  static constexpr bool is_specialized = true;
  static constexpr bool is_signed = __bitint_width<_Tp> != 0 ? __bitint_info<_Tp>::is_signed : is_signed_v<_Tp>;
  static constexpr int width = __bitint_width<_Tp> != 0 ? __bitint_width<_Tp> : static_cast<int>(sizeof(_Tp) * __CHAR_BIT__);
  static constexpr int digits = width - (is_signed ? 1 : 0);
  static constexpr int digits10 = __floor_log10_pow2(digits);
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr int radix = 2;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = !is_signed;
  // Only types not subject to integral promotion can trap on division (char types never do).
  static constexpr bool traps = __cfg::__integer_division_traps && __bitint_width<_Tp> == 0 &&
                                __is_signed_or_unsigned_integer<_Tp> && sizeof(_Tp) >= sizeof(int);

  // 2^digits - 1, built without overflow for any width (including _BitInt(N)).
  static constexpr _Tp __max_value = [] {
    _Tp r = 0;
    for (int i = 0; i < digits; ++i)
      r = static_cast<_Tp>(r * 2 + 1);
    return r;
  }();
  static constexpr _Tp __min_value = is_signed ? static_cast<_Tp>(-__max_value - 1) : _Tp(0);

  static constexpr _Tp(min)() noexcept { return __min_value; }
  static constexpr _Tp(max)() noexcept { return __max_value; }
  static constexpr _Tp lowest() noexcept { return __min_value; }
  static constexpr _Tp epsilon() noexcept { return _Tp(0); }
  static constexpr _Tp round_error() noexcept { return _Tp(0); }
  static constexpr _Tp infinity() noexcept { return _Tp(0); }
  static constexpr _Tp quiet_NaN() noexcept { return _Tp(0); }
  static constexpr _Tp signaling_NaN() noexcept { return _Tp(0); }
  static constexpr _Tp denorm_min() noexcept { return _Tp(0); }
};

struct __bool_limits : __limits_base {
  static constexpr bool is_specialized = true;
  static constexpr int digits = 1;
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr int radix = 2;
  static constexpr bool is_bounded = true;
  static constexpr bool(min)() noexcept { return false; }
  static constexpr bool(max)() noexcept { return true; }
  static constexpr bool lowest() noexcept { return false; }
  static constexpr bool epsilon() noexcept { return false; }
  static constexpr bool round_error() noexcept { return false; }
  static constexpr bool infinity() noexcept { return false; }
  static constexpr bool quiet_NaN() noexcept { return false; }
  static constexpr bool signaling_NaN() noexcept { return false; }
  static constexpr bool denorm_min() noexcept { return false; }
};

// Signaling NaN for a binary interchange format: exponent all ones, quiet bit clear,
// lowest payload bit set. Built from the format facts, then bit_cast.
template <class _Tp>
consteval _Tp __make_signaling_nan() {
  constexpr __fp_format_info __f = __fp_format<_Tp>;
  if constexpr (__is_same(_Tp, long double) && __f.digits == 64) {
    return __builtin_nansl(""); // x87 extended: explicit integer bit, not an interchange format
  } else {
    using _Up = std::conditional_t<sizeof(_Tp) == 2, unsigned short,
                            std::conditional_t<sizeof(_Tp) == 4, unsigned int,
                                          std::conditional_t<sizeof(_Tp) == 8, unsigned long long, __uint128>>>;
    constexpr int __mant_bits = __f.digits - 1;
    constexpr int __exp_bits = static_cast<int>(sizeof(_Tp) * __CHAR_BIT__) - 1 - __mant_bits;
    _Up bits = ((_Up(1) << __exp_bits) - 1) << __mant_bits; // exponent all ones
    bits |= _Up(1) << (__mant_bits - 2);                 // non-zero payload, quiet bit (top) clear
    return __builtin_bit_cast(_Tp, bits);
  }
}

template <class _Tp>
struct __fp_limits : __limits_base {
  static constexpr __fp_format_info __fmt = __fp_format<_Tp>;

  static constexpr bool is_specialized = true;
  static constexpr int digits = __fmt.digits;
  static constexpr int digits10 = __floor_log10_pow2(digits - 1);
  static constexpr int max_digits10 = 2 + __floor_log10_pow2(digits);
  static constexpr bool is_signed = true;
  static constexpr int radix = 2;
  static constexpr int min_exponent = __fmt.__min_exp;
  static constexpr int min_exponent10 = -__floor_log10_pow2(1 - __fmt.__min_exp);
  static constexpr int max_exponent = __fmt.__max_exp;
  static constexpr int max_exponent10 = __floor_log10_pow2(__fmt.__max_exp);
  static constexpr bool has_infinity = true;
  static constexpr bool has_quiet_NaN = true;
  static constexpr bool has_signaling_NaN = true;
  // bfloat16 (8-bit mantissa) is not an ISO/IEC 60559 format.
  static constexpr bool is_iec559 = __fmt.digits != 8;
  static constexpr bool is_bounded = true;
  static constexpr std::float_round_style round_style = std::round_to_nearest;
  // Every supported format has subnormals (denorm_min() is one); the values are unspecified.
  [[deprecated("has_denorm is deprecated ([depr.numeric.limits.has.denorm])")]]
  static constexpr std::float_denorm_style has_denorm = std::denorm_present;
  [[deprecated("has_denorm_loss is deprecated ([depr.numeric.limits.has.denorm])")]]
  static constexpr bool has_denorm_loss = false;

  static constexpr _Tp(min)() noexcept { return __pow2<_Tp>(__fmt.__min_exp - 1); }
  static constexpr _Tp(max)() noexcept {
    // (2 - 2^(1-digits)) * 2^(max_exp-1), computed without overflow.
    return (_Tp(2) - __pow2<_Tp>(1 - __fmt.digits)) * __pow2<_Tp>(__fmt.__max_exp - 1);
  }
  static constexpr _Tp lowest() noexcept { return -(max)(); }
  static constexpr _Tp epsilon() noexcept { return __pow2<_Tp>(1 - __fmt.digits); }
  static constexpr _Tp round_error() noexcept { return _Tp(0.5); }
  static constexpr _Tp infinity() noexcept { return static_cast<_Tp>(__builtin_huge_valf()); }
  static constexpr _Tp quiet_NaN() noexcept { return static_cast<_Tp>(__builtin_nanf("")); }
  static constexpr _Tp signaling_NaN() noexcept { return __make_signaling_nan<_Tp>(); }
  static constexpr _Tp denorm_min() noexcept { return __pow2<_Tp>(__fmt.__min_exp - __fmt.digits); }
};

template <class _Tp>
struct __generic_limits : __limits_base {
  static constexpr _Tp(min)() noexcept { return _Tp(); }
  static constexpr _Tp(max)() noexcept { return _Tp(); }
  static constexpr _Tp lowest() noexcept { return _Tp(); }
  static constexpr _Tp epsilon() noexcept { return _Tp(); }
  static constexpr _Tp round_error() noexcept { return _Tp(); }
  static constexpr _Tp infinity() noexcept { return _Tp(); }
  static constexpr _Tp quiet_NaN() noexcept { return _Tp(); }
  static constexpr _Tp signaling_NaN() noexcept { return _Tp(); }
  static constexpr _Tp denorm_min() noexcept { return _Tp(); }
};

template <class _Tp>
consteval auto __select_limits() {
  if constexpr (__is_same(_Tp, bool))
    return __bool_limits{};
  else if constexpr (is_integral_v<_Tp> || __bitint_width<_Tp> != 0)
    return __int_limits<_Tp>{};
  else if constexpr (__is_floating_v<_Tp> || __is_same(_Tp, __gnu_float128))
    return __fp_limits<_Tp>{};
  else
    return __generic_limits<_Tp>{};
}

}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__("hidden")]] std {

template <class _Tp>
class numeric_limits : public decltype(__ycxx::__detail::__select_limits<_Tp>()) {};

template <class _Tp>
class numeric_limits<const _Tp> : public numeric_limits<_Tp> {};
template <class _Tp>
class numeric_limits<volatile _Tp> : public numeric_limits<_Tp> {};
template <class _Tp>
class numeric_limits<const volatile _Tp> : public numeric_limits<_Tp> {};

} // namespace std
