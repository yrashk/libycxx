// call_once with many flags at once, nested on different flags, and an active execution that
// waits for another flag's active execution on another thread.
//   [thread.once.callonce]/2: "An execution of call_once that does not call its func is a
//     passive execution. An execution of call_once that calls its func is an active execution.
//     An active execution evaluates INVOKE(...). If such a call to func throws an exception the
//     execution is exceptional, otherwise it is returning. An exceptional execution propagates
//     the exception to the caller of call_once. Among all executions of call_once for any given
//     once_flag: at most one is a returning execution; if there is a returning execution, it is
//     the last active execution; and there are passive executions only if there is a returning
//     execution." /3 Synchronization: the returning execution synchronizes with the return of
//     the passive ones (for the same flag). Nothing ties the executions of one flag to those of
//     another: func may call call_once on another flag, and an active execution may wait for an
//     active execution of another flag on another thread.
// 20000 flags used from 8 threads at once; chains of 1500 nested call_once calls on distinct
// flags from 4 threads; a nested exceptional execution that makes the enclosing one exceptional
// too; flags that live briefly on the stack.
// FLAGS: -pthread
#include <atomic>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>
#include "check.hpp"
#include "watchdog.hpp"

constexpr int Flags = 20000, Threads = 8, Depth = 1500;

static std::unique_ptr<std::once_flag[]> chain;
static std::atomic<int> chain_runs[Depth];
static void run_chain(int k) {
  if (k == Depth) return;
  std::call_once(chain[static_cast<std::size_t>(k)], [k] {
    chain_runs[k].fetch_add(1);
    run_chain(k + 1);
  });
}

int main() {
  watchdog(10);
  {
    std::unique_ptr<std::once_flag[]> flags(new std::once_flag[Flags]);
    std::unique_ptr<std::atomic<int>[]> runs(new std::atomic<int>[Flags]);
    for (int i = 0; i < Flags; ++i) runs[static_cast<std::size_t>(i)].store(0);
    std::vector<std::thread> ts;
    for (int t = 0; t < Threads; ++t)
      ts.emplace_back([&, t] {
        for (int n = 0; n < Flags; ++n) {
          const auto i = static_cast<std::size_t>((n * (2 * t + 1) + t * 977) % Flags);
          std::call_once(flags[i], [&] { runs[i].fetch_add(1); });
          if (runs[i].load() != 1) runs[i].store(-1000);  // the passive return synchronizes
        }
      });
    for (auto& th : ts) th.join();
    for (int i = 0; i < Flags; ++i) CHECK(runs[static_cast<std::size_t>(i)].load() == 1);
  }

  // Nested chains on distinct flags, entered from several threads.
  chain.reset(new std::once_flag[Depth]);
  {
    std::vector<std::thread> ts;
    for (int t = 0; t < 4; ++t) ts.emplace_back([] { run_chain(0); });
    for (auto& th : ts) th.join();
    for (int k = 0; k < Depth; ++k) CHECK(chain_runs[k].load() == 1);
  }

  // A nested exceptional execution: the enclosing execution is exceptional as well, and both
  // flags run their functions again on the next call.
  {
    std::once_flag outer, inner;
    int outer_runs = 0, inner_runs = 0;
    for (int attempt = 0; attempt < 3; ++attempt) {
      try {
        std::call_once(outer, [&] {
          ++outer_runs;
          std::call_once(inner, [&] {
            ++inner_runs;
            if (attempt < 2) throw std::runtime_error("inner");
          });
        });
        CHECK(attempt == 2);
      } catch (const std::runtime_error&) {
        CHECK(attempt < 2);
      }
    }
    CHECK(outer_runs == 3);
    CHECK(inner_runs == 3);
    std::call_once(outer, [&] { ++outer_runs; });
    std::call_once(inner, [&] { ++inner_runs; });
    CHECK(outer_runs == 3 && inner_runs == 3);
  }

  // An active execution of f1 (thread A) waits for thread B's active execution of f2, which
  // itself waits until A's is under way; passive executions of f1 wait meanwhile.
  {
    std::once_flag f1, f2;
    std::atomic<bool> a_in{false}, b_in{false};
    std::atomic<int> f1_runs{0}, f2_runs{0};
    std::thread b([&] {
      std::call_once(f2, [&] {
        b_in.store(true);
        while (!a_in.load()) std::this_thread::yield();
        f2_runs.fetch_add(1);
      });
    });
    std::thread a([&] {
      while (!b_in.load()) std::this_thread::yield();
      std::call_once(f1, [&] {
        a_in.store(true);
        std::call_once(f2, [&] { f2_runs.fetch_add(100); });  // passive: waits for B's
        f1_runs.fetch_add(1);
      });
    });
    std::vector<std::thread> passive;
    for (int i = 0; i < 6; ++i)
      passive.emplace_back([&] {
        while (!a_in.load()) std::this_thread::yield();
        std::call_once(f1, [&] { f1_runs.fetch_add(100); });
        CHECK(f1_runs.load() == 1);
      });
    a.join();
    b.join();
    for (auto& th : passive) th.join();
    CHECK(f1_runs.load() == 1 && f2_runs.load() == 1);
  }

  // Short-lived flags on the stack, one after another.
  long total = 0;
  for (int i = 0; i < 100000; ++i) {
    std::once_flag f;
    std::call_once(f, [&] { ++total; });
    std::call_once(f, [&] { total += 1000; });
  }
  CHECK(total == 100000);
  return 0;
}
