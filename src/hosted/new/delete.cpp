// libycxx hosted runtime: one replaceable allocation function per file (see src/runtime/new).
#include <new>
#include <ycxx/core/error.hpp>
#include <ycxx/pal.h>
#include "../../runtime/new/hidden.hpp"
#include "../../runtime/new/allocation_table.hpp"

asm((ycxx::detail::hide_allocation_function("_ZdlPv")));

void operator delete(void* p) noexcept {
  if (auto f = ycxx_allocation_functions.delete_; f != ycxx::detail::own_allocation_functions.delete_)
    return f(p, 0, 0);
  ycxx_pal_deallocate(p, 0, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
}
