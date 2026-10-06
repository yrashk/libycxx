// libycxx core: the coroutine utilities of [exec.coro.util]: as_awaitable ([exec.as.awaitable])
// and with_awaitable_senders ([exec.with.awaitable.senders]).
#pragma once

#include <ycxx/core/exec_run_loop.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// The environment of awaitable-receiver ([exec.as.awaitable]/4.4): the promise's, forwarding
// queries only.
template <class Promise>
struct exec_awaitable_env {
  std::coroutine_handle<Promise> continuation;
  template <::ycxx::detail::exec::forwarding_query_c Q, class... As>
    requires ::ycxx::detail::exec::callable<Q, std::execution::env_of_t<const Promise&>, As...>
  constexpr decltype(auto) query(Q q, As&&... as) const noexcept {
    return q(std::execution::get_env(std::as_const(continuation.promise())), static_cast<As&&>(as)...);
  }
};

// sender-awaitable<Sndr, Promise> ([exec.as.awaitable]/2)
template <class Sndr, class Promise>
class exec_sender_awaitable {
  struct unit {};
  using value_type = ::ycxx::detail::exec::single_sender_value_type<Sndr, std::execution::env_of_t<Promise>>;
  using result_type = std::conditional_t<std::is_void_v<value_type>, unit, value_type>;
  using variant_type = std::variant<std::monostate, result_type, std::exception_ptr>;

public:
  struct awaitable_receiver {
    using receiver_concept = std::execution::receiver_tag;
    variant_type* result_ptr;
    std::coroutine_handle<Promise> continuation;

