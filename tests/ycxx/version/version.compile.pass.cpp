// [version.syn]/2-3: <version> defines each __cpp_lib_ macro of the synopsis with the given
// value. Checked here for the macros whose facilities libycxx provides (every header named in
// the macro's comment exists, and the facility compiles). The draft defines these
// unconditionally; [version.syn]/6 (freestanding: "only define a macro ... if the
// implementation provides the corresponding facility in its entirety") does not apply to a
// hosted build. __cpp_lib_is_within_lifetime is left out: it needs a compiler builtin GCC
// lacks (STATUS.md); its value is checked by values.compile.pass.cpp where it is defined.
#include <version>

#if !defined(__cpp_lib_addressof_constexpr)
#  error "__cpp_lib_addressof_constexpr is not defined"
#elif __cpp_lib_addressof_constexpr != 201603L
#  error "__cpp_lib_addressof_constexpr != 201603L"
#endif

#if !defined(__cpp_lib_allocate_at_least)
#  error "__cpp_lib_allocate_at_least is not defined"
#elif __cpp_lib_allocate_at_least != 202302L
#  error "__cpp_lib_allocate_at_least != 202302L"
#endif

#if !defined(__cpp_lib_allocator_traits_is_always_equal)
#  error "__cpp_lib_allocator_traits_is_always_equal is not defined"
#elif __cpp_lib_allocator_traits_is_always_equal != 201411L
#  error "__cpp_lib_allocator_traits_is_always_equal != 201411L"
#endif

#if !defined(__cpp_lib_any)
#  error "__cpp_lib_any is not defined"
#elif __cpp_lib_any != 201606L
#  error "__cpp_lib_any != 201606L"
#endif

#if !defined(__cpp_lib_array_constexpr)
#  error "__cpp_lib_array_constexpr is not defined"
#elif __cpp_lib_array_constexpr != 201811L
#  error "__cpp_lib_array_constexpr != 201811L"
#endif

#if !defined(__cpp_lib_as_const)
#  error "__cpp_lib_as_const is not defined"
#elif __cpp_lib_as_const != 201510L
#  error "__cpp_lib_as_const != 201510L"
#endif

#if !defined(__cpp_lib_assume_aligned)
#  error "__cpp_lib_assume_aligned is not defined"
#elif __cpp_lib_assume_aligned != 201811L
#  error "__cpp_lib_assume_aligned != 201811L"
#endif

#if !defined(__cpp_lib_bit_cast)
#  error "__cpp_lib_bit_cast is not defined"
#elif __cpp_lib_bit_cast != 201806L
#  error "__cpp_lib_bit_cast != 201806L"
#endif

#if !defined(__cpp_lib_bitops)
#  error "__cpp_lib_bitops is not defined"
#elif __cpp_lib_bitops != 202607L
#  error "__cpp_lib_bitops != 202607L"
#endif

#if !defined(__cpp_lib_bitset)
#  error "__cpp_lib_bitset is not defined"
#elif __cpp_lib_bitset != 202306L
#  error "__cpp_lib_bitset != 202306L"
#endif

#if !defined(__cpp_lib_bool_constant)
#  error "__cpp_lib_bool_constant is not defined"
#elif __cpp_lib_bool_constant != 201505L
#  error "__cpp_lib_bool_constant != 201505L"
#endif

#if !defined(__cpp_lib_bounded_array_traits)
#  error "__cpp_lib_bounded_array_traits is not defined"
#elif __cpp_lib_bounded_array_traits != 201902L
#  error "__cpp_lib_bounded_array_traits != 201902L"
#endif

#if !defined(__cpp_lib_byte)
#  error "__cpp_lib_byte is not defined"
#elif __cpp_lib_byte != 201603L
#  error "__cpp_lib_byte != 201603L"
#endif

#if !defined(__cpp_lib_byteswap)
#  error "__cpp_lib_byteswap is not defined"
#elif __cpp_lib_byteswap != 202110L
#  error "__cpp_lib_byteswap != 202110L"
#endif

#if !defined(__cpp_lib_common_reference)
#  error "__cpp_lib_common_reference is not defined"
#elif __cpp_lib_common_reference != 202302L
#  error "__cpp_lib_common_reference != 202302L"
#endif

