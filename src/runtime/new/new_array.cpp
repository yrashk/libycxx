// libycxx runtime (hosted and freestanding): new array.
#include <new>
#include "hidden.hpp"
#include "allocation_table.hpp"
#include "default_identity.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_Zna#")));
// What a ThreadSanitizer build's link options name as undefined (hidden.hpp).
extern "C" [[__gnu__::__visibility__("hidden")]] const char __ycxx_allocation_anchor_new_array = 0;

#if !_YCXX_HAS_EXCEPTIONS && _YCXX_NEW_MACHO_SECTION
[[__gnu__::__section__("__TEXT,__ycxx_new,regular,pure_instructions")]]
#endif
[[__gnu__::__weak__]] void* operator new[](std::size_t n) {
  if (auto __f = __ycxx_allocation_functions.__new_array; __f != __ycxx::__detail::__own_allocation_functions.__new_array)
    return __f(n, 0);
  return ::operator new(n);
}
// Marks that the default (forwarding) operator new[] is linked (see
// src/freestanding/new/try_or_null.hpp).
extern "C" [[__gnu__::__visibility__("hidden")]] const char __ycxx_default_new_array = 0;

#if !_YCXX_HAS_EXCEPTIONS && _YCXX_NEW_ELF_ALIAS
extern "C" [[__gnu__::__alias__("_Zna" _YCXX_NEW_SIZE_CODE),
             __gnu__::__visibility__("hidden"), __gnu__::__malloc__, __gnu__::__alloc_size__(1)]]
void* __ycxx_hosted_new_array_identity(std::size_t);
#endif
