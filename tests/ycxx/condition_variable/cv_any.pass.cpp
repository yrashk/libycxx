// [thread.condition.condvarany.general]: condition_variable_any works with any Lock meeting
// Cpp17BasicLockable (here shared_mutex via unique_lock, a recursive mutex held once, and a
// user lock type); wait(lock, pred), wait_for with a predicate returning pred() on timeout.
// FLAGS: -pthread
#include <condition_variable>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <chrono>
#include <type_traits>
#include "check.hpp"

static_assert(!std::is_copy_constructible_v<std::condition_variable_any>);

struct MyLock {
  std::mutex m;
  int locks = 0;
  void lock() { m.lock(); ++locks; }
  void unlock() { m.unlock(); }
};

int main() {
  std::condition_variable_any cv;

  std::shared_mutex sm;
  bool flag = false;
  std::thread t([&] {
    std::unique_lock<std::shared_mutex> l(sm);
    flag = true;
    cv.notify_all();
  });
  {
    std::unique_lock<std::shared_mutex> l(sm);
    cv.wait(l, [&] { return flag; });
    CHECK(l.owns_lock());
  }
  t.join();

  MyLock ml;
  int n = 0;
  std::thread t2([&] {
    ml.lock();
    n = 1;
    ml.unlock();
    cv.notify_one();
  });
  ml.lock();
  cv.wait(ml, [&] { return n == 1; });
  ml.unlock();
  t2.join();
  CHECK(ml.locks >= 2);

  std::recursive_mutex rm;
  std::unique_lock<std::recursive_mutex> rl(rm);
  CHECK(!cv.wait_for(rl, std::chrono::milliseconds(2), [] { return false; }));
  CHECK(rl.owns_lock());
  CHECK(cv.wait_until(rl, std::chrono::steady_clock::now(), [] { return true; }));
  return 0;
}
