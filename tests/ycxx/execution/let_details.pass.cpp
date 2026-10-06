// [exec.let]:
//   /2.1, /8: the sender returned by the function is connected to receiver2, whose environment
//     answers first from let-env (SCHED-ENV of the child's completion scheduler for set-cpo:
//     get_start_scheduler is that scheduler), then forwards forwarding queries (only) to the
//     outer receiver's environment;
//   /10, /12: the result datums are decay-copied into the state and the function receives them as
//     lvalues (decay_t<Ts>&), which stay alive while the second operation runs; completions other
//     than set-cpo pass through unchanged; an exception from the function is set_error(
//     exception_ptr), and that signature is absent when nothing can throw;
//   /9: a function that does not return a sender, or whose sender has no completion signatures
//     in receiver2's environment (/9.4), makes the completion signatures invalid (sender_in is
//     false).
// REQUIRES: exceptions
#include <execution>
#include <exception>
#include <stdexcept>
#include <stop_token>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;
using exec_test::done;

struct local_q_t { // not a forwarding query
  template <class E>
    requires requires(const E& e, const local_q_t& q) { e.query(q); }
  constexpr auto operator()(const E& e) const noexcept { return e.query(*this); }
};
inline constexpr local_q_t local_q{};
struct fwd_q_t : std::forwarding_query_t {
  template <class E>
    requires requires(const E& e, const fwd_q_t& q) { e.query(q); }
  constexpr auto operator()(const E& e) const noexcept { return e.query(*this); }
};
inline constexpr fwd_q_t fwd_q{};

template <class S, class E>
concept reads = ex::sender_in<S, E>;

int main() {
  // /2.1, [exec.snd.expos]/10 (SCHED-ENV answers get_start_scheduler): get_start_scheduler
  // inside the function's sender is the child's completion scheduler, not the outer receiver's
  // start scheduler; get_scheduler (not in SCHED-ENV, forwarding) is the outer receiver's.
  {
    ex::run_loop a, b;
    auto s = ex::schedule(a.get_scheduler()) | ex::let_value([] {
               return ex::when_all(ex::read_env(ex::get_start_scheduler), ex::read_env(ex::get_scheduler));
             });
    using Sch = decltype(a.get_scheduler());
    exec_test::record<int, Sch, Sch> rec;
    auto env = ex::env{ex::prop(ex::get_start_scheduler, b.get_scheduler()), ex::prop(ex::get_scheduler, b.get_scheduler())};
    auto op = ex::connect(std::move(s), exec_test::receiver_for(rec, env));
    ex::start(op);
    a.finish();
    a.run();
    CHECK(rec.how == done::value && std::get<0>(*rec.values) == a.get_scheduler());
    CHECK(std::get<1>(*rec.values) == b.get_scheduler());
  }
  // /8.2: forwarding queries come from the outer receiver, the others do not.
  {
    auto env = ex::env{ex::prop(fwd_q, 7), ex::prop(local_q, 8)};
    exec_test::record<int, int> rec;
    exec_test::run(ex::just() | ex::let_value([] { return ex::read_env(fwd_q); }), exec_test::receiver_for(rec, env));
    CHECK(rec.how == done::value && std::get<0>(*rec.values) == 7);
    using L = decltype(ex::just() | ex::let_value([] { return ex::read_env(local_q); }));
    static_assert(!reads<L, decltype(env)>);
  }
  // /10: the function gets lvalues of the decayed datums, alive during the second operation.
  {
    const int* seen = nullptr;
    auto r = tt::sync_wait(ex::just(41) | ex::let_value([&](int& v) {
                             seen = &v;
                             return ex::just() | ex::then([&v] { return ++v; });
                           }));
    CHECK(r && std::get<0>(*r) == 42 && seen != nullptr);
  }
  // Other completions pass through.
  {
    exec_test::record<int, int> rec;
    exec_test::run(ex::just_error(3) | ex::let_value([] { return ex::just(0); }), exec_test::receiver_for(rec));
    CHECK(rec.how == done::error && *rec.error == 3);
    exec_test::record<int, int> rec2;
    exec_test::run(ex::just(4) | ex::let_error([](int) { return ex::just(0); }), exec_test::receiver_for(rec2));
    CHECK(rec2.how == done::value && std::get<0>(*rec2.values) == 4);
    exec_test::record<int, int> rec3;
    exec_test::run(ex::just_stopped() | ex::let_stopped([] { return ex::just(9); }), exec_test::receiver_for(rec3));
    CHECK(rec3.how == done::value && std::get<0>(*rec3.values) == 9);
  }
  // An exception from the function: set_error(exception_ptr).
  {
    auto s = ex::just(1) | ex::let_value([](int) -> decltype(ex::just(0)) { throw std::logic_error("f"); });
    static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(s), ex::env<>>,
                                 ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr)>>);
    exec_test::record<std::exception_ptr, int> rec;
    exec_test::run(std::move(s), exec_test::receiver_for(rec));
    CHECK(rec.how == done::error && *rec.error);
    // Nothing can throw: no set_error.
    auto n = ex::just(1) | ex::let_value([](int v) noexcept { return ex::just(v); });
    static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(n), ex::env<>>,
                                 ex::completion_signatures<ex::set_value_t(int)>>);
  }
  // /9: not a sender. (With a non-dependent child the adaptor itself is ill-formed,
  // [exec.snd.expos]/24.4; a dependent child defers the check to an environment.)
  {
    using Bad = decltype(ex::read_env(fwd_q) | ex::let_value([](int) { return 5; }));
    static_assert(ex::sender<Bad> && !ex::sender_in<Bad, ex::prop<fwd_q_t, int>>);
    using Good = decltype(ex::read_env(fwd_q) | ex::let_value([](int v) { return ex::just(v); }));
    static_assert(ex::sender_in<Good, ex::prop<fwd_q_t, int>>);
  }
  return 0;
}