#if !defined(__cpp_lib_common_reference_wrapper)
#  error "__cpp_lib_common_reference_wrapper is not defined"
#elif __cpp_lib_common_reference_wrapper != 202302L
#  error "__cpp_lib_common_reference_wrapper != 202302L"
#endif

#if !defined(__cpp_lib_concepts)
#  error "__cpp_lib_concepts is not defined"
#elif __cpp_lib_concepts != 202207L
#  error "__cpp_lib_concepts != 202207L"
#endif

#if !defined(__cpp_lib_constexpr_bitset)
#  error "__cpp_lib_constexpr_bitset is not defined"
#elif __cpp_lib_constexpr_bitset != 202207L
#  error "__cpp_lib_constexpr_bitset != 202207L"
#endif

#if !defined(__cpp_lib_constexpr_dynamic_alloc)
#  error "__cpp_lib_constexpr_dynamic_alloc is not defined"
#elif __cpp_lib_constexpr_dynamic_alloc != 201907L
#  error "__cpp_lib_constexpr_dynamic_alloc != 201907L"
#endif

#if !defined(__cpp_lib_constexpr_iterator)
#  error "__cpp_lib_constexpr_iterator is not defined"
#elif __cpp_lib_constexpr_iterator != 201811L
#  error "__cpp_lib_constexpr_iterator != 201811L"
#endif

#if !defined(__cpp_lib_constexpr_new)
#  error "__cpp_lib_constexpr_new is not defined"
#elif __cpp_lib_constexpr_new != 202406L
#  error "__cpp_lib_constexpr_new != 202406L"
#endif

#if !defined(__cpp_lib_constexpr_string_view)
#  error "__cpp_lib_constexpr_string_view is not defined"
#elif __cpp_lib_constexpr_string_view != 201811L
#  error "__cpp_lib_constexpr_string_view != 201811L"
#endif

#if !defined(__cpp_lib_constexpr_tuple)
#  error "__cpp_lib_constexpr_tuple is not defined"
#elif __cpp_lib_constexpr_tuple != 201811L
#  error "__cpp_lib_constexpr_tuple != 201811L"
#endif

#if !defined(__cpp_lib_constexpr_typeinfo)
#  error "__cpp_lib_constexpr_typeinfo is not defined"
#elif __cpp_lib_constexpr_typeinfo != 202106L
#  error "__cpp_lib_constexpr_typeinfo != 202106L"
#endif

#if !defined(__cpp_lib_constexpr_utility)
#  error "__cpp_lib_constexpr_utility is not defined"
#elif __cpp_lib_constexpr_utility != 201811L
#  error "__cpp_lib_constexpr_utility != 201811L"
#endif

#if !defined(__cpp_lib_constrained_equality)
#  error "__cpp_lib_constrained_equality is not defined"
#elif __cpp_lib_constrained_equality != 202411L
#  error "__cpp_lib_constrained_equality != 202411L"
#endif

#if !defined(__cpp_lib_coroutine)
#  error "__cpp_lib_coroutine is not defined"
#elif __cpp_lib_coroutine != 201902L
#  error "__cpp_lib_coroutine != 201902L"
#endif

#if !defined(__cpp_lib_destroying_delete)
#  error "__cpp_lib_destroying_delete is not defined"
#elif __cpp_lib_destroying_delete != 201806L
#  error "__cpp_lib_destroying_delete != 201806L"
#endif

#if !defined(__cpp_lib_endian)
#  error "__cpp_lib_endian is not defined"
#elif __cpp_lib_endian != 201907L
#  error "__cpp_lib_endian != 201907L"
#endif

#if !defined(__cpp_lib_exchange_function)
#  error "__cpp_lib_exchange_function is not defined"
#elif __cpp_lib_exchange_function != 201304L
#  error "__cpp_lib_exchange_function != 201304L"
#endif

#if !defined(__cpp_lib_expected)
#  error "__cpp_lib_expected is not defined"
#elif __cpp_lib_expected != 202606L
#  error "__cpp_lib_expected != 202606L"
#endif

#if !defined(__cpp_lib_forward_like)
#  error "__cpp_lib_forward_like is not defined"
#elif __cpp_lib_forward_like != 202207L
#  error "__cpp_lib_forward_like != 202207L"
#endif

#if !defined(__cpp_lib_freestanding_array)
#  error "__cpp_lib_freestanding_array is not defined"
#elif __cpp_lib_freestanding_array != 202311L
#  error "__cpp_lib_freestanding_array != 202311L"
#endif

