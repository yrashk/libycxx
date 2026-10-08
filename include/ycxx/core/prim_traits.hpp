// libycxx core: primary type classification usable before <type_traits> is complete.
//
// Only a subset of the classification builtins exists on both GCC and Clang. The rest are
// implemented here once, portably, as variable templates in __ycxx::__detail.
#pragma once

#include <ycxx/config.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {

template <class _Tp, class... _Us>
inline constexpr bool __is_any_of = (__is_same(_Tp, _Us) || ...);

template <class _Tp>
inline constexpr bool __is_lref_v = false;
template <class _Tp>
inline constexpr bool __is_lref_v<_Tp&> = true;
template <class _Tp>
inline constexpr bool __is_rref_v = false;
template <class _Tp>
inline constexpr bool __is_rref_v<_Tp&&> = true;

template <class _Tp>
inline constexpr bool is_void_v = __is_same(__remove_cv(_Tp), void);

template <class _Tp>
inline constexpr bool is_integral_v =
    __is_any_of<__remove_cv(_Tp), bool, char, signed char, unsigned char, wchar_t, char8_t, char16_t, char32_t, short,
              unsigned short, int, unsigned int, long, unsigned long, long long, unsigned long long, __y_int128, __uint128>;

template <class _Tp>
inline constexpr bool __is_floating_v =
    __is_any_of<__remove_cv(_Tp), float, double, long double, __float16, __float32, __float64, __y_float128, __bfloat16>;

template <class _Tp>
inline constexpr bool is_arithmetic_v = is_integral_v<_Tp> || __is_floating_v<_Tp>;

template <class _Tp>
inline constexpr bool is_null_pointer_v = __is_same(__remove_cv(_Tp), decltype(nullptr));

template <class _Tp>
inline constexpr bool is_reflection_v = __is_same(__remove_cv(_Tp), __reflection);

template <class _Tp>
inline constexpr bool is_fundamental_v =
    is_arithmetic_v<_Tp> || is_void_v<_Tp> || is_null_pointer_v<_Tp> || is_reflection_v<_Tp>;

template <class _Tp>
inline constexpr bool is_scalar_v = is_arithmetic_v<_Tp> || __is_enum(_Tp) || __is_pointer(_Tp) || __is_member_pointer(_Tp) ||
                                    is_null_pointer_v<_Tp> || is_reflection_v<_Tp>;

template <class _Tp>
consteval bool __signed_impl() {
  if constexpr (is_arithmetic_v<_Tp>)
    return _Tp(-1) < _Tp(0);
  else
    return false;
}
template <class _Tp>
inline constexpr bool is_signed_v = __signed_impl<_Tp>();
template <class _Tp>
consteval bool __unsigned_impl() {
  if constexpr (is_arithmetic_v<_Tp>)
    return _Tp(0) < _Tp(-1);
  else
    return false;
}
template <class _Tp>
inline constexpr bool is_unsigned_v = __unsigned_impl<_Tp>();

}} // namespace __ycxx::__detail


