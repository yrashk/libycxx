// libycxx runtime (hosted and freestanding): delete array.
#include <new>

[[gnu::weak]] void operator delete[](void* p) noexcept { ::operator delete(p); }
