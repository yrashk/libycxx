// [atomics.ref.generic.general]/1: atomic_ref "applies atomic operations to the object
// referenced by *ptr"; [atomics.ref.ops]: the constructor references obj (/6); the copy
// constructor references the same object (/8); store/load/exchange/compare_exchange on the
// referenced object; operator= returns desired (/15); the conversion is load() (/19);
// /25: expected is replaced only on failure. Operations are const members.
// FLAGS: -latomic
// (-latomic: the toolchain's out-of-line atomics for types that are not lock-free; Clang does not link it implicitly)
#include <atomic>
#include "check.hpp"

struct Pair { int a, b; };

int main() {
  alignas(std::atomic_ref<int>::required_alignment) int x = 5;
  const std::atomic_ref<int> r(x);
  CHECK(r.load() == 5);
  r.store(6);
  CHECK(r.load() == 6);
  CHECK((r = 7) == 7);
  CHECK(r.load() == 7);
  CHECK(static_cast<int>(r) == 7);
  CHECK(r.exchange(8, std::memory_order::acq_rel) == 7);
  CHECK(r.load() == 8);
  int e = 1;
  CHECK(!r.compare_exchange_strong(e, 9));
  CHECK(e == 8);
  CHECK(r.compare_exchange_strong(e, 9));
  CHECK(e == 8 && r.load() == 9);
  e = 9;
  while (!r.compare_exchange_weak(e, 10, std::memory_order::release, std::memory_order::relaxed))
    CHECK(e == 9);
  CHECK(r.load() == 10);

  std::atomic_ref<int> r2(r);  // references the same object
  r2.store(11);
  CHECK(r.load() == 11);
  CHECK(r.is_lock_free() == r2.is_lock_free());

  // All access goes through atomic_ref while any reference exists.
  r.store(12);
  CHECK(std::atomic_ref<int>(x).load() == 12);

  alignas(std::atomic_ref<Pair>::required_alignment) Pair p{1, 2};
  std::atomic_ref<Pair> rp(p);
  Pair old = rp.exchange(Pair{3, 4});
  CHECK(old.a == 1 && old.b == 2);
  Pair current = rp.load();
  CHECK(current.a == 3 && current.b == 4);
  Pair ep{0, 0};
  CHECK(!rp.compare_exchange_strong(ep, Pair{5, 6}));
  CHECK(ep.a == 3 && ep.b == 4);
  CHECK(rp.compare_exchange_strong(ep, Pair{5, 6}));
  CHECK(rp.load().a == 5);

  alignas(std::atomic_ref<bool>::required_alignment) bool b = false;
  std::atomic_ref<bool> rb(b);
  CHECK(!rb.exchange(true));
  CHECK(rb.load());

  // wait returns when the value differs
  r.wait(0);
  r.notify_one();
  r.notify_all();
  // Ordinary access resumes after the last referencing instance is destroyed.
  alignas(std::atomic_ref<int>::required_alignment) int plain = 0;
  { std::atomic_ref<int> temporary(plain); temporary.store(13); }
  CHECK(plain == 13);
  return 0;
}
