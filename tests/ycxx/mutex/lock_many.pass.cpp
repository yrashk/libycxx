// [thread.lock.algorithm]/5: lock(l1, l2, l3...): "All arguments are locked via a sequence of
// calls to lock(), try_lock(), or unlock() on each argument. The sequence of calls does not
// result in deadlock". [thread.lock.scoped]/2: scoped_lock(m...) with several mutexes is
// lock(m...). Six mutexes of mixed types (mutex, recursive_mutex, timed_mutex, and unique_locks
// around them, [thread.lock.unique] meets Cpp17Lockable), four threads each locking all of them
// in a different, adversarial argument order (reversed, rotated, interleaved), many times; plus
// threads locking overlapping subsets. On return every argument is owned: the non-atomic
// counters guarded by the whole set stay consistent.
// FLAGS: -pthread
#include <mutex>
#include <thread>
#include <vector>
#include "check.hpp"
#include "watchdog.hpp"

std::mutex m0, m3;
std::recursive_mutex m1, m4;
std::timed_mutex m2, m5;
long counter[6];

static void bump_all() {
  for (long& c : counter) ++c;
}

int main() {
  watchdog(30);
  constexpr int iters = 3000;
  std::vector<std::thread> ts;
  ts.emplace_back([] {
    for (int i = 0; i < iters; ++i) {
      std::lock(m0, m1, m2, m3, m4, m5);
      bump_all();
      m0.unlock(); m1.unlock(); m2.unlock(); m3.unlock(); m4.unlock(); m5.unlock();
    }
  });
  ts.emplace_back([] {
    for (int i = 0; i < iters; ++i) {
      std::lock(m5, m4, m3, m2, m1, m0);
      bump_all();
      m5.unlock(); m4.unlock(); m3.unlock(); m2.unlock(); m1.unlock(); m0.unlock();
    }
  });
  ts.emplace_back([] {
    for (int i = 0; i < iters; ++i) {
      std::unique_lock<std::timed_mutex> l2(m2, std::defer_lock), l5(m5, std::defer_lock);
      std::unique_lock<std::mutex> l3(m3, std::defer_lock);
      std::lock(l3, m4, l5, m0, l2, m1);  // rotated, with unique_locks
      CHECK(l2.owns_lock() && l3.owns_lock() && l5.owns_lock());
      bump_all();
      m4.unlock(); m0.unlock(); m1.unlock();
    }
  });
  ts.emplace_back([] {
    for (int i = 0; i < iters; ++i) {
      std::scoped_lock l(m1, m3, m5, m0, m2, m4);  // interleaved
      bump_all();
    }
  });
  // subsets that overlap the full set
  ts.emplace_back([] {
    for (int i = 0; i < iters; ++i) {
      std::scoped_lock l(m4, m2, m0);
      std::lock_guard<std::recursive_mutex> again(m4);  // recursive: one more level
    }
  });
  ts.emplace_back([] {
    for (int i = 0; i < iters; ++i) {
      std::scoped_lock l(m5, m3, m1);
    }
  });
  for (auto& t : ts) t.join();
  for (long c : counter) CHECK(c == 4L * iters);
  // everything is unlocked again
  CHECK(std::try_lock(m0, m1, m2, m3, m4, m5) == -1);
  m0.unlock(); m1.unlock(); m2.unlock(); m3.unlock(); m4.unlock(); m5.unlock();
  return 0;
}
