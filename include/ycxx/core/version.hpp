// libycxx core: library feature-test macros ([version.syn]).
//
// A macro is only defined once the corresponding feature is implemented. Every public header
// includes this file, which is permitted: each header must define *at least* its own macros.
#pragma once

#define __cpp_lib_freestanding_feature_test_macros 202306L

// <cstddef> <new> <exception> <initializer_list>
#define __cpp_lib_byte 201603L
#define __cpp_lib_launder 201606L
#define __cpp_lib_hardware_interference_size 201703L
#define __cpp_lib_destroying_delete 201806L
#define __cpp_lib_constexpr_new 202406L
#define __cpp_lib_uncaught_exceptions 201411L
#define __cpp_lib_exception_ptr_cast 202603L
// P3068/P3378 constexpr exceptions: needs throwing during constant evaluation and exception_ptr
// there (GCC 16; Clang 23 cannot throw during constant evaluation). DECISIONS §4.
#if YCXX_HAS_CONSTEXPR_EXCEPTIONS && YCXX_HAS_CONSTEXPR_EXCEPTION_PTR
#  define __cpp_lib_constexpr_exceptions 202502L
#endif
#define __cpp_lib_initializer_list 202511L

// <compare> <concepts>
#define __cpp_lib_three_way_comparison 201907L
#define __cpp_lib_type_order 202506L
#define __cpp_lib_concepts 202207L

// <utility>
#define __cpp_lib_as_const 201510L
#define __cpp_lib_exchange_function 201304L
#define __cpp_lib_forward_like 202207L
#define __cpp_lib_integer_comparison_functions 202002L
#define __cpp_lib_integer_sequence 202511L
#define __cpp_lib_to_underlying 202102L
#define __cpp_lib_unreachable 202202L
#define __cpp_lib_constexpr_utility 201811L
#define __cpp_lib_tuples_by_type 201304L
#define __cpp_lib_tuple_like 202311L
#define __cpp_lib_apply 202603L
#define __cpp_lib_observable_checkpoint 202506L
#define __cpp_lib_freestanding_utility 202306L
#define __cpp_lib_constrained_equality 202411L
#define __cpp_lib_invoke 201411L
#define __cpp_lib_invoke_r 202106L
#define __cpp_lib_result_of_sfinae 201210L

// <source_location> <coroutine> <memory>
#define __cpp_lib_source_location 201907L
#define __cpp_lib_coroutine 201902L
#define __cpp_lib_addressof_constexpr 201603L
#define __cpp_lib_to_address 201711L
#define __cpp_lib_assume_aligned 201811L
#define __cpp_lib_is_sufficiently_aligned 202411L
#define __cpp_lib_start_lifetime_as 202207L
// start_lifetime must leave an object already within its lifetime alone ([obj.lifetime]/2); in
// constant evaluation that needs __builtin_is_within_lifetime.
#if YCXX_HAS_IS_WITHIN_LIFETIME
#  define __cpp_lib_start_lifetime 202603L
#endif
#define __cpp_lib_freestanding_memory 202502L
#define __cpp_lib_allocate_at_least 202302L
#define __cpp_lib_allocator_traits_is_always_equal 201411L
#define __cpp_lib_constexpr_dynamic_alloc 201907L
#define __cpp_lib_raw_memory_algorithms 202411L
#define __cpp_lib_make_unique 201304L
#define __cpp_lib_smart_ptr_for_overwrite 202002L
#define __cpp_lib_shared_ptr_arrays 201707L
#define __cpp_lib_shared_ptr_weak_type 201606L
#define __cpp_lib_enable_shared_from_this 201603L
#define __cpp_lib_smart_ptr_owner_equality 202306L
#define __cpp_lib_hardened_shared_ptr_array 202506L
#define __cpp_lib_constexpr_memory 202506L
#define __cpp_lib_out_ptr 202311L
#define __cpp_lib_indirect 202502L
#define __cpp_lib_polymorphic 202502L
#define __cpp_lib_parallel_algorithm 202506L
#define __cpp_lib_transparent_operators 201510L

