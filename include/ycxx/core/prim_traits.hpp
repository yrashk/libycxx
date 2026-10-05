// libycxx core: primary type classification usable before <type_traits> is complete.
//
// Only a subset of the classification builtins exists on both GCC and Clang. The rest are
// implemented here once, portably, as variable templates in ycxx::detail.
#pragma once

#include <ycxx/config.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {

template <class T, class... Us>
inline constexpr bool is_any_of = (__is_same(T, Us) || ...);

template <class T>
inline constexpr bool is_lref_v = false;
template <class T>
inline constexpr bool is_lref_v<T&> = true;
template <class T>
inline constexpr bool is_rref_v = false;
template <class T>
inline constexpr bool is_rref_v<T&&> = true;

template <class T>
inline constexpr bool is_void_v = __is_same(__remove_cv(T), void);

template <class T>
inline constexpr bool is_integral_v =
    is_any_of<__remove_cv(T), bool, char, signed char, unsigned char, wchar_t, char8_t, char16_t, char32_t, short,
              unsigned short, int, unsigned int, long, unsigned long, long long, unsigned long long, int128, uint128>;

template <class T>
inline constexpr bool is_floating_v =
    is_any_of<__remove_cv(T), float, double, long double, float16, float32, float64, float128, bfloat16>;

template <class T>
inline constexpr bool is_arithmetic_v = is_integral_v<T> || is_floating_v<T>;

template <class T>
inline constexpr bool is_null_pointer_v = __is_same(__remove_cv(T), decltype(nullptr));

template <class T>
inline constexpr bool is_reflection_v = __is_same(__remove_cv(T), reflection);

template <class T>
inline constexpr bool is_fundamental_v =
    is_arithmetic_v<T> || is_void_v<T> || is_null_pointer_v<T> || is_reflection_v<T>;

template <class T>
inline constexpr bool is_scalar_v = is_arithmetic_v<T> || __is_enum(T) || __is_pointer(T) || __is_member_pointer(T) ||
                                    is_null_pointer_v<T> || is_reflection_v<T>;

template <class T>
consteval bool signed_impl() {
  if constexpr (is_arithmetic_v<T>)
    return T(-1) < T(0);
  else
    return false;
}
template <class T>
inline constexpr bool is_signed_v = signed_impl<T>();
template <class T>
consteval bool unsigned_impl() {
  if constexpr (is_arithmetic_v<T>)
    return T(0) < T(-1);
  else
    return false;
}
template <class T>
inline constexpr bool is_unsigned_v = unsigned_impl<T>();

}} // namespace ycxx::detail


