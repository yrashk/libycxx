// libycxx runtime (hosted and freestanding): delete array align nothrow.
#include <new>
#include "hidden.hpp"

asm((ycxx::detail::hide_allocation_function("_ZdaPvSt11align_val_tRKSt9nothrow_t")));

void operator delete[](void* p, std::align_val_t a, const std::nothrow_t&) noexcept { ::operator delete[](p, a); }
