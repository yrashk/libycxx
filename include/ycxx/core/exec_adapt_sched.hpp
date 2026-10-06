// libycxx core: the sender adaptors of [exec.adapt] that move work between execution agents or
// combine operations: schedule_from, continues_on, starts_on, on, affine ([exec.affine]),
// when_all, when_all_with_variant, and the exposition-only stop-when ([exec.stop.when]).
#pragma once

#include <ycxx/core/exec_adapt.hpp>

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class Sch>
struct bind_scheduler : std::bool_constant<std::execution::scheduler<Sch>> {};
}}} // namespace ycxx::detail::exec

// ---------------------------------------------------------------------------------------------
// [exec.schedule.from], [exec.continues.on]
namespace [[gnu::visibility("hidden")]] std { namespace execution {
struct schedule_from_t {
  template <sender Sndr>
  constexpr auto operator()(Sndr&& sndr) const noexcept(is_nothrow_constructible_v<decay_t<Sndr>, Sndr>) {
    return ycxx::detail::exec::make_sender(*this, ycxx::detail::exec::empty_data(), static_cast<Sndr&&>(sndr));
  }
};
inline constexpr schedule_from_t schedule_from{};
struct continues_on_t;
}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {

// The completion signatures of schedule(sch) other than its value completion.
template <class Sch, class... Env>
struct schedule_non_value_sigs {
  using CS = csigs_of_t<std::execution::schedule_result_t<Sch>, fwd_env_t<Env>...>;
  template <class Sig>
  using f = std::conditional_t<std::is_same_v<typename sig_tag<Sig>::type, set_value_t>, no_sigs, std::execution::completion_signatures<Sig>>;
  using type = sigs_map_t<CS, f>;
};

template <class Sch, class ChildSigs, class... Env>
struct continues_on_sigs {
  static auto pick() {
    using SchSigs = csigs_of_t<std::execution::schedule_result_t<Sch>, fwd_env_t<Env>...>;
    if constexpr (!is_csigs<SchSigs>)
      return std::type_identity<SchSigs>{};
    else if constexpr (!is_csigs<ChildSigs>)
      return std::type_identity<ChildSigs>{};
    else if constexpr (!decay_copyable_sigs<ChildSigs>)
      return std::type_identity<invalid_sigs<result_datums_not_decay_copyable, ChildSigs>>{};
    else
      return std::type_identity<sigs_concat_t<sigs_map_t<ChildSigs, decayed_sig_t>,
                                              std::conditional_t<nothrow_decay_copy_sigs<ChildSigs>, no_sigs, eptr_sigs>,
                                              typename schedule_non_value_sigs<Sch, Env...>::type>>{};
  }
  using type = typename decltype(pick())::type;
};

template <class Sig>
struct as_tuple_of_sig;
template <class Tag, class... Args>
struct as_tuple_of_sig<Tag(Args...)> {
  using type = decayed_tuple<Tag, Args...>;
};
template <class CS>
struct continues_on_variant;
template <class... Sigs>
struct continues_on_variant<std::execution::completion_signatures<Sigs...>> {
  using type = std::conditional_t<
      (nothrow_decay_copy_sig<Sigs> && ...),
      apply_unique_t<std::variant, std::monostate, typename as_tuple_of_sig<Sigs>::type...>,
      apply_unique_t<std::variant, std::monostate, typename as_tuple_of_sig<Sigs>::type..., std::tuple<set_error_t, std::exception_ptr>>>;
};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// The state of continues_on ([exec.continues.on]/5): the child's result, kept until the schedule
// operation completes.
template <class Sch, class Child, class Rcvr>
struct exec_continues_on_state {
  using variant_t =
      typename ::ycxx::detail::exec::continues_on_variant<std::execution::completion_signatures_of_t<Child, ::ycxx::detail::exec::fwd_env_t<std::execution::env_of_t<Rcvr>>>>::type;

  Rcvr& rcvr;
  variant_t async_result;

  struct receiver_t {
    using receiver_concept = std::execution::receiver_tag;
    exec_continues_on_state* state;
    void set_value() && noexcept {
      std::visit(
          [this]<class Tuple>(Tuple& result) noexcept -> void {
            if constexpr (!std::is_same_v<std::monostate, Tuple>) {
              std::apply(
                  [this](auto& tag, auto&... args) noexcept {
                    tag(static_cast<Rcvr&&>(state->rcvr), static_cast<std::remove_reference_t<decltype(args)>&&>(args)...);
                  },
                  result);
            }
          },
          state->async_result);
    }
    template <class Error>
    void set_error(Error&& err) && noexcept {
      std::execution::set_error(static_cast<Rcvr&&>(state->rcvr), static_cast<Error&&>(err));
    }
    void set_stopped() && noexcept { std::execution::set_stopped(static_cast<Rcvr&&>(state->rcvr)); }
    decltype(auto) get_env() const noexcept { return ::ycxx::detail::exec::fwd_env(std::execution::get_env(state->rcvr)); }
  };
  using operation_t = std::execution::connect_result_t<std::execution::schedule_result_t<Sch&>, receiver_t>;

  operation_t op_state;

  explicit exec_continues_on_state(Sch& sch, Rcvr& r) noexcept(
      std::is_nothrow_invocable_v<std::execution::connect_t, std::execution::schedule_result_t<Sch&>, receiver_t>)
      : rcvr(r), op_state(std::execution::connect(std::execution::schedule(sch), receiver_t{this})) {}
  exec_continues_on_state(exec_continues_on_state&&) = delete;
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class Sch>
struct continues_on_attrs {
  Sch sch;
  template <class... Envs>
  constexpr auto query(std::execution::get_completion_scheduler_t<set_value_t>, const Envs&... envs) const noexcept {
    if constexpr (sizeof...(Envs) != 0)
      return std::execution::get_completion_scheduler<set_value_t>(sch, envs...);
    else
      return sch;
  }
  template <class... Envs>
    requires requires(const Sch& s, const Envs&... e) { std::execution::get_completion_domain<set_value_t>(s, e...); }
  constexpr auto query(std::execution::get_completion_domain_t<set_value_t>, const Envs&... envs) const noexcept {
    return std::execution::get_completion_domain<set_value_t>(sch, envs...);
  }
};

template <>
struct impls_for<std::execution::continues_on_t> : default_impls {
  template <class Sch, class Child>
  static constexpr auto get_attrs(const Sch& sch, const Child&) noexcept {
    return continues_on_attrs<Sch>{sch};
  }
  template <class Sndr, class Rcvr>
    requires std::execution::sender_in<child_type<Sndr>, fwd_env_t<std::execution::env_of_t<Rcvr>>>
  static constexpr auto get_state(Sndr&& sndr, Rcvr& rcvr) noexcept(
      std::is_nothrow_constructible_v<::ycxx::adl_free::exec_continues_on_state<std::decay_t<data_type<Sndr>>, child_type<Sndr>, Rcvr>,
                                      std::decay_t<data_type<Sndr>>&, Rcvr&>) {
    using sched_t = std::decay_t<data_type<Sndr>>;
    auto sch = static_cast<Sndr&&>(sndr).template get<1>();
    return ::ycxx::adl_free::exec_continues_on_state<sched_t, child_type<Sndr>, Rcvr>(sch, rcvr);
  }
  template <class Index, class State, class Rcvr, class Tag, class... Args>
  static constexpr void complete(Index, State& state, Rcvr&, Tag, Args&&... args) noexcept {
    using result_t = decayed_tuple<Tag, Args...>;
    constexpr bool nothrow = (std::is_nothrow_constructible_v<std::decay_t<Args>, Args> && ...);
    if constexpr (nothrow || !cfg::exceptions) {
      state.async_result.template emplace<result_t>(Tag(), static_cast<Args&&>(args)...);
    } else {
      try {
        state.async_result.template emplace<result_t>(Tag(), static_cast<Args&&>(args)...);
      } catch (...) {
        state.async_result.template emplace<std::tuple<set_error_t, std::exception_ptr>>(std::execution::set_error, std::current_exception());
      }
    }
    std::execution::start(state.op_state);
  }
  template <class Sndr, class... Env>
  using csigs = typename continues_on_sigs<std::decay_t<data_type<Sndr>>, child_sigs_t<Sndr, Env...>, Env...>::type;
};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {
struct continues_on_t : ycxx::detail::exec::pipeable_adaptor<continues_on_t, 1, ycxx::detail::exec::bind_scheduler> {
  using ycxx::detail::exec::pipeable_adaptor<continues_on_t, 1, ycxx::detail::exec::bind_scheduler>::operator();
  template <sender Sndr, scheduler Sch>
  constexpr auto operator()(Sndr&& sndr, Sch&& sch) const {
    return ycxx::detail::exec::make_sender(*this, static_cast<Sch&&>(sch), schedule_from(static_cast<Sndr&&>(sndr)));
  }
};
inline constexpr continues_on_t continues_on{};
}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// [exec.starts.on], [exec.on]
namespace [[gnu::visibility("hidden")]] std { namespace execution {
struct starts_on_t {
  template <scheduler Sch, sender Sndr>
  constexpr auto operator()(Sch&& sch, Sndr&& sndr) const {
    return ycxx::detail::exec::make_sender(*this, static_cast<Sch&&>(sch), static_cast<Sndr&&>(sndr));
  }
  template <class OutSndr, class Env>
    requires is_same_v<tag_of_t<OutSndr>, starts_on_t>
  static constexpr auto transform_sender(set_value_t, OutSndr&& out_sndr, const Env&) {
    using S = decay_t<ycxx::detail::exec::child_type<OutSndr>>;
    return let_value(continues_on(just(), static_cast<OutSndr&&>(out_sndr).template get<1>()),
                     [sndr = static_cast<OutSndr&&>(out_sndr).template get<2>()]() mutable noexcept(is_nothrow_move_constructible_v<S>) {
                       return static_cast<S&&>(sndr);
                     });
  }
};
inline constexpr starts_on_t starts_on{};
}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <>
struct impls_for<std::execution::starts_on_t> : default_impls {
  template <class Sndr, class... Env>
  using csigs = sigs_concat_t<child_sigs_t<Sndr, Env...>, typename schedule_non_value_sigs<std::decay_t<data_type<Sndr>>, Env...>::type>;
};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {
struct on_t {
  template <scheduler Sch, sender Sndr>
    requires(!ycxx::detail::exec::pipeable_closure<Sndr>)
  constexpr auto operator()(Sch&& sch, Sndr&& sndr) const {
    return ycxx::detail::exec::make_sender(*this, static_cast<Sch&&>(sch), static_cast<Sndr&&>(sndr));
  }
  template <sender Sndr, scheduler Sch, ycxx::detail::exec::pipeable_closure Closure>
  constexpr auto operator()(Sndr&& sndr, Sch&& sch, Closure&& closure) const {
    return ycxx::detail::exec::make_sender(*this, ycxx::detail::exec::make_product(static_cast<Sch&&>(sch), static_cast<Closure&&>(closure)),
                                           static_cast<Sndr&&>(sndr));
  }
  // on(sch, closure): the pipeable partial application of on(sndr, sch, closure).
  template <scheduler Sch, ycxx::detail::exec::pipeable_closure Closure>
    requires(!sender<Closure>)
  constexpr auto operator()(Sch&& sch, Closure&& closure) const {
    return ycxx::detail::exec::bind_closure(*this, static_cast<Sch&&>(sch), static_cast<Closure&&>(closure));
  }

  template <class OutSndr, class Env>
    requires is_same_v<tag_of_t<OutSndr>, on_t>
  static constexpr auto transform_sender(set_value_t, OutSndr&& out_sndr, const Env& env) {
    auto&& data = static_cast<OutSndr&&>(out_sndr).template get<1>();
    auto&& child = static_cast<OutSndr&&>(out_sndr).template get<2>();
    if constexpr (scheduler<decltype(data)>) {
      auto orig_sch = ycxx::detail::exec::call_with_default(get_start_scheduler, ycxx::adl_free::exec_not_a_scheduler(), env);
      return continues_on(starts_on(std::forward_like<OutSndr>(data), std::forward_like<OutSndr>(child)), static_cast<decltype(orig_sch)&&>(orig_sch));
    } else {
      auto orig_sch = ycxx::detail::exec::call_with_default(get_completion_scheduler<set_value_t>, ycxx::adl_free::exec_not_a_scheduler(),
                                                            get_env(child), env);
      return continues_on(std::forward_like<OutSndr>(data.template get<1>())(
                              continues_on(std::forward_like<OutSndr>(child), std::forward_like<OutSndr>(data.template get<0>()))),
                          static_cast<decltype(orig_sch)&&>(orig_sch));
    }
  }
};
inline constexpr on_t on{};
}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// on's completions depend on the scheduler the receiver's environment names; it is always
// transformed (to continues_on/starts_on) before it is connected.
template <>
struct impls_for<std::execution::on_t> : default_impls {
  template <class Sndr, class... Env>
  struct sigs {
    using type = dependent_sigs;
  };
  template <class Sndr, class Env>
  struct sigs<Sndr, Env> {
    using type = invalid_sigs<environment_has_no_start_scheduler, Sndr, Env>;
  };
  template <class Sndr, class... Env>
  using csigs = typename sigs<Sndr, Env...>::type;
};
}}} // namespace ycxx::detail::exec

// ---------------------------------------------------------------------------------------------
// [exec.affine]
namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// UNSTOPPABLE-SCHEDULER(sch) ([exec.affine]/4)
template <class Sch>
struct exec_unstoppable_scheduler {
  using scheduler_concept = std::execution::scheduler_tag;
  Sch sch;
  constexpr auto schedule() const noexcept(noexcept(std::execution::unstoppable(std::execution::schedule(sch)))) {
    return std::execution::unstoppable(std::execution::schedule(sch));
  }
  template <class Q, class... As>
    requires requires(const Sch& s, Q q, As&&... as) { s.query(q, static_cast<As&&>(as)...); }
  constexpr decltype(auto) query(Q q, As&&... as) const noexcept(noexcept(sch.query(q, static_cast<As&&>(as)...))) {
    return sch.query(q, static_cast<As&&>(as)...);
  }
  friend constexpr bool operator==(const exec_unstoppable_scheduler& a, const exec_unstoppable_scheduler& b) noexcept {
    return a.sch == b.sch;
  }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// infallible-scheduler<Sch, Env> ([exec.sched]/8)
template <class Sch, class Env>
concept infallible_scheduler =
    std::execution::scheduler<Sch> &&
    (std::same_as<std::execution::completion_signatures<set_value_t()>, csigs_of_t<decltype(std::execution::schedule(std::declval<Sch>())), Env>> ||
     (!std::unstoppable_token<std::stop_token_of_t<Env>> &&
      (std::same_as<std::execution::completion_signatures<set_value_t(), set_stopped_t()>,
                    csigs_of_t<decltype(std::execution::schedule(std::declval<Sch>())), Env>> ||
       std::same_as<std::execution::completion_signatures<set_stopped_t(), set_value_t()>,
                    csigs_of_t<decltype(std::execution::schedule(std::declval<Sch>())), Env>>)));

template <class Child>
concept has_affine_member = requires(Child&& c) { static_cast<Child&&>(c).affine(); };

template <class Sndr, class... Env>
struct affine_sigs {
  static auto pick() {
    using Child = child_type<Sndr>;
    if constexpr (has_affine_member<Child>) {
      return std::type_identity<csigs_of_t<decltype(std::declval<Child>().affine()), Env...>>{};
    } else if constexpr (sizeof...(Env) == 0) {
      return std::type_identity<dependent_sigs>{};
    } else if constexpr (!requires(const Env...[0]& e) { std::execution::get_start_scheduler(e); }) {
      return std::type_identity<invalid_sigs<environment_has_no_start_scheduler, Sndr, Env...>>{};
    } else {
      return std::type_identity<invalid_sigs<start_scheduler_is_not_infallible, Sndr, Env...>>{};
    }
  }
  using type = typename decltype(pick())::type;
};

}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {
struct affine_t : sender_adaptor_closure<affine_t> {
  template <sender Sndr>
  constexpr auto operator()(Sndr&& sndr) const {
    return ycxx::detail::exec::make_sender(*this, env<>(), static_cast<Sndr&&>(sndr));
  }
  // [exec.affine]/5 (a set_value transformation, like the other lowered adaptors). Without a
  // start scheduler that is infallible in the environment, the sender stays as it is and its
  // completion signatures report the error ([exec.affine]/7).
  template <class Sndr, class Env>
    requires is_same_v<tag_of_t<Sndr>, affine_t> &&
             (ycxx::detail::exec::has_affine_member<ycxx::detail::exec::child_type<Sndr>> ||
              requires(const Env& ev) {
                requires ycxx::detail::exec::infallible_scheduler<decltype(get_start_scheduler(ev)), Env>;
              })
  static constexpr auto transform_sender(set_value_t, Sndr&& sndr, const Env& ev) {
    if constexpr (ycxx::detail::exec::has_affine_member<ycxx::detail::exec::child_type<Sndr>>) {
      return static_cast<Sndr&&>(sndr).template get<2>().affine();
    } else {
      using S = decay_t<decltype(get_start_scheduler(ev))>;
      return continues_on(static_cast<Sndr&&>(sndr).template get<2>(), ycxx::adl_free::exec_unstoppable_scheduler<S>{get_start_scheduler(ev)});
    }
  }
};
inline constexpr affine_t affine{};
}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <>
struct impls_for<std::execution::affine_t> : default_impls {
  template <class Sndr, class... Env>
  using csigs = typename affine_sigs<Sndr, Env...>::type;
};
}}} // namespace ycxx::detail::exec

