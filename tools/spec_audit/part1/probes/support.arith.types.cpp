// [support.arith.types], [numeric.limits.general]/? : <stdfloat>'s extended floating-point types
// exist where the implementation defines __STDCPP_<T>_T__, and numeric_limits is specialized for
// every arithmetic type ([numeric.limits.general]/5 "for each arithmetic type"), which includes
// the extended floating-point types ([basic.fundamental]).
// FREESTANDING
#include <stdfloat>
#include <limits>
#include <type_traits>

template <class T>
constexpr bool limits_ok = std::numeric_limits<T>::is_specialized && !std::numeric_limits<T>::is_integer &&
                           std::numeric_limits<T>::radix == 2 &&
                           std::numeric_limits<T>::max() > T(1) && std::numeric_limits<T>::epsilon() > T(0);
#if defined(__STDCPP_FLOAT16_T__)
static_assert(std::is_floating_point_v<std::float16_t> && limits_ok<std::float16_t>);
static_assert(std::numeric_limits<std::float16_t>::digits == 11 && std::numeric_limits<std::float16_t>::max_exponent == 16);
#endif
#if defined(__STDCPP_FLOAT32_T__)
static_assert(limits_ok<std::float32_t> && std::numeric_limits<std::float32_t>::digits == 24);
#endif
#if defined(__STDCPP_FLOAT64_T__)
static_assert(limits_ok<std::float64_t> && std::numeric_limits<std::float64_t>::digits == 53);
#endif
#if defined(__STDCPP_FLOAT128_T__)
static_assert(limits_ok<std::float128_t> && std::numeric_limits<std::float128_t>::digits == 113);
#endif
#if defined(__STDCPP_BFLOAT16_T__)
static_assert(limits_ok<std::bfloat16_t> && std::numeric_limits<std::bfloat16_t>::digits == 8);
#endif
