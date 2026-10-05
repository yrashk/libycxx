// libycxx runtime (hosted and freestanding): delete array sized.
#include <new>
#include "hidden.hpp"
#include "allocation_table.hpp"

asm((ycxx::detail::hide_allocation_function("_ZdaPv#")));

void operator delete[](void* p, std::size_t n) noexcept {
  if (auto f = ycxx_allocation_functions.delete_array_sized; f != ycxx::detail::own_allocation_functions.delete_array_sized)
    return f(p, n, 0);
  ::operator delete[](p);
}
