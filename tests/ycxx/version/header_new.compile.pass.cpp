// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <new> in their comment. Only <new> is included.
#include <new>

#if !defined(__cpp_lib_constexpr_new)
#  error "__cpp_lib_constexpr_new is not defined"
#elif __cpp_lib_constexpr_new != 202406L
#  error "__cpp_lib_constexpr_new != 202406L"
#endif

#if !defined(__cpp_lib_destroying_delete)
#  error "__cpp_lib_destroying_delete is not defined"
#elif __cpp_lib_destroying_delete != 201806L
#  error "__cpp_lib_destroying_delete != 201806L"
#endif

#if !defined(__cpp_lib_freestanding_operator_new) || !(__cpp_lib_freestanding_operator_new == 202306L || __cpp_lib_freestanding_operator_new == 0)
#  error "__cpp_lib_freestanding_operator_new must be defined to 202306L or 0 ([version.syn]/4)"
#endif

#if !defined(__cpp_lib_hardware_interference_size)
#  error "__cpp_lib_hardware_interference_size is not defined"
#elif __cpp_lib_hardware_interference_size != 201703L
#  error "__cpp_lib_hardware_interference_size != 201703L"
#endif

#if !defined(__cpp_lib_launder)
#  error "__cpp_lib_launder is not defined"
#elif __cpp_lib_launder != 201606L
#  error "__cpp_lib_launder != 201606L"
#endif
