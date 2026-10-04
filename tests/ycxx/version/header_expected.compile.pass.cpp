// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <expected> in their comment. Only <expected> is included.
#include <expected>

#if !defined(__cpp_lib_constrained_equality)
#  error "__cpp_lib_constrained_equality is not defined"
#elif __cpp_lib_constrained_equality != 202411L
#  error "__cpp_lib_constrained_equality != 202411L"
#endif

#if !defined(__cpp_lib_expected)
#  error "__cpp_lib_expected is not defined"
#elif __cpp_lib_expected != 202606L
#  error "__cpp_lib_expected != 202606L"
#endif

#if !defined(__cpp_lib_freestanding_expected)
#  error "__cpp_lib_freestanding_expected is not defined"
#elif __cpp_lib_freestanding_expected != 202311L
#  error "__cpp_lib_freestanding_expected != 202311L"
#endif
