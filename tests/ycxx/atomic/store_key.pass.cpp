// [atomics.types.int]/10-13 and [atomics.types.pointer]/11-: store_key (store_add, store_sub,
// store_and, store_or, store_xor, store_max, store_min) return void and "Atomically replaces
// the value pointed to by this with the result of the computation applied to the value pointed
// to by this and the given operand." Signed arithmetic wraps (/13); store_max/store_min
// compare as signed. [atomics.types.float]: store_add/store_sub for floating-point types.
// FLAGS: -latomic
// (-latomic: the toolchain's out-of-line atomics for types that are not lock-free; Clang does not link it implicitly)
#include <atomic>
#include <climits>
#include <type_traits>
#include "check.hpp"

int main() {
  std::atomic<int> a(10);
  static_assert(std::is_void_v<decltype(a.store_add(1))>);
  a.store_add(5);
  CHECK(a.load() == 15);
  a.store_sub(20, std::memory_order::release);
  CHECK(a.load() == -5);
  a.store_max(-7);
  CHECK(a.load() == -5);
  a.store_max(3);
  CHECK(a.load() == 3);
  a.store_min(-1, std::memory_order::relaxed);
  CHECK(a.load() == -1);
  a.store_and(6);
  CHECK(a.load() == 6);
  a.store_or(9);
  CHECK(a.load() == 15);
  a.store_xor(5);
  CHECK(a.load() == 10);
  a = INT_MAX;
  a.store_add(1);
  CHECK(a.load() == INT_MIN);

  int arr[5] = {};
  std::atomic<int*> p(arr);
  p.store_add(3);
  CHECK(p.load() == arr + 3);
  p.store_sub(1);
  CHECK(p.load() == arr + 2);
  p.store_max(arr + 4);
  CHECK(p.load() == arr + 4);
  p.store_min(arr + 1);
  CHECK(p.load() == arr + 1);

  std::atomic<double> d(1.5);
  d.store_add(2.0);
  CHECK(d.load() == 3.5);
  d.store_sub(0.5);
  CHECK(d.load() == 3.0);
  return 0;
}