// <array>
#define __cpp_lib_array_constexpr 201811L
#define __cpp_lib_to_array 201907L
#define __cpp_lib_freestanding_array 202311L

// <optional>
#define __cpp_lib_optional 202506L
#define __cpp_lib_optional_range_support 202406L
#define __cpp_lib_freestanding_optional 202506L

// <variant>
#define __cpp_lib_variant 202306L
#define __cpp_lib_freestanding_variant 202311L

// <expected>
#define __cpp_lib_expected 202606L
#define __cpp_lib_freestanding_expected 202311L

// <span>
#define __cpp_lib_span 202311L

// <mdspan>
#define __cpp_lib_mdspan 202406L
#define __cpp_lib_freestanding_mdspan 202311L
#define __cpp_lib_submdspan 202603L
#define __cpp_lib_aligned_accessor 202411L
#define __cpp_lib_mdspan_copy 202606L

// <linalg>
#define __cpp_lib_linalg 202511L

// <any>
#define __cpp_lib_any 201606L

// <memory_resource>
#define __cpp_lib_memory_resource 201603L
#define __cpp_lib_polymorphic_allocator 201902L

// <string_view>, <string>
#define __cpp_lib_string_view 202403L
#define __cpp_lib_constexpr_string_view 201811L
#define __cpp_lib_freestanding_string_view 202311L
#define __cpp_lib_freestanding_char_traits 202306L
#define __cpp_lib_starts_ends_with 201711L
#define __cpp_lib_string_contains 202011L
#define __cpp_lib_string_subview 202506L
#define __cpp_lib_constexpr_string 202511L
#define __cpp_lib_string_resize_and_overwrite 202110L
#define __cpp_lib_string_udls 201304L
#define __cpp_lib_to_string 202306L

// <vector> <inplace_vector>
#define __cpp_lib_constexpr_vector 201907L
#define __cpp_lib_inplace_vector 202603L
#define __cpp_lib_constexpr_inplace_vector 202502L

// <bitset>
#define __cpp_lib_bitset 202306L
#define __cpp_lib_constexpr_bitset 202207L

// <charconv>, errc
#define __cpp_lib_to_chars 202606L
#define __cpp_lib_constexpr_charconv 202207L
#define __cpp_lib_freestanding_charconv 202306L
#define __cpp_lib_freestanding_errc 202306L

// <bit>
#define __cpp_lib_bit_cast 201806L
#define __cpp_lib_bitops 202607L
#define __cpp_lib_byteswap 202110L
#define __cpp_lib_endian 201907L
#define __cpp_lib_int_pow2 202002L

// <type_traits>
#define __cpp_lib_bool_constant 201505L
#define __cpp_lib_bounded_array_traits 201902L
#define __cpp_lib_common_reference 202302L
#define __cpp_lib_has_unique_object_representations 201606L
#define __cpp_lib_integral_constant_callable 201304L
#define __cpp_lib_is_aggregate 201703L
#define __cpp_lib_is_constant_evaluated 201811L
#define __cpp_lib_is_final 201402L
#define __cpp_lib_is_implicit_lifetime 202302L
#define __cpp_lib_is_invocable 201703L
#define __cpp_lib_is_layout_compatible 201907L
#define __cpp_lib_is_nothrow_convertible 201806L
#define __cpp_lib_is_null_pointer 201309L
#define __cpp_lib_is_scoped_enum 202011L
#if YCXX_HAS_IS_STRUCTURAL
#  define __cpp_lib_is_structural 202603L
#endif
#define __cpp_lib_is_swappable 201603L
#define __cpp_lib_is_virtual_base_of 202406L
#if YCXX_HAS_IS_WITHIN_LIFETIME
#  define __cpp_lib_is_within_lifetime 202603L
#endif
#define __cpp_lib_logical_traits 201510L
#define __cpp_lib_reference_from_temporary 202202L
#define __cpp_lib_remove_cvref 201711L
#define __cpp_lib_transformation_trait_aliases 201304L
#define __cpp_lib_type_identity 201806L
#define __cpp_lib_type_trait_variable_templates 201510L
#define __cpp_lib_unwrap_ref 201811L
#define __cpp_lib_void_t 201411L
#if YCXX_HAS_MEMBER_INTERCONVERTIBILITY
#  define __cpp_lib_is_pointer_interconvertible 201907L
#endif


