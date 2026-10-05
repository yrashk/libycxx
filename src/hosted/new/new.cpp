// libycxx hosted runtime: one replaceable allocation function per file (see src/runtime/new).
#include <new>
#include <ycxx/core/error.hpp>
#include <ycxx/pal.h>

[[gnu::weak]] void* operator new(std::size_t n) {
  if (n == 0)
    n = 1;
  for (;;) {
    if (void* p = ycxx_pal_allocate(n, __STDCPP_DEFAULT_NEW_ALIGNMENT__))
      return p;
    std::new_handler h = std::get_new_handler();
    if (!h)
      ycxx::detail::throw_bad_alloc();
    h();
  }
}
