// libycxx hosted runtime: one replaceable allocation function per file (see src/runtime/new).
#include <new>
#include <ycxx/core/error.hpp>
#include <ycxx/pal.h>
#include "../../runtime/new/hidden.hpp"
#include "../../runtime/new/allocation_table.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_ZdlPvSt11align_val_t")));
// What a ThreadSanitizer build's link options name as undefined (hidden.hpp).
extern "C" [[__gnu__::__visibility__("hidden")]] const char __ycxx_allocation_anchor_delete_align = 0;

[[__gnu__::__weak__]] void operator delete(void* p, std::align_val_t a) noexcept {
  if (auto __f = __ycxx_allocation_functions.__delete_align; __f != __ycxx::__detail::__own_allocation_functions.__delete_align)
    return __f(p, 0, static_cast<std::size_t>(a));
  ycxx_pal_deallocate(p, 0, static_cast<std::size_t>(a));
}
