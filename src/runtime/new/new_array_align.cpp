// libycxx runtime (hosted and freestanding): new array align.
#include <new>

void* operator new[](std::size_t n, std::align_val_t a) { return ::operator new(n, a); }
