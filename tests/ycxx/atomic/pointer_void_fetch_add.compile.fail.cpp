// [atomics.types.pointer]/6: fetch_key: "Mandates: T is a complete object type. [Note 1:
// Pointer arithmetic on void* or function pointers is ill-formed.]"
#include <atomic>

void f(std::atomic<void*>& p) {
  p.fetch_add(1);
}
