// libycxx runtime (hosted and freestanding): delete array sized.
#include <new>

void operator delete[](void* p, std::size_t) noexcept { ::operator delete[](p); }
