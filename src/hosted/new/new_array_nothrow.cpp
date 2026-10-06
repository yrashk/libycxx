// libycxx hosted runtime: one replaceable allocation function per file (see src/runtime/new).
#include <new>
#include <ycxx/core/error.hpp>
#include <ycxx/pal.h>
#include "../../runtime/new/hidden.hpp"
#include "../../runtime/new/allocation_table.hpp"

asm((ycxx::detail::hide_allocation_function("_Zna#RKSt9nothrow_t")));

void* operator new[](std::size_t n, const std::nothrow_t&) noexcept {
  if (auto f = ycxx_allocation_functions.new_array_nothrow; f != ycxx::detail::own_allocation_functions.new_array_nothrow)
    return f(n, 0);
  try {
    return ::operator new[](n);
  } catch (...) {
    return nullptr;
  }
}