#if !defined(__cpp_lib_freestanding_char_traits)
#  error "__cpp_lib_freestanding_char_traits is not defined"
#elif __cpp_lib_freestanding_char_traits != 202306L
#  error "__cpp_lib_freestanding_char_traits != 202306L"
#endif

#if !defined(__cpp_lib_freestanding_cstring)
#  error "__cpp_lib_freestanding_cstring is not defined"
#elif __cpp_lib_freestanding_cstring != 202311L
#  error "__cpp_lib_freestanding_cstring != 202311L"
#endif

#if !defined(__cpp_lib_freestanding_cwchar)
#  error "__cpp_lib_freestanding_cwchar is not defined"
#elif __cpp_lib_freestanding_cwchar != 202306L
#  error "__cpp_lib_freestanding_cwchar != 202306L"
#endif

#if !defined(__cpp_lib_freestanding_expected)
#  error "__cpp_lib_freestanding_expected is not defined"
#elif __cpp_lib_freestanding_expected != 202311L
#  error "__cpp_lib_freestanding_expected != 202311L"
#endif

#if !defined(__cpp_lib_freestanding_feature_test_macros)
#  error "__cpp_lib_freestanding_feature_test_macros is not defined"
#elif __cpp_lib_freestanding_feature_test_macros != 202306L
#  error "__cpp_lib_freestanding_feature_test_macros != 202306L"
#endif

#if !defined(__cpp_lib_freestanding_iterator)
#  error "__cpp_lib_freestanding_iterator is not defined"
#elif __cpp_lib_freestanding_iterator != 202306L
#  error "__cpp_lib_freestanding_iterator != 202306L"
#endif

#if !defined(__cpp_lib_freestanding_operator_new) || !(__cpp_lib_freestanding_operator_new == 202306L || __cpp_lib_freestanding_operator_new == 0)
#  error "__cpp_lib_freestanding_operator_new must be defined to 202306L or 0 ([version.syn]/4)"
#endif

#if !defined(__cpp_lib_freestanding_optional)
#  error "__cpp_lib_freestanding_optional is not defined"
#elif __cpp_lib_freestanding_optional != 202506L
#  error "__cpp_lib_freestanding_optional != 202506L"
#endif

#if !defined(__cpp_lib_freestanding_string_view)
#  error "__cpp_lib_freestanding_string_view is not defined"
#elif __cpp_lib_freestanding_string_view != 202311L
#  error "__cpp_lib_freestanding_string_view != 202311L"
#endif

#if !defined(__cpp_lib_freestanding_tuple)
#  error "__cpp_lib_freestanding_tuple is not defined"
#elif __cpp_lib_freestanding_tuple != 202306L
#  error "__cpp_lib_freestanding_tuple != 202306L"
#endif

#if !defined(__cpp_lib_freestanding_utility)
#  error "__cpp_lib_freestanding_utility is not defined"
#elif __cpp_lib_freestanding_utility != 202306L
#  error "__cpp_lib_freestanding_utility != 202306L"
#endif

#if !defined(__cpp_lib_freestanding_variant)
#  error "__cpp_lib_freestanding_variant is not defined"
#elif __cpp_lib_freestanding_variant != 202311L
#  error "__cpp_lib_freestanding_variant != 202311L"
#endif

#if !defined(__cpp_lib_hardware_interference_size)
#  error "__cpp_lib_hardware_interference_size is not defined"
#elif __cpp_lib_hardware_interference_size != 201703L
#  error "__cpp_lib_hardware_interference_size != 201703L"
#endif

#if !defined(__cpp_lib_has_unique_object_representations)
#  error "__cpp_lib_has_unique_object_representations is not defined"
#elif __cpp_lib_has_unique_object_representations != 201606L
#  error "__cpp_lib_has_unique_object_representations != 201606L"
#endif

#if !defined(__cpp_lib_initializer_list)
#  error "__cpp_lib_initializer_list is not defined"
#elif __cpp_lib_initializer_list != 202511L
#  error "__cpp_lib_initializer_list != 202511L"
#endif

#if !defined(__cpp_lib_int_pow2)
#  error "__cpp_lib_int_pow2 is not defined"
#elif __cpp_lib_int_pow2 != 202002L
#  error "__cpp_lib_int_pow2 != 202002L"
#endif

