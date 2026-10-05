// Which of the macros that name <format> in [version.syn] are defined after including only
// <format> (for version/header_macros_tu.pass.cpp).
#include <format>
#include "../version_tu.hpp"

extern const macro_value macros_format[] = {
#ifdef __cpp_lib_constexpr_exceptions
    {"__cpp_lib_constexpr_exceptions", __cpp_lib_constexpr_exceptions},
#else
    {"__cpp_lib_constexpr_exceptions", 0},
#endif
#ifdef __cpp_lib_constexpr_format
    {"__cpp_lib_constexpr_format", __cpp_lib_constexpr_format},
#else
    {"__cpp_lib_constexpr_format", 0},
#endif
#ifdef __cpp_lib_format
    {"__cpp_lib_format", __cpp_lib_format},
#else
    {"__cpp_lib_format", 0},
#endif
#ifdef __cpp_lib_format_ranges
    {"__cpp_lib_format_ranges", __cpp_lib_format_ranges},
#else
    {"__cpp_lib_format_ranges", 0},
#endif
#ifdef __cpp_lib_format_uchar
    {"__cpp_lib_format_uchar", __cpp_lib_format_uchar},
#else
    {"__cpp_lib_format_uchar", 0},
#endif
    {nullptr, 0}};
