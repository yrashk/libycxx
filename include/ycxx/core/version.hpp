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
#define __cpp_lib_observable_checkpoint 202506L
#define __cpp_lib_freestanding_utility 202306L
#define __cpp_lib_constrained_equality 202411L
#define __cpp_lib_invoke 201411L
#define __cpp_lib_invoke_r 202106L

// <source_location> <coroutine> <memory>
#define __cpp_lib_source_location 201907L
#define __cpp_lib_coroutine 201902L
#define __cpp_lib_addressof_constexpr 201603L
#define __cpp_lib_to_address 201711L
#define __cpp_lib_assume_aligned 201811L
#define __cpp_lib_is_sufficiently_aligned 202411L
#define __cpp_lib_start_lifetime_as 202207L
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

// <any>
#define __cpp_lib_any 201606L

// <string_view>, <string> (char_traits)
#define __cpp_lib_string_view 202403L
#define __cpp_lib_constexpr_string_view 201811L
#define __cpp_lib_freestanding_string_view 202311L
#define __cpp_lib_freestanding_char_traits 202306L
#define __cpp_lib_starts_ends_with 201711L
#define __cpp_lib_string_contains 202011L
#define __cpp_lib_string_subview 202506L

// <bitset>
#define __cpp_lib_bitset 202306L
#define __cpp_lib_constexpr_bitset 202207L

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
#define __cpp_lib_not_fn 202306L
#define __cpp_lib_constant_wrapper 202606L
#define __cpp_lib_move_only_function 202110L
#define __cpp_lib_copyable_function 202306L
#define __cpp_lib_function_ref 202604L
#define __cpp_lib_common_reference_wrapper 202302L
#define __cpp_lib_constexpr_typeinfo 202106L
// [version.syn]/4: 202306L when the default allocation functions are those of a hosted
// implementation; 0 for libycxx-freestanding.a, whose defaults have no heap.
#if YCXX_HOSTED
#  define __cpp_lib_freestanding_operator_new 202306L
// The freestanding parts of <cstring>/<cwchar> exist only as the hosted C library wrappers.
#  define __cpp_lib_freestanding_cstring 202311L
#  define __cpp_lib_freestanding_cwchar 202306L
#else
#  define __cpp_lib_freestanding_operator_new 0
#endif

// [version.syn]/3: defined only by a hardened implementation (YCXX_HARDENED=1), for the headers
// whose hardened preconditions are checked.
#if YCXX_HARDENED
#  define __cpp_lib_hardened_array 202502L
#  define __cpp_lib_hardened_basic_string_view 202502L
#  define __cpp_lib_hardened_bitset 202502L
#  define __cpp_lib_hardened_expected 202502L
#  define __cpp_lib_hardened_optional 202502L
#  define __cpp_lib_hardened_span 202502L
#endif
