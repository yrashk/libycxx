// libycxx runtime (hosted and freestanding): delete array sized.
#include <new>
#include "hidden.hpp"
#include "allocation_table.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_ZdaPv#")));
// What a ThreadSanitizer build's link options name as undefined (hidden.hpp).
extern "C" [[__gnu__::__visibility__("hidden")]] const char __ycxx_allocation_anchor_delete_array_sized = 0;

[[__gnu__::__weak__]] void operator delete[](void* p, std::size_t n) noexcept {
  if (auto __f = __ycxx_allocation_functions.__delete_array_sized; __f != __ycxx::__detail::__own_allocation_functions.__delete_array_sized)
    return __f(p, n, 0);
  ::operator delete[](p);
}
