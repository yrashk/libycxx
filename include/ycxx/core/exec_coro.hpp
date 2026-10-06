// libycxx core: the coroutine utilities of [exec.coro.util]: as_awaitable ([exec.as.awaitable])
// and with_awaitable_senders ([exec.with.awaitable.senders]).
#pragma once

#include <ycxx/core/exec_run_loop.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// The environment of awaitable-receiver ([exec.as.awaitable]/4.4): the promise's, forwarding
// queries only.
template <class _Promise>
struct __exec_awaitable_env {
  std::coroutine_handle<_Promise> continuation;
  template <::__ycxx::__detail::__exec::__forwarding_query_c _Qp, class... _As>
    requires ::__ycxx::__detail::__exec::__callable<_Qp, std::execution::env_of_t<const _Promise&>, _As...>
  constexpr decltype(auto) query(_Qp __q, _As&&... __as) const noexcept {
    return __q(std::execution::get_env(std::as_const(continuation.promise())), static_cast<_As&&>(__as)...);
  }
};

// sender-awaitable<Sndr, Promise> ([exec.as.awaitable]/2)
template <class _Sndr, class _Promise>
class __exec_sender_awaitable {
  struct __unit {};
  using value_type = ::__ycxx::__detail::__exec::__single_sender_value_type<_Sndr, std::execution::env_of_t<_Promise>>;
  using result_type = std::conditional_t<std::is_void_v<value_type>, __unit, value_type>;
  using __variant_type = std::variant<std::monostate, result_type, std::exception_ptr>;

public:
  struct __awaitable_receiver {
    using receiver_concept = std::execution::receiver_tag;
    __variant_type* __result_ptr;
    std::coroutine_handle<_Promise> continuation;

