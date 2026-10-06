// libycxx core: sender adaptors of [exec.adapt] that need no synchronization: write_env,
// unstoppable, then/upon_error/upon_stopped, let_value/let_error/let_stopped, bulk/
// bulk_chunked/bulk_unchunked, into_variant, stopped_as_optional, stopped_as_error.
#pragma once

#include <ycxx/core/exec_factories.hpp>
#include <ycxx/core/execution_policy.hpp>
#include <ycxx/core/optional.hpp>

// ---------------------------------------------------------------------------------------------
// The attributes of an adaptor with one child ([exec.adapt.general]/3.2, [exec.snd.general]/3-4):
// the child's forwarding queries, and for each completion tag T the completion scheduler and
// domain derived from the child completions (tags Srcs<T>) whose agents complete the adaptor's
// T operations. A scheduler is reported only for a single source; a domain is the
// COMMON-DOMAIN of the sources'.
namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class Q>
inline constexpr bool is_completion_query = false;
template <class T>
inline constexpr bool is_completion_query<std::execution::get_completion_scheduler_t<T>> = true;
template <class T>
inline constexpr bool is_completion_query<std::execution::get_completion_domain_t<T>> = true;

template <class VS, class ES, class SS, class T>
using attr_sources_t = std::conditional_t<std::is_same_v<T, std::execution::set_value_t>, VS,
                                          std::conditional_t<std::is_same_v<T, std::execution::set_error_t>, ES, SS>>;

template <class A, class Srcs, class... Envs>
struct sources_domain {};
template <class A, class... Srcs, class... Envs>
  requires(sizeof...(Srcs) != 0 && (!std::is_void_v<compl_domain_t<Srcs, A, Envs...>> && ...))
struct sources_domain<A, tlist<Srcs...>, Envs...> {
  using type = common_domain_t<compl_domain_t<Srcs, A, Envs...>...>;
};

template <class Srcs>
struct single_source {};
template <class S>
struct single_source<tlist<S>> {
  using type = S;
};
template <class Srcs>
using single_source_t = typename single_source<Srcs>::type;

using values_only = tlist<std::execution::set_value_t>;
using errors_only = tlist<std::execution::set_error_t>;
using stopped_only = tlist<std::execution::set_stopped_t>;
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
template <class A, class VS, class ES, class SS>
struct exec_mapped_attrs {
  A ycxx_attrs;

  template <::ycxx::detail::exec::forwarding_query_c Q, class... As>
    requires(!::ycxx::detail::exec::is_completion_query<Q>) && ::ycxx::detail::exec::has_query<std::remove_cvref_t<A>, Q, As...>
  constexpr decltype(auto) query(Q q, As&&... as) const
      noexcept(noexcept(::ycxx::detail::exec::as_const_ref(ycxx_attrs).query(q, static_cast<As&&>(as)...))) {
    return ::ycxx::detail::exec::as_const_ref(ycxx_attrs).query(q, static_cast<As&&>(as)...);
  }
  template <class T, class... Envs, class S = ::ycxx::detail::exec::single_source_t<::ycxx::detail::exec::attr_sources_t<VS, ES, SS, T>>>
    requires requires(const std::remove_cvref_t<A>& a, const Envs&... e) { std::execution::get_completion_scheduler_t<S>{}(a, e...); }
  constexpr auto query(std::execution::get_completion_scheduler_t<T>, const Envs&... envs) const noexcept {
    return std::execution::get_completion_scheduler_t<S>{}(::ycxx::detail::exec::as_const_ref(ycxx_attrs), envs...);
  }
  template <class T, class... Envs>
    requires requires {
      typename ::ycxx::detail::exec::sources_domain<std::remove_cvref_t<A>, ::ycxx::detail::exec::attr_sources_t<VS, ES, SS, T>, Envs...>::type;
    }
  constexpr auto query(std::execution::get_completion_domain_t<T>, const Envs&...) const noexcept {
    return typename ::ycxx::detail::exec::sources_domain<std::remove_cvref_t<A>, ::ycxx::detail::exec::attr_sources_t<VS, ES, SS, T>,
                                                         Envs...>::type();
  }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class VS, class ES, class SS, class Child>
constexpr auto mapped_attrs(const Child& child) noexcept {
  using A = env_member_t<decltype(std::execution::get_env(child))>;
  return ::ycxx::adl_free::exec_mapped_attrs<A, VS, ES, SS>{std::execution::get_env(child)};
}

// An environment's stand-in receiver: what connect is asked about when only the environment is
// known (whether a connect can throw is a function of the environment, [exec.connect]/6).
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
template <class Env>
struct exec_probe_receiver {
  using receiver_concept = std::execution::receiver_tag;
  template <class... As>
  void set_value(As&&...) && noexcept;
  template <class E>
  void set_error(E&&) && noexcept;
  void set_stopped() && noexcept;
  Env get_env() const noexcept;
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class S, class Env>
consteval bool nothrow_connect_in() {
  if constexpr (requires { std::execution::connect(std::declval<S>(), std::declval<::ycxx::adl_free::exec_probe_receiver<Env>>()); })
    return noexcept(std::execution::connect(std::declval<S>(), std::declval<::ycxx::adl_free::exec_probe_receiver<Env>>()));
  else
    return false;
}
}}} // namespace ycxx::detail::exec

