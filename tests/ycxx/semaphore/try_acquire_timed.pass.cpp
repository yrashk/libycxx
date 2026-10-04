// [thread.sema.cnt]/18-: try_acquire_for / try_acquire_until: "Repeatedly performs the following
// steps, in order: Evaluates try_acquire(). If the result is true, returns true. Blocks on
// *this until counter is greater than zero or until the timeout expires. If it is unblocked by
// the timeout expiring, returns false." The timeout is [thread.req.timing]'s.
// FLAGS: -pthread
#include <semaphore>
#include <thread>
#include <chrono>
#include "check.hpp"

using namespace std::chrono;

int main() {
  std::counting_semaphore<3> s(1);
  CHECK(s.try_acquire_for(seconds(5)));
  auto t0 = steady_clock::now();
  CHECK(!s.try_acquire_for(milliseconds(3)));
  CHECK(steady_clock::now() - t0 >= milliseconds(3));
  CHECK(!s.try_acquire_until(steady_clock::now() + milliseconds(2)));
  CHECK(!s.try_acquire_until(system_clock::now() - seconds(1)));
  CHECK(!s.try_acquire_for(milliseconds(-1)));

  std::thread t([&] {
    std::this_thread::sleep_for(milliseconds(1));
    s.release();
  });
  CHECK(s.try_acquire_for(seconds(30)));
  t.join();
  s.release(2);
  CHECK(s.try_acquire_until(steady_clock::now() + seconds(30)));
  CHECK(s.try_acquire_for(milliseconds(0)) || s.try_acquire_for(seconds(1)));
  return 0;
}
