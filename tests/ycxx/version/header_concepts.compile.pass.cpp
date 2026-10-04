// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <concepts> in their comment. Only <concepts> is included.
#include <concepts>

#if !defined(__cpp_lib_concepts)
#  error "__cpp_lib_concepts is not defined"
#elif __cpp_lib_concepts != 202207L
#  error "__cpp_lib_concepts != 202207L"
#endif
