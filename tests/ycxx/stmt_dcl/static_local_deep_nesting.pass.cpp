// Block-scope statics whose initializers initialize other block-scope statics, nested several
// hundred deep in one thread, with an exception from the innermost initializer on the first
// attempt, and other threads entering the same declarations meanwhile:
//   [stmt.dcl]/3: "Dynamic initialization of a block variable with static storage duration ...
//     is performed the first time control passes through its declaration; such a variable is
//     considered initialized upon the completion of its initialization. If the initialization
//     exits by throwing an exception, the initialization is not complete, so it will be tried
//     again the next time control enters the declaration. If control enters the declaration
//     concurrently while the variable is being initialized, the concurrent execution shall wait
//     for completion of the initialization." [Note 2: "A conforming implementation cannot
//     introduce any deadlock around execution of the initializer."]
// So: the first attempt runs every initializer once and none completes; the second runs each
// once more and all complete, in nesting order; afterwards nothing runs again; threads that
// enter an outer declaration while the chain is being initialized wait and then see the value.
// FLAGS: -pthread
#include <atomic>
#include <stdexcept>
#include <thread>
#include <utility>
#include "check.hpp"

constexpr int Depth = 400;
static int runs[Depth + 1];
static std::atomic<bool> fail_innermost{true};
static std::atomic<bool> inside{false};
static std::atomic<int> waiting{0};

template <int I>
int level() {
  static int v = [] {
    ++runs[I];
    if constexpr (I == Depth) {
      inside = true;
      // Let the other threads enter the outer declarations while every level is in progress.
      while (waiting.load() < 2) std::this_thread::yield();
      if (fail_innermost.exchange(false)) throw std::runtime_error("innermost");
      return 1;
    } else {
      return level<I + 1>() + 1;
    }
  }();
  return v;
}

template <int... I>
bool all_initialized(std::integer_sequence<int, I...>) {
  return ((level<I>() == Depth - I + 1) && ...);
}

int main() {
  std::thread waiters[2];
  for (auto& w : waiters)
    w = std::thread([] {
      while (!inside.load()) std::this_thread::yield();
      waiting.fetch_add(1);
      // Enters level<0> while the main thread initializes it: waits for that attempt, and if it
      // failed, initializes the chain itself (or waits for the other thread to).
      for (;;) {
        try {
          CHECK(level<0>() == Depth + 1);
          return;
        } catch (const std::runtime_error&) {
          CHECK(false);  // only the first attempt (the main thread's) throws
        }
      }
    });
  bool threw = false;
  try {
    (void)level<0>();
  } catch (const std::runtime_error&) {
    threw = true;
  }
  for (auto& w : waiters) w.join();
  CHECK(threw);  // the main thread's attempt, the first, failed
  CHECK(level<0>() == Depth + 1);
  for (int i = 0; i <= Depth; ++i) CHECK(runs[i] == 2);
  CHECK(all_initialized(std::make_integer_sequence<int, Depth + 1>()));
  for (int i = 0; i <= Depth; ++i) CHECK(runs[i] == 2);
  return 0;
}
