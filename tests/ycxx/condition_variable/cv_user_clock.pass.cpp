// [thread.condition.condvar]/18-21: wait_until(lock, abs_time) unblocks on notification,
// "expiration of the absolute timeout ([thread.req.timing]) specified by abs_time, or
// spuriously"; returns cv_status::timeout if the timeout expired, otherwise no_timeout;
// "(18.4) If the function exits via an exception, lock.lock() is called prior to exiting the
// function"; "Throws: Timeout-related exceptions". /29-: the predicate form returns pred().
// [thread.condvarany.wait]/6.4, /9 likewise for condition_variable_any with any lock type;
// [thread.condvarany.intwait]/7: the stop_token form returns pred() after a timeout.
// [thread.req.timing]/4, /8: a user-defined clock is used for the absolute timeout; the return
// is no earlier than the time point (the clock is not adjusted); exceptions thrown by the
// clock propagate. Nobody notifies here, so every wait ends by timeout (or spuriously:
// no_timeout, after which the loops wait again).
// FLAGS: -pthread
// REQUIRES: exceptions
#include <condition_variable>
#include <mutex>
#include <stop_token>
#include <chrono>
#include "check.hpp"
#include "test_clocks.hpp"
#include "watchdog.hpp"

using namespace std::chrono_literals;

struct UserLock {  // Cpp17BasicLockable
  std::mutex m;
  bool held = false;
  void lock() { m.lock(); held = true; }
  void unlock() { held = false; m.unlock(); }
};

int main() {
  watchdog(20);
  {
    std::mutex m;
    std::condition_variable cv;
    std::unique_lock<std::mutex> lk(m);
    auto dl = offset_clock::now() + 10ms;
    while (cv.wait_until(lk, dl) != std::cv_status::timeout) {}
    CHECK(offset_clock::now() >= dl);
    CHECK(lk.owns_lock());
    int pred_calls = 0;
    auto dl2 = offset_clock::now() + 5ms;
    CHECK(!cv.wait_until(lk, dl2, [&] { ++pred_calls; return false; }));
    CHECK(offset_clock::now() >= dl2 && pred_calls >= 2 && lk.owns_lock());
    // a deadline in the past: timeout without blocking
    CHECK(cv.wait_until(lk, offset_clock::now() - 1h) == std::cv_status::timeout);

    auto tp = throwing_clock::now() + 10ms;
    throwing_clock::armed = true;
    bool thrown = false;
    try {
      (void)cv.wait_until(lk, tp);
    } catch (clock_error) {
      thrown = true;
    }
    throwing_clock::armed = false;
    CHECK(thrown);
    CHECK(lk.owns_lock());  // re-locked before the exception left the function
    thrown = false;
    throwing_clock::armed = true;
    try {
      (void)cv.wait_until(lk, tp, [] { return false; });
    } catch (clock_error) {
      thrown = true;
    }
    throwing_clock::armed = false;
    CHECK(thrown && lk.owns_lock());
  }
  {
    UserLock ul;
    std::condition_variable_any cv;
    ul.lock();
    auto dl = offset_clock::now() + 10ms;
    while (cv.wait_until(ul, dl) != std::cv_status::timeout) {}
    CHECK(offset_clock::now() >= dl && ul.held);
    auto dl2 = offset_clock::now() + 5ms;
    CHECK(!cv.wait_until(ul, dl2, [] { return false; }));
    CHECK(offset_clock::now() >= dl2 && ul.held);
    std::stop_source ss;
    auto dl3 = offset_clock::now() + 5ms;
    CHECK(!cv.wait_until(ul, ss.get_token(), dl3, [] { return false; }));
    CHECK(offset_clock::now() >= dl3 && ul.held);
    CHECK(!ss.stop_requested());

    auto tp = throwing_clock::now() + 10ms;
    throwing_clock::armed = true;
    bool thrown = false;
    try {
      (void)cv.wait_until(ul, tp);
    } catch (clock_error) {
      thrown = true;
    }
    throwing_clock::armed = false;
    CHECK(thrown && ul.held);
    ul.unlock();
  }
  return 0;
}
