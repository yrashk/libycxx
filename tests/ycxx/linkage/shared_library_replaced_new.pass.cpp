// A program's replacement of the global allocation functions is the one used everywhere in the
// program, also by the code of a shared library that uses the library (linked through the same
// wrapper, so with a static libycxx it carries its own copy of the library):
//   [replacement.functions]/2-3: "A C++ program may provide the definition for any of the
//     following dynamic memory allocation function signatures declared in header <new>"; "The
//     program's definitions are used instead of the default versions supplied by the
//     implementation". The standard has no shared libraries; a split into a program and a shared
//     library must not change which definitions are used.
//   [new.delete.single], [new.delete.array]: the default array and aligned forms call the
//     corresponding single-object form, so replacing those (and their aligned forms) observes
//     every allocation; the nothrow single-object form is replaced too, and the shared library's
//     nothrow allocation must reach that replacement.
// FLAGS: -fPIC
// UNSUPPORTED-SANITIZER: asan,tsan  the sanitizer runtime's replacement would compete with the test's own
// SHARED: ../support/linkage/shared_alloc_lib.cpp
#include <atomic>
#include <cstdlib>
#include <memory>
#include <new>
#include <string>
#include <vector>
#include "check.hpp"
#include "../support/linkage/shared_alloc.hpp"

// A std::string / std::vector<int> made in the shared library, owned (and destroyed) here.
template <class T>
struct Made {
  alignas(T) unsigned char buf[sizeof(T)];
  explicit Made(void (*make)(void*)) { make(buf); }
  ~Made() { std::destroy_at(get()); }
  T* get() { return std::launder(reinterpret_cast<T*>(buf)); }
};

namespace {
std::atomic<long> news{0}, deletes{0}, nothrow_news{0};
void* allocate(std::size_t n, std::size_t align) {
  ++news;
  void* p = nullptr;
  if (::posix_memalign(&p, align < sizeof(void*) ? sizeof(void*) : align, n ? n : 1) != 0)
    throw std::bad_alloc();
  return p;
}
void release(void* p) {
  if (p) {
    ++deletes;
    std::free(p);
  }
}
} // namespace

void* operator new(std::size_t n) { return allocate(n, __STDCPP_DEFAULT_NEW_ALIGNMENT__); }
void* operator new(std::size_t n, std::align_val_t a) { return allocate(n, static_cast<std::size_t>(a)); }
void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
  ++nothrow_news;
  try {
    return allocate(n, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
  } catch (...) {
    return nullptr;
  }
}
void operator delete(void* p) noexcept { release(p); }
void operator delete(void* p, std::size_t) noexcept { release(p); }
void operator delete(void* p, std::align_val_t) noexcept { release(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { release(p); }

int main() {
  long n0 = news, d0 = deletes, t0 = nothrow_news;
  lib_allocate_and_free(); // 7 allocations, each freed, all inside the shared library
  CHECK(news - n0 == 7);
  CHECK(deletes - d0 == 7);
  CHECK(nothrow_news - t0 == 1); // new (nothrow) int ([new.delete.array]: new[] nothrow calls new[](size))

  // Objects allocated in the shared library and freed here, and the reverse.
  n0 = news, d0 = deletes;
  {
    Made<std::string> ms(lib_make_string);
    Made<std::vector<int>> mv(lib_make_vector);
    std::string& s = *ms.get();
    std::vector<int>& v = *mv.get();
    CHECK(s.size() == 200 && v.size() == 100 && v[99] == 7);
  }
  CHECK(news - n0 == 2 && deletes - d0 == 2);
  n0 = news, d0 = deletes;
  alignas(std::string) unsigned char sbuf[sizeof(std::string)];
  alignas(std::vector<int>) unsigned char vbuf[sizeof(std::vector<int>)];
  lib_destroy_string(::new (sbuf) std::string(300, 'y'));
  lib_destroy_vector(::new (vbuf) std::vector<int>(50, 1));
  CHECK(news - n0 == 2 && deletes - d0 == 2);
  return 0;
}
