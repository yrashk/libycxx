// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <optional> in their comment. Only <optional> is included.
#include <optional>

#if !defined(__cpp_lib_constrained_equality)
#  error "__cpp_lib_constrained_equality is not defined"
#elif __cpp_lib_constrained_equality != 202411L
#  error "__cpp_lib_constrained_equality != 202411L"
#endif

#if !defined(__cpp_lib_freestanding_optional)
#  error "__cpp_lib_freestanding_optional is not defined"
#elif __cpp_lib_freestanding_optional != 202506L
#  error "__cpp_lib_freestanding_optional != 202506L"
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
