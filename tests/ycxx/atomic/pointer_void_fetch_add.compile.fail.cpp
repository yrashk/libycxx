// EXPECT-ERROR-GCC: error: static assertion failed[^\n]*atomic pointer arithmetic needs a pointer to a complete object type
// EXPECT-ERROR-CLANG: error: invalid application of 'sizeof' to an incomplete type [^\n]*\(aka 'void'\)
// [atomics.types.pointer]/6: fetch_key: "Mandates: T is a complete object type. [Note 1:
// Pointer arithmetic on void* or function pointers is ill-formed.]"
#include <atomic>

void f(std::atomic<void*>& p) {
  p.fetch_add(1);
}
