// libycxx core: <cstdarg> ([cstdarg.syn]; all freestanding).
//
// Defined from the compilers' builtins in every mode, without the C library or the compiler's
// <stdarg.h>: Clang 23's <stdarg.h> accepts only the two-argument va_start in C++ and defines no
// __STDC_VERSION_STDARG_H__. va_start(V, ...) discards everything after V ([cstdarg.syn]/1.2):
// GCC has the C23 builtin in C++ too; Clang's builtin needs a second argument, which it ignores
// apart from a -Wvarargs check, silenced here (the code it generates does not depend on it).
// A later <stdarg.h> may redefine the macros with the compiler's own forms.
#pragma once

#include <ycxx/config.hpp>

#define __STDC_VERSION_STDARG_H__ 202311L

// Also in the global namespace, as the C header (which other headers may include) declares it.
typedef __builtin_va_list va_list;
namespace [[gnu::visibility("hidden")]] std {
using ::va_list;
} // namespace std

#undef va_start
#undef va_arg
#undef va_copy
#undef va_end
#if YCXX_HAS_C23_VA_START
#  define va_start(V, ...) __builtin_c23_va_start(V)
#else
#  define va_start(V, ...)                                                                                     \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wvarargs\"") __builtin_va_start(V, 0) \
        _Pragma("clang diagnostic pop")
#endif
#define va_arg(V, P) __builtin_va_arg(V, P)
#define va_copy(VDST, VSRC) __builtin_va_copy(VDST, VSRC)
#define va_end(V) __builtin_va_end(V)
