// libycxx runtime (hosted and freestanding): delete array.
#include <new>
#include "hidden.hpp"
#include "allocation_table.hpp"

asm((ycxx::detail::hide_allocation_function("_ZdaPv")));

void operator delete[](void* p) noexcept {
  if (auto f = ycxx_allocation_functions.delete_array; f != ycxx::detail::own_allocation_functions.delete_array)
    return f(p, 0, 0);
  ::operator delete(p);
}
