// [thread.sema.cnt]: counting_semaphore(desired) initializes the counter; max() >=
// least_max_value; release(update) "Atomically execute counter += update. Then, unblocks any
// threads that are waiting for counter to be greater than zero."; try_acquire() decrements if
// positive ("An implementation may fail to decrement counter even if it is positive"; it never
// succeeds when the counter is zero); acquire() blocks until it can decrement.
// [semaphore.syn]: binary_semaphore is counting_semaphore<1>.
// FLAGS: -pthread
#include <semaphore>
#include <thread>
#include <atomic>
#include <vector>
#include <type_traits>
#include "check.hpp"

static_assert(std::is_same_v<std::binary_semaphore, std::counting_semaphore<1>>);
static_assert(std::counting_semaphore<5>::max() >= 5);
static_assert(std::binary_semaphore::max() >= 1);
static_assert(std::counting_semaphore<>::max() >= 1);
static_assert(noexcept(std::counting_semaphore<5>::max()));
static_assert(!std::is_copy_constructible_v<std::binary_semaphore>);
static_assert(!std::is_convertible_v<std::ptrdiff_t, std::binary_semaphore>);  // explicit
static_assert(noexcept(std::declval<std::binary_semaphore&>().try_acquire()));
constinit std::counting_semaphore<4> global(2);

template<class S>
static bool try_acquire_retry(S& s) {
  for (int i = 0; i < 10000; ++i)
    if (s.try_acquire()) return true;
  return false;
}

int main() {
  CHECK(try_acquire_retry(global));
  CHECK(try_acquire_retry(global));
  CHECK(!global.try_acquire());  // zero: never succeeds
  global.release(2);
  CHECK(try_acquire_retry(global));
  global.acquire();
  CHECK(!global.try_acquire());

  std::counting_semaphore<10> s(0);
  s.release(0);
  CHECK(!s.try_acquire());
  s.release(3);
  for (int i = 0; i < 3; ++i) s.acquire();
  CHECK(!s.try_acquire());

  // acquire blocks until another thread releases
  std::binary_semaphore ping(0), pong(0);
  int value = 0;
  std::thread t([&] {
    for (int i = 0; i < 100; ++i) {
      ping.acquire();
      ++value;
      pong.release();
    }
  });
  for (int i = 0; i < 100; ++i) {
    ping.release();
    pong.acquire();
    CHECK(value == i + 1);
  }
  t.join();

  // bounded concurrency
  std::counting_semaphore<2> slots(2);
  std::atomic<int> inside(0), peak(0);
  std::vector<std::thread> ts;
  for (int i = 0; i < 6; ++i)
    ts.emplace_back([&] {
      for (int k = 0; k < 200; ++k) {
        slots.acquire();
        int now = inside.fetch_add(1) + 1;
        peak.fetch_max(now);
        inside.fetch_sub(1);
        slots.release();
      }
    });
  for (auto& th : ts) th.join();
  CHECK(peak.load() <= 2 && peak.load() >= 1);
  return 0;
}
