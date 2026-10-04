// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <tuple> in their comment. Only <tuple> is included.
#include <tuple>

#if !defined(__cpp_lib_constexpr_tuple)
#  error "__cpp_lib_constexpr_tuple is not defined"
#elif __cpp_lib_constexpr_tuple != 201811L
#  error "__cpp_lib_constexpr_tuple != 201811L"
#endif

#if !defined(__cpp_lib_constrained_equality)
#  error "__cpp_lib_constrained_equality is not defined"
#elif __cpp_lib_constrained_equality != 202411L
#  error "__cpp_lib_constrained_equality != 202411L"
#endif

#if !defined(__cpp_lib_freestanding_tuple)
#  error "__cpp_lib_freestanding_tuple is not defined"
#elif __cpp_lib_freestanding_tuple != 202306L
#  error "__cpp_lib_freestanding_tuple != 202306L"
#endif

#if !defined(__cpp_lib_make_from_tuple)
#  error "__cpp_lib_make_from_tuple is not defined"
#elif __cpp_lib_make_from_tuple != 201606L
#  error "__cpp_lib_make_from_tuple != 201606L"
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
