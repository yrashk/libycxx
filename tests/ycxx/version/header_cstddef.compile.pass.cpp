// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <cstddef> in their comment. Only <cstddef> is included.
#include <cstddef>

#if !defined(__cpp_lib_byte)
#  error "__cpp_lib_byte is not defined"
#elif __cpp_lib_byte != 201603L
#  error "__cpp_lib_byte != 201603L"
#endif
