// libycxx runtime (hosted and freestanding): delete sized.
#include <new>
#include "hidden.hpp"
#include "allocation_table.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_ZdlPv#")));

void operator delete(void* p, std::size_t n) noexcept {
  if (auto __f = __ycxx_allocation_functions.__delete_sized; __f != __ycxx::__detail::__own_allocation_functions.__delete_sized)
    return __f(p, n, 0);
  ::operator delete(p);
}
