// libycxx runtime (hosted and freestanding): new array align.
#include <new>

void* operator new[](std::size_t n, std::align_val_t a) { return ::operator new(n, a); }
// Marks that the default (forwarding) operator new[] is linked (see
// src/freestanding/new/try_or_null.hpp).
extern "C" const char ycxx_default_new_array_align = 0;
