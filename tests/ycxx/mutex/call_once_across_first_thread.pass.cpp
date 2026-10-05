// [thread.once.callonce]/2-3 for once_flags used both before the program's first thread starts
// and afterwards: "at most one is a returning execution; if there is a returning execution, it
// is the last active execution; and there are passive executions only if there is a returning
// execution"; "all active executions occur in a total order; completion of an active execution
// synchronizes with the start of the next one in this total order; and the returning
// execution synchronizes with the return from all passive executions".
//  - flag A: exceptional executions while single-threaded, then many threads race: the
//    function throws a few more times (in whichever threads), then returns once; every other
//    execution is passive and sees the data written by the returning one (a plain int);
//  - flag B: completed (returning) while single-threaded: every later call is passive;
//  - flag C: never used before the threads; contended from the start.
// Active executions for one flag never overlap (checked with a plain "inside" flag per
// once_flag, guarded only by the total order).
// FLAGS: -pthread
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include "check.hpp"
#include "watchdog.hpp"

static std::once_flag A, B, C;
static int a_data = 0, b_data = 0, c_data = 0;  // written only by the returning execution
static int a_actives = 0, c_actives = 0;        // touched only inside active executions
static bool inside_a = false, inside_c = false;  // per flag
static std::atomic<int> overlap{0}, wrong{0}, a_exceptional{0};

struct Boom {};

static void active_a(int throws_until) {
  if (inside_a) ++overlap;
  inside_a = true;
  int n = ++a_actives;
  std::this_thread::yield();
  inside_a = false;
  if (n <= throws_until) throw Boom{};
  a_data = 42;
}

int main() {
  watchdog(30);
  // single-threaded: two exceptional executions of A, one returning of B
  for (int i = 0; i < 2; ++i) {
    bool threw = false;
    try {
      std::call_once(A, active_a, 4);
    } catch (Boom) {
      threw = true;
    }
    CHECK(threw);
  }
  CHECK(a_actives == 2 && a_data == 0);
  std::call_once(B, [] { b_data = 7; });
  std::call_once(B, [] { b_data = -1; });  // passive
  CHECK(b_data == 7);

  constexpr int K = 8;
  std::atomic<bool> go{false};
  std::vector<std::thread> ts;
  for (int k = 0; k < K; ++k)
    ts.emplace_back([&] {
      while (!go.load()) std::this_thread::yield();
      for (int i = 0; i < 50; ++i) {
        try {
          std::call_once(A, active_a, 4);
          if (a_data != 42) ++wrong;  // returned: by the returning execution or passively after it
        } catch (Boom) {
          ++a_exceptional;
        }
        std::call_once(B, [] { b_data = -1; });
        if (b_data != 7) ++wrong;
        std::call_once(C, [] {
          if (inside_c) ++overlap;
          inside_c = true;
          ++c_actives;
          std::this_thread::yield();
          c_data = 99;
          inside_c = false;
        });
        if (c_data != 99) ++wrong;
      }
    });
  go = true;
  for (auto& t : ts) t.join();
  CHECK(overlap.load() == 0);
  CHECK(wrong.load() == 0);
  CHECK(a_actives == 5);            // 4 exceptional + 1 returning, in total
  CHECK(a_exceptional.load() == 2);  // the two thrown after the threads started
  CHECK(a_data == 42 && b_data == 7);
  CHECK(c_actives == 1 && c_data == 99);
  // after the threads: all passive
  std::call_once(A, active_a, 0);
  std::call_once(C, [] { c_data = 0; });
  CHECK(a_actives == 5 && c_data == 99);
}