// ---------------------------------------------------------------------------------------------
// [exec.write.env], [exec.unstoppable]
namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
struct write_env_t {
  template <class Sndr, class Env>
    requires std::execution::sender<Sndr> && queryable<std::decay_t<Env>>
  constexpr auto operator()(Sndr&& sndr, Env&& env) const
      noexcept(noexcept(::ycxx::detail::exec::make_sender(write_env_t(), static_cast<Env&&>(env), static_cast<Sndr&&>(sndr)))) {
    return ::ycxx::detail::exec::make_sender(*this, static_cast<Env&&>(env), static_cast<Sndr&&>(sndr));
  }
};

template <>
struct impls_for<write_env_t> : default_impls {
  template <class State, class Env>
  static constexpr auto join_env(const State& state, Env&& env) noexcept {
    return ::ycxx::detail::exec::join_env(state, static_cast<Env&&>(env));
  }
  template <class Index, class State, class Rcvr>
  static constexpr auto get_env(Index, const State& state, const Rcvr& rcvr) noexcept {
    return join_env(state, ::ycxx::detail::exec::fwd_env(std::execution::get_env(rcvr)));
  }
  template <class Sndr, class... Env>
  using csigs = csigs_of_t<child_type<Sndr>, join_env_t<const std::decay_t<data_type<Sndr>>&, fwd_env_t<Env>>...>;
};

struct unstoppable_t {
  template <std::execution::sender Sndr>
  constexpr auto operator()(Sndr&& sndr) const
      noexcept(noexcept(write_env_t()(static_cast<Sndr&&>(sndr), std::execution::prop(std::get_stop_token, std::never_stop_token{})))) {
    return write_env_t()(static_cast<Sndr&&>(sndr), std::execution::prop(std::get_stop_token, std::never_stop_token{}));
  }
};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {
inline constexpr ycxx::detail::exec::write_env_t write_env{};
inline constexpr ycxx::detail::exec::unstoppable_t unstoppable{};
}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// [exec.then]
namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// The pipeable forms of a sender adaptor object taking a sender and N more arguments
// ([exec.adapt.obj]/5): adaptor(args...) binds args when Bindable<Args...> (the adaptor's
// requirements on them) holds.
template <class Self, std::size_t N, template <class...> class Bindable>
struct pipeable_adaptor {
  template <class... Args>
    requires(sizeof...(Args) == N) && Bindable<Args...>::value && (std::constructible_from<std::decay_t<Args>, Args> && ...)
  constexpr auto operator()(this const Self& self, Args&&... args) noexcept((std::is_nothrow_constructible_v<std::decay_t<Args>, Args> && ...)) {
    return ::ycxx::detail::exec::bind_closure(self, static_cast<Args&&>(args)...);
  }
};
template <class F>
struct bind_movable_value : std::bool_constant<movable_value<F>> {};

template <class SetTag, class F>
struct then_sig_map {
  template <class Sig>
  struct apply {
    using type = std::execution::completion_signatures<Sig>;
  };
  template <class... Ts>
  struct apply<SetTag(Ts...)> {
    static auto pick() {
      if constexpr (!std::is_invocable_v<F, Ts...>)
        return std::type_identity<invalid_sigs<function_not_invocable_with_these_arguments, F, Ts...>>{};
      else if constexpr (std::is_nothrow_invocable_v<F, Ts...>)
        return std::type_identity<std::execution::completion_signatures<set_value_sig_t<std::invoke_result_t<F, Ts...>>>>{};
      else
        return std::type_identity<std::execution::completion_signatures<set_value_sig_t<std::invoke_result_t<F, Ts...>>,
                                                                         std::execution::set_error_t(std::exception_ptr)>>{};
    }
    using type = typename decltype(pick())::type;
  };
  template <class Sig>
  using f = typename apply<Sig>::type;
};

template <class SetTag, class VS, class ES, class SS>
struct then_impls : default_impls {
  template <class Data, class Child>
  static constexpr auto get_attrs(const Data&, const Child& child) noexcept {
    return ::ycxx::detail::exec::mapped_attrs<VS, ES, SS>(child);
  }
  template <class Index, class Fn, class Rcvr, class Tag, class... Args>
    requires(!std::is_same_v<Tag, SetTag> && callable<Tag, Rcvr, Args...>) ||
            (std::is_same_v<Tag, SetTag> && std::is_invocable_v<Fn, Args...>)
  static constexpr void complete(Index, Fn& fn, Rcvr& rcvr, Tag, Args&&... args) noexcept {
    if constexpr (std::is_same_v<Tag, SetTag>) {
      ::ycxx::detail::exec::try_set_value(rcvr, [&]() noexcept(std::is_nothrow_invocable_v<Fn, Args...>) -> decltype(auto) {
        return std::invoke(static_cast<Fn&&>(fn), static_cast<Args&&>(args)...);
      });
    } else {
      Tag()(static_cast<Rcvr&&>(rcvr), static_cast<Args&&>(args)...);
    }
  }
  template <class Sndr, class... Env>
  using csigs = sigs_map_t<child_sigs_t<Sndr, Env...>, then_sig_map<SetTag, std::remove_cvref_t<data_type<Sndr>>>::template f>;
};

