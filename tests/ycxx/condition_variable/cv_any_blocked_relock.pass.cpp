// condition_variable_any waits whose user lock blocks when the wait re-locks it, many waiters on
// many condition variables with distinct locks, and notifications made meanwhile.
//   [thread.condition.general]/2: "Condition variables permit concurrent invocation of the wait,
//     wait_for, wait_until, notify_one and notify_all member functions." /3: notify_one and
//     notify_all are atomic; a wait runs in three atomic parts, the third being "the
//     reacquisition of the lock"; /4: all of them are executed "in a single unspecified total
//     order". So a waiter that is blocked re-acquiring its lock ([thread.condvarany.wait]/1.2
//     "calls lock.lock() (possibly blocking on the lock)") does not keep notify_one/notify_all
//     on the same or another condition variable from completing: in the total order those
//     notifications come before the reacquisition completes.
//   [thread.condition.condvarany.general]/1: Lock is any Cpp17BasicLockable type; here its
//     lock() itself waits on another condition_variable_any until a gate opens.
//   [thread.condvarany.intwait] wait(lock, stoken, pred) and [thread.condvarany.wait]
//     wait_until(lock, abs_time, pred) behave the same way.
// 100 waiters, each with its own condition variable and lock. They are woken while their gates
// are closed, so each one blocks inside the wait re-locking; then the main thread notifies every
// condition variable again (the waiter's own and its neighbour's) before it opens each gate, and
// each woken waiter notifies both while holding its lock. An implementation that holds an
// internal mutex (its own, or one shared between condition variables) while re-locking the
// user lock deadlocks; the watchdog ends the test then.
// FLAGS: -pthread
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <stop_token>
#include <thread>
#include <vector>
#include "check.hpp"
#include "watchdog.hpp"

struct GateLock {
  std::mutex m;  // what the lock protects
  std::mutex gm;
  std::condition_variable_any gate_cv;
  bool open = true;
  void lock() {
    {
      std::unique_lock<std::mutex> g(gm);
      gate_cv.wait(g, [&] { return open; });
    }
    m.lock();
  }
  void unlock() { m.unlock(); }
  void set_gate(bool o) {
    {
      std::lock_guard<std::mutex> g(gm);
      open = o;
    }
    gate_cv.notify_all();
  }
};

struct Slot {
  GateLock lock;
  std::condition_variable_any cv;
  bool flag = false;  // protected by lock.m
  std::atomic<bool> waiting{false}, done{false};
};

constexpr int N = 100;

int main() {
  watchdog(8);
  std::vector<std::unique_ptr<Slot>> slots;
  for (int i = 0; i < N; ++i) slots.push_back(std::make_unique<Slot>());
  auto slot = [&](int i) -> Slot& { return *slots[static_cast<std::size_t>((i + N) % N)]; };

  std::stop_source never_stopped;
  std::vector<std::thread> ts;
  for (int i = 0; i < N; ++i) {
    ts.emplace_back([&, i] {
      Slot& s = slot(i);
      std::unique_lock<GateLock> lk(s.lock);
      auto pred = [&] {
        s.waiting.store(true);
        return s.flag;
      };
      switch (i % 3) {
        case 0: s.cv.wait(lk, pred); break;
        case 1: (void)s.cv.wait(lk, never_stopped.get_token(), pred); break;
        default: (void)s.cv.wait_until(lk, std::chrono::steady_clock::now() + std::chrono::hours(1), pred); break;
      }
      // Holding the lock: notify this and the next condition variable.
      s.cv.notify_all();
      slot(i + 1).cv.notify_one();
      lk.unlock();
      s.done.store(true);
    });
  }
  for (int i = 0; i < N; ++i)
    while (!slot(i).waiting.load()) std::this_thread::yield();

  // Close every gate, then make the predicates true and wake everybody.
  for (int i = 0; i < N; ++i) {
    Slot& s = slot(i);
    {
      std::lock_guard<std::mutex> g(s.lock.m);  // the lock's protected state, without the gate
      s.flag = true;
    }
    s.lock.set_gate(false);
  }
  for (int i = 0; i < N; ++i) slot(i).cv.notify_all();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));  // let the waiters reach the gates

  for (int i = 0; i < N; ++i) {
    slot(i + 1).cv.notify_all();
    slot(i).cv.notify_one();
    slot(i).cv.notify_all();
    slot(i).lock.set_gate(true);
  }
  for (auto& t : ts) t.join();
  for (int i = 0; i < N; ++i) CHECK(slot(i).done.load());
  return 0;
}
