// [thread.thread.this]: sleep_until(abs_time): "Effects: Blocks the calling thread for the
// absolute timeout ([thread.req.timing]) specified by abs_time." "Throws: Timeout-related
// exceptions." [thread.req.timing]/4: with a user-defined clock that is not adjusted, the return
// is no earlier than abs_time; /8: an exception thrown by the clock propagates. A time point in
// the past returns at once. sleep_for with a user duration type (rep long long, period milli).
// FLAGS: -pthread
#include <thread>
#include <chrono>
#include "check.hpp"
#include "test_clocks.hpp"
#include "watchdog.hpp"

using namespace std::chrono_literals;

int main() {
  watchdog(20);
  auto dl = offset_clock::now() + 10ms;
  std::this_thread::sleep_until(dl);
  CHECK(offset_clock::now() >= dl);
  std::this_thread::sleep_until(offset_clock::now() - 1h);

  auto t0 = std::chrono::steady_clock::now();
  std::this_thread::sleep_for(offset_clock::duration(5));
  CHECK(std::chrono::steady_clock::now() - t0 >= 5ms);

  auto tp = throwing_clock::now() + 10ms;
  throwing_clock::armed = true;
  bool thrown = false;
  try {
    std::this_thread::sleep_until(tp);
  } catch (clock_error) {
    thrown = true;
  }
  throwing_clock::armed = false;
  CHECK(thrown);
  return 0;
}
