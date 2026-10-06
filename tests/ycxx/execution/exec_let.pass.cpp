// let_value, let_error, let_stopped ([exec.let]):
//   let-cpo(sndr, f) calls f with lvalues of the decay-copied result datums of the matching
//   completion, connects the sender f returns and starts it; its completions are the result's.
//   The other completions pass through. The datums stay alive while the second operation runs.
//   An exception (copying the datums, calling f, connecting) becomes set_error(exception_ptr).
//   let_stopped requires an f invocable with no arguments ([exec.let]/3).
//   let-env ([exec.let]/2): the second sender's receiver has the first sender's completion
//   scheduler as its start scheduler, if it has one.
// FLAGS: -pthread
// REQUIRES: exceptions
#include <execution>
#include <thread>
#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

int main() {
  {
    auto s = ex::just(3) | ex::let_value([](int& x) {
               x += 1; // an lvalue of the stored datum
               return ex::just(x, std::string(2, 'a'));
             });
    record<std::exception_ptr, int, std::string> r;
    run(s, receiver_for(r));
    CHECK(r.how == done::value && std::get<0>(*r.values) == 4 && std::get<1>(*r.values) == "aa");
    static_assert(std::is_same_v<ex::value_types_of_t<decltype(s)>, std::variant<std::tuple<int, std::string>>>);
  }
  // The returned sender may complete with an error or stop.
  {
    record<int> r;
    run(ex::just(1) | ex::let_value([](int) { return ex::just_error(42); }), receiver_for(r));
    CHECK(r.how == done::error && *r.error == 42);
    record<int> r2;
    run(ex::just() | ex::let_value([] { return ex::just_stopped(); }), receiver_for(r2));
    CHECK(r2.how == done::stopped);
  }
  // let_error / let_stopped
  {
    record<int, int> r;
    run(ex::just_error(5) | ex::let_error([](int e) { return ex::just(e * 2); }), receiver_for(r));
    CHECK(r.how == done::value && std::get<0>(*r.values) == 10);
    record<int, int> r2;
    run(ex::just_stopped() | ex::let_stopped([] { return ex::just(11); }), receiver_for(r2));
    CHECK(r2.how == done::value && std::get<0>(*r2.values) == 11);
    // Values pass through let_error.
    record<int, int> r3;
    run(ex::just(6) | ex::let_error([](auto) { return ex::just(0); }), receiver_for(r3));
    CHECK(r3.how == done::value && std::get<0>(*r3.values) == 6);
    static_assert(!std::is_invocable_v<ex::let_stopped_t, decltype(ex::just()), decltype([](int) { return ex::just(); })>);
  }
  // An exception from f is an error completion.
  {
    record<std::exception_ptr, int> r;
    run(ex::just(1) | ex::let_value([](int) -> decltype(ex::just(0)) { throw std::logic_error("no"); }), receiver_for(r));
    CHECK(r.how == done::error && r.error.has_value());
    bool caught = false;
    try {
      std::rethrow_exception(*r.error);
    } catch (const std::logic_error&) {
      caught = true;
    }
    CHECK(caught);
  }
  // Nested lets, run through sync_wait.
  {
    auto s = ex::just(1) | ex::let_value([](int a) {
               return ex::just(a + 1) | ex::let_value([a](int b) { return ex::just(a * 10 + b); });
             });
    auto v = std::this_thread::sync_wait(std::move(s));
    CHECK(v && std::get<0>(*v) == 12);
  }
  // let-env: the inner sender's environment has the completion scheduler of the outer sender as
  // its start scheduler.
  {
    ex::run_loop loop;
    std::thread worker([&] { loop.run(); });
    auto sch = loop.get_scheduler();
    auto s = ex::schedule(sch) | ex::let_value([] { return ex::read_env(ex::get_start_scheduler); });
    auto v = std::this_thread::sync_wait(std::move(s));
    loop.finish();
    worker.join();
    CHECK(v && std::get<0>(*v) == sch);
  }
  return 0;
}
