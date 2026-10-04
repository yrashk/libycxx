// [atomics.flag]/15: wait(old): "Evaluates flag->test(order) != old. If the result of that
// evaluation is true, returns. Blocks until it is unblocked by an atomic notifying operation
// or is unblocked spuriously." A spin-free hand-off between two threads.
// FLAGS: -pthread
#include <atomic>
#include <thread>
#include "check.hpp"

int main() {
  std::atomic_flag ready, ack;
  int data = 0;
  std::thread t([&] {
    ready.wait(false);
    CHECK(data == 42);  // test_and_set is seq_cst: data happens-before
    data = 43;
    ack.test_and_set();
    ack.notify_one();
  });
  data = 42;
  ready.test_and_set();
  ready.notify_all();
  ack.wait(false);
  CHECK(data == 43);
  t.join();
  return 0;
}