// ---------------------------------------------------------------------------------------------
// [exec.when.all]
namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// make-when-all-env(stop_src, env) ([exec.when.all]/5)
template <class Env>
struct exec_when_all_env {
  const std::inplace_stop_source* stop_src;
  Env env;
  std::inplace_stop_token query(std::get_stop_token_t) const noexcept { return stop_src->get_token(); }
  template <::ycxx::detail::exec::forwarding_query_c Q, class... As>
    requires(!std::is_same_v<Q, std::get_stop_token_t>) && ::ycxx::detail::exec::has_query<std::remove_cvref_t<Env>, Q, As...>
  constexpr decltype(auto) query(Q q, As&&... as) const
      noexcept(noexcept(::ycxx::detail::exec::as_const_ref(env).query(q, static_cast<As&&>(as)...))) {
    return ::ycxx::detail::exec::as_const_ref(env).query(q, static_cast<As&&>(as)...);
  }
};
// on-stop-request ([exec.snd.expos]/16)
struct exec_on_stop_request {
  std::inplace_stop_source& stop_src;
  void operator()() noexcept { stop_src.request_stop(); }
};
struct exec_none_such {};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class Env>
constexpr auto make_when_all_env(std::inplace_stop_source& stop_src, Env&& env) noexcept {
  return ::ycxx::adl_free::exec_when_all_env<env_member_t<Env>>{&stop_src, static_cast<Env&&>(env)};
}
template <class Env>
using when_all_env_t = decltype(::ycxx::detail::exec::make_when_all_env(std::declval<std::inplace_stop_source&>(), std::declval<Env>()));

