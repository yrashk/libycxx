// libycxx freestanding runtime (libycxx-freestanding.a): there is no heap, so the default
// allocation functions fail. A program with a heap replaces operator new/delete (each lives in
// its own archive member, so its definitions win). The nothrow forms return null when the
// heap-less default is what is linked, and otherwise forward (try_or_null.hpp).
#include <new>
#include <ycxx/core/error.hpp>
#include "try_or_null.hpp"
#include "../../runtime/new/hidden.hpp"

asm((ycxx::detail::hide_allocation_function("_Zna#St11align_val_tRKSt9nothrow_t")));

void* operator new[](std::size_t n, std::align_val_t a, const std::nothrow_t&) noexcept {
  // The array default forwards to the single form, so both must be the defaults for null.
  return ycxx::detail::try_or_null(&ycxx_default_new_array_align != nullptr && &ycxx_fs_default_new_align != nullptr, [&] { return ::operator new[](n, a); });
}
