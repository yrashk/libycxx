// libycxx hosted runtime: one replaceable allocation function per file (see src/runtime/new).
#include <new>
#include <ycxx/core/error.hpp>
#include <ycxx/pal.h>
#include "../../runtime/new/hidden.hpp"

asm((ycxx::detail::hide_allocation_function("_ZdlPvSt11align_val_t")));

void operator delete(void* p, std::align_val_t a) noexcept {
  ycxx_pal_deallocate(p, 0, static_cast<std::size_t>(a));
}
