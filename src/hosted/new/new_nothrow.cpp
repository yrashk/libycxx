// libycxx hosted runtime: one replaceable allocation function per file (see src/runtime/new).
#include <new>
#include <ycxx/core/error.hpp>
#include <ycxx/pal.h>
#include "../../runtime/new/hidden.hpp"
#include "../../runtime/new/allocation_table.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_Znw#RKSt9nothrow_t")));
// What a ThreadSanitizer build's link options name as undefined (hidden.hpp).
extern "C" [[__gnu__::__visibility__("hidden")]] const char __ycxx_allocation_anchor_new_nothrow = 0;

// [new.delete.single]/7: calls operator new(size); a null pointer if that throws bad_alloc.
[[__gnu__::__weak__]] void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
  if (auto __f = __ycxx_allocation_functions.__new_nothrow; __f != __ycxx::__detail::__own_allocation_functions.__new_nothrow)
    return __f(n, 0);
  try {
    return ::operator new(n);
  } catch (...) {
    return nullptr;
  }
}
