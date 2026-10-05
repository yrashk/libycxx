// libycxx runtime (hosted and freestanding): new array align.
#include <new>
#include "hidden.hpp"
#include "allocation_table.hpp"

asm((ycxx::detail::hide_allocation_function("_Zna#St11align_val_t")));

void* operator new[](std::size_t n, std::align_val_t a) {
  if (auto f = ycxx_allocation_functions.new_array_align; f != ycxx::detail::own_allocation_functions.new_array_align)
    return f(n, static_cast<std::size_t>(a));
  return ::operator new(n, a);
}
// Marks that the default (forwarding) operator new[] is linked (see
// src/freestanding/new/try_or_null.hpp).
extern "C" const char ycxx_default_new_array_align = 0;
