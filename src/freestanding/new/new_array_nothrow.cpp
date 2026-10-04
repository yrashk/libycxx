// libycxx freestanding runtime (libycxx-freestanding.a): there is no heap, so the default
// allocation functions fail. A program with a heap replaces operator new/delete (each lives in
// its own archive member, so its definitions win). The nothrow forms return null when the
// heap-less default is what is linked, and otherwise forward (try_or_null.hpp).
#include <new>
#include <ycxx/core/error.hpp>
#include "try_or_null.hpp"

void* operator new[](std::size_t n, const std::nothrow_t&) noexcept {
  // The array default forwards to the single form, so both must be the defaults for null.
  return ycxx::detail::try_or_null(&ycxx_default_new_array != nullptr && &ycxx_fs_default_new != nullptr, [&] { return ::operator new[](n); });
}
