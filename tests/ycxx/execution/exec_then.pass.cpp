// then, upon_error, upon_stopped ([exec.then]):
//   then(sndr, f) calls f with sndr's values and sends f's result as a value (no datum for a void
//   result); upon_error / upon_stopped do so for the error / stopped completion. The other
//   completions pass through unchanged. An exception from f becomes set_error(exception_ptr)
//   (TRY-SET-VALUE, [exec.snd.expos]/11), which is among the signatures only when f can throw
//   ([exec.adapt.general]/4 allows omitting it otherwise).
//   then-cpo(sndr, f) is ill-formed for a non-sender or an f that is not movable-value; an f
//   not invocable with the values makes a sender with no completion signatures (check-types).
//   Both call forms and the pipe form are equivalent ([exec.adapt.obj]).
// REQUIRES: exceptions
#include <execution>
#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

int main() {
  // Value channel.
  {
    auto s = ex::then(ex::just(2, 3), [](int a, int b) noexcept { return a * b; });
    static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(s)>, ex::completion_signatures<ex::set_value_t(int)>>);
    record<std::exception_ptr, int> r;
    run(s, receiver_for(r));
    CHECK(r.how == done::value && std::get<0>(*r.values) == 6);
  }
  // Pipe form, void result, throwing f adds set_error(exception_ptr).
  {
    int seen = 0;
    auto s = ex::just(4) | ex::then([&](int x) { seen = x; });
    static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(s)>,
                                 ex::completion_signatures<ex::set_value_t(), ex::set_error_t(std::exception_ptr)>>);
    record<std::exception_ptr> r;
    run(s, receiver_for(r));
    CHECK(r.how == done::value && seen == 4);
  }
  // A throw becomes an error completion.
  {
    record<std::exception_ptr, int> r;
    run(ex::just(1) | ex::then([](int) -> int { throw std::runtime_error("boom"); }), receiver_for(r));
    CHECK(r.how == done::error && r.error.has_value());
    try {
      std::rethrow_exception(*r.error);
    } catch (const std::runtime_error& e) {
      CHECK(std::string(e.what()) == "boom");
    }
  }
  // Other channels pass through then unchanged.
  {
    record<int, int> r;
    run(ex::just_error(7) | ex::then([](int x) noexcept { return x; }), receiver_for(r));
    CHECK(r.how == done::error && *r.error == 7);
    record<int, int> r2;
    run(ex::just_stopped() | ex::then([]() noexcept { return 1; }), receiver_for(r2));
    CHECK(r2.how == done::stopped);
    static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(ex::just_error(7) | ex::then([](int x) noexcept { return x; }))>,
                                 ex::completion_signatures<ex::set_error_t(int)>>);
  }
  // upon_error: error -> value; upon_stopped: stopped -> value.
  {
    record<int, std::string> r;
    run(ex::just_error(5) | ex::upon_error([](int e) { return std::to_string(e); }), receiver_for(r));
    CHECK(r.how == done::value && std::get<0>(*r.values) == "5");
    record<int, int> r2;
    run(ex::upon_stopped(ex::just_stopped(), []() noexcept { return 9; }), receiver_for(r2));
    CHECK(r2.how == done::value && std::get<0>(*r2.values) == 9);
    // Values pass through upon_error.
    record<int, int> r3;
    run(ex::just(8) | ex::upon_error([](auto) noexcept { return 0; }), receiver_for(r3));
    CHECK(r3.how == done::value && std::get<0>(*r3.values) == 8);
    using S = decltype(ex::just_stopped() | ex::upon_stopped([]() noexcept { return 1; }));
    static_assert(std::is_same_v<ex::completion_signatures_of_t<S>, ex::completion_signatures<ex::set_value_t(int)>>);
  }
  // f is called as an rvalue, once.
  {
    struct fn {
      int* calls;
      int operator()(int x) && noexcept {
        ++*calls;
        return x + 1;
      }
    };
    int calls = 0;
    record<int, int> r;
    run(ex::just(1) | ex::then(fn{&calls}), receiver_for(r));
    CHECK(calls == 1 && std::get<0>(*r.values) == 2);
  }
  // A function returning a reference sends the reference.
  {
    int v = 3;
    auto s = ex::just() | ex::then([&]() noexcept -> int& { return v; });
    static_assert(std::is_same_v<ex::completion_signatures_of_t<decltype(s)>, ex::completion_signatures<ex::set_value_t(int&)>>);
  }
  // Ill-formed uses: not a sender ([exec.then]/2). (An f not invocable with the values violates
  // make-sender's Mandates, a compile-time error: see exec_then_not_invocable.verify.cpp.)
  static_assert(!std::is_invocable_v<ex::then_t, int, decltype([] {})>);
  return 0;
}
