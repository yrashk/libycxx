// [thread.mutex.requirements.mutex.general]: "The implementation provides lock and unlock
// operations ... Prior unlock() operations on the same object shall synchronize with this
// operation." Mutual exclusion of a non-atomic counter across threads, for every mutex type.
// FLAGS: -pthread
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>
#include "check.hpp"

template<class M>
static void run() {
  M m;
  long counter = 0;
  std::vector<std::thread> ts;
  for (int t = 0; t < 4; ++t)
    ts.emplace_back([&] {
      for (int i = 0; i < 5000; ++i) {
        m.lock();
        ++counter;
        m.unlock();
      }
    });
  for (auto& th : ts) th.join();
  CHECK(counter == 20000);
}

int main() {
  run<std::mutex>();
  run<std::recursive_mutex>();
  run<std::timed_mutex>();
  run<std::recursive_timed_mutex>();
  run<std::shared_mutex>();
  run<std::shared_timed_mutex>();
  return 0;
}