// <algorithm> <numeric> <execution>
#define __cpp_lib_algorithm_default_value_type 202603L
#define __cpp_lib_algorithm_iterator_requirements 202207L
#define __cpp_lib_clamp 201603L
#define __cpp_lib_constexpr_algorithms 202306L
#define __cpp_lib_constexpr_numeric 201911L
#define __cpp_lib_execution 201902L
#define __cpp_lib_freestanding_execution 202502L
#define __cpp_lib_freestanding_algorithm 202502L
#define __cpp_lib_freestanding_numeric 202502L
#define __cpp_lib_gcd_lcm 201606L
#define __cpp_lib_ranges_contains 202207L
#define __cpp_lib_ranges_find_last 202207L
#define __cpp_lib_ranges_fold 202207L
#define __cpp_lib_ranges_iota 202202L
#define __cpp_lib_ranges_starts_ends_with 202106L
// <execution>: senders and receivers ([exec]); core, but parallel_scheduler's default backend is
// in the hosted runtime.
#define __cpp_lib_senders 202506L
#define __cpp_lib_counting_scope 202506L
#define __cpp_lib_task 202506L
#if YCXX_HOSTED
#  define __cpp_lib_parallel_scheduler 202506L
#endif
// <ranges>
#define __cpp_lib_ranges 202406L
#define __cpp_lib_freestanding_ranges 202306L
#define __cpp_lib_ranges_as_const 202311L
#define __cpp_lib_ranges_as_input 202502L
#define __cpp_lib_ranges_as_rvalue 202207L
// <generator>
#define __cpp_lib_generator 202207L
// <debugging>
#define __cpp_lib_debugging 202403L
// <text_encoding>
#define __cpp_lib_text_encoding 202306L
// <stacktrace>, <thread> formatters
#define __cpp_lib_formatters 202302L
#define __cpp_lib_stacktrace 202011L
#define __cpp_lib_hardened_basic_stacktrace 202506L
// The modules std and std.compat ([std.modules]): modules/std.cppm, modules/std.compat.cppm,
// built by the CMake package (ycxx::modules) or tools/ycxx-modules (DECISIONS §16).
#define __cpp_lib_modules 202207L
// <contracts>: the language feature is the compiler's (GCC 16; not Clang 23)
#if YCXX_HAS_CONTRACTS
#  define __cpp_lib_contracts 202502L
#endif
// <meta>: reflection is the compiler's (GCC 16 with -freflection; not Clang 23)
#if YCXX_HAS_REFLECTION
#  define __cpp_lib_reflection 202603L
#  define __cpp_lib_define_static 202506L
#endif
#define __cpp_lib_ranges_cache_latest 202411L
#define __cpp_lib_ranges_cartesian_product 202207L
#define __cpp_lib_ranges_chunk 202202L
#define __cpp_lib_ranges_chunk_by 202202L
#define __cpp_lib_ranges_concat 202403L
#define __cpp_lib_ranges_enumerate 202302L
#define __cpp_lib_ranges_filter 202603L
#define __cpp_lib_ranges_indices 202506L
#define __cpp_lib_ranges_join_with 202202L
#define __cpp_lib_ranges_repeat 202207L
#define __cpp_lib_ranges_reserve_hint 202502L
#define __cpp_lib_ranges_slide 202202L
#define __cpp_lib_ranges_stride 202207L
#define __cpp_lib_ranges_to_container 202202L
#define __cpp_lib_ranges_zip 202110L
#define __cpp_lib_robust_nonmodifying_seq_ops 201304L
#define __cpp_lib_sample 201603L
#define __cpp_lib_saturation_arithmetic 202603L
#define __cpp_lib_shift 202202L

