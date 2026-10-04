// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <functional> in their comment. Only <functional> is included.
#include <functional>

#if !defined(__cpp_lib_common_reference_wrapper)
#  error "__cpp_lib_common_reference_wrapper is not defined"
#elif __cpp_lib_common_reference_wrapper != 202302L
#  error "__cpp_lib_common_reference_wrapper != 202302L"
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

#if !defined(__cpp_lib_reference_wrapper)
#  error "__cpp_lib_reference_wrapper is not defined"
#elif __cpp_lib_reference_wrapper != 202403L
#  error "__cpp_lib_reference_wrapper != 202403L"
#endif
