// The inline ABI namespace std::__y1 (DECISIONS §20.4-20.5): a construct the compilers handle
// specially still works with libycxx's declarations.
// REQUIRES: clang
// (GCC 16 does not implement P2719.)
// P2719 type-aware allocation (C++26): the compiler looks up operator new/delete taking
// std::type_identity<T> as their first parameter (where it implements P2719).
#include <new>
#include <type_traits>
#include <cstdio>
#include <cstdlib>
static int calls;
struct S {
  int v = 7;
  template <class T> void* operator new(std::type_identity<T>, std::size_t n, std::align_val_t) { ++calls; return std::malloc(n); }
  template <class T> void operator delete(std::type_identity<T>, void* p, std::size_t, std::align_val_t) { ++calls; std::free(p); }
};
int main() {
  delete new S;
  std::puts(calls == 2 ? "ok" : "FAIL");
  return calls == 2 ? 0 : 1;
}
