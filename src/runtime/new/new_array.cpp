// libycxx runtime (hosted and freestanding): new array.
#include <new>
#include "hidden.hpp"

asm((ycxx::detail::hide_allocation_function("_Zna#")));

void* operator new[](std::size_t n) { return ::operator new(n); }
// Marks that the default (forwarding) operator new[] is linked (see
// src/freestanding/new/try_or_null.hpp).
extern "C" const char ycxx_default_new_array = 0;
