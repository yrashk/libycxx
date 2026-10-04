// [atomics.types.float]/6-7: fetch_add / fetch_sub "Atomically replaces the value pointed to by
// this with the result of the computation applied to the value pointed to by this and the given
// operand" and return the previous value; /8: "the operations otherwise have no undefined
// behavior". [atomics.types.operations]/10: store replaces the value. A read-modify-write
// after a store must therefore complete, for every floating-point type, including long double,
// whose object representation may contain padding bits ([basic.types.general]/4: not part of
// the value representation). Each operation follows a store / exchange / assignment directly.
// For atomic_ref ([atomics.ref.float]/6-8) the referenced object's padding bytes hold whatever
// they held before (here: all ones), which must not matter either.
// FLAGS: -latomic
// (-latomic: the toolchain's out-of-line atomics for types that are not lock-free; Clang does not link it implicitly)
#include <atomic>
#include <cstring>
#include "check.hpp"
#include "watchdog.hpp"

template<class T>
void run() {
  std::atomic<T> a(T(1));
  a.store(T(5));
  CHECK(a.fetch_sub(T(1)) == T(5));
  a.store(T(5), std::memory_order::release);
  CHECK(a.fetch_add(T(1)) == T(5));
  a = T(8);
  CHECK((a -= T(2)) == T(6));
  a.exchange(T(3));
  CHECK((a += T(1)) == T(4));
  T e = T(4);
  CHECK(a.compare_exchange_strong(e, T(9)));
  CHECK(a.fetch_sub(T(0.5)) == T(9));
  CHECK(a.load() == T(8.5));

  alignas(std::atomic_ref<T>::required_alignment) T x = T(2);
  std::atomic_ref<T> r(x);
  r.store(T(7));
  CHECK(r.fetch_sub(T(1)) == T(7));
  r = T(3);
  CHECK((r += T(1)) == T(4));

  alignas(std::atomic_ref<T>::required_alignment) T y;
  std::memset(&y, 0xff, sizeof y);
  y = T(-1);  // the value bits are set; padding bits (if any) keep their previous contents
  std::atomic_ref<T> ry(y);
  CHECK(ry.fetch_add(T(7)) == T(-1));
  CHECK(ry.fetch_sub(T(2)) == T(6));
  CHECK(ry.load() == T(4));
}

int main() {
  watchdog(10);
  run<float>();
  run<double>();
  run<long double>();
  return 0;
}
