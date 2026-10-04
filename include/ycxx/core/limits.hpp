// libycxx core: <limits>
//
// Integer limits are computed from the type. Floating-point limits are computed from the
// three per-format facts in ycxx::detail::fp_format (config.hpp): mantissa digits, min_exp and
// max_exp. Every derived value is a constant expression evaluated in the type's own arithmetic.
#pragma once

#include <ycxx/core/type_traits.hpp>

namespace std {

enum float_round_style {
  round_indeterminate = -1,
  round_toward_zero = 0,
  round_to_nearest = 1,
  round_toward_infinity = 2,
  round_toward_neg_infinity = 3
};

// [depr.numeric.limits.has.denorm] (Annex D)
enum [[deprecated("float_denorm_style is deprecated ([depr.numeric.limits.has.denorm])")]] float_denorm_style {
  denorm_indeterminate = -1,
  denorm_absent = 0,
  denorm_present = 1
};

} // namespace std

namespace ycxx::detail {

// floor(e * log10(2)) for |e| < 2^31 (log10(2) is irrational, so no product is an integer).
consteval int floor_log10_pow2(int e) {
  constexpr int128 num = 301029995663981195LL; // log10(2) * 10^18
  constexpr int128 den = 1000000000000000000LL;
  int128 p = static_cast<int128>(e) * num;
  return static_cast<int>(p >= 0 ? p / den : -((-p + den - 1) / den));
}

template <class T>
consteval T pow2(int e) {
  T r = 1;
  for (; e > 0; --e)
    r *= 2;
  for (; e < 0; ++e)
    r /= 2;
  return r;
}

// Defaults shared by every specialization ([numeric.limits.general]).
struct limits_base {
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

template <class T>
struct int_limits : limits_base {
  static constexpr bool is_specialized = true;
  static constexpr bool is_signed = bitint_width<T> != 0 ? bitint_info<T>::is_signed : is_signed_v<T>;
  static constexpr int width = bitint_width<T> != 0 ? bitint_width<T> : static_cast<int>(sizeof(T) * __CHAR_BIT__);
  static constexpr int digits = width - (is_signed ? 1 : 0);
  static constexpr int digits10 = floor_log10_pow2(digits);
  static constexpr bool is_integer = true;
  static constexpr bool is_exact = true;
  static constexpr int radix = 2;
  static constexpr bool is_bounded = true;
  static constexpr bool is_modulo = !is_signed;
  // Only types not subject to integral promotion can trap on division (char types never do).
  static constexpr bool traps = cfg::integer_division_traps && bitint_width<T> == 0 &&
                                is_signed_or_unsigned_integer<T> && sizeof(T) >= sizeof(int);

  // 2^digits - 1, built without overflow for any width (including _BitInt(N)).
  static constexpr T max_value = [] {
    T r = 0;
    for (int i = 0; i < digits; ++i)
      r = static_cast<T>(r * 2 + 1);
    return r;
  }();
  static constexpr T min_value = is_signed ? static_cast<T>(-max_value - 1) : T(0);

  static constexpr T(min)() noexcept { return min_value; }
  static constexpr T(max)() noexcept { return max_value; }
  static constexpr T lowest() noexcept { return min_value; }
  static constexpr T epsilon() noexcept { return T(0); }
  static constexpr T round_error() noexcept { return T(0); }
  static constexpr T infinity() noexcept { return T(0); }
  static constexpr T quiet_NaN() noexcept { return T(0); }
  static constexpr T signaling_NaN() noexcept { return T(0); }
  static constexpr T denorm_min() noexcept { return T(0); }
};

struct bool_limits : limits_base {
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
template <class T>
consteval T make_signaling_nan() {
  constexpr fp_format_info f = fp_format<T>;
  if constexpr (__is_same(T, long double) && f.digits == 64) {
    return __builtin_nansl(""); // x87 extended: explicit integer bit, not an interchange format
  } else {
    using U = std::conditional_t<sizeof(T) == 2, unsigned short,
                            std::conditional_t<sizeof(T) == 4, unsigned int,
                                          std::conditional_t<sizeof(T) == 8, unsigned long long, uint128>>>;
    constexpr int mant_bits = f.digits - 1;
    constexpr int exp_bits = static_cast<int>(sizeof(T) * __CHAR_BIT__) - 1 - mant_bits;
    U bits = ((U(1) << exp_bits) - 1) << mant_bits; // exponent all ones
    bits |= U(1) << (mant_bits - 2);                 // non-zero payload, quiet bit (top) clear
    return __builtin_bit_cast(T, bits);
  }
}

template <class T>
struct fp_limits : limits_base {
  static constexpr fp_format_info fmt = fp_format<T>;

