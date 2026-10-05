// libycxx core: <numbers> ([numbers]).
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/concepts.hpp>
#include <ycxx/core/math_constants.hpp>
#include <ycxx/core/meta_base.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail {
template <class T>
consteval T numbers_primary() {
  static_assert(ycxx::detail::always_false<T>,
                "[math.constants]/3: the primary template of a mathematical constant variable template is "
                "instantiated (T is not a floating-point type)");
  return T();
}
template <class T>
consteval T numbers_value(math_constant c) {
  return ycxx::detail::math_constant_value<std::remove_cv_t<T>>(c);
}
}} // namespace ycxx::detail

namespace [[gnu::visibility("hidden")]] std { namespace numbers {

template <class T>
inline constexpr T e_v = ycxx::detail::numbers_primary<T>();
template <class T>
inline constexpr T log2e_v = ycxx::detail::numbers_primary<T>();
template <class T>
inline constexpr T log10e_v = ycxx::detail::numbers_primary<T>();
template <class T>
inline constexpr T pi_v = ycxx::detail::numbers_primary<T>();
template <class T>
inline constexpr T inv_pi_v = ycxx::detail::numbers_primary<T>();
template <class T>
inline constexpr T inv_sqrtpi_v = ycxx::detail::numbers_primary<T>();
template <class T>
inline constexpr T ln2_v = ycxx::detail::numbers_primary<T>();
template <class T>
inline constexpr T ln10_v = ycxx::detail::numbers_primary<T>();
template <class T>
inline constexpr T sqrt2_v = ycxx::detail::numbers_primary<T>();
template <class T>
inline constexpr T sqrt3_v = ycxx::detail::numbers_primary<T>();
template <class T>
inline constexpr T inv_sqrt3_v = ycxx::detail::numbers_primary<T>();
template <class T>
inline constexpr T egamma_v = ycxx::detail::numbers_primary<T>();
template <class T>
inline constexpr T phi_v = ycxx::detail::numbers_primary<T>();

template <floating_point T>
inline constexpr T e_v<T> = ycxx::detail::numbers_value<T>(ycxx::detail::math_constant::e);
template <floating_point T>
inline constexpr T log2e_v<T> = ycxx::detail::numbers_value<T>(ycxx::detail::math_constant::log2e);
template <floating_point T>
inline constexpr T log10e_v<T> = ycxx::detail::numbers_value<T>(ycxx::detail::math_constant::log10e);
template <floating_point T>
inline constexpr T pi_v<T> = ycxx::detail::numbers_value<T>(ycxx::detail::math_constant::pi);
template <floating_point T>
inline constexpr T inv_pi_v<T> = ycxx::detail::numbers_value<T>(ycxx::detail::math_constant::inv_pi);
template <floating_point T>
inline constexpr T inv_sqrtpi_v<T> = ycxx::detail::numbers_value<T>(ycxx::detail::math_constant::inv_sqrtpi);
template <floating_point T>
inline constexpr T ln2_v<T> = ycxx::detail::numbers_value<T>(ycxx::detail::math_constant::ln2);
template <floating_point T>
inline constexpr T ln10_v<T> = ycxx::detail::numbers_value<T>(ycxx::detail::math_constant::ln10);
template <floating_point T>
inline constexpr T sqrt2_v<T> = ycxx::detail::numbers_value<T>(ycxx::detail::math_constant::sqrt2);
template <floating_point T>
inline constexpr T sqrt3_v<T> = ycxx::detail::numbers_value<T>(ycxx::detail::math_constant::sqrt3);
template <floating_point T>
inline constexpr T inv_sqrt3_v<T> = ycxx::detail::numbers_value<T>(ycxx::detail::math_constant::inv_sqrt3);
template <floating_point T>
inline constexpr T egamma_v<T> = ycxx::detail::numbers_value<T>(ycxx::detail::math_constant::egamma);
template <floating_point T>
inline constexpr T phi_v<T> = ycxx::detail::numbers_value<T>(ycxx::detail::math_constant::phi);

inline constexpr double e = e_v<double>;
inline constexpr double log2e = log2e_v<double>;
inline constexpr double log10e = log10e_v<double>;
inline constexpr double pi = pi_v<double>;
inline constexpr double inv_pi = inv_pi_v<double>;
inline constexpr double inv_sqrtpi = inv_sqrtpi_v<double>;
inline constexpr double ln2 = ln2_v<double>;
inline constexpr double ln10 = ln10_v<double>;
inline constexpr double sqrt2 = sqrt2_v<double>;
inline constexpr double sqrt3 = sqrt3_v<double>;
inline constexpr double inv_sqrt3 = inv_sqrt3_v<double>;
inline constexpr double egamma = egamma_v<double>;
inline constexpr double phi = phi_v<double>;

}} // namespace std::numbers
