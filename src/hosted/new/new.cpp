// libycxx hosted runtime: one replaceable allocation function per file (see src/runtime/new).
#include <new>
#include <ycxx/core/error.hpp>
#include <ycxx/pal.h>
#include "../../runtime/new/hidden.hpp"
#include "../../runtime/new/allocation_table.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_Znw#")));

void* operator new(std::size_t n) {
  if (auto __f = __ycxx_allocation_functions.__new_; __f != __ycxx::__detail::__own_allocation_functions.__new_)
    return __f(n, 0);
  if (n == 0)
    n = 1;
  for (;;) {
    if (void* p = __ycxx_pal_allocate(n, __STDCPP_DEFAULT_NEW_ALIGNMENT__))
      return p;
    std::new_handler h = std::get_new_handler();
    if (!h)
      __ycxx::__detail::__throw_bad_alloc();
    h();
  }
}
