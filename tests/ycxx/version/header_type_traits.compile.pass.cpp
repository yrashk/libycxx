// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <type_traits> in their comment. Only <type_traits> is included.
#include <type_traits>

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

#if !defined(__cpp_lib_common_reference)
#  error "__cpp_lib_common_reference is not defined"
#elif __cpp_lib_common_reference != 202302L
#  error "__cpp_lib_common_reference != 202302L"
#endif

#if !defined(__cpp_lib_has_unique_object_representations)
#  error "__cpp_lib_has_unique_object_representations is not defined"
#elif __cpp_lib_has_unique_object_representations != 201606L
#  error "__cpp_lib_has_unique_object_representations != 201606L"
#endif

#if !defined(__cpp_lib_integral_constant_callable)
#  error "__cpp_lib_integral_constant_callable is not defined"
#elif __cpp_lib_integral_constant_callable != 201304L
#  error "__cpp_lib_integral_constant_callable != 201304L"
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

#if !defined(__cpp_lib_logical_traits)
#  error "__cpp_lib_logical_traits is not defined"
#elif __cpp_lib_logical_traits != 201510L
#  error "__cpp_lib_logical_traits != 201510L"
#endif

#if !defined(__cpp_lib_reference_from_temporary)
#  error "__cpp_lib_reference_from_temporary is not defined"
#elif __cpp_lib_reference_from_temporary != 202202L
#  error "__cpp_lib_reference_from_temporary != 202202L"
#endif

#if !defined(__cpp_lib_remove_cvref)
#  error "__cpp_lib_remove_cvref is not defined"
#elif __cpp_lib_remove_cvref != 201711L
#  error "__cpp_lib_remove_cvref != 201711L"
#endif

#if !defined(__cpp_lib_transformation_trait_aliases)
#  error "__cpp_lib_transformation_trait_aliases is not defined"
#elif __cpp_lib_transformation_trait_aliases != 201304L
#  error "__cpp_lib_transformation_trait_aliases != 201304L"
#endif

#if !defined(__cpp_lib_type_identity)
#  error "__cpp_lib_type_identity is not defined"
#elif __cpp_lib_type_identity != 201806L
#  error "__cpp_lib_type_identity != 201806L"
#endif

#if !defined(__cpp_lib_type_trait_variable_templates)
#  error "__cpp_lib_type_trait_variable_templates is not defined"
#elif __cpp_lib_type_trait_variable_templates != 201510L
#  error "__cpp_lib_type_trait_variable_templates != 201510L"
#endif

#if !defined(__cpp_lib_unwrap_ref)
#  error "__cpp_lib_unwrap_ref is not defined"
#elif __cpp_lib_unwrap_ref != 201811L
#  error "__cpp_lib_unwrap_ref != 201811L"
#endif

#if !defined(__cpp_lib_void_t)
#  error "__cpp_lib_void_t is not defined"
#elif __cpp_lib_void_t != 201411L
#  error "__cpp_lib_void_t != 201411L"
#endif
