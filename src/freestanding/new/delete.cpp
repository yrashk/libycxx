// libycxx freestanding runtime (libycxx-freestanding.a): there is no heap, so the default
// allocation functions fail. A program with a heap replaces operator new/delete (each lives in
// its own archive member, so its definitions win). The nothrow forms return null when the
// heap-less default is what is linked, and otherwise forward (try_or_null.hpp).
#include <new>
#include <ycxx/core/error.hpp>
#include "../../runtime/new/hidden.hpp"
#include "../../runtime/new/allocation_table.hpp"

asm((ycxx::detail::hide_allocation_function("_ZdlPv")));

// Nothing is ever allocated by the defaults.
void operator delete(void* p) noexcept {
  if (auto f = ycxx_allocation_functions.delete_; f != ycxx::detail::own_allocation_functions.delete_)
    return f(p, 0, 0);
  
}
