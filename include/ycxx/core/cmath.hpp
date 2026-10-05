// libycxx core: <cmath> ([cmath.syn], [c.math], [sf.cmath]).
//
// Core header (DECISIONS §3): freestanding, it includes no C header; hosted, only the C library's
// <math.h>, for its macros. The functions need libm at run time (a freestanding program that
// calls them gets a link error, like the C-library parts of <string>). Everything the draft makes constexpr is constexpr,
// with the semantics of ISO/IEC 9899:2024 Annex F during constant evaluation ([library.c]/3):
// see cmath_impl.hpp for how each function is evaluated.
//
// Hosted, the macros come from the C library's <math.h> (included with #include_next, so that
// libycxx's <math.h> wrapper can be included before or after <cmath>); freestanding,
// cmath_c_macros.hpp defines them with the same values (glibc, musl, Darwin;
// src/hosted/cmath_check.cpp verifies that when libycxx is built). The classification macros of
// <math.h> are removed: in C++ they are the functions below.
#pragma once

#include <ycxx/config.hpp>
#include <ycxx/core/cmath_impl.hpp>
#include <ycxx/core/cmath_special.hpp>
#include <ycxx/core/math_abs.hpp>
#include <ycxx/core/meta_base.hpp>

#if YCXX_HOSTED
// The C library's macros, from its <math.h>: libycxx's own <math.h> (which a program may include
// before or after <cmath>) adds the C++ declarations to the global namespace, so this skips it.
#  include_next <math.h>
#else
#  include <ycxx/core/cmath_c_macros.hpp>
#endif

#undef fpclassify
#undef isfinite
#undef isinf
#undef isnan
#undef isnormal
#undef signbit
#undef isgreater
#undef isgreaterequal
#undef isless
#undef islessequal
#undef islessgreater
#undef isunordered

namespace std {
// FLT_EVAL_METHOD 0: float/double; 1: double/double; 2: long double/long double.
using float_t = conditional_t<ycxx::detail::cfg::flt_eval_method == 1, double,
                              conditional_t<ycxx::detail::cfg::flt_eval_method == 2, long double, float>>;
using double_t = conditional_t<ycxx::detail::cfg::flt_eval_method == 2, long double, double>;

// Not constexpr; templates for the reason given in cmath_std.hpp.
template <class = void>
double nan(const char* tagp) noexcept {
  return __builtin_nan(tagp);
}
template <class = void>
float nanf(const char* tagp) noexcept {
  return __builtin_nanf(tagp);
}
template <class = void>
long double nanl(const char* tagp) noexcept {
  return __builtin_nanl(tagp);
}
} // namespace std

#include <ycxx/core/cmath_std.hpp>