template <class Self>
struct then_adaptor : pipeable_adaptor<Self, 1, bind_movable_value> {
  using pipeable_adaptor<Self, 1, bind_movable_value>::operator();
  template <std::execution::sender Sndr, movable_value F>
  constexpr auto operator()(this const Self& self, Sndr&& sndr, F&& f) noexcept(
      noexcept(::ycxx::detail::exec::make_sender(self, static_cast<F&&>(f), static_cast<Sndr&&>(sndr)))) {
    return ::ycxx::detail::exec::make_sender(self, static_cast<F&&>(f), static_cast<Sndr&&>(sndr));
  }
};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {
struct then_t : ycxx::detail::exec::then_adaptor<then_t> {};
struct upon_error_t : ycxx::detail::exec::then_adaptor<upon_error_t> {};
struct upon_stopped_t : ycxx::detail::exec::then_adaptor<upon_stopped_t> {};
inline constexpr then_t then{};
inline constexpr upon_error_t upon_error{};
inline constexpr upon_stopped_t upon_stopped{};
}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
using std::execution::set_error_t;
using std::execution::set_stopped_t;
using std::execution::set_value_t;
template <>
struct impls_for<std::execution::then_t> : then_impls<set_value_t, values_only, tlist<set_error_t, set_value_t>, stopped_only> {};
template <>
struct impls_for<std::execution::upon_error_t>
    : then_impls<set_error_t, tlist<set_value_t, set_error_t>, errors_only, stopped_only> {};
template <>
struct impls_for<std::execution::upon_stopped_t>
    : then_impls<set_stopped_t, tlist<set_value_t, set_stopped_t>, tlist<set_error_t, set_stopped_t>, tlist<>> {};
}}} // namespace ycxx::detail::exec

// ---------------------------------------------------------------------------------------------
// [exec.into.variant]
namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class CS>
struct into_variant_sigs {
  using type = CS;
};
template <class... Sigs>
struct into_variant_sigs<std::execution::completion_signatures<Sigs...>> {
  using CS = std::execution::completion_signatures<Sigs...>;
  using V = gather_signatures<set_value_t, CS, decayed_tuple, variant_or_empty>;
  template <class Sig>
  using non_values = std::conditional_t<std::is_same_v<typename sig_tag<Sig>::type, set_value_t>, no_sigs,
                                        std::execution::completion_signatures<Sig>>;
  static auto pick() {
    if constexpr (!decay_copyable_sigs<CS>)
      return std::type_identity<invalid_sigs<result_datums_not_decay_copyable, CS>>{};
    else
      return std::type_identity<sigs_concat_t<std::execution::completion_signatures<set_value_t(V)>, non_values<Sigs>...,
                                              std::conditional_t<nothrow_decay_copy_sigs<CS>, no_sigs, eptr_sigs>>>{};
  }
  using type = typename decltype(pick())::type;
};

}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {
struct into_variant_t : sender_adaptor_closure<into_variant_t> {
  template <sender Sndr>
  constexpr auto operator()(Sndr&& sndr) const noexcept(is_nothrow_constructible_v<decay_t<Sndr>, Sndr>) {
    return ycxx::detail::exec::make_sender(*this, ycxx::detail::exec::empty_data(), static_cast<Sndr&&>(sndr));
  }
};
inline constexpr into_variant_t into_variant{};
}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <>
struct impls_for<std::execution::into_variant_t> : default_impls {
  template <class Data, class Child>
  static constexpr auto get_attrs(const Data&, const Child& child) noexcept {
    return ::ycxx::detail::exec::mapped_attrs<values_only, tlist<set_error_t, set_value_t>, stopped_only>(child);
  }
  template <class Sndr, class Rcvr>
  static constexpr auto get_state(Sndr&&, Rcvr&) noexcept {
    return std::type_identity<std::execution::value_types_of_t<child_type<Sndr>, fwd_env_t<std::execution::env_of_t<Rcvr>>>>{};
  }
  template <class Index, class State, class Rcvr, class Tag, class... Args>
  static constexpr void complete(Index, State, Rcvr& rcvr, Tag, Args&&... args) noexcept {
    if constexpr (std::is_same_v<Tag, set_value_t>) {
      using variant_type = typename State::type;
      ::ycxx::detail::exec::try_set_value(rcvr, [&]() noexcept(std::is_nothrow_constructible_v<decayed_tuple<Args...>, Args...>) {
        return variant_type(decayed_tuple<Args...>{static_cast<Args&&>(args)...});
      });
    } else {
      Tag()(static_cast<Rcvr&&>(rcvr), static_cast<Args&&>(args)...);
    }
  }
  template <class Sndr, class... Env>
  using csigs = typename into_variant_sigs<child_sigs_t<Sndr, Env...>>::type;
};
}}} // namespace ycxx::detail::exec

