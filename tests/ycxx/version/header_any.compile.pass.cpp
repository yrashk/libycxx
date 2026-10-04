// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <any> in their comment. Only <any> is included.
#include <any>

#if !defined(__cpp_lib_any)
#  error "__cpp_lib_any is not defined"
#elif __cpp_lib_any != 201606L
#  error "__cpp_lib_any != 201606L"
#endif
