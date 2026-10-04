// [util.smartptr.atomic.weak]: atomic<weak_ptr<T>>. /1: the default constructor value-initializes
// p (an empty weak_ptr); /2: atomic(weak_ptr<T>) initializes with desired; /4: store as if by
// p.swap(desired); /5: operator= is store; /8: load returns p; /9: the conversion is load();
// /10-11: exchange returns the previous value; /13-14: compare_exchange_* assign desired if p is
// equivalent to expected, otherwise assign p to expected; /15: "Two weak_ptr objects are
// equivalent if they store the same pointer value and either share ownership or are both
// empty." An expired weak_ptr still stores its pointer and shares ownership with the other
// weak_ptrs of its owner group ([util.smartptr.weak]), so it is equivalent to them. The
// control block is released when the last weak_ptr is gone, including the atomic's own p
// (observed through an allocator's deallocate, [util.smartptr.shared.create]).
#include <memory>
#include <atomic>
#include <cstddef>
#include "check.hpp"

static int live_blocks = 0;

template<class T>
struct CountingAlloc {
  using value_type = T;
  CountingAlloc() = default;
  template<class U> CountingAlloc(const CountingAlloc<U>&) {}
  T* allocate(std::size_t n) { ++live_blocks; return std::allocator<T>().allocate(n); }
  void deallocate(T* p, std::size_t n) { --live_blocks; std::allocator<T>().deallocate(p, n); }
  template<class U> bool operator==(const CountingAlloc<U>&) const { return true; }
};

static bool same_owner(const std::weak_ptr<int>& a, const std::weak_ptr<int>& b) {
  return !a.owner_before(b) && !b.owner_before(a);
}

int main() {
  using WP = std::weak_ptr<int>;
  std::atomic<WP> a;
  CHECK(a.load().expired() && a.load().use_count() == 0);
  CHECK(same_owner(a.load(), WP()));  // empty

  auto s = std::allocate_shared<int>(CountingAlloc<int>(), 1);
  CHECK(live_blocks == 1);
  auto t = std::make_shared<int>(2);
  std::atomic<WP> b(s);
  CHECK(b.load().lock() == s);
  CHECK(s.use_count() == 1);  // a weak_ptr does not own
  CHECK(static_cast<WP>(b).lock() == s);
  b.store(t, std::memory_order::release);
  CHECK(b.load(std::memory_order::acquire).lock() == t);
  b = WP(s);
  CHECK(b.load(std::memory_order::relaxed).lock() == s);
  WP old = b.exchange(t);
  CHECK(old.lock() == s);
  CHECK(b.exchange(old, std::memory_order::acq_rel).lock() == t);
  old.reset();

  // equivalence survives expiry
  WP e = b.load();
  s.reset();  // the object is destroyed; the control block stays (weak references)
  CHECK(b.load().expired());
  CHECK(live_blocks == 1);
  CHECK(b.compare_exchange_strong(e, t));  // same pointer, same owner group: equivalent
  CHECK(b.load().lock() == t);
  CHECK(live_blocks == 1);  // e still refers to the block
  e.reset();
  CHECK(live_blocks == 0);  // b's p no longer refers to it: the last weak reference is gone

  // failures: a different owner, or empty versus non-empty
  WP wrong;  // empty
  CHECK(!b.compare_exchange_strong(wrong, WP()));
  CHECK(wrong.lock() == t);  // expected receives p
  auto alias_owner = std::make_shared<long>(0);
  WP alias = std::shared_ptr<int>(alias_owner, t.get());  // same pointer, other owner group
  WP e2 = alias;
  CHECK(!b.compare_exchange_strong(e2, WP(), std::memory_order::seq_cst, std::memory_order::relaxed));
  CHECK(same_owner(e2, t));
  CHECK(b.load().lock() == t);
  e2 = alias;
  CHECK(!b.compare_exchange_weak(e2, WP()));  // never equivalent
  WP e3 = t;
  for (int i = 0; !b.compare_exchange_weak(e3, WP(), std::memory_order::release); ++i)
    CHECK(i < 1000 && same_owner(e3, t));
  CHECK(b.load().use_count() == 0 && same_owner(b.load(), WP()));

  // the atomic releases its weak reference when destroyed
  {
    auto u = std::allocate_shared<int>(CountingAlloc<int>(), 3);
    std::atomic<WP> c(u);
    u.reset();
    CHECK(live_blocks == 1);
  }
  CHECK(live_blocks == 0);
  return 0;
}
