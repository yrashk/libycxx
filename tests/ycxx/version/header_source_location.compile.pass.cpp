// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <source_location> in their comment. Only <source_location> is included.
#include <source_location>

#if !defined(__cpp_lib_source_location)
#  error "__cpp_lib_source_location is not defined"
#elif __cpp_lib_source_location != 201907L
#  error "__cpp_lib_source_location != 201907L"
#endif
