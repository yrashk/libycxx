// Which of the macros that name <ranges> in [version.syn] are defined after including only
// <ranges> (for version/header_macros_tu.pass.cpp).
#include <ranges>
#include "../version_tu.hpp"

extern const macro_value macros_ranges[] = {
#ifdef __cpp_lib_algorithm_default_value_type
    {"__cpp_lib_algorithm_default_value_type", __cpp_lib_algorithm_default_value_type},
#else
    {"__cpp_lib_algorithm_default_value_type", 0},
#endif
#ifdef __cpp_lib_freestanding_ranges
    {"__cpp_lib_freestanding_ranges", __cpp_lib_freestanding_ranges},
#else
    {"__cpp_lib_freestanding_ranges", 0},
#endif
#ifdef __cpp_lib_ranges
    {"__cpp_lib_ranges", __cpp_lib_ranges},
#else
    {"__cpp_lib_ranges", 0},
#endif
#ifdef __cpp_lib_ranges_as_const
    {"__cpp_lib_ranges_as_const", __cpp_lib_ranges_as_const},
#else
    {"__cpp_lib_ranges_as_const", 0},
#endif
#ifdef __cpp_lib_ranges_as_input
    {"__cpp_lib_ranges_as_input", __cpp_lib_ranges_as_input},
#else
    {"__cpp_lib_ranges_as_input", 0},
#endif
#ifdef __cpp_lib_ranges_as_rvalue
    {"__cpp_lib_ranges_as_rvalue", __cpp_lib_ranges_as_rvalue},
#else
    {"__cpp_lib_ranges_as_rvalue", 0},
#endif
#ifdef __cpp_lib_ranges_cache_latest
    {"__cpp_lib_ranges_cache_latest", __cpp_lib_ranges_cache_latest},
#else
    {"__cpp_lib_ranges_cache_latest", 0},
#endif
#ifdef __cpp_lib_ranges_cartesian_product
    {"__cpp_lib_ranges_cartesian_product", __cpp_lib_ranges_cartesian_product},
#else
    {"__cpp_lib_ranges_cartesian_product", 0},
#endif
#ifdef __cpp_lib_ranges_chunk
    {"__cpp_lib_ranges_chunk", __cpp_lib_ranges_chunk},
#else
    {"__cpp_lib_ranges_chunk", 0},
#endif
#ifdef __cpp_lib_ranges_chunk_by
    {"__cpp_lib_ranges_chunk_by", __cpp_lib_ranges_chunk_by},
#else
    {"__cpp_lib_ranges_chunk_by", 0},
#endif
#ifdef __cpp_lib_ranges_concat
    {"__cpp_lib_ranges_concat", __cpp_lib_ranges_concat},
#else
    {"__cpp_lib_ranges_concat", 0},
#endif
#ifdef __cpp_lib_ranges_enumerate
    {"__cpp_lib_ranges_enumerate", __cpp_lib_ranges_enumerate},
#else
    {"__cpp_lib_ranges_enumerate", 0},
#endif
#ifdef __cpp_lib_ranges_filter
    {"__cpp_lib_ranges_filter", __cpp_lib_ranges_filter},
#else
    {"__cpp_lib_ranges_filter", 0},
#endif
#ifdef __cpp_lib_ranges_indices
    {"__cpp_lib_ranges_indices", __cpp_lib_ranges_indices},
#else
    {"__cpp_lib_ranges_indices", 0},
#endif
#ifdef __cpp_lib_ranges_join_with
    {"__cpp_lib_ranges_join_with", __cpp_lib_ranges_join_with},
#else
    {"__cpp_lib_ranges_join_with", 0},
#endif
#ifdef __cpp_lib_ranges_repeat
    {"__cpp_lib_ranges_repeat", __cpp_lib_ranges_repeat},
#else
    {"__cpp_lib_ranges_repeat", 0},
#endif
#ifdef __cpp_lib_ranges_reserve_hint
    {"__cpp_lib_ranges_reserve_hint", __cpp_lib_ranges_reserve_hint},
#else
    {"__cpp_lib_ranges_reserve_hint", 0},
#endif
#ifdef __cpp_lib_ranges_slide
    {"__cpp_lib_ranges_slide", __cpp_lib_ranges_slide},
#else
    {"__cpp_lib_ranges_slide", 0},
#endif
#ifdef __cpp_lib_ranges_stride
    {"__cpp_lib_ranges_stride", __cpp_lib_ranges_stride},
#else
    {"__cpp_lib_ranges_stride", 0},
#endif
#ifdef __cpp_lib_ranges_to_container
    {"__cpp_lib_ranges_to_container", __cpp_lib_ranges_to_container},
#else
    {"__cpp_lib_ranges_to_container", 0},
#endif
#ifdef __cpp_lib_ranges_zip
    {"__cpp_lib_ranges_zip", __cpp_lib_ranges_zip},
#else
    {"__cpp_lib_ranges_zip", 0},
#endif
#ifdef __cpp_lib_view_interface
    {"__cpp_lib_view_interface", __cpp_lib_view_interface},
#else
    {"__cpp_lib_view_interface", 0},
#endif
#ifdef __cpp_lib_hardened_view_interface
    {"__cpp_lib_hardened_view_interface", __cpp_lib_hardened_view_interface},
#else
    {"__cpp_lib_hardened_view_interface", 0},
#endif
    {nullptr, 0}};
