// libycxx runtime (hosted and freestanding): delete array align.
#include <new>

void operator delete[](void* p, std::align_val_t a) noexcept { ::operator delete(p, a); }
