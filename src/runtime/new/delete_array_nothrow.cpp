// libycxx runtime (hosted and freestanding): delete array nothrow.
#include <new>

[[gnu::weak]] void operator delete[](void* p, const std::nothrow_t&) noexcept { ::operator delete[](p); }
