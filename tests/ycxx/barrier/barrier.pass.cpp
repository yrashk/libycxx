// [thread.barrier.class]/1-3: each phase: arrivals decrement the expected count; "Exactly once
// after the expected count reaches zero, a thread executes the completion step"; the step
// "Invokes the completion function, equivalent to completion()" and then unblocks the waiters;
// the count is reset for the next phase. "The end of the completion step strongly happens before
// the returns from all calls that were unblocked by the completion step." /23: arrive_and_wait()
// is wait(arrive()).
// FLAGS: -pthread
#include <barrier>
#include <thread>
#include <atomic>
#include <vector>
#include <type_traits>
#include "check.hpp"

static_assert(!std::is_copy_constructible_v<std::barrier<>>);
static_assert(!std::is_copy_assignable_v<std::barrier<>>);
static_assert(noexcept(std::barrier<>::max()));
static_assert(std::barrier<>::max() > 0);
static_assert(!std::is_convertible_v<std::ptrdiff_t, std::barrier<>>);  // explicit
static_assert(std::is_move_constructible_v<std::barrier<>::arrival_token>);
static_assert(std::is_move_assignable_v<std::barrier<>::arrival_token>);

int main() {
  constexpr int n = 4, phases = 50;
  int completions = 0;  // touched by the completion step, read by the participants after it
  std::vector<int> slots(n, 0);
  bool consistent = true;
  auto on_completion = [&]() noexcept {
    if (completions % 2 == 0)  // the first barrier of each round: all slots were written
      for (int s : slots)
        if (s != completions / 2 + 1) consistent = false;
    ++completions;
  };
  std::barrier sync(n, on_completion);
  static_assert(std::is_same_v<decltype(sync), std::barrier<decltype(on_completion)>>);
  std::atomic<bool> bad(false);
  std::vector<std::thread> ts;
  for (int i = 0; i < n; ++i)
    ts.emplace_back([&, i] {
      for (int p = 0; p < phases; ++p) {
        slots[i] = p + 1;
        sync.arrive_and_wait();
        if (completions != 2 * p + 1) bad = true;  // the step ran (exactly once) before our return
        sync.arrive_and_wait();  // keeps the slots stable until everyone has checked
      }
    });
  for (auto& t : ts) t.join();
  CHECK(!bad.load());
  CHECK(consistent);
  CHECK(completions == 2 * phases);
  return 0;
}
