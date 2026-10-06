// Without a replacement, objects whose storage one image allocated are destroyed by the other:
// a program and a shared library that both use the library (linked through the same wrapper, so
// with a static libycxx the shared library carries its own copy of the library).
//   [basic.stc.dynamic.deallocation]/3-4: the value of the first argument of a deallocation
//     function is one returned by an earlier call of the corresponding allocation function; the
//     program has one set of global allocation functions ([replacement.functions]), so storage
//     allocated through it in one image may be deallocated through it in the other.
//   [string.cons], [vector.cons]: the objects are usable, and destroyed, wherever they are.
// Under AddressSanitizer every allocation is checked against its deallocation
// (alloc-dealloc-mismatch, new-delete-type-mismatch).
// FLAGS: -fPIC
// SHARED: ../support/linkage/shared_alloc_lib.cpp
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

int main() {
  lib_allocate_and_free();
  for (int i = 0; i < 100; ++i) {
    Made<std::string> ms(lib_make_string);
    Made<std::vector<int>> mv(lib_make_vector);
    std::string& s = *ms.get();
    std::vector<int>& v = *mv.get();
    CHECK(s.size() == 200 && s[199] == 'x' && v.size() == 100 && v[0] == 7);
    s += std::string(500, 'z');  // reallocated here, the shared library's block freed here
    v.resize(1000);
    CHECK(s.size() == 700 && v.size() == 1000);
  }
  for (int i = 0; i < 100; ++i) {
    alignas(std::string) unsigned char sbuf[sizeof(std::string)];
    alignas(std::vector<int>) unsigned char vbuf[sizeof(std::vector<int>)];
    lib_destroy_string(::new (sbuf) std::string(300, 'y'));
    lib_destroy_vector(::new (vbuf) std::vector<int>(50, 1));
  }
  return 0;
}
