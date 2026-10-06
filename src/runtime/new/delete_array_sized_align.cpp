// libycxx runtime (hosted and freestanding): delete array sized align.
#include <new>
#include "hidden.hpp"
#include "allocation_table.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_ZdaPv#St11align_val_t")));

void operator delete[](void* p, std::size_t n, std::align_val_t a) noexcept {
  if (auto __f = __ycxx_allocation_functions.__delete_array_sized_align; __f != __ycxx::__detail::__own_allocation_functions.__delete_array_sized_align)
    return __f(p, n, static_cast<std::size_t>(a));
  ::operator delete[](p, a);
}
