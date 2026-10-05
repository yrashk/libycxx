// Which of the macros that name <inplace_vector> in [version.syn] are defined after including only
// <inplace_vector> (for version/header_macros_tu.pass.cpp).
#include <inplace_vector>
#include "../version_tu.hpp"

extern const macro_value macros_inplace_vector[] = {
#ifdef __cpp_lib_constexpr_inplace_vector
    {"__cpp_lib_constexpr_inplace_vector", __cpp_lib_constexpr_inplace_vector},
#else
    {"__cpp_lib_constexpr_inplace_vector", 0},
#endif
#ifdef __cpp_lib_inplace_vector
    {"__cpp_lib_inplace_vector", __cpp_lib_inplace_vector},
#else
    {"__cpp_lib_inplace_vector", 0},
#endif
#ifdef __cpp_lib_hardened_inplace_vector
    {"__cpp_lib_hardened_inplace_vector", __cpp_lib_hardened_inplace_vector},
#else
    {"__cpp_lib_hardened_inplace_vector", 0},
#endif
    {nullptr, 0}};
