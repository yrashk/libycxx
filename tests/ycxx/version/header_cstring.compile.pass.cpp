// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <cstring> in their comment. Only <cstring> is included.
// COUNTERPART: libstdcxx:21_strings/headers/cstring/version.cc
#include <cstring>

#if !defined(__cpp_lib_freestanding_cstring)
#  error "__cpp_lib_freestanding_cstring is not defined"
#elif __cpp_lib_freestanding_cstring != 202311L
#  error "__cpp_lib_freestanding_cstring != 202311L"
#endif
