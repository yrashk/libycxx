// [thread.condvarany.intwait]/1-4: wait(lock, stoken, pred) "Registers for the duration of this
// call *this to get notified on a stop request on stoken during this call and then equivalent
// to: while (!stoken.stop_requested()) { if (pred()) return true; wait(lock); } return pred();"
// so a stop request alone (no notify) ends the wait, which returns pred(); "Postconditions:
// lock is locked by the calling thread"; "Remarks: ... *this is no longer registered" when it
// returns. /7: wait_until(lock, stoken, abs_time, pred). [thread.condition.condvarany.general]:
// Lock is any type meeting Cpp17BasicLockable: here a user type that is not unique_lock, and
// whose mutex is a recursive_mutex. The waiter announces itself under the lock, so the request
// cannot be lost ([thread.condvarany.intwait]/1: the request is observed while registered).
// FLAGS: -pthread
#include <condition_variable>
#include <mutex>
#include <stop_token>
#include <thread>
#include <chrono>
#include "check.hpp"
#include "watchdog.hpp"

struct UserLock {
  std::recursive_mutex m;
  int depth = 0;  // guarded by m
  void lock() { m.lock(); ++depth; }
  void unlock() { --depth; m.unlock(); }
};

int main() {
  watchdog(20);
  UserLock ul;
  std::condition_variable_any cv;
  for (int round = 0; round < 50; ++round) {
    std::stop_source ss;
    bool waiting = false, result = true;
    int depth_after = -1;
    std::thread t([&] {
      ul.lock();
      waiting = true;
      result = cv.wait(ul, ss.get_token(), [] { return false; });
      depth_after = ul.depth;
      ul.unlock();
    });
    for (;;) {  // until the waiter has released the lock inside wait
      std::lock_guard<UserLock> g(ul);
      if (waiting) break;
    }
    {
      std::lock_guard<UserLock> g(ul);  // the waiter is inside wait (it released ul)
    }
    ss.request_stop();  // no notify
    t.join();
    CHECK(!result && depth_after == 1);
  }

  // the timed form with a far deadline also ends on the stop request, returning pred()
  std::stop_source ss;
  bool waiting = false, result = true, flag = false;
  std::thread t([&] {
    std::lock_guard<UserLock> g(ul);
    waiting = true;
    result = cv.wait_until(ul, ss.get_token(), std::chrono::steady_clock::now() + std::chrono::hours(1),
                           [&] { return flag; });
  });
  for (;;) {
    std::lock_guard<UserLock> g(ul);
    if (waiting) { flag = true; break; }  // pred() becomes true, but nobody notifies
  }
  ss.request_stop();
  t.join();
  CHECK(result);  // "returns pred()" after the stop request
  return 0;
}
