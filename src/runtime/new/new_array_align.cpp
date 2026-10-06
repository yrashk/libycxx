// libycxx runtime (hosted and freestanding): new array align.
#include <new>
#include "hidden.hpp"
#include "allocation_table.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_Zna#St11align_val_t")));
// What a ThreadSanitizer build's link options name as undefined (hidden.hpp).
extern "C" [[__gnu__::__visibility__("hidden")]] const char __ycxx_allocation_anchor_new_array_align = 0;

[[__gnu__::__weak__]] void* operator new[](std::size_t n, std::align_val_t a) {
  if (auto __f = __ycxx_allocation_functions.__new_array_align; __f != __ycxx::__detail::__own_allocation_functions.__new_array_align)
    return __f(n, static_cast<std::size_t>(a));
  return ::operator new(n, a);
}
// Marks that the default (forwarding) operator new[] is linked (see
// src/freestanding/new/try_or_null.hpp).
extern "C" [[__gnu__::__visibility__("hidden")]] const char __ycxx_default_new_array_align = 0;
