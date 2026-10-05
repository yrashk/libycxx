// libycxx runtime (hosted and freestanding): delete sized.
#include <new>

[[gnu::weak]] void operator delete(void* p, std::size_t) noexcept { ::operator delete(p); }