    template <class... Vs>
      requires std::constructible_from<result_type, Vs...>
    void set_value(Vs&&... vs) && noexcept {
      if constexpr (std::is_nothrow_constructible_v<result_type, Vs...> || !::ycxx::detail::cfg::exceptions) {
        result_ptr->template emplace<1>(static_cast<Vs&&>(vs)...);
      } else {
        try {
          result_ptr->template emplace<1>(static_cast<Vs&&>(vs)...);
        } catch (...) {
          result_ptr->template emplace<2>(std::current_exception());
        }
      }
      continuation.resume();
    }
    template <class Err>
    void set_error(Err&& err) && noexcept {
      result_ptr->template emplace<2>(::ycxx::detail::exec::as_except_ptr(static_cast<Err&&>(err)));
      continuation.resume();
    }
    void set_stopped() && noexcept { static_cast<std::coroutine_handle<>>(continuation.promise().unhandled_stopped()).resume(); }
    exec_awaitable_env<Promise> get_env() const noexcept { return {continuation}; }
  };

private:
  variant_type result{};
  std::execution::connect_result_t<Sndr, awaitable_receiver> state;

public:
  exec_sender_awaitable(Sndr&& sndr, Promise& p)
      : state(std::execution::connect(static_cast<Sndr&&>(sndr),
                                      awaitable_receiver{__builtin_addressof(result), std::coroutine_handle<Promise>::from_promise(p)})) {}
  static constexpr bool await_ready() noexcept { return false; }
  void await_suspend(std::coroutine_handle<Promise>) noexcept { std::execution::start(state); }
  value_type await_resume() {
    if (result.index() == 2)
      std::rethrow_exception(std::get<2>(result));
    if constexpr (!std::is_void_v<value_type>)
      return static_cast<value_type&&>(std::get<1>(result));
  }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// adapt-for-await-completion(s) ([exec.as.awaitable]/8)
template <class S>
constexpr decltype(auto) adapt_for_await_completion(S&& s) {
  if constexpr (requires { std::execution::get_await_completion_adaptor(std::execution::get_env(s))(static_cast<S&&>(s)); })
    return std::execution::get_await_completion_adaptor(std::execution::get_env(s))(static_cast<S&&>(s));
  else
    return static_cast<S&&>(s);
}

template <class Expr, class Promise>
concept as_awaitable_sender = std::execution::sender_in<Expr, std::execution::env_of_t<Promise>> &&
                              requires { typename single_sender_value_type<Expr, std::execution::env_of_t<Promise>>; };

template <class Expr, class Promise>
using awaitable_sender_t = decltype(::ycxx::detail::exec::adapt_for_await_completion(
    std::execution::transform_sender(std::declval<Expr>(), std::execution::get_env(std::declval<Promise&>()))));

// awaitable-sender<Sndr, Promise> ([exec.as.awaitable]/1)
template <class Sndr, class Promise>
concept awaitable_sender =
    single_sender<Sndr, std::execution::env_of_t<Promise>> &&
    sender_to<Sndr, typename ::ycxx::adl_free::exec_sender_awaitable<Sndr, Promise>::awaitable_receiver> &&
    requires(Promise& p) {
      { p.unhandled_stopped() } -> std::convertible_to<std::coroutine_handle<>>;
    };
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {

// [exec.as.awaitable]
struct as_awaitable_t {
  template <class Expr, class Promise>
  constexpr decltype(auto) operator()(Expr&& expr, Promise& p) const {
    using namespace ycxx::detail::exec;
    if constexpr (requires { static_cast<Expr&&>(expr).as_awaitable(p); }) {
      static_assert(is_awaitable<decltype(static_cast<Expr&&>(expr).as_awaitable(p)), Promise>,
                    "as_awaitable: the as_awaitable member must return an awaitable");
      return static_cast<Expr&&>(expr).as_awaitable(p);
    } else if constexpr (as_awaitable_sender<Expr, Promise> &&
                         requires { adapt_for_await_completion(transform_sender(static_cast<Expr&&>(expr), get_env(p))).as_awaitable(p); }) {
      return adapt_for_await_completion(transform_sender(static_cast<Expr&&>(expr), get_env(p))).as_awaitable(p);
    } else if constexpr (is_awaiter<decltype(get_awaiter(declval<Expr>(), declval<none_such_promise&>())), Promise>) {
      return static_cast<Expr&&>(expr);
    } else if constexpr (as_awaitable_sender<Expr, Promise> && awaitable_sender<awaitable_sender_t<Expr, Promise>, Promise>) {
      using S = awaitable_sender_t<Expr, Promise>;
      return ycxx::adl_free::exec_sender_awaitable<S, Promise>(adapt_for_await_completion(transform_sender(static_cast<Expr&&>(expr), get_env(p))),
                                                                p);
    } else {
      return static_cast<Expr&&>(expr);
    }
  }
};
inline constexpr as_awaitable_t as_awaitable{};

// [exec.with.awaitable.senders]
template <ycxx::detail::exec::class_type Promise>
struct with_awaitable_senders {
  template <class OtherPromise>
    requires(!same_as<OtherPromise, void>)
  void set_continuation(coroutine_handle<OtherPromise> h) noexcept {
    continuation_ = h;
    if constexpr (requires(OtherPromise& other) { other.unhandled_stopped(); }) {
      stopped_handler_ = [](void* p) noexcept -> coroutine_handle<> {
        return coroutine_handle<OtherPromise>::from_address(p).promise().unhandled_stopped();
      };
    } else {
      stopped_handler_ = &default_unhandled_stopped;
    }
  }
  coroutine_handle<> continuation() const noexcept { return continuation_; }
  coroutine_handle<> unhandled_stopped() noexcept { return stopped_handler_(continuation_.address()); }
  template <class Value>
  auto await_transform(Value&& value) -> ycxx::detail::exec::call_result_t<as_awaitable_t, Value, Promise&> {
    return as_awaitable(static_cast<Value&&>(value), static_cast<Promise&>(*this));
  }

private:
  [[noreturn]] static coroutine_handle<> default_unhandled_stopped(void*) noexcept { std::terminate(); }
  coroutine_handle<> continuation_{};
  coroutine_handle<> (*stopped_handler_)(void*) noexcept = &default_unhandled_stopped;
};

}} // namespace std::execution
