// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <cwchar> in their comment. Only <cwchar> is included.
#include <cwchar>

#if !defined(__cpp_lib_freestanding_cwchar)
#  error "__cpp_lib_freestanding_cwchar is not defined"
#elif __cpp_lib_freestanding_cwchar != 202306L
#  error "__cpp_lib_freestanding_cwchar != 202306L"
#endif
