// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <array> in their comment. Only <array> is included.
// COUNTERPART: libstdcxx:23_containers/array/requirements/version.cc
#include <array>

#if !defined(__cpp_lib_array_constexpr)
#  error "__cpp_lib_array_constexpr is not defined"
#elif __cpp_lib_array_constexpr != 201811L
#  error "__cpp_lib_array_constexpr != 201811L"
#endif

#if !defined(__cpp_lib_freestanding_array)
#  error "__cpp_lib_freestanding_array is not defined"
#elif __cpp_lib_freestanding_array != 202311L
#  error "__cpp_lib_freestanding_array != 202311L"
#endif

#if !defined(__cpp_lib_to_array)
#  error "__cpp_lib_to_array is not defined"
#elif __cpp_lib_to_array != 201907L
#  error "__cpp_lib_to_array != 201907L"
#endif
