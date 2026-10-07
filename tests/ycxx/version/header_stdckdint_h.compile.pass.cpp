// [version.syn]/2: __cpp_lib_stdckdint_h is also defined after inclusion of <stdckdint.h>, the header its
// comment in the synopsis names. Only <stdckdint.h> is included.
#include <stdckdint.h>

#if !defined(__cpp_lib_stdckdint_h)
#  error "__cpp_lib_stdckdint_h is not defined"
#elif __cpp_lib_stdckdint_h != 202603L
#  error "__cpp_lib_stdckdint_h != 202603L"
#endif
