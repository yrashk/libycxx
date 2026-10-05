// Which of the macros that name <numeric> in [version.syn] are defined after including only
// <numeric> (for version/header_macros_tu.pass.cpp).
#include <numeric>
#include "../version_tu.hpp"

extern const macro_value macros_numeric[] = {
#ifdef __cpp_lib_algorithm_iterator_requirements
    {"__cpp_lib_algorithm_iterator_requirements", __cpp_lib_algorithm_iterator_requirements},
#else
    {"__cpp_lib_algorithm_iterator_requirements", 0},
#endif
#ifdef __cpp_lib_constexpr_numeric
    {"__cpp_lib_constexpr_numeric", __cpp_lib_constexpr_numeric},
#else
    {"__cpp_lib_constexpr_numeric", 0},
#endif
#ifdef __cpp_lib_freestanding_numeric
    {"__cpp_lib_freestanding_numeric", __cpp_lib_freestanding_numeric},
#else
    {"__cpp_lib_freestanding_numeric", 0},
#endif
#ifdef __cpp_lib_gcd_lcm
    {"__cpp_lib_gcd_lcm", __cpp_lib_gcd_lcm},
#else
    {"__cpp_lib_gcd_lcm", 0},
#endif
#ifdef __cpp_lib_interpolate
    {"__cpp_lib_interpolate", __cpp_lib_interpolate},
#else
    {"__cpp_lib_interpolate", 0},
#endif
#ifdef __cpp_lib_parallel_algorithm
    {"__cpp_lib_parallel_algorithm", __cpp_lib_parallel_algorithm},
#else
    {"__cpp_lib_parallel_algorithm", 0},
#endif
#ifdef __cpp_lib_ranges_iota
    {"__cpp_lib_ranges_iota", __cpp_lib_ranges_iota},
#else
    {"__cpp_lib_ranges_iota", 0},
#endif
#ifdef __cpp_lib_saturation_arithmetic
    {"__cpp_lib_saturation_arithmetic", __cpp_lib_saturation_arithmetic},
#else
    {"__cpp_lib_saturation_arithmetic", 0},
#endif
    {nullptr, 0}};
