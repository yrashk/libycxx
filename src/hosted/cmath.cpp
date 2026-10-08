// libycxx hosted runtime: the current rounding direction for <cmath>'s rint, nearbyint and
// lrint of a type the C library does not compute (std::float128_t where libm has no *f128
// functions, cfg::c_math_float128; ycxx/core/cmath_impl.hpp). Built everywhere, so that every
// platform compiles it.
#include <cfenv>
#include <cmath>

namespace [[__gnu__::__visibility__(_YCXX_VISIBILITY)]] __ycxx { namespace __detail::__cm {

__ycxx::__detail::__fpm::__fp_rint_mode __current_rounding() noexcept {
  switch (std::fegetround()) {
  case FE_DOWNWARD:
    return __ycxx::__detail::__fpm::__fp_rint_mode::floor;
  case FE_UPWARD:
    return __ycxx::__detail::__fpm::__fp_rint_mode::ceil;
  case FE_TOWARDZERO:
    return __ycxx::__detail::__fpm::__fp_rint_mode::trunc;
  default:
    return __ycxx::__detail::__fpm::__fp_rint_mode::__half_even;
  }
}

}} // namespace __ycxx::__detail::__cm
