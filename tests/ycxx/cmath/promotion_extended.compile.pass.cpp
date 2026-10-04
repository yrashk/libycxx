// [cmath.syn]/2: an overload for each cv-unqualified floating-point type, which includes the
// extended floating-point types ([basic.extended.fp]). /3: with mixed arguments the common
// type has the greatest conversion rank and subrank; an extended type whose values are those of
// a standard type has the same rank and a greater subrank ([conv.rank]/2), integer arguments
// count as double, and if no such greatest type exists no usable candidate results.
#include <cmath>
#include <stdfloat>
#include <type_traits>

using std::is_same_v;
template <class A, class B>
concept powable = requires(A a, B b) { std::pow(a, b); };

#if defined(__STDCPP_FLOAT32_T__) && defined(__STDCPP_FLOAT64_T__)
static_assert(is_same_v<decltype(std::sqrt(std::float32_t(4))), std::float32_t>);
static_assert(is_same_v<decltype(std::pow(std::float32_t(2), 3.0f)), std::float32_t>);  // equal rank, greater subrank
static_assert(is_same_v<decltype(std::pow(std::float32_t(2), 3.0)), double>);
static_assert(is_same_v<decltype(std::pow(std::float64_t(2), 3.0)), std::float64_t>);
static_assert(is_same_v<decltype(std::pow(std::float64_t(2), 3)), std::float64_t>);   // int counts as double
static_assert(is_same_v<decltype(std::fma(std::float32_t(1), std::float64_t(1), 1.0f)), std::float64_t>);
static_assert(is_same_v<decltype(std::fabs(std::float64_t(-1))), std::float64_t>);
static_assert(is_same_v<decltype(std::lerp(std::float32_t(1), std::float32_t(2), std::float32_t(0.5))), std::float32_t>);
#endif
#if defined(__STDCPP_FLOAT16_T__) && defined(__STDCPP_BFLOAT16_T__)
// float16_t and bfloat16_t have unordered ranks: no type of greatest rank exists.
static_assert(!powable<std::float16_t, std::bfloat16_t>);
static_assert(is_same_v<decltype(std::pow(std::float16_t(1), std::float16_t(1))), std::float16_t>);
static_assert(is_same_v<decltype(std::pow(std::bfloat16_t(1), 1.0f)), float>);
#endif
#if defined(__STDCPP_FLOAT128_T__)
static_assert(is_same_v<decltype(std::sqrt(std::float128_t(4))), std::float128_t>);
static_assert(is_same_v<decltype(std::atan2(std::float128_t(1), 1.0)), std::float128_t>);
#endif
static_assert(powable<float, int>);
