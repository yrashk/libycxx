// [atomics.types.pointer]/6: fetch_key: "Mandates: T is a complete object type."
#include <atomic>

struct Incomplete;
void f(std::atomic<Incomplete*>& p) {
  p.fetch_sub(1);
}
