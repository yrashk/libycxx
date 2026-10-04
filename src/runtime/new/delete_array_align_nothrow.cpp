// libycxx runtime (hosted and freestanding): delete array align nothrow.
#include <new>

void operator delete[](void* p, std::align_val_t a, const std::nothrow_t&) noexcept { ::operator delete[](p, a); }
