// [futures.unique.future]/25-27: wait_until(abs_time): "None if the shared state contains a
// deferred function, otherwise blocks until the shared state is ready or until the absolute
// timeout ([thread.req.timing]) specified by abs_time has expired"; returns deferred, ready,
// or timeout "if the function is returning because the absolute timeout ... has expired";
// "Throws: timeout-related exceptions". [futures.shared.future] likewise. [thread.req.timing]/4,
// /8: with a user-defined clock, a timeout is no earlier than abs_time, and an exception thrown
// by the clock propagates when the call has to wait.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <future>
#include <chrono>
#include "check.hpp"
#include "test_clocks.hpp"
#include "watchdog.hpp"

using namespace std::chrono_literals;

template<class F>
void not_ready(F& f) {
  auto dl = offset_clock::now() + 10ms;
  CHECK(f.wait_until(dl) == std::future_status::timeout);
  CHECK(offset_clock::now() >= dl);
  CHECK(f.wait_until(offset_clock::now() - 1h) == std::future_status::timeout);
  auto tp = throwing_clock::now() + 10ms;
  throwing_clock::armed = true;
  bool thrown = false;
  try {
    (void)f.wait_until(tp);
  } catch (clock_error) {
    thrown = true;
  }
  throwing_clock::armed = false;
  CHECK(thrown);
  CHECK(f.valid());
}

int main() {
  watchdog(20);
  std::promise<int> p;
  std::future<int> f = p.get_future();
  not_ready(f);
  p.set_value(3);
  CHECK(f.wait_until(offset_clock::now() + 1s) == std::future_status::ready);
  CHECK(f.wait_until(offset_clock::now() - 1h) == std::future_status::ready);
  CHECK(f.get() == 3);

  std::promise<void> pv;
  std::shared_future<void> sf = pv.get_future().share();
  not_ready(sf);
  pv.set_value();
  CHECK(sf.wait_until(offset_clock::now()) == std::future_status::ready);

  int runs = 0;
  auto d = std::async(std::launch::deferred, [&] { return ++runs; });
  CHECK(d.wait_until(offset_clock::now() + 1h) == std::future_status::deferred);  // no blocking
  CHECK(runs == 0);
  CHECK(d.get() == 1);
  return 0;
}
