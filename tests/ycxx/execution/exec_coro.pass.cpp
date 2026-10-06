// Coroutine utilities ([exec.coro.util]) and awaitables as senders ([exec.awaitable],
// [exec.connect]/5):
//   with_awaitable_senders<P> as a promise base makes senders awaitable: co_await sndr gives the
//   single value (void, a value, or a tuple of several), throws the error (as an exception), and
//   for stopped calls the promise's unhandled_stopped (its continuation's, through
//   set_continuation) instead of resuming.
//   as_awaitable(expr, p): a sender becomes an awaitable; an awaitable stays as it is.
//   An awaitable is a sender: its completions are SET-VALUE-SIG(await result), set_error(
//   exception_ptr), set_stopped().
// REQUIRES: exceptions
#include <execution>
#include <coroutine>
#include <exception>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include "check.hpp"

namespace ex = std::execution;
namespace tt = std::this_thread;

struct co_task {
  struct promise_type : ex::with_awaitable_senders<promise_type> {
    int value = -1;
    bool threw = false;
    co_task get_return_object() noexcept { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_value(int v) noexcept { value = v; }
    void unhandled_exception() noexcept { threw = true; }
  };
  std::coroutine_handle<promise_type> h;
};

// A parent coroutine whose promise handles stopped completions of the child.
struct parent_promise_probe {
  static inline bool stopped = false;
};
struct stop_handler {
  struct promise_type {
    stop_handler get_return_object() noexcept { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() noexcept {}
    std::coroutine_handle<> unhandled_stopped() noexcept {
      parent_promise_probe::stopped = true;
      return std::noop_coroutine();
    }
  };
  std::coroutine_handle<promise_type> h;
};

co_task values() {
  int a = co_await ex::just(20);
  co_await ex::just();
  auto [x, y] = co_await ex::just(1, 2);
  int b = co_await (ex::just(1) | ex::then([](int v) { return v + 1; }));
  co_return a + b + x + y;
}
co_task errors() {
  try {
    co_await ex::just_error(std::make_exception_ptr(std::runtime_error("e")));
  } catch (const std::runtime_error&) {
    co_return 1;
  }
  co_return 0;
}
co_task error_values() {
  try {
    co_await (ex::just_error(7) | ex::let_error([](int e) { return ex::just_error(e + 1); }));
  } catch (int e) {
    co_return e;
  }
  co_return 0;
}
struct lazy_task {
  struct promise_type : ex::with_awaitable_senders<promise_type> {
    lazy_task get_return_object() noexcept { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_always initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() noexcept {}
  };
  std::coroutine_handle<promise_type> h;
};
lazy_task lazy_stopped() {
  co_await ex::just_stopped();
}

// An awaitable (not a sender of its own).
struct ready_awaitable {
  bool await_ready() const noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) const noexcept {}
  int await_resume() const noexcept { return 13; }
};

int main() {
  {
    auto t = values();
    CHECK(t.h.done() && t.h.promise().value == 25);
    t.h.destroy();
  }
  {
    auto t = errors();
    CHECK(t.h.promise().value == 1);
    t.h.destroy();
  }
  {
    auto t = error_values();
    CHECK(t.h.promise().value == 8);
    t.h.destroy();
  }
  // Stopped: the coroutine is not resumed; the continuation's unhandled_stopped is called.
  {
    auto parent = []() -> stop_handler { co_return; }();
    auto child = lazy_stopped();
    child.h.promise().set_continuation(parent.h);
    CHECK(child.h.promise().continuation() == parent.h);
    child.h.resume();
    CHECK(parent_promise_probe::stopped);
    CHECK(!child.h.done());
    child.h.destroy();
    parent.h.destroy();
  }
  // An awaitable is a sender.
  {
    static_assert(ex::sender<ready_awaitable>);
    static_assert(std::is_same_v<ex::completion_signatures_of_t<ready_awaitable>,
                                 ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr), ex::set_stopped_t()>>);
    auto r = tt::sync_wait(ready_awaitable{} | ex::then([](int v) { return v + 1; }));
    CHECK(r && std::get<0>(*r) == 14);
  }
  // as_awaitable leaves an awaitable as it is.
  {
    co_task::promise_type p;
    ready_awaitable a;
    static_assert(std::is_same_v<decltype(ex::as_awaitable(a, p)), ready_awaitable&>);
  }
  return 0;
}
