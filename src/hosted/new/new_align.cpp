// libycxx hosted runtime: one replaceable allocation function per file (see src/runtime/new).
#include <new>
#include <ycxx/core/error.hpp>
#include "try_allocate.hpp"
#include "../../runtime/new/hidden.hpp"
#include "../../runtime/new/allocation_table.hpp"
#include "../../runtime/new/default_identity.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_Znw#St11align_val_t")));
// What a ThreadSanitizer build's link options name as undefined (hidden.hpp).
extern "C" [[__gnu__::__visibility__("hidden")]] const char __ycxx_allocation_anchor_new_align = 0;

#if !_YCXX_HAS_EXCEPTIONS && defined(_YCXX_NEW_MACHO_SECTION) && _YCXX_NEW_MACHO_SECTION
[[__gnu__::__section__("__TEXT,__ycxx_new,regular,pure_instructions")]]
#endif
[[__gnu__::__weak__]] void* operator new(std::size_t n, std::align_val_t a) {
  if (auto __f = __ycxx_allocation_functions.__new_align; __f != __ycxx::__detail::__own_allocation_functions.__new_align)
    return __f(n, static_cast<std::size_t>(a));
  if (void* p = __ycxx::__detail::__try_allocate(n, static_cast<std::size_t>(a)))
    return p;
  __ycxx::__detail::__throw_bad_alloc();
}

#if !_YCXX_HAS_EXCEPTIONS && defined(_YCXX_NEW_ELF_ALIAS) && _YCXX_NEW_ELF_ALIAS
extern "C"
#  if _YCXX_NEW_SIZE_IS_UNSIGNED_INT
[[__gnu__::__alias__("_ZnwjSt11align_val_t")]]
#  else
[[__gnu__::__alias__("_ZnwmSt11align_val_t")]]
#  endif
[[__gnu__::__visibility__("hidden"), __gnu__::__malloc__, __gnu__::__alloc_size__(1)]]
void* __ycxx_hosted_new_align_identity(std::size_t, std::align_val_t);
#endif
