// libycxx hosted runtime: one replaceable allocation function per file (see src/runtime/new).
#include <new>
#include <ycxx/core/error.hpp>
#include "try_allocate.hpp"
#include "../../runtime/new/hidden.hpp"
#include "../../runtime/new/allocation_table.hpp"
#include "../../runtime/new/default_identity.hpp"

asm((__ycxx::__detail::__hide_allocation_function("_Znw#")));
// What a ThreadSanitizer build's link options name as undefined (hidden.hpp).
extern "C" [[__gnu__::__visibility__("hidden")]] const char __ycxx_allocation_anchor_new = 0;

#if !_YCXX_HAS_EXCEPTIONS && defined(_YCXX_NEW_MACHO_SECTION) && _YCXX_NEW_MACHO_SECTION
[[__gnu__::__section__("__TEXT,__ycxx_new,regular,pure_instructions")]]
#endif
[[__gnu__::__weak__]] void* operator new(std::size_t n) {
  if (auto __f = __ycxx_allocation_functions.__new_; __f != __ycxx::__detail::__own_allocation_functions.__new_)
    return __f(n, 0);
  if (void* p = __ycxx::__detail::__try_allocate(n, __STDCPP_DEFAULT_NEW_ALIGNMENT__))
    return p;
  __ycxx::__detail::__throw_bad_alloc();
}

#if !_YCXX_HAS_EXCEPTIONS && defined(_YCXX_NEW_ELF_ALIAS) && _YCXX_NEW_ELF_ALIAS
extern "C"
#  if _YCXX_NEW_SIZE_IS_UNSIGNED_INT
[[__gnu__::__alias__("_Znwj")]]
#  else
[[__gnu__::__alias__("_Znwm")]]
#  endif
[[__gnu__::__visibility__("hidden"), __gnu__::__malloc__, __gnu__::__alloc_size__(1)]]
void* __ycxx_hosted_new_identity(std::size_t);
#endif
