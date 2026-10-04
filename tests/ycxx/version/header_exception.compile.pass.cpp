// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <exception> in their comment. Only <exception> is included.
#include <exception>

#if !defined(__cpp_lib_uncaught_exceptions)
#  error "__cpp_lib_uncaught_exceptions is not defined"
#elif __cpp_lib_uncaught_exceptions != 201411L
#  error "__cpp_lib_uncaught_exceptions != 201411L"
#endif
