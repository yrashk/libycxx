// libycxx runtime (hosted and freestanding): delete array.
#include <new>
#include "hidden.hpp"

asm((ycxx::detail::hide_allocation_function("_ZdaPv")));

void operator delete[](void* p) noexcept { ::operator delete(p); }
