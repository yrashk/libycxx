// [thread.condvarany.intwait]: wait(lock, stoken, pred) is "while (!stoken.stop_requested())
// { if (pred()) return true; wait(lock); } return pred();" and "will be notified when there is
// a stop request on the passed stop_token"; returns pred() (possibly false) after a stop
// request; no blocking if stop was already requested. wait_for / wait_until with a stop token
// return pred() on timeout. The lock is held on return. Works with jthread's token.
// FLAGS: -pthread
#include <condition_variable>
#include <mutex>
#include <stop_token>
#include <thread>
#include <chrono>
#include "check.hpp"

using namespace std::chrono;

int main() {
  std::condition_variable_any cv;
  std::mutex m;

  // already stopped: returns pred() without blocking
  std::stop_source done;
  done.request_stop();
  {
    std::unique_lock<std::mutex> l(m);
    CHECK(!cv.wait(l, done.get_token(), [] { return false; }));
    CHECK(cv.wait(l, done.get_token(), [] { return true; }));
    CHECK(!cv.wait_for(l, done.get_token(), seconds(30), [] { return false; }));
    CHECK(!cv.wait_until(l, done.get_token(), steady_clock::now() + seconds(30), [] { return false; }));
    CHECK(l.owns_lock());
  }

  // a stop request wakes the waiter; nobody notifies the cv otherwise
  bool result = true;
  bool owned = false;
  std::jthread waiter([&](std::stop_token st) {
    std::unique_lock<std::mutex> l(m);
    result = cv.wait(l, st, [] { return false; });
    owned = l.owns_lock();
  });
  std::this_thread::sleep_for(milliseconds(2));
  waiter.request_stop();
  waiter.join();
  CHECK(!result && owned);

  // timed: stop request before the (long) timeout
  bool r2 = true;
  std::jthread w2([&](std::stop_token st) {
    std::unique_lock<std::mutex> l(m);
    r2 = cv.wait_for(l, st, seconds(60), [] { return false; });
  });
  std::this_thread::sleep_for(milliseconds(2));
  w2.request_stop();
  w2.join();
  CHECK(!r2);

  // timeout without a stop request
  std::stop_source never;
  std::unique_lock<std::mutex> l(m);
  auto t0 = steady_clock::now();
  CHECK(!cv.wait_for(l, never.get_token(), milliseconds(3), [] { return false; }));
  CHECK(steady_clock::now() - t0 >= milliseconds(3));

  // predicate becomes true with a normal notification
  bool ready = false;
  std::thread t([&] {
    std::lock_guard<std::mutex> g(m);
    ready = true;
    cv.notify_all();
  });
  CHECK(cv.wait(l, never.get_token(), [&] { return ready; }));
  t.join();
  return 0;
}