#if !defined(__cpp_lib_integer_comparison_functions)
#  error "__cpp_lib_integer_comparison_functions is not defined"
#elif __cpp_lib_integer_comparison_functions != 202002L
#  error "__cpp_lib_integer_comparison_functions != 202002L"
#endif

#if !defined(__cpp_lib_integer_sequence)
#  error "__cpp_lib_integer_sequence is not defined"
#elif __cpp_lib_integer_sequence != 202511L
#  error "__cpp_lib_integer_sequence != 202511L"
#endif

#if !defined(__cpp_lib_integral_constant_callable)
#  error "__cpp_lib_integral_constant_callable is not defined"
#elif __cpp_lib_integral_constant_callable != 201304L
#  error "__cpp_lib_integral_constant_callable != 201304L"
#endif

#if !defined(__cpp_lib_invoke)
#  error "__cpp_lib_invoke is not defined"
#elif __cpp_lib_invoke != 201411L
#  error "__cpp_lib_invoke != 201411L"
#endif

#if !defined(__cpp_lib_invoke_r)
#  error "__cpp_lib_invoke_r is not defined"
#elif __cpp_lib_invoke_r != 202106L
#  error "__cpp_lib_invoke_r != 202106L"
#endif

#if !defined(__cpp_lib_is_aggregate)
#  error "__cpp_lib_is_aggregate is not defined"
#elif __cpp_lib_is_aggregate != 201703L
#  error "__cpp_lib_is_aggregate != 201703L"
#endif

#if !defined(__cpp_lib_is_constant_evaluated)
#  error "__cpp_lib_is_constant_evaluated is not defined"
#elif __cpp_lib_is_constant_evaluated != 201811L
#  error "__cpp_lib_is_constant_evaluated != 201811L"
#endif

#if !defined(__cpp_lib_is_final)
#  error "__cpp_lib_is_final is not defined"
#elif __cpp_lib_is_final != 201402L
#  error "__cpp_lib_is_final != 201402L"
#endif

#if !defined(__cpp_lib_is_implicit_lifetime)
#  error "__cpp_lib_is_implicit_lifetime is not defined"
#elif __cpp_lib_is_implicit_lifetime != 202302L
#  error "__cpp_lib_is_implicit_lifetime != 202302L"
#endif

#if !defined(__cpp_lib_is_invocable)
#  error "__cpp_lib_is_invocable is not defined"
#elif __cpp_lib_is_invocable != 201703L
#  error "__cpp_lib_is_invocable != 201703L"
#endif

#if !defined(__cpp_lib_is_layout_compatible)
#  error "__cpp_lib_is_layout_compatible is not defined"
#elif __cpp_lib_is_layout_compatible != 201907L
#  error "__cpp_lib_is_layout_compatible != 201907L"
#endif

#if !defined(__cpp_lib_is_nothrow_convertible)
#  error "__cpp_lib_is_nothrow_convertible is not defined"
#elif __cpp_lib_is_nothrow_convertible != 201806L
#  error "__cpp_lib_is_nothrow_convertible != 201806L"
#endif

#if !defined(__cpp_lib_is_null_pointer)
#  error "__cpp_lib_is_null_pointer is not defined"
#elif __cpp_lib_is_null_pointer != 201309L
#  error "__cpp_lib_is_null_pointer != 201309L"
#endif

#if !defined(__cpp_lib_is_scoped_enum)
#  error "__cpp_lib_is_scoped_enum is not defined"
#elif __cpp_lib_is_scoped_enum != 202011L
#  error "__cpp_lib_is_scoped_enum != 202011L"
#endif

#if !defined(__cpp_lib_is_sufficiently_aligned)
#  error "__cpp_lib_is_sufficiently_aligned is not defined"
#elif __cpp_lib_is_sufficiently_aligned != 202411L
#  error "__cpp_lib_is_sufficiently_aligned != 202411L"
#endif

#if !defined(__cpp_lib_is_swappable)
#  error "__cpp_lib_is_swappable is not defined"
#elif __cpp_lib_is_swappable != 201603L
#  error "__cpp_lib_is_swappable != 201603L"
#endif

#if !defined(__cpp_lib_is_virtual_base_of)
#  error "__cpp_lib_is_virtual_base_of is not defined"
#elif __cpp_lib_is_virtual_base_of != 202406L
#  error "__cpp_lib_is_virtual_base_of != 202406L"
#endif

