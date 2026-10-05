// libycxx runtime (hosted and freestanding): delete sized.
#include <new>
#include "hidden.hpp"

asm((ycxx::detail::hide_allocation_function("_ZdlPv#")));

void operator delete(void* p, std::size_t) noexcept { ::operator delete(p); }
