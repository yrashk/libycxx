// libycxx runtime (hosted and freestanding): delete nothrow.
#include <new>

void operator delete(void* p, const std::nothrow_t&) noexcept { ::operator delete(p); }
