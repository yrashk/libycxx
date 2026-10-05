// Which of the macros that name <algorithm> in [version.syn] are defined after including only
// <algorithm> (for version/header_macros_tu.pass.cpp).
#include <algorithm>
#include "../version_tu.hpp"

extern const macro_value macros_algorithm[] = {
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
#ifdef __cpp_lib_freestanding_algorithm
    {"__cpp_lib_freestanding_algorithm", __cpp_lib_freestanding_algorithm},
#else
    {"__cpp_lib_freestanding_algorithm", 0},
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
#ifdef __cpp_lib_ranges_contains
    {"__cpp_lib_ranges_contains", __cpp_lib_ranges_contains},
#else
    {"__cpp_lib_ranges_contains", 0},
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
#ifdef __cpp_lib_ranges_starts_ends_with
    {"__cpp_lib_ranges_starts_ends_with", __cpp_lib_ranges_starts_ends_with},
#else
    {"__cpp_lib_ranges_starts_ends_with", 0},
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
#ifdef __cpp_lib_shift
    {"__cpp_lib_shift", __cpp_lib_shift},
#else
    {"__cpp_lib_shift", 0},
#endif
    {nullptr, 0}};
