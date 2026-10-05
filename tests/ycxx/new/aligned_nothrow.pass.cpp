// [new.delete.single], [new.delete.array]: the align_val_t and nothrow_t overloads of
// operator new/delete. The aligned forms return storage aligned to the requested alignment;
// the nothrow forms return nullptr instead of throwing; new-expressions for over-aligned
// types use the aligned forms. [set.new.handler], [get.new.handler].
// UNSUPPORTED-SANITIZER: asan  ASan replaces the global allocation functions: its operator new neither calls the new_handler nor throws for impossible sizes, and its other forms do not forward to a program's replacement ([new.delete])
// REQUIRES: exceptions
#include <new>
#include <cstddef>
#include <cstdint>
#include "check.hpp"

struct alignas(128) Over {
  int v = 3;
};

void handler() {}

int main() {
  void* p = ::operator new(64, std::align_val_t{256});
  CHECK(reinterpret_cast<std::uintptr_t>(p) % 256 == 0);
  ::operator delete(p, std::align_val_t{256});
  void* q = ::operator new[](40, std::align_val_t{64});
  CHECK(reinterpret_cast<std::uintptr_t>(q) % 64 == 0);
  ::operator delete[](q, std::align_val_t{64});
  void* n = ::operator new(16, std::nothrow);
  CHECK(n != nullptr);
  ::operator delete(n, std::nothrow);
  void* na = ::operator new(16, std::align_val_t{32}, std::nothrow);
  CHECK(na != nullptr && reinterpret_cast<std::uintptr_t>(na) % 32 == 0);
  ::operator delete(na, std::align_val_t{32}, std::nothrow);
  // sized deallocation forms
  void* s = ::operator new(24);
  ::operator delete(s, 24);
  void* sa = ::operator new(24, std::align_val_t{64});
  ::operator delete(sa, 24, std::align_val_t{64});

  Over* o = new Over;
  CHECK(reinterpret_cast<std::uintptr_t>(o) % 128 == 0);
  CHECK(o->v == 3);
  delete o;
  Over* oa = new Over[3];
  CHECK(reinterpret_cast<std::uintptr_t>(oa) % 128 == 0);
  delete[] oa;

  // an impossible request: nothrow form returns nullptr, throwing form throws bad_alloc
  void* huge = ::operator new(static_cast<std::size_t>(-1) / 2, std::nothrow);
  CHECK(huge == nullptr);
  bool caught = false;
  try {
    void* h2 = ::operator new(static_cast<std::size_t>(-1) / 2);
    ::operator delete(h2);
  } catch (const std::bad_alloc&) {
    caught = true;
  }
  CHECK(caught);

  std::new_handler old = std::set_new_handler(handler);
  CHECK(std::get_new_handler() == handler);
  CHECK(std::set_new_handler(old) == handler);
  CHECK(std::get_new_handler() == old);
  return 0;
}
