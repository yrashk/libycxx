// [std.modules]/2: `import std;` provides threads, synchronization, atomics and futures.
// MODULES: std
// FLAGS: -pthread
import std;
#include "module_check.hpp"

int main() {
  std::atomic<int> counter = 0;
  std::mutex m;
  int guarded = 0;
  std::latch done(2);
  std::vector<std::jthread> threads;
  for (int i = 0; i < 2; ++i)
    threads.emplace_back([&] {
      counter.fetch_add(1, std::memory_order_relaxed);
      std::lock_guard lock(m);
      ++guarded;
      done.count_down();
    });
  done.wait();
  threads.clear();
  CHECK(counter.load() == 2 && guarded == 2);
  std::promise<int> pr;
  auto fut = pr.get_future();
  std::thread th([&] { pr.set_value(42); });
  CHECK(fut.get() == 42);
  th.join();
  CHECK(std::async(std::launch::async, [] { return 7; }).get() == 7);
  std::condition_variable cv;
  bool ready = false;
  std::thread waiter([&] {
    std::unique_lock lk(m);
    cv.wait(lk, [&] { return ready; });
  });
  {
    std::lock_guard lk(m);
    ready = true;
  }
  cv.notify_one();
  waiter.join();
  std::counting_semaphore<2> sem(1);
  CHECK(sem.try_acquire() && !sem.try_acquire());
  sem.release();
  std::barrier bar(1);
  bar.arrive_and_wait();
  std::shared_mutex sm;
  {
    std::shared_lock sl(sm);
  }
  std::once_flag once;
  int calls = 0;
  std::call_once(once, [&] { ++calls; });
  std::call_once(once, [&] { ++calls; });
  CHECK(calls == 1);
  std::this_thread::sleep_for(std::chrono::microseconds(1));
  CHECK(std::this_thread::get_id() != std::thread::id());
  std::atomic_ref<int> ar(guarded);
  CHECK(ar.exchange(5) == 2 && guarded == 5);
  std::atomic_flag flag;
  CHECK(!flag.test_and_set());
  return 0;
}
