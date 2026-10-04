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

namespace ycxx::detail {
// [c.math.abs]/3: an unsigned type that integral promotion does not turn into int.
template <class T>
concept abs_unsigned_unpromotable = is_integral_v<T> && is_unsigned_v<T> && !(sizeof(T) < sizeof(int));
// The 128-bit integer type, where the target has one.
template <class T>
concept abs_int128 = is_integral_v<T> && __is_same(T, int128);
} // namespace ycxx::detail

namespace std {

template <class = void>
constexpr int abs(int j) noexcept {
  return j < 0 ? -j : j;
}
template <class = void>
constexpr long abs(long j) noexcept {
  return j < 0 ? -j : j;
}
template <class = void>
constexpr long long abs(long long j) noexcept {
  return j < 0 ? -j : j;
}
// Extension: the 128-bit integer type where the target has one (ycxx::detail::int128 is
// integral there and an incomplete type otherwise).
template <class T = ycxx::detail::int128>
  requires ycxx::detail::abs_int128<T>
constexpr T abs(type_identity_t<T> j) noexcept {
  return j < 0 ? -j : j;
}
// One overload per floating-point type, as in cmath_std.hpp.
template <class T = float>
  requires ycxx::detail::fp_is<T, float>
constexpr T abs(type_identity_t<T> x) noexcept {
  return ycxx::detail::fpm::fp_abs(x);
}
template <class T = double>
  requires ycxx::detail::fp_is<T, double>
constexpr T abs(type_identity_t<T> x) noexcept {
  return ycxx::detail::fpm::fp_abs(x);
}
template <class T = long double>
  requires ycxx::detail::fp_is<T, long double>
constexpr T abs(type_identity_t<T> x) noexcept {
  return ycxx::detail::fpm::fp_abs(x);
}
template <class T = ycxx::detail::float16>
  requires ycxx::detail::fp_is<T, ycxx::detail::float16>
constexpr T abs(type_identity_t<T> x) noexcept {
  return ycxx::detail::fpm::fp_abs(x);
}
template <class T = ycxx::detail::float32>
  requires ycxx::detail::fp_is<T, ycxx::detail::float32>
constexpr T abs(type_identity_t<T> x) noexcept {
  return ycxx::detail::fpm::fp_abs(x);
}
template <class T = ycxx::detail::float64>
  requires ycxx::detail::fp_is<T, ycxx::detail::float64>
constexpr T abs(type_identity_t<T> x) noexcept {
  return ycxx::detail::fpm::fp_abs(x);
}
template <class T = ycxx::detail::float128>
  requires ycxx::detail::fp_is<T, ycxx::detail::float128>
constexpr T abs(type_identity_t<T> x) noexcept {
  return ycxx::detail::fpm::fp_abs(x);
}
template <class T = ycxx::detail::bfloat16>
  requires ycxx::detail::fp_is<T, ycxx::detail::bfloat16>
constexpr T abs(type_identity_t<T> x) noexcept {
  return ycxx::detail::fpm::fp_abs(x);
}
// [c.math.abs]/3: "If abs is called with an argument of type X for which is_unsigned_v<X> is
// true and if X cannot be converted to int by integral promotion, the program is ill-formed."
template <class T>
  requires ycxx::detail::abs_unsigned_unpromotable<T>
void abs(T) = delete;

template <class = void>
constexpr long labs(long j) noexcept {
  return j < 0 ? -j : j;
}
template <class = void>
constexpr long long llabs(long long j) noexcept {
  return j < 0 ? -j : j;
}

} // namespace std
