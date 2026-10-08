// The shared library of linkage/shared_library_replaced_new.pass.cpp and
// linkage/shared_library_allocation_exchange.pass.cpp.
#include "shared_alloc.hpp"
#include <memory>
#include <new>
#include <string>
#include <vector>

namespace {
struct alignas(64) Over {
  char c[64];
};
// The result of a new-expression through an empty asm statement, so that the compiler cannot omit
// the allocation ([expr.new]/14; both compilers do at -O2): the calls are what the test counts
// (check.hpp's unelided).
template <class T>
T* unelided(T* p) noexcept {
  asm volatile("" : "+r"(p));
  return p;
}
} // namespace

void lib_allocate_and_free() {
  delete unelided(new int(1));            // operator new(size_t), operator delete(void*[, size_t])
  delete[] unelided(new int[3]{});        // operator new[], operator delete[]
  delete unelided(new Over);              // the align_val_t forms
  delete[] unelided(new Over[2]);
  int* p = unelided(new (std::nothrow) int(2)); // nothrow forms
  ::operator delete(p, std::nothrow);
  int* a = unelided(new (std::nothrow) int[2]);
  ::operator delete[](a, std::nothrow);
  Over* o = unelided(new (std::nothrow) Over);
  ::operator delete(o, std::align_val_t(alignof(Over)), std::nothrow);
}

void lib_make_string(void* storage) { ::new (storage) std::string(200, 'x'); }
void lib_make_vector(void* storage) { ::new (storage) std::vector<int>(100, 7); }
void lib_destroy_string(void* storage) { std::destroy_at(static_cast<std::string*>(storage)); }
void lib_destroy_vector(void* storage) { std::destroy_at(static_cast<std::vector<int>*>(storage)); }
