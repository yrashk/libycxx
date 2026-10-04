// [atomics.ref.int]/6-9, [atomics.ref.float], [atomics.ref.pointer], [atomics.ref.memop]:
// fetch_key returns the value before the effects; op= returns the new value; ++/-- (prefix:
// "return fetch_add(1) + 1;", postfix: "return fetch_add(1);"); signed arithmetic wraps (/8);
// fetch_max / fetch_min use max / min.
#include <atomic>
#include <climits>
#include "check.hpp"

int main() {
  alignas(std::atomic_ref<int>::required_alignment) int x = 5;
  std::atomic_ref<int> r(x);
  CHECK(r.fetch_add(3) == 5 && x == 8);
  CHECK(r.fetch_sub(10) == 8 && x == -2);
  CHECK(r.fetch_and(7) == -2 && x == 6);
  CHECK(r.fetch_or(9) == 6 && x == 15);
  CHECK(r.fetch_xor(5) == 15 && x == 10);
  CHECK((r += 5) == 15);
  CHECK((r -= 20) == -5);
  CHECK((r &= 6) == 2);
  CHECK((r |= 1) == 3);
  CHECK((r ^= 2) == 1);
  CHECK(++r == 2 && r++ == 2 && x == 3);
  CHECK(--r == 2 && r-- == 2 && x == 1);
  x = INT_MAX;
  CHECK(r.fetch_add(1) == INT_MAX && x == INT_MIN);
  CHECK(r.fetch_max(-5) == INT_MIN && x == -5);
  CHECK(r.fetch_min(-9) == -5 && x == -9);

  alignas(std::atomic_ref<unsigned short>::required_alignment) unsigned short us = 0;
  std::atomic_ref<unsigned short> rus(us);
  CHECK(rus.fetch_sub(1) == 0 && us == 0xFFFF);

  alignas(std::atomic_ref<double>::required_alignment) double d = 1.0;
  std::atomic_ref<double> rd(d);
  CHECK(rd.fetch_add(2.5) == 1.0 && d == 3.5);
  CHECK(rd.fetch_sub(0.5) == 3.5 && d == 3.0);
  CHECK((rd += 1.0) == 4.0);
  CHECK((rd -= 8.0) == -4.0);
  CHECK(rd.fetch_max(1.0) == -4.0 && d == 1.0);
  CHECK(rd.fetch_min(-1.0) == 1.0 && d == -1.0);

  long arr[6] = {};
  alignas(std::atomic_ref<long*>::required_alignment) long* p = arr;
  std::atomic_ref<long*> rp(p);
  CHECK(rp.fetch_add(4) == arr && p == arr + 4);
  CHECK(rp.fetch_sub(1) == arr + 4 && p == arr + 3);
  CHECK((rp += 2) == arr + 5);
  CHECK((rp -= 5) == arr);
  CHECK(++rp == arr + 1 && rp++ == arr + 1 && p == arr + 2);
  CHECK(--rp == arr + 1 && rp-- == arr + 1 && p == arr);
  CHECK(rp.fetch_max(arr + 3) == arr && p == arr + 3);
  CHECK(rp.fetch_min(arr + 1) == arr + 3 && p == arr + 1);
  return 0;
}
