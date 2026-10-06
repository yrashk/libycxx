// [exec.as.awaitable]: the cases of as_awaitable(expr, p) and the behaviour of sender-awaitable.
//   /7.1: expr.as_awaitable(p) is used when it is well-formed;
//   /7.3: an awaitable that is not a sender is returned as it is;
//   /7.4, /8: a sender becomes a sender-awaitable over adapt-for-await-completion(sndr), which is
//     get_await_completion_adaptor(get_env(sndr))(sndr) when the sender's attributes provide one;
//   /4.1, /6: set_value resumes the coroutine with the value;
//   /4.2: set_error(err) rethrows AS-EXCEPT-PTR(err) from the co_await ([exec.general]/8: an
//     error_code becomes a system_error, another error object is thrown as it is);
//   /4.4: the awaitable-receiver's environment answers forwarding queries from the promise's
//     environment (a sender reading it with read_env sees the promise's value);
//   /2: await_ready() is false.
// REQUIRES: exceptions
#include <execution>
#include <coroutine>
#include <system_error>
#include <type_traits>
#include <utility>
#include "check.hpp"

namespace ex = std::execution;

struct depth_t : std::forwarding_query_t {
  template <class E>
    requires requires(const E& e, const depth_t& q) { e.query(q); }
  constexpr auto operator()(const E& e) const noexcept { // by value: the promise's environment
    return e.query(*this);                                // is a temporary (/4.4)
  }
};
inline constexpr depth_t depth{};

struct co_task {
  struct promise_type : ex::with_awaitable_senders<promise_type> {
    int value = -1;
    bool threw = false;
    co_task get_return_object() noexcept { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void return_value(int v) noexcept { value = v; }
    void unhandled_exception() noexcept { threw = true; }
    auto get_env() const noexcept { return ex::prop(depth, 42); }
  };
  std::coroutine_handle<promise_type> h;
  int result() const { return h.promise().value; }
  ~co_task() { h.destroy(); }
};
using P = co_task::promise_type;

// /7.1: an object with a member as_awaitable(p).
int member_calls = 0;
struct has_member {
  struct awaiter {
    bool await_ready() const noexcept { return true; }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    int await_resume() const noexcept { return 7; }
  };
  awaiter as_awaitable(P&) const {
    ++member_calls;
    return {};
  }
};
// /7.3: a plain awaitable.
struct plain_awaiter {
  bool await_ready() const noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) const noexcept {}
  int await_resume() const noexcept { return 11; }
};

// /8: a sender whose attributes name an await-completion adaptor.
int adaptor_calls = 0;
struct times_ten {
  template <class S>
  auto operator()(S&& s) const {
    ++adaptor_calls;
    return ex::then(ex::just(), [v = 3] { return v * 10; });
  }
};
struct adapted_sender {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int)>();
  }
  auto get_env() const noexcept { return ex::prop(ex::get_await_completion_adaptor, times_ten{}); }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    void start() & noexcept { ex::set_value(std::move(r), 3); }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r)};
  }
};

co_task member() { co_return co_await has_member{}; }
co_task plain() { co_return co_await plain_awaiter{}; }
co_task adapted() { co_return co_await adapted_sender{}; }
co_task from_env() { co_return co_await ex::read_env(depth); }
co_task error_code_error() {
  try {
    co_await ex::just_error(std::make_error_code(std::errc::timed_out));
  } catch (const std::system_error& e) {
    co_return e.code() == std::errc::timed_out ? 1 : 2;
  }
  co_return 0;
}
co_task int_error() {
  try {
    co_await ex::just_error(5);
  } catch (int e) {
    co_return e;
  }
  co_return 0;
}
co_task values() {
  int a = co_await ex::just(1);
  co_await ex::just();
  co_return a + co_await (ex::just(2) | ex::then([](int v) { return v * 2; }));
}

int main() {
  CHECK(member().result() == 7 && member_calls == 1);
  CHECK(plain().result() == 11);
  {
    co_task t = adapted();
    CHECK(adaptor_calls == 1 && t.result() == 30);
  }
  CHECK(from_env().result() == 42);
  CHECK(error_code_error().result() == 1);
  CHECK(int_error().result() == 5);
  CHECK(values().result() == 5);

  // /2: the sender-awaitable is not ready, and as_awaitable of a sender is not the sender.
  co_task probe = plain();
  P& p = probe.h.promise();
  auto aw = ex::as_awaitable(ex::just(1), p);
  static_assert(!std::is_same_v<decltype(aw), decltype(ex::just(1))>);
  CHECK(!aw.await_ready());
  // /7.3, /7.5: an awaitable, or something neither awaitable nor a sender, is returned as is.
  static_assert(std::is_same_v<decltype(ex::as_awaitable(plain_awaiter{}, p)), plain_awaiter&&>);
  static_assert(std::is_same_v<decltype(ex::as_awaitable(5, p)), int&&>);
  return 0;
}
