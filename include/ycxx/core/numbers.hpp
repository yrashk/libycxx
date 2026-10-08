// libycxx core: <numbers> ([numbers]).
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/concepts.hpp>
#include <ycxx/core/math_constants.hpp>
#include <ycxx/core/meta_base.hpp>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail {
template <class _Tp>
consteval _Tp __numbers_primary() {
  static_assert(__ycxx::__detail::__always_false<_Tp>,
                "[math.constants]/3: the primary template of a mathematical constant variable template is "
                "instantiated (T is not a floating-point type)");
  return _Tp();
}
template <class _Tp>
consteval _Tp __numbers_value(__math_constant c) {
  return __ycxx::__detail::__math_constant_value<std::remove_cv_t<_Tp>>(c);
}
}} // namespace __ycxx::__detail

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] std { inline namespace __y1 { namespace numbers {

template <class _Tp>
inline constexpr _Tp e_v = __ycxx::__detail::__numbers_primary<_Tp>();
template <class _Tp>
inline constexpr _Tp log2e_v = __ycxx::__detail::__numbers_primary<_Tp>();
template <class _Tp>
inline constexpr _Tp log10e_v = __ycxx::__detail::__numbers_primary<_Tp>();
template <class _Tp>
inline constexpr _Tp pi_v = __ycxx::__detail::__numbers_primary<_Tp>();
template <class _Tp>
inline constexpr _Tp inv_pi_v = __ycxx::__detail::__numbers_primary<_Tp>();
template <class _Tp>
inline constexpr _Tp inv_sqrtpi_v = __ycxx::__detail::__numbers_primary<_Tp>();
template <class _Tp>
inline constexpr _Tp ln2_v = __ycxx::__detail::__numbers_primary<_Tp>();
template <class _Tp>
inline constexpr _Tp ln10_v = __ycxx::__detail::__numbers_primary<_Tp>();
template <class _Tp>
inline constexpr _Tp sqrt2_v = __ycxx::__detail::__numbers_primary<_Tp>();
template <class _Tp>
inline constexpr _Tp sqrt3_v = __ycxx::__detail::__numbers_primary<_Tp>();
template <class _Tp>
inline constexpr _Tp inv_sqrt3_v = __ycxx::__detail::__numbers_primary<_Tp>();
template <class _Tp>
inline constexpr _Tp egamma_v = __ycxx::__detail::__numbers_primary<_Tp>();
template <class _Tp>
inline constexpr _Tp phi_v = __ycxx::__detail::__numbers_primary<_Tp>();

template <floating_point _Tp>
inline constexpr _Tp e_v<_Tp> = __ycxx::__detail::__numbers_value<_Tp>(__ycxx::__detail::__math_constant::e);
template <floating_point _Tp>
inline constexpr _Tp log2e_v<_Tp> = __ycxx::__detail::__numbers_value<_Tp>(__ycxx::__detail::__math_constant::log2e);
template <floating_point _Tp>
inline constexpr _Tp log10e_v<_Tp> = __ycxx::__detail::__numbers_value<_Tp>(__ycxx::__detail::__math_constant::log10e);
template <floating_point _Tp>
inline constexpr _Tp pi_v<_Tp> = __ycxx::__detail::__numbers_value<_Tp>(__ycxx::__detail::__math_constant::pi);
template <floating_point _Tp>
inline constexpr _Tp inv_pi_v<_Tp> = __ycxx::__detail::__numbers_value<_Tp>(__ycxx::__detail::__math_constant::inv_pi);
template <floating_point _Tp>
inline constexpr _Tp inv_sqrtpi_v<_Tp> = __ycxx::__detail::__numbers_value<_Tp>(__ycxx::__detail::__math_constant::inv_sqrtpi);
template <floating_point _Tp>
inline constexpr _Tp ln2_v<_Tp> = __ycxx::__detail::__numbers_value<_Tp>(__ycxx::__detail::__math_constant::ln2);
template <floating_point _Tp>
inline constexpr _Tp ln10_v<_Tp> = __ycxx::__detail::__numbers_value<_Tp>(__ycxx::__detail::__math_constant::ln10);
template <floating_point _Tp>
inline constexpr _Tp sqrt2_v<_Tp> = __ycxx::__detail::__numbers_value<_Tp>(__ycxx::__detail::__math_constant::sqrt2);
template <floating_point _Tp>
inline constexpr _Tp sqrt3_v<_Tp> = __ycxx::__detail::__numbers_value<_Tp>(__ycxx::__detail::__math_constant::sqrt3);
template <floating_point _Tp>
inline constexpr _Tp inv_sqrt3_v<_Tp> = __ycxx::__detail::__numbers_value<_Tp>(__ycxx::__detail::__math_constant::inv_sqrt3);
template <floating_point _Tp>
inline constexpr _Tp egamma_v<_Tp> = __ycxx::__detail::__numbers_value<_Tp>(__ycxx::__detail::__math_constant::egamma);
template <floating_point _Tp>
inline constexpr _Tp phi_v<_Tp> = __ycxx::__detail::__numbers_value<_Tp>(__ycxx::__detail::__math_constant::phi);

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

}}} // namespace std::numbers