    template <class... _Vs>
      requires std::constructible_from<result_type, _Vs...>
    void set_value(_Vs&&... __vs) && noexcept {
      if constexpr (std::is_nothrow_constructible_v<result_type, _Vs...> || !::__ycxx::__detail::__cfg::exceptions) {
        __result_ptr->template emplace<1>(static_cast<_Vs&&>(__vs)...);
      } else {
        try {
          __result_ptr->template emplace<1>(static_cast<_Vs&&>(__vs)...);
        } catch (...) {
          __result_ptr->template emplace<2>(std::current_exception());
        }
      }
      continuation.resume();
    }
    template <class _Err>
    void set_error(_Err&& __err) && noexcept {
      __result_ptr->template emplace<2>(::__ycxx::__detail::__exec::__as_except_ptr(static_cast<_Err&&>(__err)));
      continuation.resume();
    }
    void set_stopped() && noexcept { static_cast<std::coroutine_handle<>>(continuation.promise().unhandled_stopped()).resume(); }
    __exec_awaitable_env<_Promise> get_env() const noexcept { return {continuation}; }
  };

private:
  __variant_type result{};
  std::execution::connect_result_t<_Sndr, __awaitable_receiver> state;

public:
  __exec_sender_awaitable(_Sndr&& __sndr, _Promise& p)
      : state(std::execution::connect(static_cast<_Sndr&&>(__sndr),
                                      __awaitable_receiver{__builtin_addressof(result), std::coroutine_handle<_Promise>::from_promise(p)})) {}
  static constexpr bool await_ready() noexcept { return false; }
  void await_suspend(std::coroutine_handle<_Promise>) noexcept { std::execution::start(state); }
  value_type await_resume() {
    if (result.index() == 2)
      std::rethrow_exception(std::get<2>(result));
    if constexpr (!std::is_void_v<value_type>)
      return static_cast<value_type&&>(std::get<1>(result));
  }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// adapt-for-await-completion(s) ([exec.as.awaitable]/8)
template <class _Sp>
constexpr decltype(auto) __adapt_for_await_completion(_Sp&& s) {
  if constexpr (requires { std::execution::get_await_completion_adaptor(std::execution::get_env(s))(static_cast<_Sp&&>(s)); })
    return std::execution::get_await_completion_adaptor(std::execution::get_env(s))(static_cast<_Sp&&>(s));
  else
    return static_cast<_Sp&&>(s);
}

template <class _Expr, class _Promise>
concept __as_awaitable_sender = std::execution::sender_in<_Expr, std::execution::env_of_t<_Promise>> &&
                              requires { typename __single_sender_value_type<_Expr, std::execution::env_of_t<_Promise>>; };

template <class _Expr, class _Promise>
using __awaitable_sender_t = decltype(::__ycxx::__detail::__exec::__adapt_for_await_completion(
    std::execution::transform_sender(std::declval<_Expr>(), std::execution::get_env(std::declval<_Promise&>()))));

// [exec.as.awaitable]/7.2: the transformed sender's member as_awaitable(p); checked only for a
// sender (a concept, so that transform_sender is not instantiated for another expression).
template <class _Expr, class _Promise>
concept __as_awaitable_transformed_member =
    __as_awaitable_sender<_Expr, _Promise> && requires(_Expr&& __e, _Promise& p) {
      ::__ycxx::__detail::__exec::__adapt_for_await_completion(
          std::execution::transform_sender(static_cast<_Expr&&>(__e), std::execution::get_env(p)))
          .as_awaitable(p);
    };

// awaitable-sender<Sndr, Promise> ([exec.as.awaitable]/1)
template <class _Sndr, class _Promise>
concept __awaitable_sender =
    __single_sender<_Sndr, std::execution::env_of_t<_Promise>> &&
    __sender_to<_Sndr, typename ::__ycxx::__adl_free::__exec_sender_awaitable<_Sndr, _Promise>::__awaitable_receiver> &&
    requires(_Promise& p) {
      { p.unhandled_stopped() } -> std::convertible_to<std::coroutine_handle<>>;
    };

// [exec.as.awaitable]/7.4: a sender whose adapted, transformed form is an awaitable-sender (a
// concept: the transformation is only formed for a sender).
template <class _Expr, class _Promise>
concept __as_awaitable_via_sender_awaitable =
    __as_awaitable_sender<_Expr, _Promise> && __awaitable_sender<__awaitable_sender_t<_Expr, _Promise>, _Promise>;
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

// [exec.as.awaitable]
struct as_awaitable_t {
  template <class _Expr, class _Promise>
  constexpr decltype(auto) operator()(_Expr&& __expr, _Promise& p) const {
    using namespace __ycxx::__detail::__exec;
    if constexpr (requires { static_cast<_Expr&&>(__expr).as_awaitable(p); }) {
      static_assert(__is_awaitable<decltype(static_cast<_Expr&&>(__expr).as_awaitable(p)), _Promise>,
                    "as_awaitable: the as_awaitable member must return an awaitable");
      return static_cast<_Expr&&>(__expr).as_awaitable(p);
    } else if constexpr (__as_awaitable_transformed_member<_Expr, _Promise>) {
      return __adapt_for_await_completion(transform_sender(static_cast<_Expr&&>(__expr), get_env(p))).as_awaitable(p);
    } else if constexpr (__is_awaiter<decltype(__get_awaiter(declval<_Expr>(), declval<__none_such_promise&>())), _Promise>) {
      return static_cast<_Expr&&>(__expr);
    } else if constexpr (__as_awaitable_via_sender_awaitable<_Expr, _Promise>) {
      using _Sp = __awaitable_sender_t<_Expr, _Promise>;
      return __ycxx::__adl_free::__exec_sender_awaitable<_Sp, _Promise>(__adapt_for_await_completion(transform_sender(static_cast<_Expr&&>(__expr), get_env(p))),
                                                                p);
    } else {
      return static_cast<_Expr&&>(__expr);
    }
  }
};
inline constexpr as_awaitable_t as_awaitable{};

// [exec.with.awaitable.senders]
template <__ycxx::__detail::__exec::__class_type _Promise>
struct with_awaitable_senders {
  template <class _OtherPromise>
    requires(!same_as<_OtherPromise, void>)
  void set_continuation(coroutine_handle<_OtherPromise> h) noexcept {
    __continuation_ = h;
    if constexpr (requires(_OtherPromise& other) { other.unhandled_stopped(); }) {
      __stopped_handler_ = [](void* p) noexcept -> coroutine_handle<> {
        return coroutine_handle<_OtherPromise>::from_address(p).promise().unhandled_stopped();
      };
    } else {
      __stopped_handler_ = &__default_unhandled_stopped;
    }
  }
  coroutine_handle<> continuation() const noexcept { return __continuation_; }
  coroutine_handle<> unhandled_stopped() noexcept { return __stopped_handler_(__continuation_.address()); }
  template <class _Value>
  auto await_transform(_Value&& value) -> __ycxx::__detail::__exec::__call_result_t<as_awaitable_t, _Value, _Promise&> {
    return as_awaitable(static_cast<_Value&&>(value), static_cast<_Promise&>(*this));
  }

private:
  [[noreturn]] static coroutine_handle<> __default_unhandled_stopped(void*) noexcept { std::terminate(); }
  coroutine_handle<> __continuation_{};
  coroutine_handle<> (*__stopped_handler_)(void*) noexcept = &__default_unhandled_stopped;
};

}} // namespace std::execution
