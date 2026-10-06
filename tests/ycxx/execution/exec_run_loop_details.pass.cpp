// execution::run_loop ([exec.run.loop]):
//   [exec.run.loop.general] synopsis: run_loop() noexcept, not movable; get_scheduler(), run()
//     and finish() are noexcept.
//   [exec.run.loop.ctor]/1: a new run_loop has count 0, state starting (so it may be destroyed
//     at once, /2).
//   [exec.run.loop.types]/4: schedule(sch) is not potentially-throwing; /5, /8.2:
//     get_completion_scheduler<set_value_t> and <set_stopped_t> of the schedule sender's
//     attributes compare equal to sch; /6: its completion signatures are set_value_t() for an
//     environment with an unstoppable token, and set_value_t(), set_stopped_t() otherwise;
//     /8.1: connect is potentially-throwing iff copying the receiver is; /10.2: when the work
//     executes, it completes with set_stopped if a stop was requested by then (even after start),
//     else with set_value; /10.3: start only enqueues.
//   [exec.run.loop.members]/6 with /1.1: run() after finish() executes what is queued, in FIFO
//     order, then returns (count 0 and state finishing).
#include <execution>
#include <stop_token>
#include <type_traits>
#include <utility>
#include <vector>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

using stoppable_env = ex::prop<std::get_stop_token_t, std::inplace_stop_token>;
using unstoppable_env = ex::prop<std::get_stop_token_t, std::never_stop_token>;

// A receiver whose copy constructor may throw (receivers are nothrow move constructible,
// [exec.recv.concepts]).
struct throwing_copy_receiver {
  using receiver_concept = ex::receiver_tag;
  throwing_copy_receiver() = default;
  throwing_copy_receiver(const throwing_copy_receiver&) noexcept(false) {}
  throwing_copy_receiver(throwing_copy_receiver&&) noexcept {}
  void set_value() && noexcept {}
  void set_stopped() && noexcept {}
};

// Records the order in which operations ran.
struct order_receiver {
  using receiver_concept = ex::receiver_tag;
  std::vector<int>* log;
  int id;
  void set_value() && noexcept { log->push_back(id); }
  void set_stopped() && noexcept { log->push_back(-id); }
};

int main() {
  static_assert(std::is_nothrow_default_constructible_v<ex::run_loop>);
  static_assert(!std::is_move_constructible_v<ex::run_loop> && !std::is_copy_constructible_v<ex::run_loop>);
  {
    ex::run_loop loop; // destroyed in its starting state with nothing queued: no effects
    static_assert(noexcept(loop.get_scheduler()) && noexcept(loop.run()) && noexcept(loop.finish()));
    auto sch = loop.get_scheduler();
    static_assert(ex::scheduler<decltype(sch)>);
    static_assert(noexcept(ex::schedule(sch)));
    auto s = ex::schedule(sch);
    CHECK(ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(s)) == sch);
    CHECK(ex::get_completion_scheduler<ex::set_stopped_t>(ex::get_env(s)) == sch);

    static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(s), unstoppable_env>,
                                 ex::completion_signatures<ex::set_value_t()>>);
    static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(s), ex::env<>>,
                                 ex::completion_signatures<ex::set_value_t()>>); // never_stop_token
    using both = ex::completion_signatures_of_t<decltype(s), stoppable_env>;
    static_assert(std::is_same_v<both, ex::completion_signatures<ex::set_value_t(), ex::set_stopped_t()>> ||
                  std::is_same_v<both, ex::completion_signatures<ex::set_stopped_t(), ex::set_value_t()>>);
    record<int> r;
    auto rr = receiver_for(r);
    throwing_copy_receiver tr;
    static_assert(noexcept(ex::connect(s, rr)));
    static_assert(noexcept(ex::connect(s, std::move(tr))));
    static_assert(!noexcept(ex::connect(s, tr))); // auto(rcvr) copies
  }
  // A stop requested after start but before the work runs: set_stopped.
  {
    ex::run_loop loop;
    std::inplace_stop_source src;
    record<int> stopped_rec, value_rec;
    auto op1 = ex::connect(ex::schedule(loop.get_scheduler()),
                           receiver_for(stopped_rec, stoppable_env(std::get_stop_token, src.get_token())));
    std::inplace_stop_source other;
    auto op2 = ex::connect(ex::schedule(loop.get_scheduler()),
                           receiver_for(value_rec, stoppable_env(std::get_stop_token, other.get_token())));
    ex::start(op1);
    ex::start(op2);
    CHECK(stopped_rec.how == done::none && value_rec.how == done::none); // start only enqueues
    src.request_stop();
    loop.finish();
    loop.run();
    CHECK(stopped_rec.how == done::stopped && stopped_rec.calls == 1);
    CHECK(value_rec.how == done::value && value_rec.calls == 1);
  }
  // finish() before run(): run executes the queue in FIFO order, then returns.
  {
    ex::run_loop loop;
    std::vector<int> log;
    auto sch = loop.get_scheduler();
    auto a = ex::connect(ex::schedule(sch), order_receiver{&log, 1});
    auto b = ex::connect(ex::schedule(sch), order_receiver{&log, 2});
    auto c = ex::connect(ex::schedule(sch), order_receiver{&log, 3});
    ex::start(b);
    ex::start(a);
    ex::start(c);
    loop.finish();
    loop.run();
    CHECK((log == std::vector<int>{2, 1, 3}));
  }
  // Schedulers compare equal iff from the same run_loop.
  {
    ex::run_loop l1, l2;
    CHECK(l1.get_scheduler() == l1.get_scheduler());
    CHECK(l1.get_scheduler() != l2.get_scheduler());
  }
}
