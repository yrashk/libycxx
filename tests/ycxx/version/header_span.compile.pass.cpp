// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <span> in their comment. Only <span> is included.
// COUNTERPART: libstdcxx:23_containers/span/version.cc
#include <span>

#if !defined(__cpp_lib_span)
#  error "__cpp_lib_span is not defined"
#elif __cpp_lib_span != 202311L
#  error "__cpp_lib_span != 202311L"
#endif
