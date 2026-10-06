// libycxx runtime (hosted and freestanding): delete array nothrow.
#include <new>
#include "hidden.hpp"
#include "allocation_table.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_ZdaPvRKSt9nothrow_t")));
// What a ThreadSanitizer build's link options name as undefined (hidden.hpp).
extern "C" [[__gnu__::__visibility__("hidden")]] const char __ycxx_allocation_anchor_delete_array_nothrow = 0;

[[__gnu__::__weak__]] void operator delete[](void* p, const std::nothrow_t&) noexcept {
  if (auto __f = __ycxx_allocation_functions.__delete_array_nothrow; __f != __ycxx::__detail::__own_allocation_functions.__delete_array_nothrow)
    return __f(p, 0, 0);
  ::operator delete[](p);
}