// <deque> <list> <forward_list> <stack> <queue>
#define __cpp_lib_adaptor_iterator_pair_constructor 202106L
#define __cpp_lib_constexpr_deque 202502L
#define __cpp_lib_constexpr_forward_list 202502L
#define __cpp_lib_constexpr_list 202502L
#define __cpp_lib_constexpr_queue 202502L
#define __cpp_lib_constexpr_stack 202502L
#define __cpp_lib_list_remove_return_type 201806L

// <map> <set>
#define __cpp_lib_constexpr_map 202502L
#define __cpp_lib_constexpr_set 202502L
#define __cpp_lib_generic_associative_lookup 201304L

// <flat_map> <flat_set>
#define __cpp_lib_constexpr_flat_map 202502L
#define __cpp_lib_constexpr_flat_set 202502L
#define __cpp_lib_flat_map 202511L
#define __cpp_lib_flat_set 202511L

// Shared by several container headers, all of which now provide the feature.
#define __cpp_lib_associative_heterogeneous_erasure 202110L
#define __cpp_lib_associative_heterogeneous_insertion 202306L
#define __cpp_lib_containers_ranges 202202L
#define __cpp_lib_erase_if 202002L
#define __cpp_lib_incomplete_container_elements 201505L
#define __cpp_lib_map_lookup 202606L
#define __cpp_lib_map_try_emplace 201411L
#define __cpp_lib_node_extract 201606L
#define __cpp_lib_nonmember_container_access 201411L

// <unordered_map> <unordered_set>
#define __cpp_lib_constexpr_unordered_map 202502L
#define __cpp_lib_constexpr_unordered_set 202502L
#define __cpp_lib_generic_unordered_lookup 201811L
#define __cpp_lib_unordered_map_try_emplace 201411L

// <hive>
#define __cpp_lib_hive 202502L

// Features of headers above that had no macro yet.
#define __cpp_lib_ssize 201902L
#define __cpp_lib_null_iterators 201304L
#define __cpp_lib_make_reverse_iterator 201402L
#define __cpp_lib_move_iterator_concept 202207L
#define __cpp_lib_constexpr_iterator 201811L
#define __cpp_lib_freestanding_iterator 202306L
#define __cpp_lib_make_from_tuple 201606L
#define __cpp_lib_constexpr_tuple 201811L
#define __cpp_lib_freestanding_tuple 202306L
#define __cpp_lib_tuple_element_t 201402L
#define __cpp_lib_reference_wrapper 202403L
#define __cpp_lib_bind_front 202306L
#define __cpp_lib_bind_back 202306L
#define __cpp_lib_constexpr_functional 201907L
#define __cpp_lib_boyer_moore_searcher 201603L
#define __cpp_lib_freestanding_functional 202306L
#define __cpp_lib_not_fn 202306L
#define __cpp_lib_constant_wrapper 202606L
#define __cpp_lib_move_only_function 202110L
#define __cpp_lib_copyable_function 202306L
#define __cpp_lib_function_ref 202604L
#define __cpp_lib_common_reference_wrapper 202302L
#define __cpp_lib_constexpr_typeinfo 202106L
// <atomic> <stdatomic.h>
#define __cpp_lib_atomic_flag_test 201907L
#define __cpp_lib_atomic_float 201711L
#define __cpp_lib_atomic_is_always_lock_free 201603L
#define __cpp_lib_atomic_lock_free_type_aliases 201907L
#define __cpp_lib_atomic_min_max 202506L
#define __cpp_lib_atomic_reductions 202506L
#define __cpp_lib_atomic_ref 202603L
#define __cpp_lib_atomic_value_initialization 201911L
#define __cpp_lib_atomic_wait 201907L
#define __cpp_lib_constexpr_atomic 202411L
#define __cpp_lib_atomic_shared_ptr 201711L
#define __cpp_lib_stdatomic_h 202011L
// [version.syn]/4: 202306L when the default allocation functions are those of a hosted
// implementation; 0 for libycxx-freestanding.a, whose defaults have no heap.
#if YCXX_HOSTED
// <thread> <stop_token> <mutex> <shared_mutex> <semaphore> <latch> <barrier>
#  define __cpp_lib_jthread 201911L
#  define __cpp_lib_thread_attributes 202606L
#  define __cpp_lib_scoped_lock 201703L
#  define __cpp_lib_shared_mutex 201505L
#  define __cpp_lib_shared_timed_mutex 201402L
#  define __cpp_lib_semaphore 201907L
#  define __cpp_lib_latch 201907L
#  define __cpp_lib_barrier 202302L
// <rcu> <hazard_pointer>
#  define __cpp_lib_rcu 202306L
#  define __cpp_lib_hazard_pointer 202606L
#  define __cpp_lib_freestanding_operator_new 202306L
#else
#  define __cpp_lib_freestanding_operator_new 0
#endif
// The freestanding parts of <cstdlib>/<cstring>/<cwchar>: the C library's when hosted,
// ycxx/core/c_stdlib.hpp and c_string.hpp otherwise.
#define __cpp_lib_freestanding_cstring 202311L
#define __cpp_lib_freestanding_cwchar 202306L
#define __cpp_lib_freestanding_cstdlib 202306L
// <stdbit.h>
#define __cpp_lib_stdbit_h 202603L
// <stdckdint.h>
#define __cpp_lib_stdckdint_h 202603L