// ---------------------------------------------------------------------------------------------
// [exec.let]
namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// SCHED-ENV(sch) ([exec.snd.expos]/10)
template <class Sch>
struct exec_sched_env {
  Sch sch;
  constexpr Sch query(std::execution::get_start_scheduler_t) const noexcept { return sch; }
  constexpr auto query(std::execution::get_domain_t) const noexcept
    requires requires(const Sch& s) { s.query(std::execution::get_domain_t{}); }
  {
    return sch.query(std::execution::get_domain_t{});
  }
};
// let-data
template <class Sndr, class Fn>
struct exec_let_data {
  Sndr sndr;
  Fn fn;
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// let-env(sndr, env) ([exec.let]/2)
template <class SetTag, class Sndr, class Env>
constexpr auto let_env(const Sndr& sndr, const Env& env) noexcept {
  if constexpr (requires { std::execution::get_completion_scheduler<SetTag>(std::execution::get_env(sndr), ::ycxx::detail::exec::fwd_env(env)); })
    return ::ycxx::adl_free::exec_sched_env<decltype(std::execution::get_completion_scheduler<SetTag>(
        std::execution::get_env(sndr), ::ycxx::detail::exec::fwd_env(env)))>{
        std::execution::get_completion_scheduler<SetTag>(std::execution::get_env(sndr), ::ycxx::detail::exec::fwd_env(env))};
  else if constexpr (requires { std::execution::get_completion_domain<SetTag>(std::execution::get_env(sndr), ::ycxx::detail::exec::fwd_env(env)); })
    return std::execution::prop(std::execution::get_domain,
                                std::execution::get_completion_domain<SetTag>(std::execution::get_env(sndr), ::ycxx::detail::exec::fwd_env(env)));
  else
    return std::execution::env<>();
}
template <class SetTag, class Sndr, class Env>
using let_env_t = decltype(::ycxx::detail::exec::let_env<SetTag>(std::declval<const std::remove_cvref_t<Sndr>&>(), std::declval<const Env&>()));
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// receiver2 ([exec.let]/8)
template <class Rcvr, class Env>
struct exec_let_receiver2 {
  using receiver_concept = std::execution::receiver_tag;
  template <class... Args>
  constexpr void set_value(Args&&... args) && noexcept {
    std::execution::set_value(static_cast<Rcvr&&>(*rcvr), static_cast<Args&&>(args)...);
  }
  template <class Error>
  constexpr void set_error(Error&& err) && noexcept {
    std::execution::set_error(static_cast<Rcvr&&>(*rcvr), static_cast<Error&&>(err));
  }
  constexpr void set_stopped() && noexcept { std::execution::set_stopped(static_cast<Rcvr&&>(*rcvr)); }
  constexpr auto get_env() const noexcept {
    return ::ycxx::detail::exec::join_env(env, ::ycxx::detail::exec::fwd_env(std::execution::get_env(*rcvr)));
  }
  Rcvr* rcvr;
  Env env;
};

// The receiver of the first operation of a let state. It names the state by its parameters
// rather than its type: the state's operation variant holds the operation connected to it.
template <class State>
struct exec_let_receiver;
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class Cpo, class Sndr, class Fn, class Rcvr>
struct let_state_key {};

template <class Cpo, class Sndr, class Fn, class Rcvr>
struct let_types {
  using child_sigs = std::execution::completion_signatures_of_t<Sndr, fwd_env_t<std::execution::env_of_t<Rcvr>>>;
  using env_t = let_env_t<Cpo, Sndr, std::execution::env_of_t<Rcvr>>;
  using receiver2 = ::ycxx::adl_free::exec_let_receiver2<Rcvr, env_t>;
  using let_args = sigs_args_t<Cpo, child_sigs>;
  template <class Args>
  struct per_args;
  template <class... Ts>
  struct per_args<tlist<Ts...>> {
    using tuple_t = decayed_tuple<Ts...>;
    using sndr2 = std::invoke_result_t<Fn, std::decay_t<Ts>&...>;
    using op2 = std::execution::connect_result_t<sndr2, receiver2>;
  };
  template <class ArgLists>
  struct variants;
  template <class... ArgLists>
  struct variants<tlist<ArgLists...>> {
    using args_variant = apply_unique_t<std::variant, std::monostate, typename per_args<ArgLists>::tuple_t...>;
    using ops_variant =
        apply_unique_t<::ycxx::adl_free::exec_op_variant,
                       std::execution::connect_result_t<Sndr, ::ycxx::adl_free::exec_let_receiver<let_state_key<Cpo, Sndr, Fn, Rcvr>>>,
                       typename per_args<ArgLists>::op2...>;
  };
  using args_variant = typename variants<let_args>::args_variant;
  using ops_variant = typename variants<let_args>::ops_variant;
};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] ycxx { namespace adl_free {
// let-state ([exec.let]/10)
template <class Cpo, class Sndr, class Fn, class Rcvr>
struct exec_let_state {
  using types = ::ycxx::detail::exec::let_types<Cpo, Sndr, Fn, Rcvr>;
  using env_t = typename types::env_t;
  using receiver = exec_let_receiver<::ycxx::detail::exec::let_state_key<Cpo, Sndr, Fn, Rcvr>>;
  using op_t = std::execution::connect_result_t<Sndr, receiver>;

  Fn fn;
  env_t env;
  typename types::args_variant args;
  typename types::ops_variant ops;

  constexpr exec_let_state(Sndr&& sndr, Fn f, Rcvr& rcvr)
      : fn(static_cast<Fn&&>(f)), env(::ycxx::detail::exec::let_env<Cpo>(sndr, std::execution::get_env(rcvr))),
        ops() {
    ops.template emplace_from_fn<op_t>(
        [&]() { return std::execution::connect(static_cast<Sndr&&>(sndr), receiver{this, __builtin_addressof(rcvr)}); });
  }
  exec_let_state(exec_let_state&&) = delete;

  template <class Tag, class... Ts>
  constexpr void impl(Rcvr& rcvr, Tag tag, Ts&&... ts) noexcept {
    if constexpr (std::is_same_v<Tag, Cpo>) {
      using args_t = ::ycxx::detail::exec::decayed_tuple<Ts...>;
      using receiver_type = typename types::receiver2;
      using sender_type = std::invoke_result_t<Fn, std::decay_t<Ts>&...>;
      using op2_t = std::execution::connect_result_t<sender_type, receiver_type>;
      constexpr bool nothrow = std::is_nothrow_constructible_v<args_t, Ts...> && std::is_nothrow_invocable_v<Fn, std::decay_t<Ts>&...> &&
                               std::is_nothrow_invocable_v<std::execution::connect_t, sender_type, receiver_type>;
      auto body = [&]() noexcept(nothrow) {
        auto& tuple = args.template emplace<args_t>(static_cast<Ts&&>(ts)...);
        ops.reset();
        auto&& sndr = std::apply(static_cast<Fn&&>(fn), tuple);
        auto& op = ops.template emplace_from_fn<op2_t>(
            [&]() { return std::execution::connect(static_cast<sender_type&&>(sndr), receiver_type{__builtin_addressof(rcvr), env}); });
        std::execution::start(op);
      };
      if constexpr (nothrow || !::ycxx::detail::cfg::exceptions) {
        body();
      } else {
        try {
          body();
        } catch (...) {
          std::execution::set_error(static_cast<Rcvr&&>(rcvr), std::current_exception());
        }
      }
    } else {
      tag(static_cast<Rcvr&&>(rcvr), static_cast<Ts&&>(ts)...);
    }
  }
};

template <class Cpo, class Sndr, class Fn, class Rcvr>
struct exec_let_receiver<::ycxx::detail::exec::let_state_key<Cpo, Sndr, Fn, Rcvr>> {
  using receiver_concept = std::execution::receiver_tag;
  using state_t = exec_let_state<Cpo, Sndr, Fn, Rcvr>;
  void* state;
  Rcvr* rcvr;
  template <class... Args>
  constexpr void set_value(Args&&... args) && noexcept {
    static_cast<state_t*>(state)->impl(*rcvr, std::execution::set_value, static_cast<Args&&>(args)...);
  }
  template <class Error>
  constexpr void set_error(Error&& err) && noexcept {
    static_cast<state_t*>(state)->impl(*rcvr, std::execution::set_error, static_cast<Error&&>(err));
  }
  constexpr void set_stopped() && noexcept { static_cast<state_t*>(state)->impl(*rcvr, std::execution::set_stopped); }
  constexpr decltype(auto) get_env() const noexcept { return std::execution::get_env(*rcvr); }
};
}} // namespace ycxx::adl_free

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
// let-tag
template <class Cpo>
struct let_tag {};

