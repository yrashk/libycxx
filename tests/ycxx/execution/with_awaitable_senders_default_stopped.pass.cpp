// [exec.with.awaitable.senders]/1: with_awaitable_senders' stopped-handler is initially
// default-unhandled-stopped, which calls terminate; /2: set_continuation with a promise that has
// no unhandled_stopped keeps (resets to) that default. So a coroutine whose awaited sender
// completes with set_stopped, with no continuation that handles stopped, ends the program
// through terminate ([exec.as.awaitable]: the awaitable calls the promise's unhandled_stopped).
// The handler is reached: continuation() is the handle given to set_continuation.
// EXPECT-TERMINATE: terminate handler called
#include <coroutine>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <execution>
#include "check.hpp"

namespace ex = std::execution;

struct plain_caller {
  struct promise_type {
    plain_caller get_return_object() noexcept { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() noexcept {}
  };
  std::coroutine_handle<promise_type> h;
};

struct co_task {
  struct promise_type : ex::with_awaitable_senders<promise_type> {
    co_task get_return_object() noexcept { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() noexcept { std::abort(); }
  };
  std::coroutine_handle<promise_type> h;
};

co_task body() {
  co_await ex::just_stopped();
  std::puts("resumed after stopped");
  std::exit(1);
}
plain_caller caller() { co_return; }

int main() {
  std::set_terminate([] {
    std::puts("terminate handler called");
    std::fflush(stdout);
    std::abort();
  });
  auto c = caller();
  auto t = body();
  t.h.promise().set_continuation(c.h);
  CHECK(t.h.promise().continuation() == c.h);
  t.h.resume();
  std::puts("returned");
  return 1;
}
