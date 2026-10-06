// [version.syn]: __cpp_lib_modules is defined by <version> (and every header) when the
// implementation provides the modules std and std.compat ([std.modules]).
#include <version>

#ifndef __cpp_lib_modules
#  error "__cpp_lib_modules is not defined"
#endif
static_assert(__cpp_lib_modules >= 202207L);
