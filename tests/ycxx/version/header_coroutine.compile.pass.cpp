// [version.syn]/2: "Each of the macros defined in <version> is also defined after inclusion
// of any member of the set of library headers indicated in the corresponding comment in
// this synopsis." These macros name <coroutine> in their comment. Only <coroutine> is included.
#include <coroutine>

#if !defined(__cpp_lib_coroutine)
#  error "__cpp_lib_coroutine is not defined"
#elif __cpp_lib_coroutine != 201902L
#  error "__cpp_lib_coroutine != 201902L"
#endif
