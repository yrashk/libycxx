// libycxx hosted runtime: one replaceable allocation function per file (see src/runtime/new).
#include <new>
#include <ycxx/core/error.hpp>
#include <ycxx/pal.h>
#include "../../runtime/new/hidden.hpp"
#include "../../runtime/new/allocation_table.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_Zna#St11align_val_tRKSt9nothrow_t")));

void* operator new[](std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept {
  if (auto __f = __ycxx_allocation_functions.__new_array_align_nothrow; __f != __ycxx::__detail::__own_allocation_functions.__new_array_align_nothrow)
    return __f(n, static_cast<std::size_t>(a));
  try {
    return ::operator new[](n, a);
  } catch (...) {
    return nullptr;
  }
}
