// libycxx runtime (hosted and freestanding): delete array sized align.
#include <new>

void operator delete[](void* p, std::size_t, std::align_val_t a) noexcept { ::operator delete[](p, a); }
