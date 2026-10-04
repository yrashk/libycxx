// [thread.thread.this]: yield(); sleep_for(rel_time) "Blocks the calling thread for the relative
// timeout specified by rel_time"; sleep_until(abs_time) blocks until the absolute timeout;
// [thread.req.timing]/... : a negative or past timeout returns at once. hardware_concurrency()
// is noexcept ("the number of hardware thread contexts ... or 0").
// FLAGS: -pthread
#include <thread>
#include <chrono>
#include "check.hpp"

static_assert(noexcept(std::this_thread::yield()));
static_assert(noexcept(std::thread::hardware_concurrency()));
static_assert(noexcept(std::jthread::hardware_concurrency()));

int main() {
  using namespace std::chrono;
  std::this_thread::yield();
  auto t0 = steady_clock::now();
  std::this_thread::sleep_for(milliseconds(2));
  CHECK(steady_clock::now() - t0 >= milliseconds(2));
  auto target = steady_clock::now() + milliseconds(2);
  std::this_thread::sleep_until(target);
  CHECK(steady_clock::now() >= target);
  std::this_thread::sleep_until(system_clock::now() + microseconds(500));

  // past / negative: no wait
  std::this_thread::sleep_for(milliseconds(-100));
  std::this_thread::sleep_for(nanoseconds(0));
  std::this_thread::sleep_until(steady_clock::now() - hours(1));
  std::this_thread::sleep_for(duration<double, std::milli>(0.5));

  unsigned n = std::thread::hardware_concurrency();
  CHECK(n == std::jthread::hardware_concurrency());
  return 0;
}