// The completion signatures of when_all(sndrs...) ([exec.when.all]/9, /15).
template <class Sig>
struct when_all_error_sig {
  using type = no_sigs;
};
template <class E>
struct when_all_error_sig<set_error_t(E)> {
  using type = std::execution::completion_signatures<set_error_t(std::decay_t<E>)>;
};
template <class Sig>
using when_all_error_sig_t = typename when_all_error_sig<Sig>::type;
template <class ArgLists>
struct only_args {
  using type = tlist<>;
};
template <class Args>
struct only_args<tlist<Args>> {
  using type = Args;
};
template <class Values>
struct when_all_value_sig;
template <class... Vs>
struct when_all_value_sig<tlist<Vs...>> {
  using type = std::execution::completion_signatures<set_value_t(std::decay_t<Vs>...)>;
};

template <class... ChildSigs>
struct when_all_sigs {
  template <class CS>
  using non_values = sigs_map_t<CS, when_all_error_sig_t>;

  static auto pick() {
    if constexpr (!(is_csigs<ChildSigs> && ...)) {
      return std::type_identity<sigs_concat_t<ChildSigs...>>{};
    } else if constexpr (((sigs_count<set_value_t, ChildSigs> >= 2) || ...)) {
      return std::type_identity<invalid_sigs<when_all_child_has_more_than_one_value_completion, ChildSigs...>>{};
    } else if constexpr (!(decay_copyable_sigs<ChildSigs> && ...)) {
      return std::type_identity<invalid_sigs<result_datums_not_decay_copyable, ChildSigs...>>{};
    } else {
      constexpr bool all_values = ((sigs_count<set_value_t, ChildSigs> == 1) && ...);
      using value_sigs = std::conditional_t<
          all_values,
          typename when_all_value_sig<typename tlist_concat<typename only_args<sigs_args_t<set_value_t, ChildSigs>>::type...>::type>::type,
          no_sigs>;
      constexpr bool nothrow = (nothrow_decay_copy_sigs<ChildSigs> && ...);
      constexpr bool stopped = ((sigs_count<set_stopped_t, ChildSigs> != 0) || ...);
      return std::type_identity<sigs_concat_t<value_sigs, non_values<ChildSigs>..., std::conditional_t<nothrow, no_sigs, eptr_sigs>,
                                              std::conditional_t<stopped, std::execution::completion_signatures<set_stopped_t()>, no_sigs>>>{};
    }
  }
  using type = typename decltype(pick())::type;
};

