// [support.c.headers.other]/1: <math.h> behaves as if each name that <cmath> places in namespace
// std is placed within the global namespace scope (except the special math functions
// [sf.cmath] and lerp), so the C++ overload sets of <cmath> ([cmath.syn]: one function per
// floating-point type, "sufficient additional overloads" for arithmetic arguments, /3:
// integer arguments are treated as double; the classification functions are functions
// returning bool, [c.math.fpclass]) are available as ::name. [c.math.abs]: abs for the
// floating-point types.
#include <math.h>
#include <stdfloat>
#include <type_traits>

template <class A, class B>
constexpr bool same = std::is_same_v<A, B>;

static_assert(same<decltype(::sqrt(1.0f)), float> && same<decltype(::sqrt(1.0)), double> &&
              same<decltype(::sqrt(1.0L)), long double> && same<decltype(::sqrt(2)), double>);
static_assert(same<decltype(::fabs(1.0f)), float> && same<decltype(::abs(-1.5f)), float> &&
              same<decltype(::abs(-1.5L)), long double>);
static_assert(same<decltype(::pow(2.0f, 3.0f)), float> && same<decltype(::pow(2, 3.0f)), double> &&
              same<decltype(::pow(2.0f, 3.0L)), long double>);
static_assert(same<decltype(::fma(1.0f, 2.0f, 3.0f)), float> && same<decltype(::fma(1.0f, 2, 3.0f)), double>);
static_assert(same<decltype(::hypot(1.0f, 2.0f, 3.0f)), float>);  // three-argument hypot of <cmath>
static_assert(same<decltype(::isnan(1.0f)), bool> && same<decltype(::isinf(1.0)), bool> &&
              same<decltype(::signbit(1.0L)), bool> && same<decltype(::isfinite(3)), bool> &&
              same<decltype(::isgreater(1.0f, 2.0)), bool> && same<decltype(::fpclassify(1.0f)), int>);
static_assert(same<decltype(::frexp(1.0f, static_cast<int*>(nullptr))), float>);
static_assert(same<decltype(::ldexp(1.0L, 3)), long double> && same<decltype(::ilogb(1.0f)), int>);
static_assert(same<decltype(::lround(1.5f)), long> && same<decltype(::llrint(1.5L)), long long>);
static_assert(same<decltype(::remquo(5.0f, 3.0f, static_cast<int*>(nullptr))), float>);
static_assert(same<decltype(::nextafter(1.0f, 2.0f)), float> && same<decltype(::nexttoward(1.0f, 2.0L)), float>);
static_assert(same<::float_t, std::float_t> && same<::double_t, std::double_t>);
#if defined(__STDCPP_FLOAT32_T__)
static_assert(same<decltype(::sqrt(std::float32_t(1))), std::float32_t>);
static_assert(same<decltype(::fmax(std::float32_t(1), std::float32_t(2))), std::float32_t>);
#endif
#if defined(__STDCPP_FLOAT64_T__)
static_assert(same<decltype(::cos(std::float64_t(1))), std::float64_t>);
#endif

int f() { return ::isnan(::nan("")) && ::signbit(-0.0f) && HUGE_VALF > 0 && FP_NAN != FP_ZERO ? 0 : 1; }