// The completion signatures of let-cpo(sndr, fn) in Env... ([exec.let]/9).
template <class Cpo, class Child, class Fn, class... Env>
struct let_sigs {
  using CS = csigs_of_t<Child, fwd_env_t<Env>...>;
  template <class Sig>
  struct apply {
    using type = std::execution::completion_signatures<Sig>;
  };
  template <class... Ts>
  struct apply<Cpo(Ts...)> {
    static auto pick() {
      if constexpr (!(std::is_constructible_v<std::decay_t<Ts>, Ts> && ...))
        return std::type_identity<invalid_sigs<result_datums_not_decay_copyable, Cpo(Ts...)>>{};
      else if constexpr (!std::is_invocable_v<Fn, std::decay_t<Ts>&...>)
        return std::type_identity<invalid_sigs<function_not_invocable_with_these_arguments, Fn, std::decay_t<Ts>&...>>{};
      else {
        using S2 = std::invoke_result_t<Fn, std::decay_t<Ts>&...>;
        if constexpr (!std::execution::sender<S2>) {
          return std::type_identity<invalid_sigs<let_function_must_return_a_sender, Fn, S2>>{};
        } else {
          if constexpr (sizeof...(Env) == 0)
            return pick2<S2>();
          else
            return pick2<S2, join_env_t<const let_env_t<Cpo, Child, Env...[0]>&, fwd_env_t<Env...[0]>>>();
        }
      }
    }
    template <class S2, class... Env2>
    static auto pick2() {
      constexpr bool nothrow = (std::is_nothrow_constructible_v<std::decay_t<Ts>, Ts> && ...) &&
                               std::is_nothrow_invocable_v<Fn, std::decay_t<Ts>&...> &&
                               (::ycxx::detail::exec::nothrow_connect_in<S2, Env2>() && ...) && sizeof...(Env2) != 0;
      return std::type_identity<sigs_concat_t<csigs_of_t<S2, Env2...>, std::conditional_t<nothrow, no_sigs, eptr_sigs>>>{};
    }
    using type = typename decltype(pick())::type;
  };
  template <class Sig>
  using f = typename apply<Sig>::type;
  using type = sigs_map_t<CS, f>;
};

template <class Cpo>
struct let_impls_base : default_impls {
  template <class Data, class... Child>
  static constexpr auto get_attrs(const Data&, const Child&...) noexcept {
    return std::execution::env<>();
  }
};

// The let_value/let_error/let_stopped senders before their transformation into let-tag senders
// (which happens when they are connected).
template <class Cpo>
struct let_cpo_impls : let_impls_base<Cpo> {
  template <class Sndr, class... Env>
  using csigs = typename let_sigs<Cpo, child_type<Sndr>, std::remove_cvref_t<data_type<Sndr>>, Env...>::type;
};

template <class Cpo>
struct impls_for<let_tag<Cpo>> : let_impls_base<Cpo> {
  template <class Sndr, class Rcvr>
  static constexpr auto get_state(Sndr&& sndr, Rcvr& rcvr) {
    using data_t = std::remove_cvref_t<data_type<Sndr>>;
    using child_t = decltype(std::forward_like<Sndr>(std::declval<data_t&>().sndr));
    using fn_t = std::decay_t<decltype(std::declval<data_t&>().fn)>;
    auto&& data = static_cast<Sndr&&>(sndr).template get<1>();
    return ::ycxx::adl_free::exec_let_state<Cpo, child_t, fn_t, Rcvr>(std::forward_like<Sndr>(data.sndr), std::forward_like<Sndr>(data.fn), rcvr);
  }
  template <class State, class Rcvr>
  static constexpr void start(State& state, Rcvr&) noexcept {
    std::execution::start(state.ops.template get<typename State::op_t>());
  }
  template <class Sndr, class... Env>
  using csigs = typename let_sigs<Cpo, decltype(std::forward_like<Sndr>(std::declval<std::remove_cvref_t<data_type<Sndr>>&>().sndr)),
                                  std::decay_t<decltype(std::declval<std::remove_cvref_t<data_type<Sndr>>&>().fn)>, Env...>::type;
};

