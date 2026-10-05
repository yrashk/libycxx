// libycxx runtime (hosted and freestanding): delete sized align.
#include <new>
#include "hidden.hpp"

asm((ycxx::detail::hide_allocation_function("_ZdlPv#St11align_val_t")));

void operator delete(void* p, std::size_t, std::align_val_t a) noexcept { ::operator delete(p, a); }
