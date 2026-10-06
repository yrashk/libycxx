// libycxx freestanding runtime (libycxx-freestanding.a): there is no heap, so the default
// allocation functions fail. A program with a heap replaces operator new/delete (each lives in
// its own archive member, so its definitions win). The nothrow forms return null when the
// heap-less default is what is linked, and otherwise forward (try_or_null.hpp).
#include <new>
#include <ycxx/core/error.hpp>
#include "../../runtime/new/hidden.hpp"
#include "../../runtime/new/allocation_table.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_ZdlPvSt11align_val_t")));

void operator delete(void* p, std::align_val_t a) noexcept {
  if (auto __f = __ycxx_allocation_functions.__delete_align; __f != __ycxx::__detail::__own_allocation_functions.__delete_align)
    return __f(p, 0, static_cast<std::size_t>(a));
  
}
