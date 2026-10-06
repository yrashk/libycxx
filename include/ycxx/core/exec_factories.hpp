// libycxx core: the sender factories of [exec.factories] (just, just_error, just_stopped,
// read_env; schedule is in exec_core.hpp) and execution::inline_scheduler ([exec.inline.scheduler]).
#pragma once

#include <ycxx/core/exec_basic.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {

// impls-for<just-cpo> ([exec.just]/2)
template <class _SetTag>
struct __just_impls : __default_impls {
  static constexpr bool __ycxx_completes_inline = true;
  template <class _Data>
  static constexpr auto __get_attrs(const _Data&) noexcept {
    return __inline_attrs<_SetTag>();
  }
  template <class _State, class _Rcvr>
  static constexpr void start(_State& state, _Rcvr& __rcvr) noexcept {
    state.apply([&](auto&... __ts) noexcept { _SetTag()(static_cast<_Rcvr&&>(__rcvr), static_cast<std::remove_reference_t<decltype(__ts)>&&>(__ts)...); });
  }
  template <class _Data>
  struct __sig;
  template <class _Is, class... _Ts>
  struct __sig<::__ycxx::__adl_free::__exec_product<_Is, _Ts...>> {
    using type = std::execution::completion_signatures<_SetTag(_Ts...)>;
  };
  template <class _Sndr, class... _Env>
  using __csigs = typename __sig<std::remove_cvref_t<__data_type<_Sndr>>>::type;
};

}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
struct just_t;
struct just_error_t;
struct just_stopped_t;
}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <>
struct __impls_for<std::execution::just_t> : __just_impls<std::execution::set_value_t> {};
template <>
struct __impls_for<std::execution::just_error_t> : __just_impls<std::execution::set_error_t> {};
template <>
struct __impls_for<std::execution::just_stopped_t> : __just_impls<std::execution::set_stopped_t> {};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {

// [exec.just]
struct just_t {
  template <class... _Ts>
    requires(__ycxx::__detail::__exec::__movable_value<_Ts> && ...)
  constexpr auto operator()(_Ts&&... __ts) const noexcept((is_nothrow_constructible_v<decay_t<_Ts>, _Ts> && ...)) {
    return __ycxx::__detail::__exec::__make_sender(*this, __ycxx::__detail::__exec::__make_product(static_cast<_Ts&&>(__ts)...));
  }
};
struct just_error_t {
  template <class _Ep>
    requires __ycxx::__detail::__exec::__movable_value<_Ep>
  constexpr auto operator()(_Ep&& e) const noexcept(is_nothrow_constructible_v<decay_t<_Ep>, _Ep>) {
    return __ycxx::__detail::__exec::__make_sender(*this, __ycxx::__detail::__exec::__make_product(static_cast<_Ep&&>(e)));
  }
};
struct just_stopped_t {
  constexpr auto operator()() const noexcept { return __ycxx::__detail::__exec::__make_sender(*this, __ycxx::__detail::__exec::__make_product()); }
};
inline constexpr just_t just{};
inline constexpr just_error_t just_error{};
inline constexpr just_stopped_t just_stopped{};

}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// [exec.read.env]
struct __read_env_t {
  template <class _Qp>
  constexpr auto operator()(_Qp __q) const noexcept(std::is_nothrow_move_constructible_v<_Qp>) {
    return ::__ycxx::__detail::__exec::__make_sender(*this, static_cast<_Qp&&>(__q));
  }
};

template <class _Qp, class _Env>
struct __read_env_sigs {
  static auto __pick() {
    if constexpr (!requires(const _Env& env) { _Qp()(env); })
      return std::type_identity<__invalid_sigs<__read_env_query_ill_formed_or_void, _Qp, _Env>>{};
    else if constexpr (std::is_void_v<decltype(_Qp()(std::declval<const _Env&>()))>)
      return std::type_identity<__invalid_sigs<__read_env_query_ill_formed_or_void, _Qp, _Env>>{};
    else if constexpr (noexcept(std::declval<_Qp&>()(std::declval<const _Env&>())))
      return std::type_identity<std::execution::completion_signatures<std::execution::set_value_t(decltype(_Qp()(std::declval<const _Env&>())))>>{};
    else
      return std::type_identity<std::execution::completion_signatures<std::execution::set_value_t(decltype(_Qp()(std::declval<const _Env&>()))),
                                                                       std::execution::set_error_t(std::exception_ptr)>>{};
  }
  using type = typename decltype(__pick())::type;
};

