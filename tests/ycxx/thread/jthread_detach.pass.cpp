// [thread.jthread.mem]/8-9: detach(): *this no longer represents the thread; get_id() == id().
// The stop source stays with the jthread object ([thread.jthread.stop]); the detached thread's
// token still observes request_stop() made through it.
// FLAGS: -pthread
#include <thread>
#include <stop_token>
#include <atomic>
#include "check.hpp"

int main() {
  std::atomic<int> state(0);
  std::jthread t([&](std::stop_token st) {
    while (!st.stop_requested()) std::this_thread::yield();
    state.store(1);
    state.notify_one();
  });
  t.detach();
  CHECK(!t.joinable());
  CHECK(t.get_id() == std::jthread::id());
  CHECK(t.request_stop());
  state.wait(0);
  CHECK(state.load() == 1);
  return 0;  // ~jthread: not joinable, nothing to do
}
