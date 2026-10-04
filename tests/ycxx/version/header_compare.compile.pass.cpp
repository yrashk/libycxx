// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <compare> in their comment. Only <compare> is included.
#include <compare>

#if !defined(__cpp_lib_concepts)
#  error "__cpp_lib_concepts is not defined"
#elif __cpp_lib_concepts != 202207L
#  error "__cpp_lib_concepts != 202207L"
#endif

#if !defined(__cpp_lib_three_way_comparison)
#  error "__cpp_lib_three_way_comparison is not defined"
#elif __cpp_lib_three_way_comparison != 201907L
#  error "__cpp_lib_three_way_comparison != 201907L"
#endif

#if !defined(__cpp_lib_type_order)
#  error "__cpp_lib_type_order is not defined"
#elif __cpp_lib_type_order != 202506L
#  error "__cpp_lib_type_order != 202506L"
#endif