#if !defined(__cpp_lib_launder)
#  error "__cpp_lib_launder is not defined"
#elif __cpp_lib_launder != 201606L
#  error "__cpp_lib_launder != 201606L"
#endif

#if !defined(__cpp_lib_logical_traits)
#  error "__cpp_lib_logical_traits is not defined"
#elif __cpp_lib_logical_traits != 201510L
#  error "__cpp_lib_logical_traits != 201510L"
#endif

#if !defined(__cpp_lib_make_from_tuple)
#  error "__cpp_lib_make_from_tuple is not defined"
#elif __cpp_lib_make_from_tuple != 201606L
#  error "__cpp_lib_make_from_tuple != 201606L"
#endif

#if !defined(__cpp_lib_make_reverse_iterator)
#  error "__cpp_lib_make_reverse_iterator is not defined"
#elif __cpp_lib_make_reverse_iterator != 201402L
#  error "__cpp_lib_make_reverse_iterator != 201402L"
#endif

#if !defined(__cpp_lib_move_iterator_concept)
#  error "__cpp_lib_move_iterator_concept is not defined"
#elif __cpp_lib_move_iterator_concept != 202207L
#  error "__cpp_lib_move_iterator_concept != 202207L"
#endif

#if !defined(__cpp_lib_null_iterators)
#  error "__cpp_lib_null_iterators is not defined"
#elif __cpp_lib_null_iterators != 201304L
#  error "__cpp_lib_null_iterators != 201304L"
#endif

#if !defined(__cpp_lib_observable_checkpoint)
#  error "__cpp_lib_observable_checkpoint is not defined"
#elif __cpp_lib_observable_checkpoint != 202506L
#  error "__cpp_lib_observable_checkpoint != 202506L"
#endif

#if !defined(__cpp_lib_optional)
#  error "__cpp_lib_optional is not defined"
#elif __cpp_lib_optional != 202506L
#  error "__cpp_lib_optional != 202506L"
#endif

#if !defined(__cpp_lib_optional_range_support)
#  error "__cpp_lib_optional_range_support is not defined"
#elif __cpp_lib_optional_range_support != 202406L
#  error "__cpp_lib_optional_range_support != 202406L"
#endif

#if !defined(__cpp_lib_reference_from_temporary)
#  error "__cpp_lib_reference_from_temporary is not defined"
#elif __cpp_lib_reference_from_temporary != 202202L
#  error "__cpp_lib_reference_from_temporary != 202202L"
#endif

#if !defined(__cpp_lib_reference_wrapper)
#  error "__cpp_lib_reference_wrapper is not defined"
#elif __cpp_lib_reference_wrapper != 202403L
#  error "__cpp_lib_reference_wrapper != 202403L"
#endif

#if !defined(__cpp_lib_remove_cvref)
#  error "__cpp_lib_remove_cvref is not defined"
#elif __cpp_lib_remove_cvref != 201711L
#  error "__cpp_lib_remove_cvref != 201711L"
#endif

#if !defined(__cpp_lib_source_location)
#  error "__cpp_lib_source_location is not defined"
#elif __cpp_lib_source_location != 201907L
#  error "__cpp_lib_source_location != 201907L"
#endif

#if !defined(__cpp_lib_span)
#  error "__cpp_lib_span is not defined"
#elif __cpp_lib_span != 202311L
#  error "__cpp_lib_span != 202311L"
#endif

#if !defined(__cpp_lib_ssize)
#  error "__cpp_lib_ssize is not defined"
#elif __cpp_lib_ssize != 201902L
#  error "__cpp_lib_ssize != 201902L"
#endif

#if !defined(__cpp_lib_start_lifetime_as)
#  error "__cpp_lib_start_lifetime_as is not defined"
#elif __cpp_lib_start_lifetime_as != 202207L
#  error "__cpp_lib_start_lifetime_as != 202207L"
#endif

#if !defined(__cpp_lib_starts_ends_with)
#  error "__cpp_lib_starts_ends_with is not defined"
#elif __cpp_lib_starts_ends_with != 201711L
#  error "__cpp_lib_starts_ends_with != 201711L"
#endif

#if !defined(__cpp_lib_string_contains)
#  error "__cpp_lib_string_contains is not defined"
#elif __cpp_lib_string_contains != 202011L
#  error "__cpp_lib_string_contains != 202011L"
#endif

