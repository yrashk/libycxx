// libycxx hosted runtime: one replaceable allocation function per file (see src/runtime/new).
#include <new>
#include <ycxx/core/error.hpp>
#include <ycxx/pal.h>

void* operator new(std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept {
  try {
    return ::operator new(n, a);
  } catch (...) {
    return nullptr;
  }
}
