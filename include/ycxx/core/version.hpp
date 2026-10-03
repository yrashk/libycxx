// libycxx core: library feature-test macros ([version.syn]).
//
// A macro is only defined once the corresponding feature is implemented. Every public header
// includes this file, which is permitted: each header must define *at least* its own macros.
#ifndef YCXX_CORE_VERSION_HPP
#define YCXX_CORE_VERSION_HPP

#define __cpp_lib_freestanding_feature_test_macros 202306L

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
#define __cpp_lib_is_within_lifetime 202306L
#define __cpp_lib_logical_traits 201510L
#define __cpp_lib_reference_from_temporary 202202L
#define __cpp_lib_remove_cvref 201711L
#define __cpp_lib_transformation_trait_aliases 201304L
#define __cpp_lib_type_identity 201806L
#define __cpp_lib_type_trait_variable_templates 201510L
#define __cpp_lib_unwrap_ref 201811L
#define __cpp_lib_void_t 201411L
#if YCXX_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS && YCXX_HAS_IS_CORRESPONDING_MEMBER
#  define __cpp_lib_is_pointer_interconvertible 201907L
#endif

#endif // YCXX_CORE_VERSION_HPP
