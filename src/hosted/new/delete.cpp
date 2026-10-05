// libycxx hosted runtime: one replaceable allocation function per file (see src/runtime/new).
#include <new>
#include <ycxx/core/error.hpp>
#include <ycxx/pal.h>

[[gnu::weak]] void operator delete(void* p) noexcept { ycxx_pal_deallocate(p, 0, __STDCPP_DEFAULT_NEW_ALIGNMENT__); }
