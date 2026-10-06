// libycxx runtime (hosted and freestanding): delete array.
#include <new>
#include "hidden.hpp"
#include "allocation_table.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_ZdaPv")));

void operator delete[](void* p) noexcept {
  if (auto __f = __ycxx_allocation_functions.__delete_array; __f != __ycxx::__detail::__own_allocation_functions.__delete_array)
    return __f(p, 0, 0);
  ::operator delete(p);
}
