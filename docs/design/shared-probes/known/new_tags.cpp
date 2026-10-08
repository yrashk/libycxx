// The compiler names std::align_val_t (over-aligned new-expressions), std::nothrow_t (through the
// program) and std::destroying_delete_t (a destroying operator delete).
#include <new>
#include <cstdio>
struct alignas(64) Over { char c[64]; };
struct Destroying {
  int v = 1;
  void operator delete(Destroying* p, std::destroying_delete_t) { p->~Destroying(); ::operator delete(p); }
};
int main() {
  Over* o = new Over;
  bool aligned = reinterpret_cast<__UINTPTR_TYPE__>(o) % 64 == 0;
  delete o;
  int* q = new (std::nothrow) int(3);
  bool nothrow = q && *q == 3;
  delete q;
  delete new Destroying;
  bool r = aligned && nothrow;
  std::puts(r ? "ok" : "FAIL");
  return r ? 0 : 1;
}
