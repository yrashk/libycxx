// The Annex F special values of cmath/annex_f_all (support/annex_f.hpp; ISO/IEC 9899:2024 F.10,
// [library.c]/2-3) for the extended floating-point types: [cmath.syn]/2: "For each function with
// at least one parameter of type floating-point-type, the implementation provides an overload for
// each cv-unqualified floating-point type", and [basic.extended.fp] (std::float16_t etc. when the
// __STDCPP_FLOAT16_T__ ... macros are defined). The C library has no functions for most of these
// types, so the library's own overloads must give C's semantics (std::float32_t, float64_t
// behave as float and double; std::float16_t, bfloat16_t, float128_t have their own formats).
// Clang 23 defines none of these types; the test then checks nothing.
#include <cmath>
#include <stdfloat>
#include "annex_f.hpp"
#include "check.hpp"

#if defined(__STDCPP_FLOAT16_T__)
static_assert(annex_f<std::float16_t>());
#endif
#if defined(__STDCPP_BFLOAT16_T__)
static_assert(annex_f<std::bfloat16_t>());
#endif
#if defined(__STDCPP_FLOAT32_T__)
static_assert(annex_f<std::float32_t>());
#endif
#if defined(__STDCPP_FLOAT64_T__)
static_assert(annex_f<std::float64_t>());
#endif
#if defined(__STDCPP_FLOAT128_T__)
static_assert(annex_f<std::float128_t>());
#endif

int main() {
#if defined(__STDCPP_FLOAT16_T__)
  CHECK(annex_f<std::float16_t>());
#endif
#if defined(__STDCPP_BFLOAT16_T__)
  CHECK(annex_f<std::bfloat16_t>());
#endif
#if defined(__STDCPP_FLOAT32_T__)
  CHECK(annex_f<std::float32_t>());
#endif
#if defined(__STDCPP_FLOAT64_T__)
  CHECK(annex_f<std::float64_t>());
#endif
#if defined(__STDCPP_FLOAT128_T__)
  CHECK(annex_f<std::float128_t>());
#endif
  return 0;
}
