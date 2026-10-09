// EXPECT-ERROR-GCC: error: invalid application of 'sizeof' to incomplete type [^\n]*Incomplete
// EXPECT-ERROR-CLANG: error: invalid application of 'sizeof' to an incomplete type [^\n]*Incomplete
// [atomics.types.pointer]/6: fetch_key: "Mandates: T is a complete object type."
#include <atomic>

struct Incomplete;
void f(std::atomic<Incomplete*>& p) {
  p.fetch_sub(1);
}
