// [thread.condition.condvar]: wait_for / wait_until without a predicate return
// cv_status::timeout "if the absolute timeout ([thread.req.timing]) specified by abs_time
// expired, otherwise cv_status::no_timeout"; with a predicate they return pred() ("Returns:
// pred()" - wait_until(lock, abs_time, pred) is "while (!pred()) if (wait_until(lock,
// abs_time) == cv_status::timeout) return pred(); return true;"). The lock is held on return.
// FLAGS: -pthread
#include <condition_variable>
#include <mutex>
#include <thread>
#include <chrono>
#include "check.hpp"

using namespace std::chrono;

int main() {
  std::mutex m;
  std::condition_variable cv;
  std::unique_lock<std::mutex> l(m);

  // no notification at all: must time out (spurious wake-ups are absorbed by the loop)
  auto deadline = steady_clock::now() + milliseconds(3);
  std::cv_status st = std::cv_status::no_timeout;
  while (st == std::cv_status::no_timeout && steady_clock::now() < deadline)
    st = cv.wait_until(l, deadline);
  CHECK(steady_clock::now() >= deadline);
  CHECK(l.owns_lock());

  auto t0 = steady_clock::now();
  bool r = cv.wait_for(l, milliseconds(3), [] { return false; });
  CHECK(!r);
  CHECK(steady_clock::now() - t0 >= milliseconds(3));
  CHECK(l.owns_lock());
  CHECK(cv.wait_for(l, milliseconds(-1), [] { return true; }));
  CHECK(!cv.wait_until(l, system_clock::now() - seconds(1), [] { return false; }));
  CHECK(cv.wait_for(l, milliseconds(0)) == std::cv_status::timeout);

  // a predicate that turns true before the timeout
  bool ready = false;
  std::thread t([&] {
    std::lock_guard<std::mutex> g(m);
    ready = true;
    cv.notify_one();
  });
  CHECK(cv.wait_for(l, seconds(30), [&] { return ready; }));
  t.join();
  CHECK(cv.wait_until(l, steady_clock::now() + seconds(30), [&] { return ready; }));
  return 0;
}
