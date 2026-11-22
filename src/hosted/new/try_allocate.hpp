// The PAL retry shared by the throwing and nothrow hosted scalar defaults.
#pragma once

#include <new>
#include <ycxx/pal.h>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail {

inline void* __try_allocate(std::size_t __n, std::size_t __alignment) {
  if (__n == 0)
    __n = 1;
  for (;;) {
    if (void* __p = ycxx_pal_allocate(__n, __alignment))
      return __p;
    std::new_handler __handler = std::get_new_handler();
    if (!__handler)
      return nullptr;
    __handler();
  }
}

}} // namespace __ycxx::__detail
