// [thread.condition.condvar]: wait(lock, pred) is "while (!pred()) wait(lock);" (so it does not
// block when pred() is already true); notify_one / notify_all unblock waiters; on return the
// lock is held ("Postconditions: lock.owns_lock() is true and lock.mutex() is locked by the
// calling thread"). The class is standard-layout, not copyable.
// FLAGS: -pthread
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_standard_layout_v<std::condition_variable>);
static_assert(!std::is_copy_constructible_v<std::condition_variable>);
static_assert(!std::is_copy_assignable_v<std::condition_variable>);
static_assert(noexcept(std::declval<std::condition_variable&>().notify_one()));
static_assert(noexcept(std::declval<std::condition_variable&>().notify_all()));

int main() {
  std::mutex m;
  std::condition_variable cv;
  {
    std::unique_lock<std::mutex> l(m);
    int evaluated = 0;
    cv.wait(l, [&] { ++evaluated; return true; });  // no blocking
    CHECK(evaluated == 1 && l.owns_lock());
  }

  // producer / consumer with notify_one
  int stage = 0;
  std::thread consumer([&] {
    std::unique_lock<std::mutex> l(m);
    cv.wait(l, [&] { return stage == 1; });
    CHECK(l.owns_lock());
    stage = 2;
    l.unlock();
    cv.notify_one();
  });
  {
    std::lock_guard<std::mutex> g(m);
    stage = 1;
  }
  cv.notify_one();
  {
    std::unique_lock<std::mutex> l(m);
    cv.wait(l, [&] { return stage == 2; });
  }
  consumer.join();

  // notify_all wakes every waiter
  bool go = false;
  int woke = 0;
  std::vector<std::thread> ts;
  for (int i = 0; i < 4; ++i)
    ts.emplace_back([&] {
      std::unique_lock<std::mutex> l(m);
      cv.wait(l, [&] { return go; });
      ++woke;
    });
  {
    std::lock_guard<std::mutex> g(m);
    go = true;
  }
  cv.notify_all();
  for (auto& t : ts) t.join();
  CHECK(woke == 4);
  return 0;
}
