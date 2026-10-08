// libycxx core: abs, labs, llabs ([c.math.abs]), declared by both <cmath> and <cstdlib>.
//
// Templates, like the rest of <cmath> (ycxx/core/cmath_std.hpp): the C library's
// `int abs(int)` (from <stdlib.h>) is then preferred by unqualified calls under
// `using namespace std;` instead of being ambiguous with std::abs.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/cmath_exact.hpp>
#include <ycxx/core/cmath_promote.hpp>
#include <ycxx/core/prim_traits.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
// [c.math.abs]/3: an unsigned type that integral promotion does not turn into int.
template <class _Tp>
concept __abs_unsigned_unpromotable = is_integral_v<_Tp> && is_unsigned_v<_Tp> && !(sizeof(_Tp) < sizeof(int));
// The 128-bit integer type, where the target has one.
template <class _Tp>
concept __abs_int128 = is_integral_v<_Tp> && __is_same(_Tp, __y_int128);
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 {

template <class = void>
constexpr int abs(int __j) noexcept {
  return __j < 0 ? -__j : __j;
}
template <class = void>
constexpr long abs(long __j) noexcept {
  return __j < 0 ? -__j : __j;
}
template <class = void>
constexpr long long abs(long long __j) noexcept {
  return __j < 0 ? -__j : __j;
}
// Extension: the 128-bit integer type where the target has one (__ycxx::__detail::__y_int128 is
// integral there and an incomplete type otherwise).
template <class _Tp = __ycxx::__detail::__y_int128>
  requires __ycxx::__detail::__abs_int128<_Tp>
constexpr _Tp abs(type_identity_t<_Tp> __j) noexcept {
  return __j < 0 ? -__j : __j;
}
// One overload per floating-point type, as in cmath_std.hpp.
template <class _Tp = float>
  requires __ycxx::__detail::__fp_is<_Tp, float>
constexpr _Tp abs(type_identity_t<_Tp> __x) noexcept {
  return __ycxx::__detail::__fpm::__fp_abs(__x);
}
template <class _Tp = double>
  requires __ycxx::__detail::__fp_is<_Tp, double>
constexpr _Tp abs(type_identity_t<_Tp> __x) noexcept {
  return __ycxx::__detail::__fpm::__fp_abs(__x);
}
template <class _Tp = long double>
  requires __ycxx::__detail::__fp_is<_Tp, long double>
constexpr _Tp abs(type_identity_t<_Tp> __x) noexcept {
  return __ycxx::__detail::__fpm::__fp_abs(__x);
}
template <class _Tp = __ycxx::__detail::__float16>
  requires __ycxx::__detail::__fp_is<_Tp, __ycxx::__detail::__float16>
constexpr _Tp abs(type_identity_t<_Tp> __x) noexcept {
  return __ycxx::__detail::__fpm::__fp_abs(__x);
}
template <class _Tp = __ycxx::__detail::__float32>
  requires __ycxx::__detail::__fp_is<_Tp, __ycxx::__detail::__float32>
constexpr _Tp abs(type_identity_t<_Tp> __x) noexcept {
  return __ycxx::__detail::__fpm::__fp_abs(__x);
}
template <class _Tp = __ycxx::__detail::__float64>
  requires __ycxx::__detail::__fp_is<_Tp, __ycxx::__detail::__float64>
constexpr _Tp abs(type_identity_t<_Tp> __x) noexcept {
  return __ycxx::__detail::__fpm::__fp_abs(__x);
}
template <class _Tp = __ycxx::__detail::__y_float128>
  requires __ycxx::__detail::__fp_is<_Tp, __ycxx::__detail::__y_float128>
constexpr _Tp abs(type_identity_t<_Tp> __x) noexcept {
  return __ycxx::__detail::__fpm::__fp_abs(__x);
}
template <class _Tp = __ycxx::__detail::__bfloat16>
  requires __ycxx::__detail::__fp_is<_Tp, __ycxx::__detail::__bfloat16>
constexpr _Tp abs(type_identity_t<_Tp> __x) noexcept {
  return __ycxx::__detail::__fpm::__fp_abs(__x);
}
// [c.math.abs]/3: "If abs is called with an argument of type X for which is_unsigned_v<X> is
// true and if X cannot be converted to int by integral promotion, the program is ill-formed."
template <class _Tp>
  requires __ycxx::__detail::__abs_unsigned_unpromotable<_Tp>
void abs(_Tp) = delete;

template <class = void>
constexpr long labs(long __j) noexcept {
  return __j < 0 ? -__j : __j;
}
template <class = void>
constexpr long long llabs(long long __j) noexcept {
  return __j < 0 ? -__j : __j;
}

}} // namespace std
