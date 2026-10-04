// [atomics.types.operations]/31: wait(old): "Repeatedly performs the following steps, in
// order: Evaluates load(order) and compares its value representation for equality against that
// of old. If they compare unequal, returns. Blocks until it is unblocked by an atomic notifying
// operation or is unblocked spuriously." /34, /37: notify_one / notify_all unblock eligible
// waiters. [atomics.wait]. Also the non-member atomic_wait / atomic_notify_*.
// FLAGS: -latomic -pthread
// (-latomic: the toolchain's out-of-line atomics for types that are not lock-free; Clang does not link it implicitly)
#include <atomic>
#include <thread>
#include <vector>
#include "check.hpp"

int main() {
  // wait returns at once when the value differs
  std::atomic<int> a(1);
  a.wait(0);
  a.wait(2, std::memory_order::acquire);
  std::atomic_wait(&a, 5);
  std::atomic_wait_explicit(&a, 5, std::memory_order::relaxed);

  // one waiter, notify_one
  std::atomic<int> v(0);
  std::thread t([&] {
    v.wait(0);
    CHECK(v.load() == 1);
    v.store(2);
    v.notify_one();
  });
  v.store(1);
  v.notify_one();
  v.wait(1);
  CHECK(v.load() == 2);
  t.join();

  // several waiters, notify_all
  std::atomic<bool> go(false);
  std::atomic<int> done(0);
  std::vector<std::thread> ts;
  for (int i = 0; i < 4; ++i)
    ts.emplace_back([&] {
      go.wait(false);
      done.fetch_add(1);
      done.notify_all();
    });
  go.store(true);
  go.notify_all();
  for (int d = done.load(); d != 4; d = done.load()) done.wait(d);
  for (auto& th : ts) th.join();
  CHECK(done.load() == 4);

  // user type and pointer
  struct P { int x, y; };
  std::atomic<P> ap(P{1, 2});
  ap.wait(P{1, 3});  // value representation differs
  int i1 = 0, i2 = 0;
  std::atomic<int*> pp(&i1);
  std::thread t2([&] { pp.store(&i2); std::atomic_notify_one(&pp); });
  pp.wait(&i1);
  CHECK(pp.load() == &i2);
  t2.join();
  std::atomic_notify_all(&pp);

  // floating point
  std::atomic<double> d(0.0);
  d.wait(-0.0);  // different value representation: returns at once
  return 0;
}
