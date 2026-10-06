// libycxx runtime (hosted and freestanding): new array.
#include <new>
#include "hidden.hpp"
#include "allocation_table.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_Zna#")));

void* operator new[](std::size_t n) {
  if (auto __f = __ycxx_allocation_functions.__new_array; __f != __ycxx::__detail::__own_allocation_functions.__new_array)
    return __f(n, 0);
  return ::operator new(n);
}
// Marks that the default (forwarding) operator new[] is linked (see
// src/freestanding/new/try_or_null.hpp).
extern "C" [[__gnu__::__visibility__("hidden")]] const char __ycxx_default_new_array = 0;
