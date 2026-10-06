// libycxx runtime (hosted and freestanding): delete array nothrow.
#include <new>
#include "hidden.hpp"
#include "allocation_table.hpp"

asm((ycxx::detail::hide_allocation_function("_ZdaPvRKSt9nothrow_t")));

void operator delete[](void* p, const std::nothrow_t&) noexcept {
  if (auto f = ycxx_allocation_functions.delete_array_nothrow; f != ycxx::detail::own_allocation_functions.delete_array_nothrow)
    return f(p, 0, 0);
  ::operator delete[](p);
}
