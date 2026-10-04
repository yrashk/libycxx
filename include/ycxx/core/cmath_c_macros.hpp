// libycxx core: the macros of <cmath> ([cmath.syn]) for freestanding builds, with the values of
// the C libraries libycxx supports (glibc, musl). Hosted builds take them from the C library's
// <math.h> instead (ycxx/core/cmath.hpp); src/hosted/cmath_check.cpp checks that they agree.
#pragma once

#include <ycxx/config.hpp>

#define HUGE_VAL (__builtin_huge_val ())
#define HUGE_VALF (__builtin_huge_valf ())
#define HUGE_VALL (__builtin_huge_vall ())
#define INFINITY (__builtin_inff ())
#define NAN (__builtin_nanf (""))
#define FP_NAN 0
#define FP_INFINITE 1
#define FP_ZERO 2
#define FP_SUBNORMAL 3
#define FP_NORMAL 4
#if YCXX_FP_FAST_FMA
#  define FP_FAST_FMA 1
#endif
#if YCXX_FP_FAST_FMAF
#  define FP_FAST_FMAF 1
#endif
#if YCXX_FP_FAST_FMAL
#  define FP_FAST_FMAL 1
#endif
#define FP_ILOGB0 (-2147483647 - 1)
#define FP_ILOGBNAN YCXX_FP_ILOGBNAN
#define MATH_ERRNO 1
#define MATH_ERREXCEPT 2
#if YCXX_MATH_ERRNO
#  define math_errhandling (MATH_ERRNO | MATH_ERREXCEPT)
#else
#  define math_errhandling (MATH_ERREXCEPT)
#endif