// [version.syn]/3: defined only by a hardened implementation (YCXX_HARDENED=1), for the headers
// whose hardened preconditions are checked.
#if YCXX_HARDENED
#  define __cpp_lib_hardened_array 202502L
#  define __cpp_lib_hardened_basic_string 202502L
#  define __cpp_lib_hardened_basic_string_view 202502L
#  define __cpp_lib_hardened_bitset 202502L
#  define __cpp_lib_hardened_deque 202502L
#  define __cpp_lib_hardened_expected 202502L
#  define __cpp_lib_hardened_forward_list 202502L
#  define __cpp_lib_hardened_list 202502L
#  define __cpp_lib_hardened_optional 202502L
#  define __cpp_lib_hardened_span 202502L
#  define __cpp_lib_hardened_mdspan 202502L
#  define __cpp_lib_hardened_vector 202502L
#  define __cpp_lib_hardened_inplace_vector 202502L
#  define __cpp_lib_hardened_valarray 202502L
#endif

// <ratio> <numbers> <cmath> <complex> <valarray>
#define __cpp_lib_ratio 202306L
#define __cpp_lib_freestanding_ratio 202306L
#define __cpp_lib_math_constants 201907L
#define __cpp_lib_constexpr_cmath 202306L
#define __cpp_lib_hypot 201603L
#define __cpp_lib_interpolate 201902L
#define __cpp_lib_math_special_functions 201603L
#define __cpp_lib_complex_udls 201309L
#define __cpp_lib_constexpr_complex 202306L
#define __cpp_lib_valarray 202511L

// <simd>
#define __cpp_lib_simd 202606L
#define __cpp_lib_simd_bitops 202607L
#define __cpp_lib_simd_complex 202502L
#define __cpp_lib_simd_permutations 202506L

// <ios> <istream> <ostream> <sstream> <spanstream> <fstream> <syncstream> <iomanip> <locale>
#define __cpp_lib_char8_t 201907L
#define __cpp_lib_ios_noreplace 202207L
#define __cpp_lib_fstream_native_handle 202306L
#define __cpp_lib_quoted_string_io 201304L
#define __cpp_lib_spanstream 202106L
#define __cpp_lib_sstream_from_string_view 202306L
#define __cpp_lib_syncbuf 201803L

// <format> <print>
#define __cpp_lib_format 202603L
#define __cpp_lib_format_ranges 202207L
#define __cpp_lib_format_uchar 202311L
#define __cpp_lib_constexpr_format 202511L
#define __cpp_lib_print 202406L

// <filesystem>
#define __cpp_lib_format_path 202506L
#define __cpp_lib_filesystem 201703L
// <random>
#define __cpp_lib_freestanding_random 202502L
#define __cpp_lib_philox_engine 202406L
#define __cpp_lib_ranges_generate_random 202403L

// <chrono>
#define __cpp_lib_chrono 202306L
#define __cpp_lib_chrono_udls 201304L
