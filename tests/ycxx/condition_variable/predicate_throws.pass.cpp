// A predicate that throws inside a waiting function. The predicate forms are specified as
// loops that evaluate pred() with the lock held ([thread.condition.condvar]/15, /22, /36;
// [thread.condvarany.wait]/?; [thread.condvarany.intwait]/2, /7, /13), so an exception from
// pred() leaves the function directly: "Throws: ... any exception thrown by pred" and
// "Postconditions: lock is locked by the calling thread" (the lock is held while pred runs).
// For the interruptible waits, the registration "for the duration of this call" to be notified
// on a stop request ends with the call: a later request_stop() on the same source must not
// touch the finished wait (checked by requesting stop after the condition variable object and
// the lock are destroyed - a lingering registration would use them) and must still run other
// callbacks.
// Exceptions also cross threads: the waiter is a jthread whose predicate throws on its third
// evaluation (after two notifications); the exception is caught in that thread and handed to
// the main thread through a promise ([futures.promise] set_exception).
// FLAGS: -pthread
#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <stop_token>
#include <thread>
#include "watchdog.hpp"
#include "check.hpp"

using namespace std::chrono_literals;
struct Boom {};

int main() {
  watchdog(30);
  // Single-threaded: the predicate throws on its first evaluation.
  {
    std::mutex m;
    std::condition_variable cv;
    std::unique_lock lk(m);
    bool caught = false;
    try {
      cv.wait(lk, []() -> bool { throw Boom{}; });
    } catch (Boom) {
      caught = true;
    }
    CHECK(caught && lk.owns_lock());
    caught = false;
    try {
      cv.wait_for(lk, 1h, []() -> bool { throw Boom{}; });
    } catch (Boom) {
      caught = true;
    }
    CHECK(caught && lk.owns_lock());
  }
  {
    std::shared_mutex m;
    std::condition_variable_any cv;
    std::unique_lock lk(m);
    bool caught = false;
    try {
      cv.wait_until(lk, std::chrono::steady_clock::now() + 1h, []() -> bool { throw Boom{}; });
    } catch (Boom) {
      caught = true;
    }
    CHECK(caught && lk.owns_lock());
  }

  // Interruptible waits: the registration ends with the call.
  std::stop_source src;
  for (int form = 0; form < 3; ++form) {
    auto m = std::make_unique<std::mutex>();
    auto cv = std::make_unique<std::condition_variable_any>();
    {
      std::unique_lock lk(*m);
      bool caught = false;
      try {
        auto pred = []() -> bool { throw Boom{}; };
        if (form == 0) cv->wait(lk, src.get_token(), pred);
        if (form == 1) cv->wait_for(lk, src.get_token(), 1h, pred);
        if (form == 2) cv->wait_until(lk, src.get_token(), std::chrono::steady_clock::now() + 1h, pred);
      } catch (Boom) {
        caught = true;
      }
      CHECK(caught && lk.owns_lock());
    }
    cv.reset();
    m.reset();
  }
  bool cb_ran = false;
  std::stop_callback cb(src.get_token(), [&] { cb_ran = true; });
  CHECK(src.request_stop());
  CHECK(cb_ran);

  // Across threads.
  {
    std::mutex m;
    std::condition_variable_any cv;
    int evaluations = 0;
    std::promise<void> p;
    std::future<void> f = p.get_future();
    std::jthread t([&](std::stop_token st) {
      std::unique_lock lk(m);
      try {
        cv.wait(lk, st, [&] {
          if (++evaluations == 3) throw Boom{};
          return false;
        });
        p.set_value();
      } catch (...) {
        CHECK(lk.owns_lock());
        p.set_exception(std::current_exception());
      }
    });
    for (int i = 0; i < 2;) {
      std::this_thread::sleep_for(1ms);
      std::lock_guard g(m);
      if (evaluations == i + 1) {
        ++i;
        cv.notify_all();
      }
    }
    bool caught = false;
    try {
      f.get();
    } catch (Boom) {
      caught = true;
    }
    CHECK(caught);
    CHECK(evaluations == 3);
  }
  return 0;
}
