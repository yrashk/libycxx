// libycxx freestanding runtime (libycxx-freestanding.a): there is no heap, so the default
// allocation functions fail. A program with a heap replaces operator new/delete (each lives in
// its own archive member, so its definitions win). The nothrow forms return null when the
// heap-less default is what is linked, and otherwise forward (try_or_null.hpp).
#include <new>
#include <ycxx/core/error.hpp>
#include "try_or_null.hpp"
#include "../../runtime/new/hidden.hpp"
#include "../../runtime/new/allocation_table.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_Znw#RKSt9nothrow_t")));

void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
  if (auto __f = __ycxx_allocation_functions.__new_nothrow; __f != __ycxx::__detail::__own_allocation_functions.__new_nothrow)
    return __f(n, 0);
  return __ycxx::__detail::__try_or_null(&__ycxx_fs_default_new != nullptr, [&] { return ::operator new(n); });
}
