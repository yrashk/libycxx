// libycxx runtime (hosted and freestanding): delete array align.
#include <new>
#include "hidden.hpp"
#include "allocation_table.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_ZdaPvSt11align_val_t")));
// What a ThreadSanitizer build's link options name as undefined (hidden.hpp).
extern "C" [[__gnu__::__visibility__("hidden")]] const char __ycxx_allocation_anchor_delete_array_align = 0;

[[__gnu__::__weak__]] void operator delete[](void* p, std::align_val_t a) noexcept {
  if (auto __f = __ycxx_allocation_functions.__delete_array_align; __f != __ycxx::__detail::__own_allocation_functions.__delete_array_align)
    return __f(p, 0, static_cast<std::size_t>(a));
  ::operator delete(p, a);
}
