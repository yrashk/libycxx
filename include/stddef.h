// -*- C++ -*-  libycxx: <stddef.h> ([support.c.headers.other])   [core; all freestanding; also usable from C]
//
// In C++, <cstddef>, whose names [support.c.headers.other]/1 places in the global namespace (all
// but std::byte and its operations): core declares ::size_t, ::ptrdiff_t, ::nullptr_t and reads
// the compiler's own <stddef.h> for ::max_align_t, NULL and offsetof (the compiler's header alone
// lacks ::nullptr_t in C++ with Clang). In C, and whenever a C library header asks for a single
// type (the __need_* protocol of the compilers' <stddef.h>), the compiler's header itself. No
// #pragma once: such partial requests may come in any number and order.
#if defined(__cplusplus) && !defined(__need_size_t) && !defined(__need_ptrdiff_t) && \
    !defined(__need_wchar_t) && !defined(__need_wint_t) && !defined(__need_NULL) &&   \
    !defined(__need_nullptr_t) && !defined(__need_max_align_t) && !defined(__need_offsetof) && \
    !defined(__need_rsize_t) && !defined(__need_unreachable)
extern "C++" {
#  include <cstddef>
}
#else
#  include_next <stddef.h>
#endif
