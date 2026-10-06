// [exec.as.awaitable]:
//   /2, /6 and [exec.snd.expos] single-sender-value-type: co_await of a sender gives the decayed
//     single value (2.1), nothing for no value datums or no value completion (2.2), a
//     decayed-tuple for several (2.3);
//   /4.1: an exception from storing the value (a throwing decay-copy) is stored and rethrown by
//     await_resume (/6);
//   /7.4: the awaited sender is transform_sender(expr, get_env(p)): the domain of the promise's
//     environment transforms it ([exec.snd.transform]);
//   /7.5: an expression that is neither awaitable nor a sender is returned unchanged;
//   /7: as_awaitable(expr, p) is ill-formed when p is not an lvalue.
// REQUIRES: exceptions
#include <execution>
#include <coroutine>
#include <exception>
#include <tuple>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace ex = std::execution;

// A dependent sender a domain turns into just(42).
struct dep_s {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    if constexpr (sizeof...(Env) == 0)
      return (throw ex::dependent_sender_error(), ex::completion_signatures<ex::set_value_t(int)>());
    else
      return ex::completion_signatures<ex::set_value_t(int)>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    void start() & noexcept { ex::set_value(std::move(r), 0); }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r)};
  }
};
struct to_just_domain {
  template <class S, class E>
    requires std::is_same_v<std::remove_cvref_t<S>, dep_s>
  auto transform_sender(ex::start_t, S&&, const E&) const noexcept {
    return ex::just(42);
  }
};

struct co_task {
  struct promise_type : ex::with_awaitable_senders<promise_type> {
    bool done = false;
    std::exception_ptr error;
    co_task get_return_object() noexcept { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_void() noexcept { done = true; }
    void unhandled_exception() noexcept { error = std::current_exception(); }
    ex::env<ex::prop<ex::get_domain_t, to_just_domain>> get_env() const noexcept { return {ex::prop(ex::get_domain, to_just_domain())}; }
  };
  std::coroutine_handle<promise_type> h;
  ~co_task() {
    if (h)
      h.destroy();
  }
};
using P = co_task::promise_type;

bool copy_throws = false;
struct thrower {
  int v;
  thrower(int x) : v(x) {}
  thrower(const thrower& o) : v(o.v) {
    if (copy_throws)
      throw 42;
  }
};
thrower global_thrower{9};
struct ref_sender {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(thrower&)>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    void start() & noexcept { ex::set_value(std::move(r), global_thrower); }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r)};
  }
};

int results = 0;
co_task body() {
  auto t = co_await ex::just(1, 2.5);
  static_assert(std::is_same_v<decltype(t), std::tuple<int, double>>);
  CHECK(std::get<0>(t) == 1 && std::get<1>(t) == 2.5);
  static_assert(std::is_void_v<decltype(ex::as_awaitable(ex::just(), std::declval<P&>()).await_resume())>);
  co_await ex::just();
  const int c = 3;
  auto v = co_await (ex::just() | ex::then([&]() noexcept -> const int& { return c; }));
  static_assert(std::is_same_v<decltype(v), int>);
  CHECK(v == 3);
  auto th = co_await ref_sender{};
  CHECK(th.v == 9);
  // /7.4
  auto d = co_await dep_s{};
  CHECK(d == 42);
  ++results;
  // /4.1
  copy_throws = true;
  bool caught = false;
  try {
    (void)co_await ref_sender{};
  } catch (int e) {
    caught = e == 42;
  }
  copy_throws = false;
  CHECK(caught);
  // 2.2: no value completion at all: void, and the error is thrown.
  static_assert(std::is_void_v<decltype(ex::as_awaitable(ex::just_error(5), std::declval<P&>()).await_resume())>);
  caught = false;
  try {
    co_await ex::just_error(5);
  } catch (int e) {
    caught = e == 5;
  }
  CHECK(caught);
  ++results;
}

int main() {
  {
    auto t = body();
    CHECK(t.h.promise().done && !t.h.promise().error && results == 2);
  }
  // /7.5 and /7
  {
    auto t = body();
    P& p = t.h.promise();
    CHECK(ex::as_awaitable(7, p) == 7);
    int x = 8;
    CHECK(ex::as_awaitable(x, p) == 8);
    static_assert(std::is_invocable_v<ex::as_awaitable_t, int, P&>);
    static_assert(!std::is_invocable_v<ex::as_awaitable_t, int, P&&>);
    static_assert(!std::is_invocable_v<ex::as_awaitable_t, int, const P&&>);
  }
  return 0;
}
