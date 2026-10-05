// libycxx runtime (hosted and freestanding): delete array align.
#include <new>
#include "hidden.hpp"

asm((ycxx::detail::hide_allocation_function("_ZdaPvSt11align_val_t")));

void operator delete[](void* p, std::align_val_t a) noexcept { ::operator delete(p, a); }
