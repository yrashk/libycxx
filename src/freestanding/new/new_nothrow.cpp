// libycxx freestanding runtime (libycxx-freestanding.a): there is no heap, so the default
// allocation functions fail. A program with a heap replaces operator new/delete (each lives in
// its own archive member, so its definitions win). The nothrow forms return null when the
// heap-less default is what is linked, and otherwise forward (try_or_null.hpp).
#include <new>
#include <ycxx/core/error.hpp>
#include "try_or_null.hpp"
#include "../../runtime/new/hidden.hpp"
#include "../../runtime/new/allocation_table.hpp"

asm((ycxx::detail::hide_allocation_function("_Znw#RKSt9nothrow_t")));

void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
  if (auto f = ycxx_allocation_functions.new_nothrow; f != ycxx::detail::own_allocation_functions.new_nothrow)
    return f(n, 0);
  return ycxx::detail::try_or_null(&ycxx_fs_default_new != nullptr, [&] { return ::operator new(n); });
}
