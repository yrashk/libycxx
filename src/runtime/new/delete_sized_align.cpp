// libycxx runtime (hosted and freestanding): delete sized align.
#include <new>

[[gnu::weak]] void operator delete(void* p, std::size_t, std::align_val_t a) noexcept { ::operator delete(p, a); }
