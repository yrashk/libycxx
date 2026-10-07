// [version.syn]/2: __cpp_lib_stdbit_h is also defined after inclusion of <stdbit.h>, the header its
// comment in the synopsis names. Only <stdbit.h> is included.
#include <stdbit.h>

#if !defined(__cpp_lib_stdbit_h)
#  error "__cpp_lib_stdbit_h is not defined"
#elif __cpp_lib_stdbit_h != 202603L
#  error "__cpp_lib_stdbit_h != 202603L"
#endif
