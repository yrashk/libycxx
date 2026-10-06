// libycxx freestanding runtime (libycxx-freestanding.a): there is no heap, so the default
// allocation functions fail. A program with a heap replaces operator new/delete (each lives in
// its own archive member, so its definitions win). The nothrow forms return null when the
// heap-less default is what is linked, and otherwise forward (try_or_null.hpp).
#include <new>
#include <ycxx/core/error.hpp>
#include "../../runtime/new/hidden.hpp"
#include "../../runtime/new/allocation_table.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_ZdlPv")));

// Nothing is ever allocated by the defaults.
void operator delete(void* p) noexcept {
  if (auto __f = __ycxx_allocation_functions.__delete_; __f != __ycxx::__detail::__own_allocation_functions.__delete_)
    return __f(p, 0, 0);
  
}
