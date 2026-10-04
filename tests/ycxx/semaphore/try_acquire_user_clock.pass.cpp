// [thread.sema.cnt]/18: try_acquire_until: "Repeatedly performs the following steps, in order:
// Evaluates try_acquire(). If the result is true, returns true. Blocks on *this until counter is
// greater than zero or until the timeout expires. If it is unblocked by the timeout expiring,
// returns false. The timeout expires ([thread.req.timing]) when the current time is after
// abs_time". /19: "Throws: Timeout-related exceptions". With a user-defined clock
// ([thread.req.timing]/4, /8): a zero counter times out no earlier than abs_time; a clock that
// throws while the counter stays zero makes the call throw; a release before the deadline is
// observed.
// FLAGS: -pthread
#include <semaphore>
#include <thread>
#include <chrono>
#include "check.hpp"
#include "test_clocks.hpp"
#include "watchdog.hpp"

using namespace std::chrono_literals;

int main() {
  watchdog(20);
  std::counting_semaphore<4> s(0);
  auto dl = offset_clock::now() + 10ms;
  CHECK(!s.try_acquire_until(dl));
  CHECK(offset_clock::now() >= dl);
  CHECK(!s.try_acquire_until(offset_clock::now() - 1h));

  auto tp = throwing_clock::now() + 10ms;
  throwing_clock::armed = true;
  bool thrown = false;
  try {
    (void)s.try_acquire_until(tp);
  } catch (clock_error) {
    thrown = true;
  }
  throwing_clock::armed = false;
  CHECK(thrown);

  std::binary_semaphore b(0);
  std::thread t([&] { b.release(); });
  CHECK(b.try_acquire_until(offset_clock::now() + 30s));  // released long before the deadline
  t.join();

  s.release(2);
  CHECK(s.try_acquire_until(offset_clock::now() + 1s));
  CHECK(s.try_acquire_until(offset_clock::now() + 1s));
  return 0;
}
