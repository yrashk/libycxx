// [thread.thread.assign]/1: thread& operator=(thread&& x) noexcept: "Effects: If joinable(),
// invokes terminate ([except.terminate])."
// FLAGS: -pthread
#include <thread>
#include <exception>
#include <cstdlib>
#include <atomic>

static std::atomic<bool> release(false);

int main() {
  std::thread t([] { release.wait(false); });
  std::set_terminate([] { std::_Exit(0); });
  t = std::thread();
  return 1;  // not reached
}