// disposition ([exec.when.all]/11), as plain values: the __atomic builtins take no enumerations.
struct when_all_disposition {
  static constexpr unsigned char started = 0, error = 1, stopped = 2;
};

// values_tuple ([exec.when.all]/13): the values of every child as optionals, or tuple<> when a
// child has no single value completion.
template <class Env, class... Children>
struct when_all_values {
  using type = std::tuple<>;
};
template <class Env, class... Children>
  requires(requires { typename std::execution::value_types_of_t<Children, Env, decayed_tuple, std::optional>; } && ...)
struct when_all_values<Env, Children...> {
  using type = std::tuple<std::execution::value_types_of_t<Children, Env, decayed_tuple, std::optional>...>;
};

template <class Rcvr, class... Children>
struct when_all_types {
  using env_t = when_all_env_t<std::execution::env_of_t<Rcvr>>;
  using values_tuple = typename when_all_values<env_t, Children...>::type;
  using all_sigs = sigs_concat_t<std::execution::completion_signatures_of_t<Children, env_t>...>;
  using copy_fail = std::conditional_t<nothrow_decay_copy_sigs<all_sigs>, ::ycxx::adl_free::exec_none_such, std::exception_ptr>;
  template <class Args>
  struct decayed_error;
  template <class E>
  struct decayed_error<tlist<E>> {
    using type = std::decay_t<E>;
  };
  template <class Lists>
  struct errors_variant_of;
  template <class... Lists>
  struct errors_variant_of<tlist<Lists...>> {
    using type = apply_unique_t<std::variant, ::ycxx::adl_free::exec_none_such, copy_fail, typename decayed_error<Lists>::type...>;
  };
  using errors_variant = typename errors_variant_of<sigs_args_t<set_error_t, all_sigs>>::type;
  static constexpr bool sends_stopped = sigs_count<set_stopped_t, all_sigs> != 0;
  using stop_callback = std::stop_callback_for_t<std::stop_token_of_t<std::execution::env_of_t<Rcvr>>, ::ycxx::adl_free::exec_on_stop_request>;
};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
template <class Rcvr, class... Children>
struct exec_when_all_state {
  using types = ::ycxx::detail::exec::when_all_types<Rcvr, Children...>;
  using disposition = ::ycxx::detail::exec::when_all_disposition;

