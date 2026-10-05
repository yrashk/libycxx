// libycxx runtime (hosted and freestanding): delete nothrow.
#include <new>
#include "hidden.hpp"

asm((ycxx::detail::hide_allocation_function("_ZdlPvRKSt9nothrow_t")));

void operator delete(void* p, const std::nothrow_t&) noexcept { ::operator delete(p); }
