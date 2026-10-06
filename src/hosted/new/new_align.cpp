// libycxx hosted runtime: one replaceable allocation function per file (see src/runtime/new).
#include <new>
#include <ycxx/core/error.hpp>
#include <ycxx/pal.h>
#include "../../runtime/new/hidden.hpp"
#include "../../runtime/new/allocation_table.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_Znw#St11align_val_t")));

void* operator new(std::size_t n, std::align_val_t a) {
  if (auto __f = __ycxx_allocation_functions.__new_align; __f != __ycxx::__detail::__own_allocation_functions.__new_align)
    return __f(n, static_cast<std::size_t>(a));
  if (n == 0)
    n = 1;
  for (;;) {
    if (void* p = ycxx_pal_allocate(n, static_cast<std::size_t>(a)))
      return p;
    std::new_handler h = std::get_new_handler();
    if (!h)
      __ycxx::__detail::__throw_bad_alloc();
    h();
  }
}
