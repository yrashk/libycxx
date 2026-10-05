// [thread.timedmutex.requirements.general]: try_lock_for(rel_time): "The function attempts to
// obtain ownership of the mutex within the relative timeout specified by rel_time. If the time
// specified by rel_time is less than or equal to rel_time.zero(), the function attempts to
// obtain ownership without blocking (as if by calling try_lock()). The function returns within
// the timeout specified by rel_time only if it has obtained ownership of the mutex object."
// try_lock_until likewise with an absolute timeout. Returns: true if ownership was obtained.
// FLAGS: -pthread
// COUNTERPART: libstdcxx:30_threads/recursive_timed_mutex/try_lock_for/2.cc
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <chrono>
#include "check.hpp"

using namespace std::chrono;

template<class M>
static void run() {
  M m;
  CHECK(m.try_lock_for(milliseconds(50)));  // free: obtained
  bool r1 = true, r2 = true, r3 = true;
  steady_clock::duration waited{};
  std::thread([&] {
    auto t0 = steady_clock::now();
    r1 = m.try_lock_for(milliseconds(3));
    waited = steady_clock::now() - t0;
    r2 = m.try_lock_until(steady_clock::now() + milliseconds(2));
    r3 = m.try_lock_for(milliseconds(-5));  // non-blocking
  }).join();
  CHECK(!r1 && !r2 && !r3);
  CHECK(waited >= milliseconds(3));  // returned within the timeout only if it got the lock
  m.unlock();

  // a waiter gets the lock once it is released
  m.lock();
  bool got = false;
  std::thread waiter([&] { got = m.try_lock_for(seconds(30)); if (got) m.unlock(); });
  std::this_thread::sleep_for(milliseconds(1));
  m.unlock();
  waiter.join();
  CHECK(got);
  CHECK(m.try_lock_until(system_clock::now() + milliseconds(10)));
  m.unlock();
}

int main() {
  run<std::timed_mutex>();
  run<std::recursive_timed_mutex>();
  run<std::shared_timed_mutex>();
  return 0;
}
