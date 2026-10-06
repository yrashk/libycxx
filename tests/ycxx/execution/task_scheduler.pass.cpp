// [exec.task.scheduler]: task_scheduler is a type-erased scheduler.
//   /1: it models scheduler; get_forward_progress_guarantee(s) is that of SCHED(s).
//   /2-3, /5: the constructor (explicit) takes any infallible scheduler and an allocator; it
//     allocates its backend with allocate_shared and a copy of the allocator.
//   /6-7: s == sch is false if SCHED(s) is not of sch's type, else SCHED(s) == sch; s1 == s2 is
//     s1 == SCHED(s2).
//   /13.1: the completion scheduler of schedule()'s sender is *this; /13.4: its completion
//     signatures are set_value_t() for an unstoppable stop token, else set_value_t() and
//     set_stopped_t(); /13.3, /10: starting it schedules on SCHED(s): with a run_loop's scheduler
//     the receiver completes when the loop runs, and with set_stopped when
//     stop was requested through the receiver's environment (the proxy forwards the stop token).
// FLAGS: -pthread
#include <execution>
#include <cstddef>
#include <memory>
#include <stop_token>
#include <thread>
#include <type_traits>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using exec_test::done;

int allocations = 0;
template <class T>
struct CountingAlloc {
  using value_type = T;
  CountingAlloc() = default;
  template <class U>
  CountingAlloc(const CountingAlloc<U>&) noexcept {}
  T* allocate(std::size_t n) {
    ++allocations;
    return std::allocator<T>().allocate(n);
  }
  void deallocate(T* p, std::size_t n) noexcept { std::allocator<T>().deallocate(p, n); }
  template <class U>
  bool operator==(const CountingAlloc<U>&) const noexcept { return true; }
};

using TS = ex::task_scheduler;
using LoopSched = decltype(std::declval<ex::run_loop&>().get_scheduler());
static_assert(ex::scheduler<TS> && std::copyable<TS> && std::equality_comparable<TS>);
static_assert(std::is_constructible_v<TS, ex::inline_scheduler> && !std::is_convertible_v<ex::inline_scheduler, TS>);
static_assert(std::is_constructible_v<TS, LoopSched, CountingAlloc<int>>);
static_assert(!std::is_default_constructible_v<TS>);

using stoppable_env = ex::prop<std::get_stop_token_t, std::inplace_stop_token>;
using Sndr = decltype(ex::schedule(std::declval<TS&>()));
static_assert(std::is_same_v<ex::completion_signatures_of_t<Sndr, ex::env<>>, ex::completion_signatures<ex::set_value_t()>>);
static_assert(std::is_same_v<ex::completion_signatures_of_t<Sndr, stoppable_env>,
                             ex::completion_signatures<ex::set_value_t(), ex::set_stopped_t()>>);

int main() {
  ex::run_loop loop1, loop2;
  // /6-7
  TS a(loop1.get_scheduler());
  TS b(loop1.get_scheduler());
  TS c(loop2.get_scheduler());
  TS in(ex::inline_scheduler{});
  CHECK(a == loop1.get_scheduler() && !(a == loop2.get_scheduler()) && !(a == ex::inline_scheduler{}));
  CHECK(in == ex::inline_scheduler{} && !(in == loop1.get_scheduler()));
  CHECK(a == b && b == a && !(a == c) && !(a == in) && !(in == a));
  TS copy = c;
  CHECK(copy == c && copy == loop2.get_scheduler());
  copy = a;
  CHECK(copy == a && copy == loop1.get_scheduler());

  // /1
  CHECK(ex::get_forward_progress_guarantee(a) == ex::get_forward_progress_guarantee(loop1.get_scheduler()));
  CHECK(ex::get_forward_progress_guarantee(in) == ex::get_forward_progress_guarantee(ex::inline_scheduler{}));

  // /3, /5: the allocator is used for the backend.
  allocations = 0;
  TS counted(loop1.get_scheduler(), CountingAlloc<int>());
  CHECK(allocations >= 1 && counted == a);

  // /13.1
  auto s = ex::schedule(a);
  CHECK(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(s)) == a);

  // Through the inline scheduler, the operation completes inside start.
  CHECK(std::this_thread::sync_wait(ex::schedule(in)).has_value());

  // Through a run_loop: nothing happens until the loop runs.
  {
    exec_test::record<int> rec;
    auto op = ex::connect(ex::schedule(a), exec_test::receiver_for(rec));
    ex::start(op);
    CHECK(rec.how == done::none);
    std::thread t([&] { loop1.run(); });
    loop1.finish();
    t.join();
    CHECK(rec.how == done::value && rec.calls == 1);
  }
  // Stop requested through the receiver's environment: the run_loop sees it and stops.
  {
    exec_test::record<int> rec;
    std::inplace_stop_source src;
    auto op = ex::connect(ex::schedule(c), exec_test::receiver_for(rec, stoppable_env(std::get_stop_token, src.get_token())));
    ex::start(op);
    src.request_stop();
    loop2.finish();
    loop2.run();
    CHECK(rec.how == done::stopped && rec.calls == 1);
  }
  return 0;
}
