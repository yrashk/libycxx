// [thread.thread.member]/8-9: detach(): "The thread represented by *this continues execution
// without the calling thread blocking. When detach() returns, *this no longer represents the
// possibly continuing thread of execution." Postconditions: get_id() == id().
// FLAGS: -pthread
#include <thread>
#include <atomic>
#include "check.hpp"

int main() {
  std::atomic<int> state(0);
  std::thread t([&] {
    state.wait(0);  // blocked until main releases it: main is not blocked by detach
    state.store(2);
    state.notify_one();
  });
  t.detach();
  CHECK(!t.joinable());
  CHECK(t.get_id() == std::thread::id());
  state.store(1);
  state.notify_one();
  state.wait(1);
  CHECK(state.load() == 2);
  return 0;
}
