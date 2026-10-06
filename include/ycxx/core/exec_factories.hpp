// libycxx core: the sender factories of [exec.factories] (just, just_error, just_stopped,
// read_env; schedule is in exec_core.hpp) and execution::inline_scheduler ([exec.inline.scheduler]).
#pragma once

#include <ycxx/core/exec_basic.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {

// impls-for<just-cpo> ([exec.just]/2)
template <class SetTag>
struct just_impls : default_impls {
  static constexpr bool ycxx_completes_inline = true;
  template <class Data>
  static constexpr auto get_attrs(const Data&) noexcept {
    return inline_attrs<SetTag>();
  }
  template <class State, class Rcvr>
  static constexpr void start(State& state, Rcvr& rcvr) noexcept {
    state.apply([&](auto&... ts) noexcept { SetTag()(static_cast<Rcvr&&>(rcvr), static_cast<std::remove_reference_t<decltype(ts)>&&>(ts)...); });
  }
  template <class Data>
  struct sig;
  template <class Is, class... Ts>
  struct sig<::ycxx::adl_free::exec_product<Is, Ts...>> {
    using type = std::execution::completion_signatures<SetTag(Ts...)>;
  };
  template <class Sndr, class... Env>
  using csigs = typename sig<std::remove_cvref_t<data_type<Sndr>>>::type;
};

}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {
struct just_t;
struct just_error_t;
struct just_stopped_t;
}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <>
struct impls_for<std::execution::just_t> : just_impls<std::execution::set_value_t> {};
template <>
struct impls_for<std::execution::just_error_t> : just_impls<std::execution::set_error_t> {};
template <>
struct impls_for<std::execution::just_stopped_t> : just_impls<std::execution::set_stopped_t> {};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {

// [exec.just]
struct just_t {
  template <class... Ts>
    requires(ycxx::detail::exec::movable_value<Ts> && ...)
  constexpr auto operator()(Ts&&... ts) const noexcept((is_nothrow_constructible_v<decay_t<Ts>, Ts> && ...)) {
    return ycxx::detail::exec::make_sender(*this, ycxx::detail::exec::make_product(static_cast<Ts&&>(ts)...));
  }
};
struct just_error_t {
  template <class E>
    requires ycxx::detail::exec::movable_value<E>
  constexpr auto operator()(E&& e) const noexcept(is_nothrow_constructible_v<decay_t<E>, E>) {
    return ycxx::detail::exec::make_sender(*this, ycxx::detail::exec::make_product(static_cast<E&&>(e)));
  }
};
struct just_stopped_t {
  constexpr auto operator()() const noexcept { return ycxx::detail::exec::make_sender(*this, ycxx::detail::exec::make_product()); }
};
inline constexpr just_t just{};
inline constexpr just_error_t just_error{};
inline constexpr just_stopped_t just_stopped{};

}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// [exec.read.env]
struct read_env_t {
  template <class Q>
  constexpr auto operator()(Q q) const noexcept(std::is_nothrow_move_constructible_v<Q>) {
    return ::ycxx::detail::exec::make_sender(*this, static_cast<Q&&>(q));
  }
};

template <class Q, class Env>
struct read_env_sigs {
  static auto pick() {
    if constexpr (!requires(const Env& env) { Q()(env); })
      return std::type_identity<invalid_sigs<read_env_query_ill_formed_or_void, Q, Env>>{};
    else if constexpr (std::is_void_v<decltype(Q()(std::declval<const Env&>()))>)
      return std::type_identity<invalid_sigs<read_env_query_ill_formed_or_void, Q, Env>>{};
    else if constexpr (noexcept(std::declval<Q&>()(std::declval<const Env&>())))
      return std::type_identity<std::execution::completion_signatures<std::execution::set_value_t(decltype(Q()(std::declval<const Env&>())))>>{};
    else
      return std::type_identity<std::execution::completion_signatures<std::execution::set_value_t(decltype(Q()(std::declval<const Env&>()))),
                                                                       std::execution::set_error_t(std::exception_ptr)>>{};
  }
  using type = typename decltype(pick())::type;
};

template <>
struct impls_for<read_env_t> : default_impls {
  static constexpr bool ycxx_completes_inline = true;
  template <class Data>
  static constexpr auto get_attrs(const Data&) noexcept {
    return inline_attrs<std::execution::set_value_t>();
  }
  template <class Q, class Rcvr>
  static constexpr void start(Q query, Rcvr& rcvr) noexcept {
    ::ycxx::detail::exec::try_set_value(rcvr, [&]() noexcept(noexcept(query(std::execution::get_env(rcvr)))) -> decltype(auto) {
      return query(std::execution::get_env(rcvr));
    });
  }
  template <class Sndr, class... Env>
  struct sigs {
    using type = dependent_sigs;
  };
  template <class Sndr, class Env>
  struct sigs<Sndr, Env> {
    using type = typename read_env_sigs<std::decay_t<data_type<Sndr>>, Env>::type;
  };
  template <class Sndr, class... Env>
  using csigs = typename sigs<Sndr, Env...>::type;
};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {
inline constexpr ycxx::detail::exec::read_env_t read_env{};
}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// [exec.inline.scheduler]
namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
template <class Rcvr>
struct exec_inline_state {
  using operation_state_concept = std::execution::operation_state_tag;
  Rcvr rcvr;
  constexpr void start() & noexcept { std::execution::set_value(static_cast<Rcvr&&>(rcvr)); }
};
struct exec_inline_sender {
  using sender_concept = std::execution::sender_tag;
  template <class Self, class... Env>
  using ycxx_csigs = std::execution::completion_signatures<std::execution::set_value_t()>;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return std::execution::completion_signatures<std::execution::set_value_t()>();
  }
  constexpr auto get_env() const noexcept { return ::ycxx::detail::exec::inline_attrs<std::execution::set_value_t>(); }
  template <class Rcvr>
  constexpr exec_inline_state<std::remove_cvref_t<Rcvr>> connect(Rcvr&& rcvr) const
      noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<Rcvr>, Rcvr>) {
    return {static_cast<Rcvr&&>(rcvr)};
  }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std { namespace execution {
class inline_scheduler {
public:
  using scheduler_concept = scheduler_tag;
  // const (the draft's is not), so that a const inline_scheduler models scheduler too.
  constexpr ycxx::adl_free::exec_inline_sender schedule() const noexcept { return {}; }
  constexpr bool operator==(const inline_scheduler&) const noexcept = default;

  // sch.query(q, args...) is inline-attrs<set_value_t>().query(q, args...) ([exec.inline.scheduler]/1)
  template <class Q, class... As>
    requires requires(Q q, As&&... as) { ycxx::detail::exec::inline_attrs<set_value_t>().query(q, static_cast<As&&>(as)...); }
  constexpr auto query(Q q, As&&... as) const noexcept {
    return ycxx::detail::exec::inline_attrs<set_value_t>().query(q, static_cast<As&&>(as)...);
  }
  // Not in [exec.inline.scheduler], but the scheduler concept requires it (DECISIONS: <execution>).
  constexpr forward_progress_guarantee query(get_forward_progress_guarantee_t) const noexcept {
    return forward_progress_guarantee::weakly_parallel;
  }
};
}} // namespace std::execution
