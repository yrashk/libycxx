// [atomics.types.int]/9: "For fetch_max and fetch_min, the maximum and minimum computation is
// performed as if by max and min algorithms ([alg.min.max]), respectively, with the object
// value and the first parameter as the arguments." (signed comparison: no unsigned conversion,
// /8 excludes them). Returns the old value (/7). [atomics.types.pointer]/10: the same for
// pointers. [atomics.nonmembers]: atomic_fetch_max / atomic_fetch_min(_explicit).
#include <atomic>
#include <climits>
#include "check.hpp"

int main() {
  std::atomic<int> a(5);
  CHECK(a.fetch_max(3) == 5);
  CHECK(a.load() == 5);
  CHECK(a.fetch_max(9) == 5);
  CHECK(a.load() == 9);
  CHECK(a.fetch_min(12) == 9);
  CHECK(a.load() == 9);
  CHECK(a.fetch_min(-4, std::memory_order::relaxed) == 9);
  CHECK(a.load() == -4);  // signed comparison
  CHECK(a.fetch_max(INT_MIN) == -4);
  CHECK(a.load() == -4);

  std::atomic<unsigned> u(5);
  CHECK(u.fetch_max(UINT_MAX) == 5);
  CHECK(u.load() == UINT_MAX);
  CHECK(u.fetch_min(0u) == UINT_MAX);
  CHECK(u.load() == 0);

  volatile std::atomic<long> v(1);
  if constexpr (std::atomic<long>::is_always_lock_free) {
    CHECK(v.fetch_max(2) == 1);
    CHECK(v.fetch_min(0) == 2);
    CHECK(v.load() == 0);
  }

  CHECK(std::atomic_fetch_max(&a, 100) == -4);
  CHECK(std::atomic_fetch_min_explicit(&a, 50, std::memory_order::seq_cst) == 100);
  CHECK(a.load() == 50);

  int arr[4] = {};
  std::atomic<int*> p(arr + 1);
  CHECK(p.fetch_max(arr + 3) == arr + 1);
  CHECK(p.load() == arr + 3);
  CHECK(p.fetch_min(arr + 2) == arr + 3);
  CHECK(p.load() == arr + 2);
  CHECK(p.fetch_max(arr) == arr + 2);
  CHECK(p.load() == arr + 2);
  return 0;
}