template <class Self, class SetTag>
struct let_adaptor : pipeable_adaptor<Self, 1, bind_movable_value> {
  using pipeable_adaptor<Self, 1, bind_movable_value>::operator();
  template <std::execution::sender Sndr, movable_value F>
    requires(!std::is_same_v<SetTag, set_stopped_t> || std::invocable<std::decay_t<F>>)
  constexpr auto operator()(this const Self& self, Sndr&& sndr, F&& f) noexcept(
      noexcept(::ycxx::detail::exec::make_sender(self, static_cast<F&&>(f), static_cast<Sndr&&>(sndr)))) {
    return ::ycxx::detail::exec::make_sender(self, static_cast<F&&>(f), static_cast<Sndr&&>(sndr));
  }
  // let-cpo.transform_sender ([exec.let]/6), on set_value like the other lowered adaptors.
  template <class Sndr, class Env>
    requires std::is_same_v<std::execution::tag_of_t<Sndr>, Self>
  static constexpr auto transform_sender(set_value_t, Sndr&& s, const Env&) {
    using child_t = std::decay_t<child_type<Sndr>>;
    using fn_t = std::decay_t<data_type<Sndr>>;
    return ::ycxx::detail::exec::make_sender(let_tag<SetTag>{}, ::ycxx::adl_free::exec_let_data<child_t, fn_t>{
                                                                    static_cast<Sndr&&>(s).template get<2>(), static_cast<Sndr&&>(s).template get<1>()});
  }
};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {
struct let_value_t : ycxx::detail::exec::let_adaptor<let_value_t, set_value_t> {};
struct let_error_t : ycxx::detail::exec::let_adaptor<let_error_t, set_error_t> {};
struct let_stopped_t : ycxx::detail::exec::let_adaptor<let_stopped_t, set_stopped_t> {};
inline constexpr let_value_t let_value{};
inline constexpr let_error_t let_error{};
inline constexpr let_stopped_t let_stopped{};
}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <>
struct impls_for<std::execution::let_value_t> : let_cpo_impls<set_value_t> {};
template <>
struct impls_for<std::execution::let_error_t> : let_cpo_impls<set_error_t> {};
template <>
struct impls_for<std::execution::let_stopped_t> : let_cpo_impls<set_stopped_t> {};
}}} // namespace ycxx::detail::exec

// ---------------------------------------------------------------------------------------------
// [exec.stopped.opt], [exec.stopped.err]
namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <class CS>
struct stopped_as_optional_sigs {
  using type = CS;
};
template <class Args>
struct optional_value;
template <class T>
struct optional_value<tlist<T>> {
  using type = std::decay_t<T>;
};
template <class T0, class T1, class... Ts>
struct optional_value<tlist<T0, T1, Ts...>> {
  using type = decayed_tuple<T0, T1, Ts...>;
};
template <class V, class Args>
inline constexpr bool nothrow_optional_from = false;
template <class V, class... Ts>
inline constexpr bool nothrow_optional_from<V, tlist<Ts...>> = std::is_nothrow_constructible_v<V, Ts...>;

template <class... Sigs>
struct stopped_as_optional_sigs<std::execution::completion_signatures<Sigs...>> {
  using CS = std::execution::completion_signatures<Sigs...>;
  template <class Sig>
  using errors = std::conditional_t<std::is_same_v<typename sig_tag<Sig>::type, set_error_t>,
                                    std::execution::completion_signatures<Sig>, no_sigs>;
  template <class ArgLists>
  struct pick;
  // Exactly one value completion, with at least one datum (single-sender-value-type not void).
  template <class Args>
    requires requires { typename optional_value<Args>::type; }
  struct pick<tlist<Args>> {
    using V = typename optional_value<Args>::type;
    using type = sigs_concat_t<std::execution::completion_signatures<set_value_t(std::optional<V>)>, errors<Sigs>...,
                               std::conditional_t<nothrow_optional_from<V, Args>, no_sigs, eptr_sigs>>;
  };
  template <class ArgLists>
  struct pick {
    using type = invalid_sigs<sender_has_not_exactly_one_value_completion, CS>;
  };
  using type = typename pick<sigs_args_t<set_value_t, CS>>::type;
};

