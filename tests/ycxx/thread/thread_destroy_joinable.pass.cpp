// [thread.thread.destr]: ~thread(): "If joinable(), invokes terminate ([except.terminate]).
// Otherwise, has no effects." The terminate handler exits with status 0.
// FLAGS: -pthread
#include <thread>
#include <exception>
#include <cstdlib>
#include <atomic>

static std::atomic<bool> release(false);

int main() {
  std::set_terminate([] { std::_Exit(0); });
  {
    std::thread t([] { release.wait(false); });
  }
  return 1;  // not reached
}
