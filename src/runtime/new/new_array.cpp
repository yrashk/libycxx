// libycxx runtime (hosted and freestanding): new array.
#include <new>

void* operator new[](std::size_t n) { return ::operator new(n); }
