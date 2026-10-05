// libycxx hosted runtime: one replaceable allocation function per file (see src/runtime/new).
#include <new>
#include <ycxx/core/error.hpp>
#include <ycxx/pal.h>
#include "../../runtime/new/hidden.hpp"

asm((ycxx::detail::hide_allocation_function("_Znw#RKSt9nothrow_t")));

// [new.delete.single]/7: calls operator new(size); a null pointer if that throws bad_alloc.
void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
  try {
    return ::operator new(n);
  } catch (...) {
    return nullptr;
  }
}
