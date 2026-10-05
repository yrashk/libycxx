// libycxx hosted runtime: one replaceable allocation function per file (see src/runtime/new).
#include <new>
#include <ycxx/core/error.hpp>
#include <ycxx/pal.h>
#include "../../runtime/new/hidden.hpp"

asm((ycxx::detail::hide_allocation_function("_Zna#RKSt9nothrow_t")));

void* operator new[](std::size_t n, const std::nothrow_t&) noexcept {
  try {
    return ::operator new[](n);
  } catch (...) {
    return nullptr;
  }
}
