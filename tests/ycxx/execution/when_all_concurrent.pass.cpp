// when_all with children that complete concurrently on the parallel scheduler's threads
// ([exec.when.all]/15-17; count and disposition are atomic): one completion reaches the
// receiver; when errors race, it is one of the errors sent (the first, /17), never a value (the
// stop request it makes may stop the other children before their work runs); when one child
// stops and none fails, stopped; all values: the values, each in its child's position.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <execution>
#include <atomic>
#include <exception>
#include <thread>
#include <tuple>
#include <utility>
#include "check.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;

// Declares a value completion, completes with stopped.
struct stops {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_stopped_t()>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    void start() & noexcept { ex::set_stopped(std::move(r)); }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r)};
  }
};

int main() {
  auto sch = ex::get_parallel_scheduler();
  for (int round = 0; round < 200; ++round) {
    std::atomic<int> done{0};
    auto child = [&](int i, bool fail) {
      return ex::schedule(sch) | ex::then([&done, i, fail]() -> int {
               ++done;
               if (fail)
                 throw i;
               return i;
             });
    };
    // Errors race: one of them.
    {
      done = 0;
      int got = -1;
      try {
        (void)tt::sync_wait(ex::when_all(child(1, true), child(2, false), child(3, true), child(4, true)));
      } catch (int e) {
        got = e;
      }
      CHECK(got == 1 || got == 3 || got == 4);
      // (the others may have been stopped before their then ran: the error requests a stop)
      CHECK(done >= 1 && done <= 4);
    }
    // Values only.
    {
      done = 0;
      auto r = tt::sync_wait(ex::when_all(child(5, false), child(6, false), child(7, false)));
      CHECK(r && std::get<0>(*r) == 5 && std::get<1>(*r) == 6 && std::get<2>(*r) == 7 && done == 3);
    }
    // A stop among values: stopped.
    {
      done = 0;
      auto r = tt::sync_wait(ex::when_all(child(8, false), ex::schedule(sch) | ex::let_value([] { return stops(); }),
                                          child(9, false)));
      CHECK(!r && done <= 2);
    }
  }
  return 0;
}
