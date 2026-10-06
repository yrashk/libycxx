// Execution resources and transitions between them:
//   [exec.run.loop]: run() executes the scheduled work in FIFO order on the calling thread until
//     finish() and an empty queue; a schedule sender of a run_loop scheduler completes with
//     set_value, or set_stopped when its receiver's stop token is stopped when the work runs.
//     Its schedulers compare equal iff from the same run_loop; get_completion_scheduler of its
//     schedule sender is the scheduler.
//   [exec.continues.on]: continues_on(sndr, sch) starts sndr here and completes on sch.
//   [exec.starts.on]: starts_on(sch, sndr) starts sndr on sch.
//   [exec.on]: on(sch, sndr) starts sndr on sch and comes back to the receiver's start scheduler;
//     on(sndr, sch, closure) runs closure on sch and comes back to where sndr completed.
//   [exec.schedule.from]: schedule_from(sndr) completes as sndr does.
//   [exec.affine]: affine(sndr) completes on the receiver's start scheduler.
//   [exec.inline.scheduler]: inline_scheduler's schedule sender completes in start.
// FLAGS: -pthread
#include <execution>
#include <thread>
#include <type_traits>
#include <vector>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;
using namespace exec_test;

struct loop_thread {
  ex::run_loop loop;
  std::thread t{[this] { loop.run(); }};
  std::thread::id id = t.get_id();
  ~loop_thread() {
    loop.finish();
    t.join();
  }
};

int main() {
  // run_loop on this thread: FIFO order.
  {
    ex::run_loop loop;
    auto sch = loop.get_scheduler();
    static_assert(ex::scheduler<decltype(sch)>);
    CHECK(sch == loop.get_scheduler());
    ex::run_loop other;
    CHECK(!(sch == other.get_scheduler()));
    CHECK(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(ex::schedule(sch))) == sch);
    CHECK(ex::get_completion_scheduler<ex::set_stopped_t>(ex::get_env(ex::schedule(sch))) == sch);
    std::vector<int> order;
    auto s1 = ex::schedule(sch) | ex::then([&]() noexcept { order.push_back(1); });
    auto s2 = ex::schedule(sch) | ex::then([&]() noexcept { order.push_back(2); });
    record<int> r1, r2;
    auto op1 = ex::connect(std::move(s1), receiver_for(r1));
    auto op2 = ex::connect(std::move(s2), receiver_for(r2));
    ex::start(op1);
    ex::start(op2);
    CHECK(r1.how == done::none);
    loop.finish();
    loop.run();
    CHECK(order.size() == 2 && order[0] == 1 && order[1] == 2);
    CHECK(r1.how == done::value && r2.how == done::value);
  }
  // A stop request before the work runs: set_stopped.
  {
    ex::run_loop loop;
    std::inplace_stop_source src;
    record<int> r;
    auto op = ex::connect(ex::schedule(loop.get_scheduler()), receiver_for(r, ex::env{ex::prop(std::get_stop_token, src.get_token())}));
    ex::start(op);
    src.request_stop();
    loop.finish();
    loop.run();
    CHECK(r.how == done::stopped);
    // Signatures: set_stopped only when the token can be stopped.
    using S = ex::schedule_result_t<decltype(loop.get_scheduler())>;
    static_assert(std::is_same_v<ex::completion_signatures_of_t<S, ex::env<>>, ex::completion_signatures<ex::set_value_t()>>);
    static_assert(ex::sends_stopped<S, decltype(ex::env{ex::prop(std::get_stop_token, src.get_token())})>);
  }
  // continues_on, starts_on, on.
  {
    loop_thread a, b;
    auto r = tt::sync_wait(ex::just(1) | ex::continues_on(a.loop.get_scheduler()) |
                           ex::then([](int x) { return std::pair(x, tt::get_id()); }));
    CHECK(r && std::get<0>(*r).first == 1 && std::get<0>(*r).second == a.id);

    auto s = ex::starts_on(b.loop.get_scheduler(), ex::just() | ex::then([] { return tt::get_id(); }));
    auto r2 = tt::sync_wait(std::move(s));
    CHECK(r2 && std::get<0>(*r2) == b.id);

    // on(sch, sndr): runs on b, completes back on the receiver's start scheduler (sync_wait's
    // run_loop, on this thread).
    auto me = tt::get_id();
    auto r3 = tt::sync_wait(ex::on(b.loop.get_scheduler(), ex::just() | ex::then([] { return tt::get_id(); })) |
                            ex::then([](std::thread::id inner) { return std::pair(inner, tt::get_id()); }));
    CHECK(r3 && std::get<0>(*r3).first == b.id && std::get<0>(*r3).second == me);

    // on(sndr, sch, closure): the closure runs on a, then back where sndr completed (b).
    auto r4 = tt::sync_wait(ex::schedule(b.loop.get_scheduler()) |
                            ex::on(a.loop.get_scheduler(), ex::then([] { return tt::get_id(); })) |
                            ex::then([](std::thread::id inner) { return std::pair(inner, tt::get_id()); }));
    CHECK(r4 && std::get<0>(*r4).first == a.id && std::get<0>(*r4).second == b.id);

    // schedule_from completes as its child does.
    auto r5 = tt::sync_wait(ex::schedule_from(ex::just(7)));
    CHECK(r5 && std::get<0>(*r5) == 7);

    // affine: back on the start scheduler of the receiver.
    auto r6 = tt::sync_wait(ex::affine(ex::schedule(a.loop.get_scheduler()) | ex::then([] { return tt::get_id(); })) |
                            ex::then([](std::thread::id inner) { return std::pair(inner, tt::get_id()); }));
    CHECK(r6 && std::get<0>(*r6).first == a.id && std::get<0>(*r6).second == me);
    // affine of just needs no scheduling.
    auto r7 = tt::sync_wait(ex::just(3) | ex::affine);
    CHECK(r7 && std::get<0>(*r7) == 3);
  }
  // inline_scheduler
  {
    ex::inline_scheduler sch;
    record<int> r;
    run(ex::schedule(sch), receiver_for(r));
    CHECK(r.how == done::value);
    static_assert(std::is_same_v<ex::completion_signatures_of_t<ex::schedule_result_t<ex::inline_scheduler>>,
                                 ex::completion_signatures<ex::set_value_t()>>);
  }
  return 0;
}
