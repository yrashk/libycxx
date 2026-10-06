// libycxx hosted runtime: one replaceable allocation function per file (see src/runtime/new).
#include <new>
#include <ycxx/core/error.hpp>
#include <ycxx/pal.h>
#include "../../runtime/new/hidden.hpp"
#include "../../runtime/new/allocation_table.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_ZdlPv")));

void operator delete(void* p) noexcept {
  if (auto __f = __ycxx_allocation_functions.__delete_; __f != __ycxx::__detail::__own_allocation_functions.__delete_)
    return __f(p, 0, 0);
  __ycxx_pal_deallocate(p, 0, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
}
