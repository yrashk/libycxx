// execution::task's scheduler affinity:
//   [task.promise]/6: await_transform(sndr) is as_awaitable(affine(sndr), *this) unless
//     start_scheduler_type is inline_scheduler, when it is as_awaitable(sndr, *this): with the
//     default start_scheduler_type (task_scheduler) the coroutine resumes on its start scheduler
//     after a co_await of a sender that completes elsewhere; with inline_scheduler it resumes
//     where the sender completed;
//   [task.state]/4.3: the start scheduler SCHED is start_scheduler_type(get_start_scheduler(
//     get_env(rcvr))): under starts_on(sch, task) that is sch ([exec.starts.on]/4: let_value's
//     receiver has SCHED-ENV(sch), [exec.let]/2.1), so the task comes back to sch's thread;
//     [task.promise]/12.1: get_start_scheduler in the task is that scheduler.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <execution>
#include <thread>
#include <tuple>
#include <utility>
#include "check.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;

struct inline_env {
  using start_scheduler_type = ex::inline_scheduler;
};

using ids = std::tuple<std::thread::id, std::thread::id, std::thread::id>;

// Where the task starts, where the awaited sender completes, where the task resumes.
ex::task<ids> hop_default(ex::run_loop& other) {
  auto start = tt::get_id();
  auto there = co_await (ex::schedule(other.get_scheduler()) | ex::then([] { return tt::get_id(); }));
  co_return ids{start, there, tt::get_id()};
}
ex::task<ids, inline_env> hop_inline(ex::run_loop& other) {
  auto start = tt::get_id();
  auto there = co_await (ex::schedule(other.get_scheduler()) | ex::then([] { return tt::get_id(); }));
  co_return ids{start, there, tt::get_id()};
}
ex::task<bool> start_scheduler_is(decltype(std::declval<ex::run_loop&>().get_scheduler()) expected) {
  auto s = co_await ex::read_env(ex::get_start_scheduler);
  co_return s == ex::task_scheduler(expected);
}

int main() {
  ex::run_loop other, home;
  std::thread t_other([&] { other.run(); });
  std::thread t_home([&] { home.run(); });
  const auto me = tt::get_id(), other_id = t_other.get_id(), home_id = t_home.get_id();

  // Default: back on the start scheduler (sync_wait's run_loop, this thread).
  {
    auto r = tt::sync_wait(hop_default(other));
    CHECK(r.has_value());
    auto [start, there, resumed] = std::get<0>(*r);
    CHECK(start == me && there == other_id && resumed == me);
  }
  // inline_scheduler: no affinity; the coroutine continues on the other thread.
  {
    auto r = tt::sync_wait(hop_inline(other));
    CHECK(r.has_value());
    auto [start, there, resumed] = std::get<0>(*r);
    CHECK(start == me && there == other_id && resumed == other_id);
  }
  // starts_on(home, task): starts on home's thread and comes back there.
  {
    auto r = tt::sync_wait(ex::starts_on(home.get_scheduler(), hop_default(other)));
    CHECK(r.has_value());
    auto [start, there, resumed] = std::get<0>(*r);
    CHECK(start == home_id && there == other_id && resumed == home_id);
    auto s = tt::sync_wait(ex::starts_on(home.get_scheduler(), start_scheduler_is(home.get_scheduler())));
    CHECK(s && std::get<0>(*s));
  }
  other.finish();
  home.finish();
  t_other.join();
  t_home.join();
  return 0;
}
