// [thread.condition.condvarany.general]/1: "A Lock type shall meet the Cpp17BasicLockable
// requirements" -- std::shared_lock<std::shared_mutex> does (lock/unlock acquire and release
// shared ownership), so several threads may wait on one condition_variable_any while holding
// shared ownership. [thread.condvarany.wait]/5-8: wait(lock, pred) returns with the lock held,
// pred() true. [thread.condvarany.intwait]/3-7: wait(lock, stoken, pred) "returns pred()" when
// a stop is requested, lock reacquired ("Postconditions: lock is locked by the calling thread").
// A writer takes exclusive ownership to change the state, so the waiters must release their
// shared ownership while blocked (otherwise the writer would deadlock; watchdog guards).
// FLAGS: -pthread
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <shared_mutex>
#include <stop_token>
#include <thread>
#include <vector>
#include "check.hpp"
#include "watchdog.hpp"

int main() {
  watchdog(20);
  std::shared_mutex sm;
  std::condition_variable_any cv;
  int stage = 0;
  std::atomic<int> waiting = 0, woke = 0;
  {
    std::vector<std::jthread> readers;
    for (int t = 0; t < 4; ++t)
      readers.emplace_back([&] {
        std::shared_lock l(sm);
        ++waiting;
        cv.wait(l, [&] { return stage == 1; });
        CHECK(l.owns_lock() && stage == 1);
        ++woke;
      });
    while (waiting.load() < 4) std::this_thread::yield();
    {
      std::unique_lock w(sm);  // possible only if every waiter released its shared ownership
      stage = 1;
    }
    cv.notify_all();
  }
  CHECK(woke.load() == 4);

  std::atomic<bool> in_wait = false;
  bool result = true, owned = false;
  std::jthread j([&](std::stop_token st) {
    std::shared_lock l(sm);
    in_wait = true;
    result = cv.wait(l, st, [&] { return stage == 2; });
    owned = l.owns_lock();
  });
  while (!in_wait.load()) std::this_thread::yield();
  {
    std::unique_lock w(sm);  // the waiter has released its shared lock
  }
  j.request_stop();
  j.join();
  CHECK(!result && owned);
  return 0;
}
