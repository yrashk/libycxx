// [exec.connect]: connecting an awaitable that is not a sender with a connect member.
//   /6.1: new_sndr.connect(rcvr) is used when it is well-formed, even for an awaitable;
//   /6.2, /5: otherwise connect-awaitable: co_await the awaitable, then set_value with its result
//     (none for void), set_error(exception_ptr) for an exception, and its completion
//     signatures are SET-VALUE-SIG(V), set_error_t(exception_ptr), set_stopped_t();
//   /3: the promise's get_env() is the receiver's environment, and its unhandled_stopped()
//     completes with set_stopped;
//   /3, [exec.awaitable]/5: the promise derives from with-await-transform, so an object with an
//     as_awaitable(promise) member is awaited through what that returns, and is a sender
//     ([exec.snd.concepts]: enable-sender through env-promise), a dependent one
//     ([exec.getcomplsigs]/3.3-3.4).
// REQUIRES: exceptions
#include <execution>
#include <coroutine>
#include <exception>
#include <type_traits>
#include <utility>
#include "check.hpp"
#include "exec_support.hpp"

namespace ex = std::execution;
using namespace exec_test;

// Ready immediately with a value.
struct ready_int {
  int v;
  bool await_ready() const noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) const noexcept {}
  int await_resume() const noexcept { return v; }
};
struct ready_void {
  bool await_ready() const noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) const noexcept {}
  void await_resume() const noexcept {}
};
struct throwing {
  bool await_ready() const noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) const noexcept {}
  int await_resume() const { throw 7; }
};
// Suspends and asks the promise to complete with stopped.
struct stops {
  bool await_ready() const noexcept { return false; }
  template <class P>
  void await_suspend(std::coroutine_handle<P> h) const noexcept {
    h.promise().unhandled_stopped().resume();
  }
  int await_resume() const noexcept { return 0; }
};

struct q_t {
  static constexpr bool query(std::forwarding_query_t) noexcept { return true; }
  template <class Env>
  constexpr auto operator()(const Env& e) const noexcept -> decltype(e.query(*this)) {
    return e.query(*this);
  }
};
inline constexpr q_t q{};

// Reads q from the promise's environment while suspended, then resumes with it.
struct reads_env {
  int got = -1;
  bool await_ready() const noexcept { return false; }
  template <class P>
  bool await_suspend(std::coroutine_handle<P> h) noexcept {
    if constexpr (requires { q(ex::get_env(h.promise())); })
      got = q(ex::get_env(h.promise()));
    return false; // resume at once
  }
  int await_resume() const noexcept { return got; }
};

// Not awaitable itself, but has as_awaitable(promise).
struct via_member {
  int v;
  template <class P>
  ready_int as_awaitable(P&) const noexcept {
    return {v};
  }
};

// A sender with a connect member that is also awaitable.
struct both {
  using sender_concept = ex::sender_tag;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(std::exception_ptr), ex::set_stopped_t()>();
  }
  template <class R>
  struct op {
    using operation_state_concept = ex::operation_state_tag;
    R r;
    void start() & noexcept { ex::set_value(std::move(r), 1); }
  };
  template <class R>
  op<R> connect(R r) const noexcept {
    return {std::move(r)};
  }
  bool await_ready() const noexcept { return true; }
  void await_suspend(std::coroutine_handle<>) const noexcept {}
  int await_resume() const noexcept { return 2; }
};

using EP = std::exception_ptr;

int main() {
  static_assert(ex::sender<ready_int> && ex::sender<ready_void> && ex::sender<stops> && ex::sender<via_member>);
  static_assert(same_sigs<ex::completion_signatures_of_t<ready_int>,
                          ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(EP), ex::set_stopped_t()>>);
  static_assert(same_sigs<ex::completion_signatures_of_t<ready_void>,
                          ex::completion_signatures<ex::set_value_t(), ex::set_error_t(EP), ex::set_stopped_t()>>);
  // Without an environment, via_member is not awaitable (GET-AWAITER(c) uses a promise without
  // await_transform, [exec.awaitable]/2), so it is a dependent sender ([exec.getcomplsigs]/3.4);
  // with one, env-promise<Env> awaits it through as_awaitable (/3.3).
  static_assert(ex::dependent_sender<via_member> && !ex::dependent_sender<ready_int>);
  static_assert(same_sigs<ex::completion_signatures_of_t<via_member, ex::env<>>,
                          ex::completion_signatures<ex::set_value_t(int), ex::set_error_t(EP), ex::set_stopped_t()>>);
  {
    record<EP, int> rec;
    run(ready_int{5}, receiver_for(rec));
    CHECK(rec.how == done::value && rec.calls == 1 && std::get<0>(*rec.values) == 5);
  }
  {
    record<EP> rec;
    run(ready_void{}, receiver_for(rec));
    CHECK(rec.how == done::value && rec.calls == 1);
  }
  {
    record<EP, int> rec;
    run(throwing{}, receiver_for(rec));
    CHECK(rec.how == done::error && rec.calls == 1);
    CHECK(throws_value([&] { std::rethrow_exception(*rec.error); }, 7));
  }
  {
    record<EP, int> rec;
    run(stops{}, receiver_for(rec));
    CHECK(rec.how == done::stopped && rec.calls == 1);
  }
  {
    record<EP, int> rec;
    run(reads_env{}, receiver_for(rec, ex::env{ex::prop(q, 77)}));
    CHECK(rec.how == done::value && std::get<0>(*rec.values) == 77);
  }
  {
    record<EP, int> rec;
    run(via_member{9}, receiver_for(rec));
    CHECK(rec.how == done::value && std::get<0>(*rec.values) == 9);
  }
  {
    record<EP, int> rec;
    run(both{}, receiver_for(rec));
    CHECK(rec.how == done::value && std::get<0>(*rec.values) == 1);
  }
  return 0;
}
