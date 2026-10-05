// libycxx runtime (hosted and freestanding): delete array sized.
#include <new>
#include "hidden.hpp"

asm((ycxx::detail::hide_allocation_function("_ZdaPv#")));

void operator delete[](void* p, std::size_t) noexcept { ::operator delete[](p); }