  std::size_t count = sizeof...(Children);
  std::inplace_stop_source stop_src{};
  unsigned char disp = disposition::started;
  typename types::errors_variant errors{};
  typename types::values_tuple values{};
  std::optional<typename types::stop_callback> on_stop{std::nullopt};

  exec_when_all_state() = default;
  exec_when_all_state(exec_when_all_state&&) = delete;

  void arrive(Rcvr& rcvr) noexcept {
    if (__atomic_sub_fetch(&count, 1, __ATOMIC_ACQ_REL) == 0)
      complete(rcvr);
  }
  void complete(Rcvr& rcvr) noexcept {
    const unsigned char d = __atomic_load_n(&disp, __ATOMIC_ACQUIRE);
    if (d == disposition::started) {
      if constexpr (!std::is_same_v<typename types::values_tuple, std::tuple<>>) {
        on_stop.reset();
        std::apply(
            [&](auto&... opts) noexcept {
              std::apply(
                  [&](auto&... t) noexcept {
                    std::execution::set_value(static_cast<Rcvr&&>(rcvr), static_cast<std::remove_reference_t<decltype(t)>&&>(t)...);
                  },
                         std::tuple_cat(std::apply([](auto&... v) noexcept { return std::tuple<decltype(v)&...>(v...); }, *opts)...));
            },
            values);
      }
    } else if (d == disposition::error) {
      on_stop.reset();
      std::visit(
          [&]<class Error>(Error& error) noexcept {
            if constexpr (!std::is_same_v<Error, exec_none_such>)
              std::execution::set_error(static_cast<Rcvr&&>(rcvr), static_cast<Error&&>(error));
          },
          errors);
    } else {
      if constexpr (types::sends_stopped) {
        on_stop.reset();
        std::execution::set_stopped(static_cast<Rcvr&&>(rcvr));
      }
    }
  }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] std { namespace execution {
struct when_all_t;
struct when_all_with_variant_t;
}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class Rcvr>
struct when_all_make_state {
  template <class Tag, class Data, class... Children>
  auto operator()(Tag, Data, Children&&...) const {
    return ::ycxx::adl_free::exec_when_all_state<Rcvr, Children&&...>();
  }
};

template <>
struct impls_for<std::execution::when_all_t> : default_impls {
  template <class Index, class State, class Rcvr>
  static constexpr auto get_env(Index, State& state, const Rcvr& rcvr) noexcept {
    return ::ycxx::detail::exec::make_when_all_env(state.stop_src, std::execution::get_env(rcvr));
  }
  template <class Sndr, class Rcvr>
  static constexpr auto get_state(Sndr&& sndr, Rcvr&) noexcept(noexcept(static_cast<Sndr&&>(sndr).apply(when_all_make_state<Rcvr>()))) {
    return static_cast<Sndr&&>(sndr).apply(when_all_make_state<Rcvr>());
  }
  template <class State, class Rcvr, class... Ops>
  static constexpr void start(State& state, Rcvr& rcvr, Ops&... ops) noexcept {
    state.on_stop.emplace(std::get_stop_token(std::execution::get_env(rcvr)), ::ycxx::adl_free::exec_on_stop_request{state.stop_src});
    (std::execution::start(ops), ...);
  }
  template <class Index, class State, class Rcvr, class Set, class... Args>
  static constexpr void complete(Index, State& state, Rcvr& rcvr, Set, Args&&... args) noexcept {
    using disposition = when_all_disposition;
    if constexpr (std::is_same_v<Set, set_error_t>) {
      if (__atomic_exchange_n(&state.disp, disposition::error, __ATOMIC_ACQ_REL) != disposition::error) {
        state.stop_src.request_stop();
        using E = std::decay_t<Args...[0]>;
        if constexpr (std::is_nothrow_constructible_v<E, Args...> || !cfg::exceptions) {
          state.errors.template emplace<E>(static_cast<Args&&>(args)...);
        } else {
          try {
            state.errors.template emplace<E>(static_cast<Args&&>(args)...);
          } catch (...) {
            state.errors.template emplace<std::exception_ptr>(std::current_exception());
          }
        }
      }
    } else if constexpr (std::is_same_v<Set, set_stopped_t>) {
      unsigned char expected = disposition::started;
      if (__atomic_compare_exchange_n(&state.disp, &expected, disposition::stopped, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
        state.stop_src.request_stop();
    } else if constexpr (!std::is_same_v<decltype(State::values), std::tuple<>>) {
      if (__atomic_load_n(&state.disp, __ATOMIC_ACQUIRE) == disposition::started) {
        auto& opt = std::get<Index::value>(state.values);
        if constexpr (std::is_nothrow_constructible_v<decayed_tuple<Args...>, Args...> || !cfg::exceptions) {
          opt.emplace(static_cast<Args&&>(args)...);
        } else {
          try {
            opt.emplace(static_cast<Args&&>(args)...);
          } catch (...) {
            complete(Index(), state, rcvr, std::execution::set_error, std::current_exception());
            return;
          }
        }
      }
    }
    state.arrive(rcvr);
  }
  template <class Sndr, class Is, class... Env>
  struct sigs;
  template <class Sndr, std::size_t... Is, class... Env>
  struct sigs<Sndr, std::index_sequence<Is...>, Env...> {
    using type = typename when_all_sigs<csigs_of_t<child_type<Sndr, Is>, when_all_env_t<Env>...>...>::type;
  };
  template <class Sndr, class... Env>
  using csigs = typename sigs<Sndr, indices_for<Sndr>, Env...>::type;
};

template <>
struct impls_for<std::execution::when_all_with_variant_t> : default_impls {
  template <class Sndr, class Is, class... Env>
  struct sigs;
  template <class Sndr, std::size_t... Is, class... Env>
  struct sigs<Sndr, std::index_sequence<Is...>, Env...> {
    using type = typename impls_for<std::execution::when_all_t>::template csigs<
        basic_sender_t<std::execution::when_all_t, empty_data,
                       basic_sender_t<std::execution::into_variant_t, empty_data, std::decay_t<child_type<Sndr, Is>>>...>,
        Env...>;
  };
  template <class Sndr, class... Env>
  using csigs = typename sigs<Sndr, indices_for<Sndr>, Env...>::type;
};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {
struct when_all_t {
  template <sender... Sndrs>
    requires(sizeof...(Sndrs) != 0)
  constexpr auto operator()(Sndrs&&... sndrs) const noexcept((is_nothrow_constructible_v<decay_t<Sndrs>, Sndrs> && ...)) {
    return ycxx::detail::exec::make_sender(*this, ycxx::detail::exec::empty_data(), static_cast<Sndrs&&>(sndrs)...);
  }
};
inline constexpr when_all_t when_all{};

struct when_all_with_variant_t {
  template <sender... Sndrs>
    requires(sizeof...(Sndrs) != 0)
  constexpr auto operator()(Sndrs&&... sndrs) const noexcept((is_nothrow_constructible_v<decay_t<Sndrs>, Sndrs> && ...)) {
    return ycxx::detail::exec::make_sender(*this, ycxx::detail::exec::empty_data(), static_cast<Sndrs&&>(sndrs)...);
  }
  template <class Sndr, class Env>
    requires is_same_v<tag_of_t<Sndr>, when_all_with_variant_t>
  static constexpr auto transform_sender(set_value_t, Sndr&& sndr, const Env&) {
    return [&]<size_t... Is>(index_sequence<Is...>) {
      return when_all(into_variant(static_cast<Sndr&&>(sndr).template get<Is + 2>())...);
    }(ycxx::detail::exec::indices_for<Sndr>());
  }
};
inline constexpr when_all_with_variant_t when_all_with_variant{};
}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// stop-when(sndr, token) ([exec.stop.when])
namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// stoken-t: stop requested when either token's is, callbacks run on either's stop request.
template <class T1, class T2>
struct exec_either_stop_token {
  T1 t1;
  T2 t2;

  template <class Fn>
  struct callback_type {
    struct forward {
      callback_type* self;
      void operator()() noexcept {
        if (!__atomic_exchange_n(&self->fired, true, __ATOMIC_ACQ_REL))
          static_cast<Fn&&>(self->fn)();
      }
    };
    Fn fn;
    bool fired = false;
    std::stop_callback_for_t<T1, forward> cb1;
    std::stop_callback_for_t<T2, forward> cb2;

    template <class Init>
    callback_type(const exec_either_stop_token& tok, Init&& init) noexcept(std::is_nothrow_constructible_v<Fn, Init>)
        : fn(static_cast<Init&&>(init)), cb1(tok.t1, forward{this}), cb2(tok.t2, forward{this}) {}
    callback_type(callback_type&&) = delete;
  };

  bool stop_requested() const noexcept { return t1.stop_requested() || t2.stop_requested(); }
  bool stop_possible() const noexcept { return t1.stop_possible() || t2.stop_possible(); }
  bool operator==(const exec_either_stop_token&) const = default;
};

template <class Sndr, class Token>
struct exec_stop_when_sender {
  using sender_concept = std::execution::sender_tag;
  Sndr sndr;
  Token token;

  template <class Env>
  using stoken_for = std::conditional_t<std::unstoppable_token<std::stop_token_of_t<Env>>, Token,
                                        exec_either_stop_token<Token, std::stop_token_of_t<Env>>>;
  template <class Self, class Env>
  using inner_t = decltype(std::execution::write_env(std::declval<std::remove_cvref_t<Self>>().sndr,
                                                     std::execution::prop(std::get_stop_token, std::declval<stoken_for<Env>>())));
  template <class Self, class... Env>
  struct sigs {
    using type = ::ycxx::detail::exec::csigs_of_t<::ycxx::detail::forward_like_t<Self, Sndr>>;
  };
  template <class Self, class Env>
  struct sigs<Self, Env> {
    using type = ::ycxx::detail::exec::csigs_of_t<inner_t<Self, Env>, Env>;
  };
  template <class Self, class... Env>
  using ycxx_csigs = typename sigs<Self, Env...>::type;
  template <class Self, class... Env>
  static consteval auto get_completion_signatures() {
    return ::ycxx::detail::exec::checked_sigs<ycxx_csigs<Self, Env...>>();
  }

  decltype(auto) get_env() const noexcept { return ::ycxx::detail::exec::fwd_env(std::execution::get_env(sndr)); }

  template <::ycxx::detail::exec::decays_to<exec_stop_when_sender> Self, std::execution::receiver Rcvr>
  auto connect(this Self&& self, Rcvr rcvr) {
    auto rtoken = std::get_stop_token(std::execution::get_env(rcvr));
    if constexpr (std::unstoppable_token<decltype(rtoken)>) {
      return std::execution::connect(std::execution::write_env(std::forward_like<Self>(self.sndr),
                                                               std::execution::prop(std::get_stop_token, std::forward_like<Self>(self.token))),
                                     static_cast<Rcvr&&>(rcvr));
    } else {
      using stoken_t = exec_either_stop_token<Token, decltype(rtoken)>;
      return std::execution::connect(
          std::execution::write_env(std::forward_like<Self>(self.sndr),
                                    std::execution::prop(std::get_stop_token, stoken_t{std::forward_like<Self>(self.token), rtoken})),
          static_cast<Rcvr&&>(rcvr));
    }
  }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <std::execution::sender Sndr, std::stoppable_token Token>
constexpr auto stop_when(Sndr&& sndr, Token token) {
  if constexpr (std::unstoppable_token<Token>)
    return std::decay_t<Sndr>(static_cast<Sndr&&>(sndr));
  else
    return ::ycxx::adl_free::exec_stop_when_sender<std::decay_t<Sndr>, Token>{static_cast<Sndr&&>(sndr), static_cast<Token&&>(token)};
}
}}} // namespace ycxx::detail::exec
