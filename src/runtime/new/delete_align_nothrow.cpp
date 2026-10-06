// libycxx runtime (hosted and freestanding): delete align nothrow.
#include <new>
#include "hidden.hpp"
#include "allocation_table.hpp"

asm((ycxx::detail::hide_allocation_function("_ZdlPvSt11align_val_tRKSt9nothrow_t")));

void operator delete(void* p, std::align_val_t a, const std::nothrow_t&) noexcept {
  if (auto f = ycxx_allocation_functions.delete_align_nothrow; f != ycxx::detail::own_allocation_functions.delete_align_nothrow)
    return f(p, 0, static_cast<std::size_t>(a));
  ::operator delete(p, a);
}
