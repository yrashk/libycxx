// [thread.latch.class]: latch(expected) initializes the counter; max() >= ... is a static
// constexpr noexcept; count_down(update) "Atomically decrements counter by update. If counter
// is equal to zero, unblocks all threads blocked on *this."; try_wait(): "With very low
// probability false. Otherwise counter == 0."; wait() returns immediately if counter is zero;
// arrive_and_wait(update) is count_down(update); wait(). Not copyable. constexpr constructor.
// FLAGS: -pthread
#include <latch>
#include <thread>
#include <atomic>
#include <vector>
#include <type_traits>
#include "check.hpp"

static_assert(!std::is_copy_constructible_v<std::latch>);
static_assert(!std::is_copy_assignable_v<std::latch>);
static_assert(!std::is_convertible_v<std::ptrdiff_t, std::latch>);  // explicit
static_assert(noexcept(std::latch::max()));
static_assert(std::latch::max() > 0);
static_assert(noexcept(std::declval<const std::latch&>().try_wait()));
constinit std::latch global(0);

int main() {
  global.wait();  // counter already zero
  CHECK(global.try_wait() || global.try_wait() || global.try_wait());

  std::latch l(3);
  CHECK(!l.try_wait());  // counter != 0: never true
  l.count_down();
  CHECK(!l.try_wait());
  l.count_down(2);
  bool done = false;
  for (int i = 0; i < 100 && !done; ++i) done = l.try_wait();
  CHECK(done);
  l.wait();

  std::latch z(0);
  z.wait();
  z.arrive_and_wait(0);

  // workers count down; the main thread waits for all
  constexpr int n = 4;
  std::latch start(1), finished(n);
  std::atomic<int> work(0);
  std::vector<std::thread> ts;
  for (int i = 0; i < n; ++i)
    ts.emplace_back([&] {
      start.wait();
      work.fetch_add(1);
      finished.count_down();
    });
  CHECK(work.load() == 0);
  start.count_down();
  finished.wait();
  CHECK(work.load() == n);  // count_down strongly happens before wait's return
  for (auto& t : ts) t.join();

  // arrive_and_wait as a rendezvous
  std::latch meet(3);
  std::atomic<int> arrived(0);
  std::vector<std::thread> ts2;
  for (int i = 0; i < 2; ++i)
    ts2.emplace_back([&] { arrived.fetch_add(1); meet.arrive_and_wait(); CHECK(arrived.load() == 2); });
  while (arrived.load() != 2) std::this_thread::yield();
  meet.arrive_and_wait();
  for (auto& t : ts2) t.join();
  return 0;
}