template <class E, class CS>
struct stopped_as_error_sigs {
  using type = CS;
};
template <class E, class... Sigs>
struct stopped_as_error_sigs<E, std::execution::completion_signatures<Sigs...>> {
  template <class Sig>
  using f = std::conditional_t<std::is_same_v<Sig, set_stopped_t()>, std::execution::completion_signatures<set_error_t(E)>,
                               std::execution::completion_signatures<Sig>>;
  using type = sigs_concat_t<f<Sigs>...>;
};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {
struct stopped_as_optional_t : sender_adaptor_closure<stopped_as_optional_t> {
  template <sender Sndr>
  constexpr auto operator()(Sndr&& sndr) const noexcept(is_nothrow_constructible_v<decay_t<Sndr>, Sndr>) {
    return ycxx::detail::exec::make_sender(*this, ycxx::detail::exec::empty_data(), static_cast<Sndr&&>(sndr));
  }
  template <class Sndr, class Env>
    requires is_same_v<tag_of_t<Sndr>, stopped_as_optional_t>
  static constexpr auto transform_sender(set_value_t, Sndr&& sndr, const Env&) {
    using child_t = ycxx::detail::exec::child_type<Sndr>;
    if constexpr (!sender_in<child_t, ycxx::detail::exec::fwd_env_t<Env>>) {
      return ycxx::adl_free::exec_not_a_sender();
    } else if constexpr (is_void_v<ycxx::detail::exec::single_sender_value_or_void<child_t, ycxx::detail::exec::fwd_env_t<Env>>>) {
      return ycxx::adl_free::exec_not_a_sender();
    } else {
      using V = ycxx::detail::exec::single_sender_value_type<child_t, ycxx::detail::exec::fwd_env_t<Env>>;
      return let_stopped_t()(then_t()(static_cast<Sndr&&>(sndr).template get<2>(),
                                      []<class... Ts>(Ts&&... ts) noexcept(is_nothrow_constructible_v<V, Ts...>) {
                                        return optional<V>(in_place, static_cast<Ts&&>(ts)...);
                                      }),
                             []() noexcept { return just_t()(optional<V>()); });
    }
  }
};
struct stopped_as_error_t : ycxx::detail::exec::pipeable_adaptor<stopped_as_error_t, 1, ycxx::detail::exec::bind_movable_value> {
  using ycxx::detail::exec::pipeable_adaptor<stopped_as_error_t, 1, ycxx::detail::exec::bind_movable_value>::operator();
  template <sender Sndr, ycxx::detail::exec::movable_value Err>
  constexpr auto operator()(Sndr&& sndr, Err&& err) const
      noexcept(is_nothrow_constructible_v<decay_t<Sndr>, Sndr> && is_nothrow_constructible_v<decay_t<Err>, Err>) {
    return ycxx::detail::exec::make_sender(*this, static_cast<Err&&>(err), static_cast<Sndr&&>(sndr));
  }
  template <class Sndr, class Env>
    requires is_same_v<tag_of_t<Sndr>, stopped_as_error_t>
  static constexpr auto transform_sender(set_value_t, Sndr&& sndr, const Env&) {
    using E = decay_t<ycxx::detail::exec::data_type<Sndr>>;
    return let_stopped_t()(static_cast<Sndr&&>(sndr).template get<2>(),
                           [err = static_cast<Sndr&&>(sndr).template get<1>()]() mutable noexcept(is_nothrow_move_constructible_v<E>) {
                             return just_error_t()(static_cast<E&&>(err));
                           });
  }
};
inline constexpr stopped_as_optional_t stopped_as_optional{};
inline constexpr stopped_as_error_t stopped_as_error{};
}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <>
struct impls_for<std::execution::stopped_as_optional_t> : default_impls {
  template <class Data, class Child>
  static constexpr auto get_attrs(const Data&, const Child& child) noexcept {
    return ::ycxx::detail::exec::mapped_attrs<tlist<set_value_t, set_stopped_t>, errors_only, tlist<>>(child);
  }
  template <class Sndr, class... Env>
  using csigs = typename stopped_as_optional_sigs<child_sigs_t<Sndr, Env...>>::type;
};
template <>
struct impls_for<std::execution::stopped_as_error_t> : default_impls {
  template <class Data, class Child>
  static constexpr auto get_attrs(const Data&, const Child& child) noexcept {
    return ::ycxx::detail::exec::mapped_attrs<values_only, tlist<set_error_t, set_stopped_t>, tlist<>>(child);
  }
  template <class Sndr, class... Env>
  using csigs = typename stopped_as_error_sigs<std::decay_t<data_type<Sndr>>, child_sigs_t<Sndr, Env...>>::type;
};
}}} // namespace ycxx::detail::exec

// ---------------------------------------------------------------------------------------------
// [exec.bulk]
namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <bool Chunked, class F, class Shape>
struct bulk_sig_map {
  template <class Sig>
  struct apply {
    using type = std::execution::completion_signatures<Sig>;
  };
  template <class... Ts>
  struct apply<set_value_t(Ts...)> {
    static constexpr bool ok = Chunked ? std::is_invocable_v<F&, Shape, Shape, std::decay_t<Ts>&...> : std::is_invocable_v<F&, Shape, std::decay_t<Ts>&...>;
    static constexpr bool nothrow =
        Chunked ? std::is_nothrow_invocable_v<F&, Shape, Shape, std::decay_t<Ts>&...> : std::is_nothrow_invocable_v<F&, Shape, std::decay_t<Ts>&...>;
    using type = std::conditional_t<!ok, invalid_sigs<function_not_invocable_with_these_arguments, F, Shape, Ts...>,
                                    sigs_concat_t<std::execution::completion_signatures<set_value_t(Ts...)>,
                                                  std::conditional_t<nothrow, no_sigs, eptr_sigs>>>;
  };
  template <class Sig>
  using f = typename apply<Sig>::type;
};

template <bool Chunked, class F, class S, class... Args>
concept bulk_invocable = (Chunked && std::invocable<F&, S, S, Args&...>) || (!Chunked && std::invocable<F&, S, Args&...>);

// The function of bulk's bulk_chunked form ([exec.bulk]/4): f called for each index of a chunk.
template <class Shape, class Func>
struct bulk_chunk_fn {
  Func func;
  template <class... Vs>
    requires std::invocable<Func&, Shape, Vs&...>
  constexpr void operator()(Shape begin, Shape end, Vs&&... vs) noexcept(std::is_nothrow_invocable_v<Func&, Shape, Vs&...>) {
    while (begin != end)
      func(begin++, vs...);
  }
};

