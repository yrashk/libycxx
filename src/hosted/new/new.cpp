// libycxx hosted runtime: one replaceable allocation function per file (see src/runtime/new).
#include <new>
#include <ycxx/core/error.hpp>
#include <ycxx/pal.h>
#include "../../runtime/new/hidden.hpp"
#include "../../runtime/new/allocation_table.hpp"

asm((ycxx::detail::hide_allocation_function("_Znw#")));

void* operator new(std::size_t n) {
  if (auto f = ycxx_allocation_functions.new_; f != ycxx::detail::own_allocation_functions.new_)
    return f(n, 0);
  if (n == 0)
    n = 1;
  for (;;) {
    if (void* p = ycxx_pal_allocate(n, __STDCPP_DEFAULT_NEW_ALIGNMENT__))
      return p;
    std::new_handler h = std::get_new_handler();
    if (!h)
      ycxx::detail::throw_bad_alloc();
    h();
  }
}
