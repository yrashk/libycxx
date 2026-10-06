// [std.modules]/2: std also exports the global allocation and deallocation functions of <new>, the
// placement forms included, and a program can still replace the replaceable ones
// ([replacement.functions]).
// MODULES: std
// REQUIRES: exceptions
import std;
#include "module_check.hpp"

int allocations = 0;
void* operator new(std::size_t n) {
  ++allocations;
  if (void* p = std::malloc(n ? n : 1))
    return p;
  throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

struct alignas(64) wide {
  int v = 3;
};

int main() {
  int start = allocations; // the library may allocate before main (static initialization)
  void* raw = ::operator new(16);
  CHECK(allocations == start + 1);
  ::operator delete(raw);
  void* arr = ::operator new[](8, std::nothrow);
  CHECK(arr != nullptr);
  ::operator delete[](arr);
  void* al = ::operator new(64, std::align_val_t{64});
  CHECK(reinterpret_cast<std::uintptr_t>(al) % 64 == 0);
  ::operator delete(al, std::align_val_t{64});
  alignas(int) unsigned char storage[sizeof(int)];
  int* placed = ::new (static_cast<void*>(storage)) int(5);
  CHECK(*placed == 5);
  auto* w = new wide;
  CHECK(w->v == 3 && reinterpret_cast<std::uintptr_t>(w) % 64 == 0);
  delete w;
  int before = allocations;
  delete new int(1);
  CHECK(allocations == before + 1);
  CHECK(std::launder(placed) == placed);
  return 0;
}
