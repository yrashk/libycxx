// [atomics.ref.ops]/25: compare_exchange "atomically compares the value representation of the
// value referenced by *ptr for equality with that previously retrieved from expected".
// [basic.types.general]/4: padding bits are not part of the value representation. So for a
// struct with padding, a compare-exchange through an atomic_ref succeeds whenever the members
// match, whatever the padding bytes of the referenced object and of expected contain. (A weak
// exchange may fail spuriously, /27, but not forever.)
// FLAGS: -latomic
// (-latomic: the toolchain's out-of-line atomics for types that are not lock-free; Clang does not link it implicitly)
#include <atomic>
#include <cstring>
#include "check.hpp"

struct padded {
  char c;
  // padding
  int i;
};
static_assert(sizeof(padded) > sizeof(char) + sizeof(int), "the test needs padding");

struct padded_big {
  char c;
  // padding
  long long v[3];
};

template<class T>
static T with_garbage(T value, unsigned char g) {
  T r;
  std::memset(&r, g, sizeof r);
  std::memcpy(&r.c, &value.c, sizeof value.c);  // set the members only; padding stays garbage
  if constexpr (requires { value.i; }) std::memcpy(&r.i, &value.i, sizeof value.i);
  else std::memcpy(&r.v, &value.v, sizeof value.v);
  return r;
}

template<class T>
void run(T v1, T v2) {
  alignas(std::atomic_ref<T>::required_alignment) T obj;
  T tmp = with_garbage(v1, 0x5A);
  std::memcpy(&obj, &tmp, sizeof obj);  // garbage padding in the referenced object
  std::atomic_ref<T> r(obj);  // from here on, obj is accessed only through atomic_refs
  T e = with_garbage(v1, 0x33);
  CHECK(r.compare_exchange_strong(e, with_garbage(v2, 0x77)));
  T e2 = with_garbage(v2, 0xEE);
  int tries = 0;
  while (!r.compare_exchange_weak(e2, v1)) CHECK(++tries < 1000);
  // a member mismatch still fails and reports the current value
  T e3 = v2;
  CHECK(!r.compare_exchange_strong(e3, v2));
  CHECK(std::memcmp(&e3.c, &v1.c, sizeof v1.c) == 0);
}

int main() {
  run(padded{'a', 1}, padded{'b', 2});
  run(padded_big{'a', {1, 2, 3}}, padded_big{'b', {4, 5, 6}});
  return 0;
}
