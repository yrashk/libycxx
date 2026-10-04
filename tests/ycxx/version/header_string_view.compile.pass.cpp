// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <string_view> in their comment. Only <string_view> is included.
#include <string_view>

#if !defined(__cpp_lib_constexpr_string_view)
#  error "__cpp_lib_constexpr_string_view is not defined"
#elif __cpp_lib_constexpr_string_view != 201811L
#  error "__cpp_lib_constexpr_string_view != 201811L"
#endif

#if !defined(__cpp_lib_freestanding_string_view)
#  error "__cpp_lib_freestanding_string_view is not defined"
#elif __cpp_lib_freestanding_string_view != 202311L
#  error "__cpp_lib_freestanding_string_view != 202311L"
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
