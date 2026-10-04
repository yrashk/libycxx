// [atomics.types.operations]/31: wait(old): "Repeatedly performs the following steps, in order:
// Evaluates load(order) and compares its value representation for equality against that of
// old. If they compare unequal, returns. Blocks until it is unblocked by an atomic notifying
// operation or is unblocked spuriously." So a notification (or a spurious wake-up) without a
// change of value does not end the wait: when wait returns, the value it observed differed
// from old. Here the waiter is notified many times while the value still equals old, and only
// then is the value changed; the waiter must observe the changed value. The same for
// atomic_ref ([atomics.ref.ops]/30), atomic_flag ([atomics.flag]/15) and
// atomic<shared_ptr<T>> ([util.smartptr.atomic.shared]/21).
// FLAGS: -latomic -pthread
// (-latomic: the toolchain's out-of-line atomics for types that are not lock-free; Clang does not link it implicitly)
#include <atomic>
#include <memory>
#include <thread>
#include "check.hpp"
#include "watchdog.hpp"

// waiter: announces it is about to wait, waits, then records what it saw
template<class Wait, class Notify, class Change, class Seen>
void scenario(Wait wait, Notify notify, Change change, Seen seen_changed) {
  std::atomic<int> state(0);  // 0: starting, 1: about to wait, 2: returned
  bool ok = false;
  std::thread t([&] {
    state.store(1);
    wait();
    ok = seen_changed();
    state.store(2);
  });
  while (state.load() == 0) std::this_thread::yield();
  for (int i = 0; i < 2000; ++i) {  // notifications without a change of value
    notify();
    CHECK(state.load() != 2);  // if wait had returned, it saw an unchanged value
    if (i % 64 == 0) std::this_thread::yield();
  }
  change();
  notify();
  t.join();
  CHECK(ok);
}

struct Big { long a[6]; };

int main() {
  watchdog(20);
  {
    std::atomic<int> v(0);
    scenario([&] { v.wait(0); }, [&] { v.notify_all(); }, [&] { v.store(1); },
             [&] { return v.load() == 1; });
  }
  {
    std::atomic<int> v(0);
    scenario([&] { v.wait(0, std::memory_order::acquire); }, [&] { v.notify_one(); },
             [&] { v.store(1, std::memory_order::release); }, [&] { return v.load() == 1; });
  }
  {
    std::atomic<Big> v(Big{});  // probably not lock-free
    scenario([&] { v.wait(Big{}); }, [&] { v.notify_all(); }, [&] { v.store(Big{{1}}); },
             [&] { return v.load().a[0] == 1; });
  }
  {
    alignas(std::atomic_ref<long>::required_alignment) long x = 0;
    std::atomic_ref<long> r(x);
    scenario([&] { r.wait(0); }, [&] { r.notify_one(); }, [&] { r.store(5); },
             [&] { return r.load() == 5; });
  }
  {
    std::atomic_flag f;
    scenario([&] { f.wait(false); }, [&] { f.notify_all(); }, [&] { f.test_and_set(); },
             [&] { return f.test(); });
  }
  {
    auto s = std::make_shared<int>(1);
    std::atomic<std::shared_ptr<int>> a(s);
    scenario([&] { a.wait(s); }, [&] { a.notify_all(); }, [&] { a.store(nullptr); },
             [&] { return a.load() == nullptr; });
  }
  return 0;
}
