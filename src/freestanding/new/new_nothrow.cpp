// libycxx freestanding runtime (libycxx-freestanding.a): there is no heap, so the default
// allocation functions fail. A program with a heap replaces operator new/delete (each lives in
// its own archive member, so its definitions win). The nothrow forms return null when the
// heap-less default is what is linked, and otherwise forward (try_or_null.hpp).
#include <new>
#include <ycxx/core/error.hpp>
#include "try_or_null.hpp"

[[gnu::weak]] void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
  return ycxx::detail::try_or_null(&ycxx_fs_default_new != nullptr, [&] { return ::operator new(n); });
}
