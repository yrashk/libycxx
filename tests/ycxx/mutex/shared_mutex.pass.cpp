// [thread.sharedmutex.requirements.general]: shared mutex types provide "shared lock ownership
// semantics, where multiple threads can simultaneously hold a shared lock ownership"; lock()
// (exclusive) cannot be obtained while any thread holds a shared lock, and try_lock_shared()
// fails while another thread holds exclusive ownership. [thread.sharedtimedmutex.requirements]:
// try_lock_shared_for / until.
// FLAGS: -pthread
#include <shared_mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <type_traits>
#include "check.hpp"

static_assert(!std::is_copy_constructible_v<std::shared_mutex>);
static_assert(std::is_standard_layout_v<std::shared_mutex>);
static_assert(!std::is_copy_constructible_v<std::shared_timed_mutex>);

template<class M>
static void run() {
  M m;
  m.lock_shared();
  // another thread can share it, but not lock it exclusively
  bool shared = false, excl = true;
  std::thread([&] {
    m.lock_shared();
    shared = true;
    m.unlock_shared();
    excl = m.try_lock();
  }).join();
  CHECK(shared && !excl);
  m.unlock_shared();

  m.lock();
  bool s2 = true, e2 = true;
  std::thread([&] { s2 = m.try_lock_shared(); e2 = m.try_lock(); }).join();
  CHECK(!s2 && !e2);
  m.unlock();

  // several readers at the same time
  std::atomic<int> inside(0), max_inside(0);
  std::atomic<bool> go(false);
  std::thread readers[3];
  for (auto& r : readers)
    r = std::thread([&] {
      go.wait(false);
      m.lock_shared();
      int n = inside.fetch_add(1) + 1;
      max_inside.fetch_max(n);
      while (inside.load() < 3 && max_inside.load() < 3) std::this_thread::yield();
      inside.fetch_sub(1);
      m.unlock_shared();
    });
  go = true;
  go.notify_all();
  for (auto& r : readers) r.join();
  CHECK(max_inside.load() == 3);  // all three held the shared lock at once
}

int main() {
  run<std::shared_mutex>();
  run<std::shared_timed_mutex>();

  std::shared_timed_mutex tm;
  if (tm.try_lock_shared_for(std::chrono::milliseconds(1))) tm.unlock_shared();
  if (tm.try_lock_shared_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(1))) tm.unlock_shared();
  tm.lock_shared();
  tm.unlock_shared();
  tm.lock();
  bool r = true;
  std::thread([&] { r = tm.try_lock_shared_for(std::chrono::milliseconds(2)); }).join();
  CHECK(!r);
  tm.unlock();
  return 0;
}
