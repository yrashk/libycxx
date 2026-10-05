// [expr.new]/20: the allocation call's arguments are the size and, "If the type of the allocated
// object has new-extended alignment, ... the type's alignment, [of] type std::align_val_t". "If
// no matching function is found then (20.1) if the allocated object type has new-extended
// alignment, the alignment argument is removed from the argument list; (20.2) otherwise, an
// argument that is the type's alignment and has type std::align_val_t is added into the argument
// list immediately after the first argument; and then overload resolution is performed again."
// ((20.2) is tested separately: new/class_aligned_lookup_added_alignment.pass.cpp.)
// [expr.new]/13: for a class type T the allocation function is looked up in T's scope first, and
// only if not found there in the global scope.
// [expr.delete]/9: "(9.2) If the type has new-extended alignment, a function with a parameter of
// type std::align_val_t is preferred; otherwise a function without such a parameter is preferred.
// If any preferred functions are found, all non-preferred functions are eliminated ... (9.3) If
// exactly one function remains, that function is selected ... (9.4) If the deallocation
// functions belong to a class scope, the one without a parameter of type std::size_t is selected."
// [new.delete.single]: the library's operator new(size_t, align_val_t) returns storage aligned
// to the requested alignment (replaced here to observe the calls).
// UNSUPPORTED-SANITIZER: asan  ASan replaces the global allocation functions: its operator new neither calls the new_handler nor throws for impossible sizes, and its other forms do not forward to a program's replacement ([new.delete])
// REQUIRES: exceptions
#include <new>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include "check.hpp"

static int global_aligned_new = 0, global_aligned_delete = 0;
void* operator new(std::size_t n, std::align_val_t a) {
  ++global_aligned_new;
  std::size_t al = static_cast<std::size_t>(a);
  if (void* p = std::aligned_alloc(al, (n + al - 1) / al * al)) return p;
  throw std::bad_alloc();
}
void operator delete(void* p, std::align_val_t) noexcept {
  ++global_aligned_delete;
  std::free(p);
}
void operator delete(void* p, std::size_t, std::align_val_t a) noexcept { operator delete(p, a); }

static int calls[16];
static std::size_t last_align = 0;
static void* raw(std::size_t n, std::size_t al) { return std::aligned_alloc(al, (n + al - 1) / al * al); }
static bool aligned(const void* p, std::size_t a) { return reinterpret_cast<std::uintptr_t>(p) % a == 0; }

// (A) over-aligned, only the unaligned class functions: (20.1) retries without the alignment.
struct alignas(64) OnlyPlain {
  char c;
  static void* operator new(std::size_t n) { ++calls[0]; return raw(n, 64); }
  static void operator delete(void* p) { ++calls[1]; std::free(p); }
};
// (B) over-aligned, both forms: the aligned ones are used.
struct alignas(64) Both {
  char c;
  static void* operator new(std::size_t n) { ++calls[2]; return raw(n, 64); }
  static void* operator new(std::size_t n, std::align_val_t a) {
    ++calls[3];
    last_align = static_cast<std::size_t>(a);
    return raw(n, static_cast<std::size_t>(a));
  }
  static void operator delete(void* p) { ++calls[4]; std::free(p); }
  static void operator delete(void* p, std::align_val_t) { ++calls[5]; std::free(p); }
};
// (C) over-aligned, no class functions: the global aligned forms.
struct alignas(64) NoClassFns { char c; };
// (E) over-aligned arrays with class array functions.
struct alignas(64) ArrayFns {
  char c;
  ~ArrayFns() {}
  static void* operator new[](std::size_t n, std::align_val_t a) { ++calls[8]; return raw(n, static_cast<std::size_t>(a)); }
  static void operator delete[](void* p, std::align_val_t) { ++calls[9]; std::free(p); }
  static void operator delete[](void* p, std::size_t, std::align_val_t) { ++calls[10]; std::free(p); }
};
// (F) default-aligned, sized and unsized class delete: (9.4) the unsized one.
struct SizedUnsized {
  int i;
  static void operator delete(void* p) { ++calls[11]; ::operator delete(p); }
  static void operator delete(void* p, std::size_t) { ++calls[12]; ::operator delete(p); }
};

int main() {
  OnlyPlain* a = new OnlyPlain;
  CHECK(calls[0] == 1 && global_aligned_new == 0 && aligned(a, 64));
  delete a;
  CHECK(calls[1] == 1 && global_aligned_delete == 0);

  Both* b = new Both;
  CHECK(calls[3] == 1 && calls[2] == 0 && last_align == 64 && aligned(b, 64));
  delete b;
  CHECK(calls[5] == 1 && calls[4] == 0);

  NoClassFns* c = new NoClassFns;
  CHECK(global_aligned_new == 1 && aligned(c, 64));
  delete c;
  CHECK(global_aligned_delete == 1);
  NoClassFns* ca = new NoClassFns[3];   // operator new[](size_t, align_val_t) -> operator new(size, al)
  CHECK(global_aligned_new == 2 && aligned(ca, 64) && aligned(ca + 1, 64));
  delete[] ca;
  CHECK(global_aligned_delete == 2);

  ArrayFns* e = new ArrayFns[4];
  CHECK(calls[8] == 1 && aligned(e, 64) && aligned(e + 3, 64));
  delete[] e;
  CHECK(calls[9] + calls[10] == 1 && calls[9] == 1);   // (9.4): class scope, the one without size_t

  SizedUnsized* f = new SizedUnsized;
  delete f;
  CHECK(calls[11] == 1 && calls[12] == 0);
  return 0;
}
