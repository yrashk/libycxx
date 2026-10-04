// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <typeinfo> in their comment. Only <typeinfo> is included.
#include <typeinfo>

#if !defined(__cpp_lib_constexpr_typeinfo)
#  error "__cpp_lib_constexpr_typeinfo is not defined"
#elif __cpp_lib_constexpr_typeinfo != 202106L
#  error "__cpp_lib_constexpr_typeinfo != 202106L"
#endif
