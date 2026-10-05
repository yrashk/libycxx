// libycxx runtime (hosted and freestanding): delete array align.
#include <new>
#include "hidden.hpp"
#include "allocation_table.hpp"

asm((ycxx::detail::hide_allocation_function("_ZdaPvSt11align_val_t")));

void operator delete[](void* p, std::align_val_t a) noexcept {
  if (auto f = ycxx_allocation_functions.delete_array_align; f != ycxx::detail::own_allocation_functions.delete_array_align)
    return f(p, 0, static_cast<std::size_t>(a));
  ::operator delete(p, a);
}
