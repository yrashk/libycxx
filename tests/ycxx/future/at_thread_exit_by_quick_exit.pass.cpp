// [support.start.term] quick_exit: "Objects shall not be destroyed as a result of calling
// quick_exit." No thread_local object is destroyed, so the thread-exit actions, which follow the
// destruction of the thread's thread_local objects ([futures.promise]/23,
// [thread.condition.nonmember]/2), do not run: the at_quick_exit function still sees them pending.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <future>

std::future<int> f;

int main() {
  auto* p = new std::promise<int>;
  f = p->get_future();
  p->set_value_at_thread_exit(1);
  std::at_quick_exit([] {
    if (f.wait_for(std::chrono::seconds(0)) != std::future_status::timeout) {
      std::fputs("FAIL: an action ran on quick_exit\n", stderr);
      std::_Exit(3);
    }
    std::_Exit(0);
  });
  std::quick_exit(2);
}
