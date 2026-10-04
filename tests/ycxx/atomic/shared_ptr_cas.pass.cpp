// [util.smartptr.atomic.shared]/14-15: compare_exchange_*: "If p is equivalent to expected,
// assigns desired to p ..., otherwise assigns p to expected"; "Returns: true if p was
// equivalent to expected". /16: "Two shared_ptr objects are equivalent if they store the same
// pointer value and either share ownership or are both empty." So the same pointer with
// different ownership (an aliasing shared_ptr of another owner), or an empty shared_ptr versus a
// non-empty one storing nullptr, is NOT equivalent. "The weak form may fail spuriously" (it may
// return false while equivalent; it then still leaves an equivalent value in expected). /17:
// on failure "expected is updated with the existing value" (with its ownership). /18-19: the
// single-order overloads.
#include <memory>
#include <atomic>
#include "check.hpp"

using SP = std::shared_ptr<int>;

// weak CAS in a loop: must succeed eventually when the values stay equivalent
static bool weak_until_success(std::atomic<SP>& a, SP& expected, SP desired) {
  SP const original = expected;
  for (int i = 0; i < 1000; ++i) {
    if (a.compare_exchange_weak(expected, desired)) return true;
    // spurious failure: expected is the existing value, still equivalent to the original one
    CHECK(expected == original && !expected.owner_before(original) && !original.owner_before(expected));
  }
  return false;
}

int main() {
  SP s = std::make_shared<int>(1);
  SP d = std::make_shared<int>(2);
  std::atomic<SP> a(s);

  // equivalent: same pointer, shared ownership (a copy)
  SP e = s;
  CHECK(a.compare_exchange_strong(e, d));
  CHECK(a.load() == d);
  CHECK(e == s);  // unchanged on success
  CHECK(s.use_count() == 2);  // s, e: a released its share

  // not equivalent: a different pointer; expected receives the existing value and ownership
  e = s;
  CHECK(!a.compare_exchange_strong(e, s));
  CHECK(e == d);
  CHECK(d.use_count() == 3);  // d, a's p, e
  CHECK(s.use_count() == 1);  // the failed exchange did not keep desired

  // same pointer value, different ownership: an aliasing pointer to *d owned by another object
  auto other_owner = std::make_shared<long>(0);
  SP alias(other_owner, d.get());
  CHECK(alias.get() == d.get());
  e = alias;
  CHECK(!a.compare_exchange_strong(e, s));
  CHECK(a.load() == d);
  CHECK(e.get() == d.get() && !e.owner_before(d) && !d.owner_before(e));  // now shares with d
  CHECK(other_owner.use_count() == 2);  // other_owner, alias: the alias in e was replaced
  e = alias;
  CHECK(!a.compare_exchange_weak(e, s));  // never equivalent, so weak fails too
  CHECK(a.load() == d);

  // empty versus non-empty storing nullptr
  std::atomic<SP> z;  // empty
  auto owner2 = std::make_shared<long>(0);
  SP null_owning(owner2, static_cast<int*>(nullptr));
  CHECK(null_owning.get() == nullptr && null_owning.use_count() == 2);
  SP en = null_owning;
  CHECK(!z.compare_exchange_strong(en, s));
  CHECK(en == nullptr && en.use_count() == 0);  // expected became the empty value
  CHECK(z.load() == nullptr);
  SP empty;
  CHECK(z.compare_exchange_strong(empty, null_owning));  // both empty: equivalent
  CHECK(z.load().use_count() == 4);  // owner2, null_owning, z's p, the returned copy
  en = SP();
  CHECK(!z.compare_exchange_strong(en, s, std::memory_order::acq_rel, std::memory_order::acquire));
  CHECK(en.use_count() == 4);  // owner2, null_owning, z's p, en
  CHECK(en.get() == nullptr && !en.owner_before(null_owning) && !null_owning.owner_before(en));
  CHECK(z.compare_exchange_strong(en, nullptr, std::memory_order::release));
  CHECK(owner2.use_count() == 3);  // owner2, null_owning, en: z released its share

  // explicit orders, weak in a loop
  SP cur = a.load();
  CHECK(weak_until_success(a, cur, s));
  CHECK(a.load() == s);
  SP cur2 = s;
  for (int i = 0; !a.compare_exchange_weak(cur2, d, std::memory_order::seq_cst, std::memory_order::relaxed); ++i)
    CHECK(i < 1000 && cur2 == s);
  CHECK(a.load() == d);
  SP cur3 = d;
  CHECK(a.compare_exchange_strong(cur3, nullptr, std::memory_order::relaxed, std::memory_order::relaxed));
  CHECK(a.load() == nullptr && a.load().use_count() == 0);
  return 0;
}
