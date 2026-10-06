// libycxx runtime (hosted and freestanding): delete array align nothrow.
#include <new>
#include "hidden.hpp"
#include "allocation_table.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_ZdaPvSt11align_val_tRKSt9nothrow_t")));

void operator delete[](void* p, std::align_val_t a, const std::nothrow_t&) noexcept {
  if (auto __f = __ycxx_allocation_functions.__delete_array_align_nothrow; __f != __ycxx::__detail::__own_allocation_functions.__delete_array_align_nothrow)
    return __f(p, 0, static_cast<std::size_t>(a));
  ::operator delete[](p, a);
}
