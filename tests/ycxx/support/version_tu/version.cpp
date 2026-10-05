// The same macros after including only <version> (for version/header_macros_tu.pass.cpp).
#include <version>
#include "../version_tu.hpp"

extern const macro_value macros_version[] = {
#ifdef __cpp_lib_algorithm_default_value_type
    {"__cpp_lib_algorithm_default_value_type", __cpp_lib_algorithm_default_value_type},
#else
    {"__cpp_lib_algorithm_default_value_type", 0},
#endif
#ifdef __cpp_lib_algorithm_iterator_requirements
    {"__cpp_lib_algorithm_iterator_requirements", __cpp_lib_algorithm_iterator_requirements},
#else
    {"__cpp_lib_algorithm_iterator_requirements", 0},
#endif
#ifdef __cpp_lib_clamp
    {"__cpp_lib_clamp", __cpp_lib_clamp},
#else
    {"__cpp_lib_clamp", 0},
#endif
#ifdef __cpp_lib_constexpr_algorithms
    {"__cpp_lib_constexpr_algorithms", __cpp_lib_constexpr_algorithms},
#else
    {"__cpp_lib_constexpr_algorithms", 0},
#endif
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
#ifdef __cpp_lib_constexpr_inplace_vector
    {"__cpp_lib_constexpr_inplace_vector", __cpp_lib_constexpr_inplace_vector},
#else
    {"__cpp_lib_constexpr_inplace_vector", 0},
#endif
#ifdef __cpp_lib_constexpr_numeric
    {"__cpp_lib_constexpr_numeric", __cpp_lib_constexpr_numeric},
#else
    {"__cpp_lib_constexpr_numeric", 0},
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
#ifdef __cpp_lib_freestanding_algorithm
    {"__cpp_lib_freestanding_algorithm", __cpp_lib_freestanding_algorithm},
#else
    {"__cpp_lib_freestanding_algorithm", 0},
#endif
#ifdef __cpp_lib_freestanding_numeric
    {"__cpp_lib_freestanding_numeric", __cpp_lib_freestanding_numeric},
#else
    {"__cpp_lib_freestanding_numeric", 0},
#endif
#ifdef __cpp_lib_freestanding_ranges
    {"__cpp_lib_freestanding_ranges", __cpp_lib_freestanding_ranges},
#else
    {"__cpp_lib_freestanding_ranges", 0},
#endif
#ifdef __cpp_lib_gcd_lcm
    {"__cpp_lib_gcd_lcm", __cpp_lib_gcd_lcm},
#else
    {"__cpp_lib_gcd_lcm", 0},
#endif
#ifdef __cpp_lib_hardened_inplace_vector
    {"__cpp_lib_hardened_inplace_vector", __cpp_lib_hardened_inplace_vector},
#else
    {"__cpp_lib_hardened_inplace_vector", 0},
#endif
#ifdef __cpp_lib_hardened_view_interface
    {"__cpp_lib_hardened_view_interface", __cpp_lib_hardened_view_interface},
#else
    {"__cpp_lib_hardened_view_interface", 0},
#endif
#ifdef __cpp_lib_inplace_vector
    {"__cpp_lib_inplace_vector", __cpp_lib_inplace_vector},
#else
    {"__cpp_lib_inplace_vector", 0},
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
#ifdef __cpp_lib_ranges_contains
    {"__cpp_lib_ranges_contains", __cpp_lib_ranges_contains},
#else
    {"__cpp_lib_ranges_contains", 0},
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
#ifdef __cpp_lib_ranges_find_last
    {"__cpp_lib_ranges_find_last", __cpp_lib_ranges_find_last},
#else
    {"__cpp_lib_ranges_find_last", 0},
#endif
#ifdef __cpp_lib_ranges_fold
    {"__cpp_lib_ranges_fold", __cpp_lib_ranges_fold},
#else
    {"__cpp_lib_ranges_fold", 0},
#endif
#ifdef __cpp_lib_ranges_indices
    {"__cpp_lib_ranges_indices", __cpp_lib_ranges_indices},
#else
    {"__cpp_lib_ranges_indices", 0},
#endif
#ifdef __cpp_lib_ranges_iota
    {"__cpp_lib_ranges_iota", __cpp_lib_ranges_iota},
#else
    {"__cpp_lib_ranges_iota", 0},
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
#ifdef __cpp_lib_ranges_starts_ends_with
    {"__cpp_lib_ranges_starts_ends_with", __cpp_lib_ranges_starts_ends_with},
#else
    {"__cpp_lib_ranges_starts_ends_with", 0},
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
#ifdef __cpp_lib_robust_nonmodifying_seq_ops
    {"__cpp_lib_robust_nonmodifying_seq_ops", __cpp_lib_robust_nonmodifying_seq_ops},
#else
    {"__cpp_lib_robust_nonmodifying_seq_ops", 0},
#endif
#ifdef __cpp_lib_sample
    {"__cpp_lib_sample", __cpp_lib_sample},
#else
    {"__cpp_lib_sample", 0},
#endif
#ifdef __cpp_lib_saturation_arithmetic
    {"__cpp_lib_saturation_arithmetic", __cpp_lib_saturation_arithmetic},
#else
    {"__cpp_lib_saturation_arithmetic", 0},
#endif
#ifdef __cpp_lib_shift
    {"__cpp_lib_shift", __cpp_lib_shift},
#else
    {"__cpp_lib_shift", 0},
#endif
#ifdef __cpp_lib_text_encoding
    {"__cpp_lib_text_encoding", __cpp_lib_text_encoding},
#else
    {"__cpp_lib_text_encoding", 0},
#endif
#ifdef __cpp_lib_view_interface
    {"__cpp_lib_view_interface", __cpp_lib_view_interface},
#else
    {"__cpp_lib_view_interface", 0},
#endif
    {nullptr, 0}};
