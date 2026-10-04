// libycxx runtime (hosted and freestanding): delete array.
#include <new>

void operator delete[](void* p) noexcept { ::operator delete(p); }