template <>
struct __impls_for<__read_env_t> : __default_impls {
  static constexpr bool __ycxx_completes_inline = true;
  template <class _Data>
  static constexpr auto __get_attrs(const _Data&) noexcept {
    return __inline_attrs<std::execution::set_value_t>();
  }
  template <class _Qp, class _Rcvr>
  static constexpr void start(_Qp query, _Rcvr& __rcvr) noexcept {
    ::__ycxx::__detail::__exec::__try_set_value(__rcvr, [&]() noexcept(noexcept(query(std::execution::get_env(__rcvr)))) -> decltype(auto) {
      return query(std::execution::get_env(__rcvr));
    });
  }
  template <class _Sndr, class... _Env>
  struct __sigs {
    using type = __dependent_sigs;
  };
  template <class _Sndr, class _Env>
  struct __sigs<_Sndr, _Env> {
    using type = typename __read_env_sigs<std::decay_t<__data_type<_Sndr>>, _Env>::type;
  };
  template <class _Sndr, class... _Env>
  using __csigs = typename __sigs<_Sndr, _Env...>::type;
};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
inline constexpr __ycxx::__detail::__exec::__read_env_t read_env{};
}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// [exec.inline.scheduler]
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
template <class _Rcvr>
struct __exec_inline_state {
  using operation_state_concept = std::execution::operation_state_tag;
  _Rcvr __rcvr;
  constexpr void start() & noexcept { std::execution::set_value(static_cast<_Rcvr&&>(__rcvr)); }
};
struct __exec_inline_sender {
  using sender_concept = std::execution::sender_tag;
  template <class _Self, class... _Env>
  using __ycxx_csigs = std::execution::completion_signatures<std::execution::set_value_t()>;
  template <class _Self, class... _Env>
  static consteval auto get_completion_signatures() {
    return std::execution::completion_signatures<std::execution::set_value_t()>();
  }
  constexpr auto get_env() const noexcept { return ::__ycxx::__detail::__exec::__inline_attrs<std::execution::set_value_t>(); }
  template <class _Rcvr>
  constexpr __exec_inline_state<std::remove_cvref_t<_Rcvr>> connect(_Rcvr&& __rcvr) const
      noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<_Rcvr>, _Rcvr>) {
    return {static_cast<_Rcvr&&>(__rcvr)};
  }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
class inline_scheduler {
public:
  using scheduler_concept = scheduler_tag;
  // const (the draft's is not), so that a const inline_scheduler models scheduler too.
  constexpr __ycxx::__adl_free::__exec_inline_sender schedule() const noexcept { return {}; }
  constexpr bool operator==(const inline_scheduler&) const noexcept = default;

  // sch.query(q, args...) is inline-attrs<set_value_t>().query(q, args...) ([exec.inline.scheduler]/1)
  template <class _Qp, class... _As>
    requires requires(_Qp __q, _As&&... __as) { __ycxx::__detail::__exec::__inline_attrs<set_value_t>().query(__q, static_cast<_As&&>(__as)...); }
  constexpr auto query(_Qp __q, _As&&... __as) const noexcept {
    return __ycxx::__detail::__exec::__inline_attrs<set_value_t>().query(__q, static_cast<_As&&>(__as)...);
  }
  // Not in [exec.inline.scheduler], but the scheduler concept requires it (DECISIONS: <execution>).
  constexpr forward_progress_guarantee query(get_forward_progress_guarantee_t) const noexcept {
    return forward_progress_guarantee::weakly_parallel;
  }
};
}} // namespace std::execution