  static constexpr bool is_specialized = true;
  static constexpr int digits = fmt.digits;
  static constexpr int digits10 = floor_log10_pow2(digits - 1);
  static constexpr int max_digits10 = 2 + floor_log10_pow2(digits);
  static constexpr bool is_signed = true;
  static constexpr int radix = 2;
  static constexpr int min_exponent = fmt.min_exp;
  static constexpr int min_exponent10 = -floor_log10_pow2(1 - fmt.min_exp);
  static constexpr int max_exponent = fmt.max_exp;
  static constexpr int max_exponent10 = floor_log10_pow2(fmt.max_exp);
  static constexpr bool has_infinity = true;
  static constexpr bool has_quiet_NaN = true;
  static constexpr bool has_signaling_NaN = true;
  // bfloat16 (8-bit mantissa) is not an ISO/IEC 60559 format.
  static constexpr bool is_iec559 = fmt.digits != 8;
  static constexpr bool is_bounded = true;
  static constexpr std::float_round_style round_style = std::round_to_nearest;
  // Every supported format has subnormals (denorm_min() is one); the values are unspecified.
  [[deprecated("has_denorm is deprecated ([depr.numeric.limits.has.denorm])")]]
  static constexpr std::float_denorm_style has_denorm = std::denorm_present;
  [[deprecated("has_denorm_loss is deprecated ([depr.numeric.limits.has.denorm])")]]
  static constexpr bool has_denorm_loss = false;

  static constexpr T(min)() noexcept { return pow2<T>(fmt.min_exp - 1); }
  static constexpr T(max)() noexcept {
    // (2 - 2^(1-digits)) * 2^(max_exp-1), computed without overflow.
    return (T(2) - pow2<T>(1 - fmt.digits)) * pow2<T>(fmt.max_exp - 1);
  }
  static constexpr T lowest() noexcept { return -(max)(); }
  static constexpr T epsilon() noexcept { return pow2<T>(1 - fmt.digits); }
  static constexpr T round_error() noexcept { return T(0.5); }
  static constexpr T infinity() noexcept { return static_cast<T>(__builtin_huge_valf()); }
  static constexpr T quiet_NaN() noexcept { return static_cast<T>(__builtin_nanf("")); }
  static constexpr T signaling_NaN() noexcept { return make_signaling_nan<T>(); }
  static constexpr T denorm_min() noexcept { return pow2<T>(fmt.min_exp - fmt.digits); }
};

template <class T>
struct generic_limits : limits_base {
  static constexpr T(min)() noexcept { return T(); }
  static constexpr T(max)() noexcept { return T(); }
  static constexpr T lowest() noexcept { return T(); }
  static constexpr T epsilon() noexcept { return T(); }
  static constexpr T round_error() noexcept { return T(); }
  static constexpr T infinity() noexcept { return T(); }
  static constexpr T quiet_NaN() noexcept { return T(); }
  static constexpr T signaling_NaN() noexcept { return T(); }
  static constexpr T denorm_min() noexcept { return T(); }
};

template <class T>
consteval auto select_limits() {
  if constexpr (__is_same(T, bool))
    return bool_limits{};
  else if constexpr (is_integral_v<T> || bitint_width<T> != 0)
    return int_limits<T>{};
  else if constexpr (is_floating_v<T> || __is_same(T, gnu_float128))
    return fp_limits<T>{};
  else
    return generic_limits<T>{};
}

} // namespace ycxx::detail

namespace std {

template <class T>
class numeric_limits : public decltype(ycxx::detail::select_limits<T>()) {};

template <class T>
class numeric_limits<const T> : public numeric_limits<T> {};
template <class T>
class numeric_limits<volatile T> : public numeric_limits<T> {};
template <class T>
class numeric_limits<const volatile T> : public numeric_limits<T> {};

} // namespace std