template <class Data>
struct bulk_data_types;
template <class Is, class P, class Shape, class Func>
struct bulk_data_types<::ycxx::adl_free::exec_product<Is, P, Shape, Func>> {
  using shape = Shape;
  using func = Func;
};

template <bool Chunked>
struct bulk_impls : default_impls {
  template <class Data, class Child>
  static constexpr auto get_attrs(const Data&, const Child& child) noexcept {
    return ::ycxx::detail::exec::mapped_attrs<values_only, tlist<set_error_t, set_value_t>, stopped_only>(child);
  }
  template <class Index, class State, class Rcvr, class Tag, class... Args>
    requires(!std::is_same_v<Tag, set_value_t>) ||
            bulk_invocable<Chunked, std::remove_reference_t<decltype(std::declval<State&>().template get<2>())>,
                           std::remove_cvref_t<decltype(std::declval<State&>().template get<1>())>, Args...>
  static constexpr void complete(Index, State& state, Rcvr& rcvr, Tag, Args&&... args) noexcept {
    if constexpr (std::is_same_v<Tag, set_value_t>) {
      auto& shape = state.template get<1>();
      auto& f = state.template get<2>();
      using S = std::remove_cvref_t<decltype(shape)>;
      if constexpr (Chunked) {
        constexpr bool nothrow = noexcept(f(S(shape), S(shape), args...));
        ::ycxx::detail::exec::try_eval(rcvr, [&]() noexcept(nothrow) {
          f(static_cast<S>(0), S(shape), args...);
          Tag()(static_cast<Rcvr&&>(rcvr), static_cast<Args&&>(args)...);
        });
      } else {
        constexpr bool nothrow = noexcept(f(S(shape), args...));
        ::ycxx::detail::exec::try_eval(rcvr, [&]() noexcept(nothrow) {
          for (S i = 0; i < shape; ++i)
            f(S(i), args...);
          Tag()(static_cast<Rcvr&&>(rcvr), static_cast<Args&&>(args)...);
        });
      }
    } else {
      Tag()(static_cast<Rcvr&&>(rcvr), static_cast<Args&&>(args)...);
    }
  }
  template <class Sndr, class... Env>
  using csigs = sigs_map_t<child_sigs_t<Sndr, Env...>,
                           bulk_sig_map<Chunked, typename bulk_data_types<std::remove_cvref_t<data_type<Sndr>>>::func,
                                        typename bulk_data_types<std::remove_cvref_t<data_type<Sndr>>>::shape>::template f>;
};

template <class Policy, class Shape, class F>
struct bind_bulk : std::bool_constant<std::is_execution_policy_v<std::remove_cvref_t<Policy>> && std::integral<std::decay_t<Shape>> &&
                                      std::copy_constructible<std::decay_t<F>>> {};

template <class Self>
struct bulk_adaptor : pipeable_adaptor<Self, 3, bind_bulk> {
  using pipeable_adaptor<Self, 3, bind_bulk>::operator();
  template <std::execution::sender Sndr, class Policy, std::integral Shape, class F>
    requires std::is_execution_policy_v<std::remove_cvref_t<Policy>> && std::copy_constructible<std::decay_t<F>>
  constexpr auto operator()(this const Self& self, Sndr&& sndr, Policy&& policy, Shape shape, F&& f) {
    using P = std::remove_cvref_t<Policy>;
    using PT = std::conditional_t<std::copy_constructible<P>, P, const P&>;
    return ::ycxx::detail::exec::make_sender(self, product_t<PT, Shape, std::decay_t<F>>{{policy}, {shape}, {static_cast<F&&>(f)}},
                                             static_cast<Sndr&&>(sndr));
  }
};
}}} // namespace ycxx::detail::exec

namespace [[gnu::visibility("hidden")]] std { namespace execution {
struct bulk_chunked_t : ycxx::detail::exec::bulk_adaptor<bulk_chunked_t> {};
struct bulk_unchunked_t : ycxx::detail::exec::bulk_adaptor<bulk_unchunked_t> {};
struct bulk_t : ycxx::detail::exec::bulk_adaptor<bulk_t> {
  // [exec.bulk]/4: bulk becomes bulk_chunked, each chunk a loop over its indices.
  template <class Sndr, class Env>
    requires is_same_v<tag_of_t<Sndr>, bulk_t>
  static constexpr auto transform_sender(set_value_t, Sndr&& sndr, const Env&) {
    auto&& data = static_cast<Sndr&&>(sndr).template get<1>();
    using Shape = remove_cvref_t<decltype(data.template get<1>())>;
    using Func = remove_cvref_t<decltype(data.template get<2>())>;
    auto new_f = ycxx::detail::exec::bulk_chunk_fn<Shape, Func>{static_cast<decltype(data)&&>(data).template get<2>()};
    return bulk_chunked_t()(static_cast<Sndr&&>(sndr).template get<2>(), data.template get<0>(), Shape(data.template get<1>()), std::move(new_f));
  }
};
inline constexpr bulk_t bulk{};
inline constexpr bulk_chunked_t bulk_chunked{};
inline constexpr bulk_unchunked_t bulk_unchunked{};
}} // namespace std::execution

namespace [[gnu::visibility("hidden")]] ycxx { namespace detail { namespace exec {
template <>
struct impls_for<std::execution::bulk_t> : bulk_impls<false> {};
template <>
struct impls_for<std::execution::bulk_chunked_t> : bulk_impls<true> {};
template <>
struct impls_for<std::execution::bulk_unchunked_t> : bulk_impls<false> {};
}}} // namespace ycxx::detail::exec
