// The noexcept waiting and notifying operations of the atomic types, and the noexcept
// operations of atomic<shared_ptr<T>> and atomic<weak_ptr<T>>, while operator new fails at its
// k-th call, for every k until the operations make no allocation that fails. The first calls of
// the program happen inside the sweep (a waiter table created lazily on first use would be
// created there). An allocation failure cannot leave a noexcept function ([except.spec]/5:
// std::terminate), so each operation must work without it.
//   [atomics.types.operations] wait(old, order) noexcept: "Repeatedly performs the following
//     steps, in order: Evaluates load(order) and compares its value representation for equality
//     against that of old. If they compare unequal, returns. Blocks until it is unblocked by an
//     atomic notifying operation or is unblocked spuriously." notify_one/notify_all noexcept.
//   [atomics.flag] atomic_flag::wait/notify_*, [atomics.ref.ops] atomic_ref wait/notify,
//   [atomics.nonmembers] atomic_wait/atomic_notify_one: likewise noexcept.
//   [util.smartptr.atomic.shared] load, store, exchange, compare_exchange_weak/strong, wait,
//     notify_one/notify_all: noexcept; [util.smartptr.atomic.weak] likewise.
// One scenario has a second thread (started before the failure is armed, and making no
// allocation) that changes the value and notifies while the main thread waits.
// FLAGS: -pthread
#include <atomic>
#include <memory>
#include <thread>
#include "exc_new.hpp"

using namespace exh;

template <class F>
void sw(const char* name, F op) {
  sweep_new(name, [&] {
    bool threw = attempt(op);
    EXH_EXPECT(!threw, "an allocation failure escaped from a noexcept atomic operation");
    return st.fired;
  }, options{true, 4000});
}

int main() {
  std::atomic<int> a{1};
  std::atomic<long long> ll{5};
  std::atomic_flag f;
  int plain = 3;
  sw("wait returns at once, notify without waiters", [&] {
    a.wait(0);  // the value differs: returns
    a.notify_one();
    a.notify_all();
    ll.wait(4, std::memory_order_acquire);
    ll.notify_all();
    f.wait(true);  // clear: returns
    f.notify_one();
    std::atomic_ref<int> r(plain);
    r.wait(2);
    r.notify_all();
    std::atomic_wait(&a, 0);
    std::atomic_notify_one(&a);
    std::atomic_notify_all(&a);
    std::atomic_flag_wait(&f, true);
    std::atomic_flag_notify_all(&f);
  });

  // A waiter and a notifier.
  sweep_new("wait woken by notify", [&] {
    std::atomic<int> v{0};
    std::atomic<bool> waiting{false};
    std::thread t([&] {
      while (!waiting.load()) std::this_thread::yield();
      for (int i = 0; i < 1000; ++i) std::this_thread::yield();
      v.store(1);
      v.notify_all();
    });
    bool threw = attempt([&] {
      waiting = true;
      v.wait(0);
      EXH_EXPECT(v.load() == 1, "wait returned before the value changed");
    });
    t.join();
    EXH_EXPECT(!threw, "an allocation failure escaped from atomic::wait");
    return st.fired;
  }, options{true, 4000});

  auto p1 = std::make_shared<int>(1), p2 = std::make_shared<int>(2);
  std::atomic<std::shared_ptr<int>> as(p1);
  std::atomic<std::weak_ptr<int>> aw{std::weak_ptr<int>(p1)};
  sw("atomic<shared_ptr>", [&] {
    std::shared_ptr<int> got = as.load();
    EXH_EXPECT(got == p1 || got == p2, "load");
    as.store(p2);
    std::shared_ptr<int> old = as.exchange(p1);
    EXH_EXPECT(old == p2, "exchange");
    std::shared_ptr<int> expected = p1;
    EXH_EXPECT(as.compare_exchange_strong(expected, p2), "compare_exchange_strong");
    expected = p1;
    EXH_EXPECT(!as.compare_exchange_strong(expected, p1) && expected == p2, "failed compare_exchange_strong");
    while (!as.compare_exchange_weak(expected, p1)) {
    }
    as.wait(p2);  // holds p1: returns
    as.notify_all();
    std::weak_ptr<int> w = aw.load();
    EXH_EXPECT(w.lock() == p1, "atomic<weak_ptr>::load");
    aw.store(std::weak_ptr<int>(p2));
    std::weak_ptr<int> ow = aw.exchange(std::weak_ptr<int>(p1));
    EXH_EXPECT(ow.lock() == p2, "atomic<weak_ptr>::exchange");
    aw.notify_one();
  });
  EXH_EXPECT(as.load() == p1 && p2.use_count() == 1, "final state");  // p2 is owned by p2 only
  return finish();
}
