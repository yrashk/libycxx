// libycxx runtime (hosted and freestanding): delete sized align.
#include <new>
#include "hidden.hpp"
#include "allocation_table.hpp"

asm((ycxx::detail::hide_allocation_function("_ZdlPv#St11align_val_t")));

void operator delete(void* p, std::size_t n, std::align_val_t a) noexcept {
  if (auto f = ycxx_allocation_functions.delete_sized_align; f != ycxx::detail::own_allocation_functions.delete_sized_align)
    return f(p, n, static_cast<std::size_t>(a));
  ::operator delete(p, a);
}
