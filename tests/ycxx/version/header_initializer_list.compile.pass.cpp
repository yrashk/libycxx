// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <initializer_list> in their comment. Only <initializer_list> is included.
#include <initializer_list>

#if !defined(__cpp_lib_initializer_list)
#  error "__cpp_lib_initializer_list is not defined"
#elif __cpp_lib_initializer_list != 202511L
#  error "__cpp_lib_initializer_list != 202511L"
#endif
