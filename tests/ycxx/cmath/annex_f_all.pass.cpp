// ISO/IEC 9899:2024 Annex F.10 special values for every <cmath> function, for float, double,
// long double and the extended floating-point types the compiler provides ([basic.extended.fp],
// [cmath.syn]: one overload per cv-unqualified floating-point type), at run time and in constant
// evaluation.
// [library.c]/2: the C++ functions have the behavior of the C functions, so at run time Annex F
// applies where the C library claims it (__STDC_IEC_559__: glibc does, Darwin's libm does not;
// without it the run-time checks are skipped with a note). [library.c]/3: in constant
// evaluation Annex F applies on every platform, but a call that raises a floating-point
// exception other than FE_INEXACT is not a constant expression, so the checks marked RT (they
// raise divide-by-zero or invalid, or overflow) run only at run time; at run time arguments pass
// through a volatile so that the library, not the compiler's folding, computes the result.
// Paragraphs: F.10.1.1-7 (acos, asin, atan, atan2, cos, sin, tan), F.10.2.1-6 (acosh, asinh,
// atanh, cosh, sinh, tanh), F.10.3.1-13 (exp, exp2, expm1, frexp, ilogb, ldexp, log, log10, log1p,
// log2, logb, modf, scalbn), F.10.4.1-5 (cbrt, fabs, hypot, pow, sqrt), F.10.5.1-4 (erf, erfc,
// lgamma, tgamma), F.10.6.1-8 (ceil, floor, nearbyint, rint, lrint, round, lround, trunc),
// F.10.7.1-3 (fmod, remainder, remquo), F.10.8.1-4 (copysign, nan, nextafter, nexttoward),
// F.10.9.1-3 (fdim, fmax, fmin), F.10.10.1 (fma).
#include <cfloat>
#include <climits>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdfloat>
#include "annex_f.hpp"
#include "check.hpp"

static_assert(annex_f<float>());
static_assert(annex_f<double>());
static_assert(annex_f<long double>());

int main() {
#if defined(__STDC_IEC_559__)
  CHECK(annex_f<float>());
  CHECK(annex_f<double>());
  CHECK(annex_f<long double>());
#else
  dprintf(2, "note: the C library does not claim Annex F (__STDC_IEC_559__); run-time checks skipped\n");
#endif
  return 0;
}
