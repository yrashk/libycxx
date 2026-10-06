// libycxx hosted runtime: the current rounding direction for <cmath>'s rint, nearbyint and
// lrint of a type the C library does not compute (std::float128_t where libm has no *f128
// functions, cfg::c_math_float128; ycxx/core/cmath_impl.hpp). Built everywhere, so that every
// platform compiles it.
#include <cfenv>
#include <cmath>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail::cm {

ycxx::detail::fpm::fp_rint_mode current_rounding() noexcept {
  switch (std::fegetround()) {
  case FE_DOWNWARD:
    return ycxx::detail::fpm::fp_rint_mode::floor;
  case FE_UPWARD:
    return ycxx::detail::fpm::fp_rint_mode::ceil;
  case FE_TOWARDZERO:
    return ycxx::detail::fpm::fp_rint_mode::trunc;
  default:
    return ycxx::detail::fpm::fp_rint_mode::half_even;
  }
}

}} // namespace ycxx::detail::cm