#if !defined(__cpp_lib_string_subview)
#  error "__cpp_lib_string_subview is not defined"
#elif __cpp_lib_string_subview != 202506L
#  error "__cpp_lib_string_subview != 202506L"
#endif

#if !defined(__cpp_lib_string_view)
#  error "__cpp_lib_string_view is not defined"
#elif __cpp_lib_string_view != 202403L
#  error "__cpp_lib_string_view != 202403L"
#endif

#if !defined(__cpp_lib_three_way_comparison)
#  error "__cpp_lib_three_way_comparison is not defined"
#elif __cpp_lib_three_way_comparison != 201907L
#  error "__cpp_lib_three_way_comparison != 201907L"
#endif

#if !defined(__cpp_lib_to_address)
#  error "__cpp_lib_to_address is not defined"
#elif __cpp_lib_to_address != 201711L
#  error "__cpp_lib_to_address != 201711L"
#endif

#if !defined(__cpp_lib_to_array)
#  error "__cpp_lib_to_array is not defined"
#elif __cpp_lib_to_array != 201907L
#  error "__cpp_lib_to_array != 201907L"
#endif

#if !defined(__cpp_lib_to_underlying)
#  error "__cpp_lib_to_underlying is not defined"
#elif __cpp_lib_to_underlying != 202102L
#  error "__cpp_lib_to_underlying != 202102L"
#endif

#if !defined(__cpp_lib_transformation_trait_aliases)
#  error "__cpp_lib_transformation_trait_aliases is not defined"
#elif __cpp_lib_transformation_trait_aliases != 201304L
#  error "__cpp_lib_transformation_trait_aliases != 201304L"
#endif

#if !defined(__cpp_lib_tuple_element_t)
#  error "__cpp_lib_tuple_element_t is not defined"
#elif __cpp_lib_tuple_element_t != 201402L
#  error "__cpp_lib_tuple_element_t != 201402L"
#endif

#if !defined(__cpp_lib_tuples_by_type)
#  error "__cpp_lib_tuples_by_type is not defined"
#elif __cpp_lib_tuples_by_type != 201304L
#  error "__cpp_lib_tuples_by_type != 201304L"
#endif

#if !defined(__cpp_lib_type_identity)
#  error "__cpp_lib_type_identity is not defined"
#elif __cpp_lib_type_identity != 201806L
#  error "__cpp_lib_type_identity != 201806L"
#endif

#if !defined(__cpp_lib_type_order)
#  error "__cpp_lib_type_order is not defined"
#elif __cpp_lib_type_order != 202506L
#  error "__cpp_lib_type_order != 202506L"
#endif

#if !defined(__cpp_lib_type_trait_variable_templates)
#  error "__cpp_lib_type_trait_variable_templates is not defined"
#elif __cpp_lib_type_trait_variable_templates != 201510L
#  error "__cpp_lib_type_trait_variable_templates != 201510L"
#endif

#if !defined(__cpp_lib_uncaught_exceptions)
#  error "__cpp_lib_uncaught_exceptions is not defined"
#elif __cpp_lib_uncaught_exceptions != 201411L
#  error "__cpp_lib_uncaught_exceptions != 201411L"
#endif

#if !defined(__cpp_lib_unreachable)
#  error "__cpp_lib_unreachable is not defined"
#elif __cpp_lib_unreachable != 202202L
#  error "__cpp_lib_unreachable != 202202L"
#endif

#if !defined(__cpp_lib_unwrap_ref)
#  error "__cpp_lib_unwrap_ref is not defined"
#elif __cpp_lib_unwrap_ref != 201811L
#  error "__cpp_lib_unwrap_ref != 201811L"
#endif

#if !defined(__cpp_lib_variant)
#  error "__cpp_lib_variant is not defined"
#elif __cpp_lib_variant != 202306L
#  error "__cpp_lib_variant != 202306L"
#endif

// [version.syn]/2: view_interface::at ([view.interface.general]/1, [view.interface.members]/5-6)
#if !defined(__cpp_lib_view_interface)
#  error "__cpp_lib_view_interface is not defined"
#elif __cpp_lib_view_interface != 202606L
#  error "__cpp_lib_view_interface != 202606L"
#endif

#if !defined(__cpp_lib_void_t)
#  error "__cpp_lib_void_t is not defined"
#elif __cpp_lib_void_t != 201411L
#  error "__cpp_lib_void_t != 201411L"
#endif
