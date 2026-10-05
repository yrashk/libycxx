// libycxx hosted runtime: a build-time check that the macros freestanding <cmath> defines itself
// (ycxx/core/cmath_c_macros.hpp) and its float_t/double_t agree with this C library's <math.h>,
// which hosted <cmath> uses.
#include <math.h> // libycxx's: the C library's header, then <cmath>

namespace {
constexpr int c_fp_nan = FP_NAN, c_fp_infinite = FP_INFINITE, c_fp_zero = FP_ZERO, c_fp_subnormal = FP_SUBNORMAL,
              c_fp_normal = FP_NORMAL;
constexpr int c_ilogb0 = FP_ILOGB0, c_ilogbnan = FP_ILOGBNAN;
constexpr int c_math_errno = MATH_ERRNO, c_math_errexcept = MATH_ERREXCEPT;
// math_errhandling is a constant in glibc and musl, but a run-time call on Darwin
// (__math_errhandling()), which is no constant expression. __builtin_constant_p is false for the
// call, which is then not evaluated: only a constant value is compared below (-1: not a
// constant). Where it is a call, libycxx's value is what config.hpp states for that C library
// (Darwin: MATH_ERREXCEPT, its libm never setting errno), and is not checked here.
constexpr int c_math_errhandling = __builtin_constant_p(math_errhandling) ? math_errhandling : -1;
using c_float_t = ::float_t;
using c_double_t = ::double_t;
} // namespace

#undef HUGE_VAL
#undef HUGE_VALF
#undef HUGE_VALL
#undef INFINITY
#undef NAN
#undef FP_NAN
#undef FP_INFINITE
#undef FP_ZERO
#undef FP_SUBNORMAL
#undef FP_NORMAL
#undef FP_FAST_FMA
#undef FP_FAST_FMAF
#undef FP_FAST_FMAL
#undef FP_ILOGB0
#undef FP_ILOGBNAN
#undef MATH_ERRNO
#undef MATH_ERREXCEPT
#undef math_errhandling

#include <ycxx/core/cmath_c_macros.hpp>

static_assert(FP_NAN == c_fp_nan && FP_INFINITE == c_fp_infinite && FP_ZERO == c_fp_zero &&
                  FP_SUBNORMAL == c_fp_subnormal && FP_NORMAL == c_fp_normal,
              "libycxx: <cmath>'s FP_* classification values differ from the C library's");
static_assert(FP_ILOGB0 == c_ilogb0 && FP_ILOGBNAN == c_ilogbnan,
              "libycxx: FP_ILOGB0 / FP_ILOGBNAN differ from the C library's (see YCXX_FP_ILOGBNAN in config.hpp)");
static_assert(MATH_ERRNO == c_math_errno && MATH_ERREXCEPT == c_math_errexcept &&
                  (c_math_errhandling == -1 || math_errhandling == c_math_errhandling),
              "libycxx: math_errhandling differs from the C library's (see YCXX_MATH_ERRNO in config.hpp)");
static_assert(__is_same(std::float_t, c_float_t) && __is_same(std::double_t, c_double_t),
              "libycxx: std::float_t / std::double_t differ from the C library's");
