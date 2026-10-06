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
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _Qp>
inline constexpr bool __is_completion_query = false;
template <class _Tp>
inline constexpr bool __is_completion_query<std::execution::get_completion_scheduler_t<_Tp>> = true;
template <class _Tp>
inline constexpr bool __is_completion_query<std::execution::get_completion_domain_t<_Tp>> = true;

template <class _VS, class _ES, class _SS, class _Tp>
using __attr_sources_t = std::conditional_t<std::is_same_v<_Tp, std::execution::set_value_t>, _VS,
                                          std::conditional_t<std::is_same_v<_Tp, std::execution::set_error_t>, _ES, _SS>>;

template <class _Ap, class _Srcs, class... _Envs>
struct __sources_domain {};
template <class _Ap, class... _Srcs, class... _Envs>
  requires(sizeof...(_Srcs) != 0 && (!std::is_void_v<__compl_domain_t<_Srcs, _Ap, _Envs...>> && ...))
struct __sources_domain<_Ap, __tlist<_Srcs...>, _Envs...> {
  using type = __common_domain_t<__compl_domain_t<_Srcs, _Ap, _Envs...>...>;
};

template <class _Srcs>
struct __single_source {};
template <class _Sp>
struct __single_source<__tlist<_Sp>> {
  using type = _Sp;
};
template <class _Srcs>
using __single_source_t = typename __single_source<_Srcs>::type;

using __values_only = __tlist<std::execution::set_value_t>;
using __errors_only = __tlist<std::execution::set_error_t>;
using __stopped_only = __tlist<std::execution::set_stopped_t>;
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
template <class _Ap, class _VS, class _ES, class _SS>
struct __exec_mapped_attrs {
  _Ap __ycxx_attrs;

  template <::__ycxx::__detail::__exec::__forwarding_query_c _Qp, class... _As>
    requires(!::__ycxx::__detail::__exec::__is_completion_query<_Qp>) && ::__ycxx::__detail::__exec::__has_query<std::remove_cvref_t<_Ap>, _Qp, _As...>
  constexpr decltype(auto) query(_Qp __q, _As&&... __as) const
      noexcept(noexcept(::__ycxx::__detail::__exec::__as_const_ref(__ycxx_attrs).query(__q, static_cast<_As&&>(__as)...))) {
    return ::__ycxx::__detail::__exec::__as_const_ref(__ycxx_attrs).query(__q, static_cast<_As&&>(__as)...);
  }
  template <class _Tp, class... _Envs, class _Sp = ::__ycxx::__detail::__exec::__single_source_t<::__ycxx::__detail::__exec::__attr_sources_t<_VS, _ES, _SS, _Tp>>>
    requires requires(const std::remove_cvref_t<_Ap>& a, const _Envs&... e) { std::execution::get_completion_scheduler_t<_Sp>{}(a, e...); }
  constexpr auto query(std::execution::get_completion_scheduler_t<_Tp>, const _Envs&... __envs) const noexcept {
    return std::execution::get_completion_scheduler_t<_Sp>{}(::__ycxx::__detail::__exec::__as_const_ref(__ycxx_attrs), __envs...);
  }
  template <class _Tp, class... _Envs>
    requires requires {
      typename ::__ycxx::__detail::__exec::__sources_domain<std::remove_cvref_t<_Ap>, ::__ycxx::__detail::__exec::__attr_sources_t<_VS, _ES, _SS, _Tp>, _Envs...>::type;
    }
  constexpr auto query(std::execution::get_completion_domain_t<_Tp>, const _Envs&...) const noexcept {
    return typename ::__ycxx::__detail::__exec::__sources_domain<std::remove_cvref_t<_Ap>, ::__ycxx::__detail::__exec::__attr_sources_t<_VS, _ES, _SS, _Tp>,
                                                         _Envs...>::type();
  }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _VS, class _ES, class _SS, class _Child>
constexpr auto __mapped_attrs(const _Child& __child) noexcept {
  using _Ap = __env_member_t<decltype(std::execution::get_env(__child))>;
  return ::__ycxx::__adl_free::__exec_mapped_attrs<_Ap, _VS, _ES, _SS>{std::execution::get_env(__child)};
}

// An environment's stand-in receiver: what connect is asked about when only the environment is
// known (whether a connect can throw is a function of the environment, [exec.connect]/6).
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
template <class _Env>
struct __exec_probe_receiver {
  using receiver_concept = std::execution::receiver_tag;
  template <class... _As>
  void set_value(_As&&...) && noexcept;
  template <class _Ep>
  void set_error(_Ep&&) && noexcept;
  void set_stopped() && noexcept;
  _Env get_env() const noexcept;
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// Whether connecting _Sp to a receiver with environment _Env is noexcept. A sender without
// completion signatures in _Env is not connected at all (connect would be ill-formed, as a hard
// error), so the caller can report the invalid signatures instead.
template <class _Sp, class _Env>
consteval bool __nothrow_connect_in() {
  if constexpr (!std::execution::sender_in<_Sp, _Env>)
    return false;
  else if constexpr (requires { std::execution::connect(std::declval<_Sp>(), std::declval<::__ycxx::__adl_free::__exec_probe_receiver<_Env>>()); })
    return noexcept(std::execution::connect(std::declval<_Sp>(), std::declval<::__ycxx::__adl_free::__exec_probe_receiver<_Env>>()));
  else
    return false;
}
}}} // namespace __ycxx::__detail::__exec

// ---------------------------------------------------------------------------------------------
// [exec.write.env], [exec.unstoppable]
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
struct __write_env_t {
  template <class _Sndr, class _Env>
    requires std::execution::sender<_Sndr> && __queryable<std::decay_t<_Env>>
  constexpr auto operator()(_Sndr&& __sndr, _Env&& env) const
      noexcept(noexcept(::__ycxx::__detail::__exec::__make_sender(__write_env_t(), static_cast<_Env&&>(env), static_cast<_Sndr&&>(__sndr)))) {
    return ::__ycxx::__detail::__exec::__make_sender(*this, static_cast<_Env&&>(env), static_cast<_Sndr&&>(__sndr));
  }
};

template <>
struct __impls_for<__write_env_t> : __default_impls {
  template <class _State, class _Env>
  static constexpr auto __join_env(const _State& state, _Env&& env) noexcept {
    return ::__ycxx::__detail::__exec::__join_env(state, static_cast<_Env&&>(env));
  }
  template <class _Index, class _State, class _Rcvr>
  static constexpr auto get_env(_Index, const _State& state, const _Rcvr& __rcvr) noexcept {
    return __join_env(state, ::__ycxx::__detail::__exec::__fwd_env(std::execution::get_env(__rcvr)));
  }
  template <class _Sndr, class... _Env>
  using __csigs = __csigs_of_t<__child_type<_Sndr>, __join_env_t<const std::decay_t<__data_type<_Sndr>>&, __fwd_env_t<_Env>>...>;
};

struct __unstoppable_t {
  template <std::execution::sender _Sndr>
  constexpr auto operator()(_Sndr&& __sndr) const
      noexcept(noexcept(__write_env_t()(static_cast<_Sndr&&>(__sndr), std::execution::prop(std::get_stop_token, std::never_stop_token{})))) {
    return __write_env_t()(static_cast<_Sndr&&>(__sndr), std::execution::prop(std::get_stop_token, std::never_stop_token{}));
  }
};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
inline constexpr __ycxx::__detail::__exec::__write_env_t write_env{};
inline constexpr __ycxx::__detail::__exec::__unstoppable_t unstoppable{};
}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// [exec.then]
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// The pipeable forms of a sender adaptor object taking a sender and N more arguments
// ([exec.adapt.obj]/5): adaptor(args...) binds args when Bindable<Args...> (the adaptor's
// requirements on them) holds.
template <class _Self, std::size_t _Np, template <class...> class _Bindable>
struct __pipeable_adaptor {
  template <class... _Args>
    requires(sizeof...(_Args) == _Np) && _Bindable<_Args...>::value && (std::constructible_from<std::decay_t<_Args>, _Args> && ...)
  constexpr auto operator()(this const _Self& __self, _Args&&... __args) noexcept((std::is_nothrow_constructible_v<std::decay_t<_Args>, _Args> && ...)) {
    return ::__ycxx::__detail::__exec::__bind_closure(__self, static_cast<_Args&&>(__args)...);
  }
};
template <class _Fp>
struct __bind_movable_value : std::bool_constant<__movable_value<_Fp>> {};

template <class _SetTag, class _Fp>
struct __then_sig_map {
  template <class _Sig>
  struct apply {
    using type = std::execution::completion_signatures<_Sig>;
  };
  template <class... _Ts>
  struct apply<_SetTag(_Ts...)> {
    static auto __pick() {
      if constexpr (!std::is_invocable_v<_Fp, _Ts...>)
        return std::type_identity<__invalid_sigs<__function_not_invocable_with_these_arguments, _Fp, _Ts...>>{};
      else if constexpr (std::is_nothrow_invocable_v<_Fp, _Ts...>)
        return std::type_identity<std::execution::completion_signatures<__set_value_sig_t<std::invoke_result_t<_Fp, _Ts...>>>>{};
      else
        return std::type_identity<std::execution::completion_signatures<__set_value_sig_t<std::invoke_result_t<_Fp, _Ts...>>,
                                                                         std::execution::set_error_t(std::exception_ptr)>>{};
    }
    using type = typename decltype(__pick())::type;
  };
  template <class _Sig>
  using __f = typename apply<_Sig>::type;
};

template <class _SetTag, class _VS, class _ES, class _SS>
struct __then_impls : __default_impls {
  template <class _Data, class _Child>
  static constexpr auto __get_attrs(const _Data&, const _Child& __child) noexcept {
    return ::__ycxx::__detail::__exec::__mapped_attrs<_VS, _ES, _SS>(__child);
  }
  template <class _Index, class _Fn, class _Rcvr, class _Tag, class... _Args>
    requires(!std::is_same_v<_Tag, _SetTag> && __callable<_Tag, _Rcvr, _Args...>) ||
            (std::is_same_v<_Tag, _SetTag> && std::is_invocable_v<_Fn, _Args...>)
  static constexpr void complete(_Index, _Fn& __fn, _Rcvr& __rcvr, _Tag, _Args&&... __args) noexcept {
    if constexpr (std::is_same_v<_Tag, _SetTag>) {
      ::__ycxx::__detail::__exec::__try_set_value(__rcvr, [&]() noexcept(std::is_nothrow_invocable_v<_Fn, _Args...>) -> decltype(auto) {
        return std::invoke(static_cast<_Fn&&>(__fn), static_cast<_Args&&>(__args)...);
      });
    } else {
      _Tag()(static_cast<_Rcvr&&>(__rcvr), static_cast<_Args&&>(__args)...);
    }
  }
  template <class _Sndr, class... _Env>
  using __csigs = __sigs_map_t<__child_sigs_t<_Sndr, _Env...>, __then_sig_map<_SetTag, std::remove_cvref_t<__data_type<_Sndr>>>::template __f>;
};

template <class _Self>
struct __then_adaptor : __pipeable_adaptor<_Self, 1, __bind_movable_value> {
  using __pipeable_adaptor<_Self, 1, __bind_movable_value>::operator();
  template <std::execution::sender _Sndr, __movable_value _Fp>
  constexpr auto operator()(this const _Self& __self, _Sndr&& __sndr, _Fp&& __f) noexcept(
      noexcept(::__ycxx::__detail::__exec::__make_sender(__self, static_cast<_Fp&&>(__f), static_cast<_Sndr&&>(__sndr)))) {
    return ::__ycxx::__detail::__exec::__make_sender(__self, static_cast<_Fp&&>(__f), static_cast<_Sndr&&>(__sndr));
  }
};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
struct then_t : __ycxx::__detail::__exec::__then_adaptor<then_t> {};
struct upon_error_t : __ycxx::__detail::__exec::__then_adaptor<upon_error_t> {};
struct upon_stopped_t : __ycxx::__detail::__exec::__then_adaptor<upon_stopped_t> {};
inline constexpr then_t then{};
inline constexpr upon_error_t upon_error{};
inline constexpr upon_stopped_t upon_stopped{};
}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
using std::execution::set_error_t;
using std::execution::set_stopped_t;
using std::execution::set_value_t;
template <>
struct __impls_for<std::execution::then_t> : __then_impls<set_value_t, __values_only, __tlist<set_error_t, set_value_t>, __stopped_only> {};
template <>
struct __impls_for<std::execution::upon_error_t>
    : __then_impls<set_error_t, __tlist<set_value_t, set_error_t>, __errors_only, __stopped_only> {};
template <>
struct __impls_for<std::execution::upon_stopped_t>
    : __then_impls<set_stopped_t, __tlist<set_value_t, set_stopped_t>, __tlist<set_error_t, set_stopped_t>, __tlist<>> {};
}}} // namespace __ycxx::__detail::__exec

// ---------------------------------------------------------------------------------------------
// [exec.into.variant]
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _CS>
struct __into_variant_sigs {
  using type = _CS;
};
template <class... _Sigs>
struct __into_variant_sigs<std::execution::completion_signatures<_Sigs...>> {
  using _CS = std::execution::completion_signatures<_Sigs...>;
  using _Vp = __gather_signatures<set_value_t, _CS, __decayed_tuple, __variant_or_empty>;
  template <class _Sig>
  using __non_values = std::conditional_t<std::is_same_v<typename __sig_tag<_Sig>::type, set_value_t>, __no_sigs,
                                        std::execution::completion_signatures<_Sig>>;
  static auto __pick() {
    if constexpr (!__decay_copyable_sigs<_CS>)
      return std::type_identity<__invalid_sigs<__result_datums_not_decay_copyable, _CS>>{};
    else
      return std::type_identity<__sigs_concat_t<std::execution::completion_signatures<set_value_t(_Vp)>, __non_values<_Sigs>...,
                                              std::conditional_t<__nothrow_decay_copy_sigs<_CS>, __no_sigs, __eptr_sigs>>>{};
  }
  using type = typename decltype(__pick())::type;
};

}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
struct into_variant_t : sender_adaptor_closure<into_variant_t> {
  template <sender _Sndr>
  constexpr auto operator()(_Sndr&& __sndr) const noexcept(is_nothrow_constructible_v<decay_t<_Sndr>, _Sndr>) {
    return __ycxx::__detail::__exec::__make_sender(*this, __ycxx::__detail::__exec::__empty_data(), static_cast<_Sndr&&>(__sndr));
  }
};
inline constexpr into_variant_t into_variant{};
}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <>
struct __impls_for<std::execution::into_variant_t> : __default_impls {
  template <class _Data, class _Child>
  static constexpr auto __get_attrs(const _Data&, const _Child& __child) noexcept {
    return ::__ycxx::__detail::__exec::__mapped_attrs<__values_only, __tlist<set_error_t, set_value_t>, __stopped_only>(__child);
  }
  template <class _Sndr, class _Rcvr>
  static constexpr auto __get_state(_Sndr&&, _Rcvr&) noexcept {
    return std::type_identity<std::execution::value_types_of_t<__child_type<_Sndr>, __fwd_env_t<std::execution::env_of_t<_Rcvr>>>>{};
  }
  template <class _Index, class _State, class _Rcvr, class _Tag, class... _Args>
  static constexpr void complete(_Index, _State, _Rcvr& __rcvr, _Tag, _Args&&... __args) noexcept {
    if constexpr (std::is_same_v<_Tag, set_value_t>) {
      using __variant_type = typename _State::type;
      ::__ycxx::__detail::__exec::__try_set_value(__rcvr, [&]() noexcept(std::is_nothrow_constructible_v<__decayed_tuple<_Args...>, _Args...>) {
        return __variant_type(__decayed_tuple<_Args...>{static_cast<_Args&&>(__args)...});
      });
    } else {
      _Tag()(static_cast<_Rcvr&&>(__rcvr), static_cast<_Args&&>(__args)...);
    }
  }
  template <class _Sndr, class... _Env>
  using __csigs = typename __into_variant_sigs<__child_sigs_t<_Sndr, _Env...>>::type;
};
}}} // namespace __ycxx::__detail::__exec

// ---------------------------------------------------------------------------------------------
// [exec.let]
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// SCHED-ENV(sch) ([exec.snd.expos]/10)
template <class _Sch>
struct __exec_sched_env {
  _Sch __sch;
  constexpr _Sch query(std::execution::get_start_scheduler_t) const noexcept { return __sch; }
  constexpr auto query(std::execution::get_domain_t) const noexcept
    requires requires(const _Sch& s) { s.query(std::execution::get_domain_t{}); }
  {
    return __sch.query(std::execution::get_domain_t{});
  }
};
// let-data
template <class _Sndr, class _Fn>
struct __exec_let_data {
  _Sndr __sndr;
  _Fn __fn;
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// let-env(sndr, env) ([exec.let]/2)
template <class _SetTag, class _Sndr, class _Env>
constexpr auto __let_env(const _Sndr& __sndr, const _Env& env) noexcept {
  if constexpr (requires { std::execution::get_completion_scheduler<_SetTag>(std::execution::get_env(__sndr), ::__ycxx::__detail::__exec::__fwd_env(env)); })
    return ::__ycxx::__adl_free::__exec_sched_env<decltype(std::execution::get_completion_scheduler<_SetTag>(
        std::execution::get_env(__sndr), ::__ycxx::__detail::__exec::__fwd_env(env)))>{
        std::execution::get_completion_scheduler<_SetTag>(std::execution::get_env(__sndr), ::__ycxx::__detail::__exec::__fwd_env(env))};
  else if constexpr (requires { std::execution::get_completion_domain<_SetTag>(std::execution::get_env(__sndr), ::__ycxx::__detail::__exec::__fwd_env(env)); })
    return std::execution::prop(std::execution::get_domain,
                                std::execution::get_completion_domain<_SetTag>(std::execution::get_env(__sndr), ::__ycxx::__detail::__exec::__fwd_env(env)));
  else
    return std::execution::env<>();
}
template <class _SetTag, class _Sndr, class _Env>
using __let_env_t = decltype(::__ycxx::__detail::__exec::__let_env<_SetTag>(std::declval<const std::remove_cvref_t<_Sndr>&>(), std::declval<const _Env&>()));
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// receiver2 ([exec.let]/8)
template <class _Rcvr, class _Env>
struct __exec_let_receiver2 {
  using receiver_concept = std::execution::receiver_tag;
  template <class... _Args>
  constexpr void set_value(_Args&&... __args) && noexcept {
    std::execution::set_value(static_cast<_Rcvr&&>(*__rcvr), static_cast<_Args&&>(__args)...);
  }
  template <class _Error>
  constexpr void set_error(_Error&& __err) && noexcept {
    std::execution::set_error(static_cast<_Rcvr&&>(*__rcvr), static_cast<_Error&&>(__err));
  }
  constexpr void set_stopped() && noexcept { std::execution::set_stopped(static_cast<_Rcvr&&>(*__rcvr)); }
  constexpr auto get_env() const noexcept {
    return ::__ycxx::__detail::__exec::__join_env(env, ::__ycxx::__detail::__exec::__fwd_env(std::execution::get_env(*__rcvr)));
  }
  _Rcvr* __rcvr;
  _Env env;
};

// The receiver of the first operation of a let state. It names the state by its parameters
// rather than its type: the state's operation variant holds the operation connected to it.
template <class _State>
struct __exec_let_receiver;
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _Cpo, class _Sndr, class _Fn, class _Rcvr>
struct __let_state_key {};

template <class _Cpo, class _Sndr, class _Fn, class _Rcvr>
struct __let_types {
  using __child_sigs = std::execution::completion_signatures_of_t<_Sndr, __fwd_env_t<std::execution::env_of_t<_Rcvr>>>;
  using __env_t = __let_env_t<_Cpo, _Sndr, std::execution::env_of_t<_Rcvr>>;
  using __receiver2 = ::__ycxx::__adl_free::__exec_let_receiver2<_Rcvr, __env_t>;
  using __let_args = __sigs_args_t<_Cpo, __child_sigs>;
  template <class _Args>
  struct __per_args;
  template <class... _Ts>
  struct __per_args<__tlist<_Ts...>> {
    using __tuple_t = __decayed_tuple<_Ts...>;
    using __sndr2 = std::invoke_result_t<_Fn, std::decay_t<_Ts>&...>;
    using __op2 = std::execution::connect_result_t<__sndr2, __receiver2>;
  };
  template <class _ArgLists>
  struct __variants;
  template <class... _ArgLists>
  struct __variants<__tlist<_ArgLists...>> {
    using __args_variant = __apply_unique_t<std::variant, std::monostate, typename __per_args<_ArgLists>::__tuple_t...>;
    using __ops_variant =
        __apply_unique_t<::__ycxx::__adl_free::__exec_op_variant,
                       std::execution::connect_result_t<_Sndr, ::__ycxx::__adl_free::__exec_let_receiver<__let_state_key<_Cpo, _Sndr, _Fn, _Rcvr>>>,
                       typename __per_args<_ArgLists>::__op2...>;
  };
  using __args_variant = typename __variants<__let_args>::__args_variant;
  using __ops_variant = typename __variants<__let_args>::__ops_variant;
};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// let-state ([exec.let]/10)
template <class _Cpo, class _Sndr, class _Fn, class _Rcvr>
struct __exec_let_state {
  using __types = ::__ycxx::__detail::__exec::__let_types<_Cpo, _Sndr, _Fn, _Rcvr>;
  using __env_t = typename __types::__env_t;
  using receiver = __exec_let_receiver<::__ycxx::__detail::__exec::__let_state_key<_Cpo, _Sndr, _Fn, _Rcvr>>;
  using __op_t = std::execution::connect_result_t<_Sndr, receiver>;

  _Fn __fn;
  __env_t env;
  typename __types::__args_variant __args;
  typename __types::__ops_variant __ops;

  constexpr __exec_let_state(_Sndr&& __sndr, _Fn __f, _Rcvr& __rcvr)
      : __fn(static_cast<_Fn&&>(__f)), env(::__ycxx::__detail::__exec::__let_env<_Cpo>(__sndr, std::execution::get_env(__rcvr))),
        __ops() {
    __ops.template __emplace_from_fn<__op_t>(
        [&]() { return std::execution::connect(static_cast<_Sndr&&>(__sndr), receiver{this, __builtin_addressof(__rcvr)}); });
  }
  __exec_let_state(__exec_let_state&&) = delete;

  template <class _Tag, class... _Ts>
  constexpr void __y_impl(_Rcvr& __rcvr, _Tag tag, _Ts&&... __ts) noexcept {
    if constexpr (std::is_same_v<_Tag, _Cpo>) {
      using __args_t = ::__ycxx::__detail::__exec::__decayed_tuple<_Ts...>;
      using __receiver_type = typename __types::__receiver2;
      using __sender_type = std::invoke_result_t<_Fn, std::decay_t<_Ts>&...>;
      using __op2_t = std::execution::connect_result_t<__sender_type, __receiver_type>;
      constexpr bool nothrow = std::is_nothrow_constructible_v<__args_t, _Ts...> && std::is_nothrow_invocable_v<_Fn, std::decay_t<_Ts>&...> &&
                               std::is_nothrow_invocable_v<std::execution::connect_t, __sender_type, __receiver_type>;
      auto __body = [&]() noexcept(nothrow) {
        auto& tuple = __args.template emplace<__args_t>(static_cast<_Ts&&>(__ts)...);
        __ops.reset();
        auto&& __sndr = std::apply(static_cast<_Fn&&>(__fn), tuple);
        auto& op = __ops.template __emplace_from_fn<__op2_t>(
            [&]() { return std::execution::connect(static_cast<__sender_type&&>(__sndr), __receiver_type{__builtin_addressof(__rcvr), env}); });
        std::execution::start(op);
      };
      if constexpr (nothrow || !::__ycxx::__detail::__cfg::exceptions) {
        __body();
      } else {
        try {
          __body();
        } catch (...) {
          std::execution::set_error(static_cast<_Rcvr&&>(__rcvr), std::current_exception());
        }
      }
    } else {
      tag(static_cast<_Rcvr&&>(__rcvr), static_cast<_Ts&&>(__ts)...);
    }
  }
};

template <class _Cpo, class _Sndr, class _Fn, class _Rcvr>
struct __exec_let_receiver<::__ycxx::__detail::__exec::__let_state_key<_Cpo, _Sndr, _Fn, _Rcvr>> {
  using receiver_concept = std::execution::receiver_tag;
  using __state_t = __exec_let_state<_Cpo, _Sndr, _Fn, _Rcvr>;
  void* state;
  _Rcvr* __rcvr;
  template <class... _Args>
  constexpr void set_value(_Args&&... __args) && noexcept {
    static_cast<__state_t*>(state)->__y_impl(*__rcvr, std::execution::set_value, static_cast<_Args&&>(__args)...);
  }
  template <class _Error>
  constexpr void set_error(_Error&& __err) && noexcept {
    static_cast<__state_t*>(state)->__y_impl(*__rcvr, std::execution::set_error, static_cast<_Error&&>(__err));
  }
  constexpr void set_stopped() && noexcept { static_cast<__state_t*>(state)->__y_impl(*__rcvr, std::execution::set_stopped); }
  constexpr decltype(auto) get_env() const noexcept { return std::execution::get_env(*__rcvr); }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// let-tag
template <class _Cpo>
struct __let_tag {};

// The completion signatures of let-cpo(sndr, fn) in Env... ([exec.let]/9).
template <class _Cpo, class _Child, class _Fn, class... _Env>
struct __let_sigs {
  using _CS = __csigs_of_t<_Child, __fwd_env_t<_Env>...>;
  template <class _Sig>
  struct apply {
    using type = std::execution::completion_signatures<_Sig>;
  };
  template <class... _Ts>
  struct apply<_Cpo(_Ts...)> {
    static auto __pick() {
      if constexpr (!(std::is_constructible_v<std::decay_t<_Ts>, _Ts> && ...))
        return std::type_identity<__invalid_sigs<__result_datums_not_decay_copyable, _Cpo(_Ts...)>>{};
      else if constexpr (!std::is_invocable_v<_Fn, std::decay_t<_Ts>&...>)
        return std::type_identity<__invalid_sigs<__function_not_invocable_with_these_arguments, _Fn, std::decay_t<_Ts>&...>>{};
      else {
        using _S2 = std::invoke_result_t<_Fn, std::decay_t<_Ts>&...>;
        if constexpr (!std::execution::sender<_S2>) {
          return std::type_identity<__invalid_sigs<__let_function_must_return_a_sender, _Fn, _S2>>{};
        } else {
          if constexpr (sizeof...(_Env) == 0)
            return __pick2<_S2>();
          else
            return __pick2<_S2, __join_env_t<const __let_env_t<_Cpo, _Child, _Env...[0]>&, __fwd_env_t<_Env...[0]>>>();
        }
      }
    }
    template <class _S2, class... _Env2>
    static auto __pick2() {
      constexpr bool nothrow = (std::is_nothrow_constructible_v<std::decay_t<_Ts>, _Ts> && ...) &&
                               std::is_nothrow_invocable_v<_Fn, std::decay_t<_Ts>&...> &&
                               (::__ycxx::__detail::__exec::__nothrow_connect_in<_S2, _Env2>() && ...) && sizeof...(_Env2) != 0;
      return std::type_identity<__sigs_concat_t<__csigs_of_t<_S2, _Env2...>, std::conditional_t<nothrow, __no_sigs, __eptr_sigs>>>{};
    }
    using type = typename decltype(__pick())::type;
  };
  template <class _Sig>
  using __f = typename apply<_Sig>::type;
  using type = __sigs_map_t<_CS, __f>;
};

template <class _Cpo>
struct __let_impls_base : __default_impls {
  template <class _Data, class... _Child>
  static constexpr auto __get_attrs(const _Data&, const _Child&...) noexcept {
    return std::execution::env<>();
  }
};

// The let_value/let_error/let_stopped senders before their transformation into let-tag senders
// (which happens when they are connected).
template <class _Cpo>
struct __let_cpo_impls : __let_impls_base<_Cpo> {
  template <class _Sndr, class... _Env>
  using __csigs = typename __let_sigs<_Cpo, __child_type<_Sndr>, std::remove_cvref_t<__data_type<_Sndr>>, _Env...>::type;
};

template <class _Cpo>
struct __impls_for<__let_tag<_Cpo>> : __let_impls_base<_Cpo> {
  template <class _Sndr, class _Rcvr>
  static constexpr auto __get_state(_Sndr&& __sndr, _Rcvr& __rcvr) {
    using __data_t = std::remove_cvref_t<__data_type<_Sndr>>;
    using __child_t = decltype(std::forward_like<_Sndr>(std::declval<__data_t&>().__sndr));
    using __fn_t = std::decay_t<decltype(std::declval<__data_t&>().__fn)>;
    auto&& data = static_cast<_Sndr&&>(__sndr).template get<1>();
    return ::__ycxx::__adl_free::__exec_let_state<_Cpo, __child_t, __fn_t, _Rcvr>(std::forward_like<_Sndr>(data.__sndr), std::forward_like<_Sndr>(data.__fn), __rcvr);
  }
  template <class _State, class _Rcvr>
  static constexpr void start(_State& state, _Rcvr&) noexcept {
    std::execution::start(state.__ops.template get<typename _State::__op_t>());
  }
  template <class _Sndr, class... _Env>
  using __csigs = typename __let_sigs<_Cpo, decltype(std::forward_like<_Sndr>(std::declval<std::remove_cvref_t<__data_type<_Sndr>>&>().__sndr)),
                                  std::decay_t<decltype(std::declval<std::remove_cvref_t<__data_type<_Sndr>>&>().__fn)>, _Env...>::type;
};

template <class _Self, class _SetTag>
struct __let_adaptor : __pipeable_adaptor<_Self, 1, __bind_movable_value> {
  using __pipeable_adaptor<_Self, 1, __bind_movable_value>::operator();
  template <std::execution::sender _Sndr, __movable_value _Fp>
    requires(!std::is_same_v<_SetTag, set_stopped_t> || std::invocable<std::decay_t<_Fp>>)
  constexpr auto operator()(this const _Self& __self, _Sndr&& __sndr, _Fp&& __f) noexcept(
      noexcept(::__ycxx::__detail::__exec::__make_sender(__self, static_cast<_Fp&&>(__f), static_cast<_Sndr&&>(__sndr)))) {
    return ::__ycxx::__detail::__exec::__make_sender(__self, static_cast<_Fp&&>(__f), static_cast<_Sndr&&>(__sndr));
  }
  // let-cpo.transform_sender ([exec.let]/6), on set_value like the other lowered adaptors.
  template <class _Sndr, class _Env>
    requires std::is_same_v<std::execution::tag_of_t<_Sndr>, _Self>
  static constexpr auto transform_sender(set_value_t, _Sndr&& s, const _Env&) {
    using __child_t = std::decay_t<__child_type<_Sndr>>;
    using __fn_t = std::decay_t<__data_type<_Sndr>>;
    return ::__ycxx::__detail::__exec::__make_sender(__let_tag<_SetTag>{}, ::__ycxx::__adl_free::__exec_let_data<__child_t, __fn_t>{
                                                                    static_cast<_Sndr&&>(s).template get<2>(), static_cast<_Sndr&&>(s).template get<1>()});
  }
};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
struct let_value_t : __ycxx::__detail::__exec::__let_adaptor<let_value_t, set_value_t> {};
struct let_error_t : __ycxx::__detail::__exec::__let_adaptor<let_error_t, set_error_t> {};
struct let_stopped_t : __ycxx::__detail::__exec::__let_adaptor<let_stopped_t, set_stopped_t> {};
inline constexpr let_value_t let_value{};
inline constexpr let_error_t let_error{};
inline constexpr let_stopped_t let_stopped{};
}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <>
struct __impls_for<std::execution::let_value_t> : __let_cpo_impls<set_value_t> {};
template <>
struct __impls_for<std::execution::let_error_t> : __let_cpo_impls<set_error_t> {};
template <>
struct __impls_for<std::execution::let_stopped_t> : __let_cpo_impls<set_stopped_t> {};
}}} // namespace __ycxx::__detail::__exec

// ---------------------------------------------------------------------------------------------
// [exec.stopped.opt], [exec.stopped.err]
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _CS>
struct __stopped_as_optional_sigs {
  using type = _CS;
};
template <class _Args>
struct __optional_value;
template <class _Tp>
struct __optional_value<__tlist<_Tp>> {
  using type = std::decay_t<_Tp>;
};
template <class _T0, class _T1, class... _Ts>
struct __optional_value<__tlist<_T0, _T1, _Ts...>> {
  using type = __decayed_tuple<_T0, _T1, _Ts...>;
};
template <class _Vp, class _Args>
inline constexpr bool __nothrow_optional_from = false;
template <class _Vp, class... _Ts>
inline constexpr bool __nothrow_optional_from<_Vp, __tlist<_Ts...>> = std::is_nothrow_constructible_v<_Vp, _Ts...>;

template <class... _Sigs>
struct __stopped_as_optional_sigs<std::execution::completion_signatures<_Sigs...>> {
  using _CS = std::execution::completion_signatures<_Sigs...>;
  template <class _Sig>
  using __errors = std::conditional_t<std::is_same_v<typename __sig_tag<_Sig>::type, set_error_t>,
                                    std::execution::completion_signatures<_Sig>, __no_sigs>;
  template <class _ArgLists>
  struct __pick;
  // Exactly one value completion, with at least one datum (single-sender-value-type not void).
  template <class _Args>
    requires requires { typename __optional_value<_Args>::type; }
  struct __pick<__tlist<_Args>> {
    using _Vp = typename __optional_value<_Args>::type;
    using type = __sigs_concat_t<std::execution::completion_signatures<set_value_t(std::optional<_Vp>)>, __errors<_Sigs>...,
                               std::conditional_t<__nothrow_optional_from<_Vp, _Args>, __no_sigs, __eptr_sigs>>;
  };
  template <class _ArgLists>
  struct __pick {
    using type = __invalid_sigs<__sender_has_not_exactly_one_value_completion, _CS>;
  };
  using type = typename __pick<__sigs_args_t<set_value_t, _CS>>::type;
};

template <class _Ep, class _CS>
struct __stopped_as_error_sigs {
  using type = _CS;
};
template <class _Ep, class... _Sigs>
struct __stopped_as_error_sigs<_Ep, std::execution::completion_signatures<_Sigs...>> {
  template <class _Sig>
  using __f = std::conditional_t<std::is_same_v<_Sig, set_stopped_t()>, std::execution::completion_signatures<set_error_t(_Ep)>,
                               std::execution::completion_signatures<_Sig>>;
  using type = __sigs_concat_t<__f<_Sigs>...>;
};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
struct stopped_as_optional_t : sender_adaptor_closure<stopped_as_optional_t> {
  template <sender _Sndr>
  constexpr auto operator()(_Sndr&& __sndr) const noexcept(is_nothrow_constructible_v<decay_t<_Sndr>, _Sndr>) {
    return __ycxx::__detail::__exec::__make_sender(*this, __ycxx::__detail::__exec::__empty_data(), static_cast<_Sndr&&>(__sndr));
  }
  template <class _Sndr, class _Env>
    requires is_same_v<tag_of_t<_Sndr>, stopped_as_optional_t>
  static constexpr auto transform_sender(set_value_t, _Sndr&& __sndr, const _Env&) {
    using __child_t = __ycxx::__detail::__exec::__child_type<_Sndr>;
    if constexpr (!sender_in<__child_t, __ycxx::__detail::__exec::__fwd_env_t<_Env>>) {
      return __ycxx::__adl_free::__exec_not_a_sender();
    } else if constexpr (is_void_v<__ycxx::__detail::__exec::__single_sender_value_or_void<__child_t, __ycxx::__detail::__exec::__fwd_env_t<_Env>>>) {
      return __ycxx::__adl_free::__exec_not_a_sender();
    } else {
      using _Vp = __ycxx::__detail::__exec::__single_sender_value_type<__child_t, __ycxx::__detail::__exec::__fwd_env_t<_Env>>;
      return let_stopped_t()(then_t()(static_cast<_Sndr&&>(__sndr).template get<2>(),
                                      []<class... _Ts>(_Ts&&... __ts) noexcept(is_nothrow_constructible_v<_Vp, _Ts...>) {
                                        return optional<_Vp>(in_place, static_cast<_Ts&&>(__ts)...);
                                      }),
                             []() noexcept { return just_t()(optional<_Vp>()); });
    }
  }
};
struct stopped_as_error_t : __ycxx::__detail::__exec::__pipeable_adaptor<stopped_as_error_t, 1, __ycxx::__detail::__exec::__bind_movable_value> {
  using __ycxx::__detail::__exec::__pipeable_adaptor<stopped_as_error_t, 1, __ycxx::__detail::__exec::__bind_movable_value>::operator();
  template <sender _Sndr, __ycxx::__detail::__exec::__movable_value _Err>
  constexpr auto operator()(_Sndr&& __sndr, _Err&& __err) const
      noexcept(is_nothrow_constructible_v<decay_t<_Sndr>, _Sndr> && is_nothrow_constructible_v<decay_t<_Err>, _Err>) {
    return __ycxx::__detail::__exec::__make_sender(*this, static_cast<_Err&&>(__err), static_cast<_Sndr&&>(__sndr));
  }
  template <class _Sndr, class _Env>
    requires is_same_v<tag_of_t<_Sndr>, stopped_as_error_t>
  static constexpr auto transform_sender(set_value_t, _Sndr&& __sndr, const _Env&) {
    using _Ep = decay_t<__ycxx::__detail::__exec::__data_type<_Sndr>>;
    return let_stopped_t()(static_cast<_Sndr&&>(__sndr).template get<2>(),
                           [__err = static_cast<_Sndr&&>(__sndr).template get<1>()]() mutable noexcept(is_nothrow_move_constructible_v<_Ep>) {
                             return just_error_t()(static_cast<_Ep&&>(__err));
                           });
  }
};
inline constexpr stopped_as_optional_t stopped_as_optional{};
inline constexpr stopped_as_error_t stopped_as_error{};
}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <>
struct __impls_for<std::execution::stopped_as_optional_t> : __default_impls {
  template <class _Data, class _Child>
  static constexpr auto __get_attrs(const _Data&, const _Child& __child) noexcept {
    return ::__ycxx::__detail::__exec::__mapped_attrs<__tlist<set_value_t, set_stopped_t>, __errors_only, __tlist<>>(__child);
  }
  template <class _Sndr, class... _Env>
  using __csigs = typename __stopped_as_optional_sigs<__child_sigs_t<_Sndr, _Env...>>::type;
};
template <>
struct __impls_for<std::execution::stopped_as_error_t> : __default_impls {
  template <class _Data, class _Child>
  static constexpr auto __get_attrs(const _Data&, const _Child& __child) noexcept {
    return ::__ycxx::__detail::__exec::__mapped_attrs<__values_only, __tlist<set_error_t, set_stopped_t>, __tlist<>>(__child);
  }
  template <class _Sndr, class... _Env>
  using __csigs = typename __stopped_as_error_sigs<std::decay_t<__data_type<_Sndr>>, __child_sigs_t<_Sndr, _Env...>>::type;
};
}}} // namespace __ycxx::__detail::__exec

// ---------------------------------------------------------------------------------------------
// [exec.bulk]
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <bool _Chunked, class _Fp, class _Shape>
struct __bulk_sig_map {
  template <class _Sig>
  struct apply {
    using type = std::execution::completion_signatures<_Sig>;
  };
  template <class... _Ts>
  struct apply<set_value_t(_Ts...)> {
    static constexpr bool ok = _Chunked ? std::is_invocable_v<_Fp&, _Shape, _Shape, std::decay_t<_Ts>&...> : std::is_invocable_v<_Fp&, _Shape, std::decay_t<_Ts>&...>;
    static constexpr bool nothrow =
        _Chunked ? std::is_nothrow_invocable_v<_Fp&, _Shape, _Shape, std::decay_t<_Ts>&...> : std::is_nothrow_invocable_v<_Fp&, _Shape, std::decay_t<_Ts>&...>;
    using type = std::conditional_t<!ok, __invalid_sigs<__function_not_invocable_with_these_arguments, _Fp, _Shape, _Ts...>,
                                    __sigs_concat_t<std::execution::completion_signatures<set_value_t(_Ts...)>,
                                                  std::conditional_t<nothrow, __no_sigs, __eptr_sigs>>>;
  };
  template <class _Sig>
  using __f = typename apply<_Sig>::type;
};

template <bool _Chunked, class _Fp, class _Sp, class... _Args>
concept __bulk_invocable = (_Chunked && std::invocable<_Fp&, _Sp, _Sp, _Args&...>) || (!_Chunked && std::invocable<_Fp&, _Sp, _Args&...>);

// The function of bulk's bulk_chunked form ([exec.bulk]/4): f called for each index of a chunk.
template <class _Shape, class _Func>
struct __bulk_chunk_fn {
  _Func __func;
  template <class... _Vs>
    requires std::invocable<_Func&, _Shape, _Vs&...>
  constexpr void operator()(_Shape begin, _Shape end, _Vs&&... __vs) noexcept(std::is_nothrow_invocable_v<_Func&, _Shape, _Vs&...>) {
    while (begin != end)
      __func(begin++, __vs...);
  }
};

template <class _Data>
struct __bulk_data_types;
template <class _Is, class _Pp, class _Shape, class _Func>
struct __bulk_data_types<::__ycxx::__adl_free::__exec_product<_Is, _Pp, _Shape, _Func>> {
  using __shape = _Shape;
  using __func = _Func;
};

template <bool _Chunked>
struct __bulk_impls : __default_impls {
  template <class _Data, class _Child>
  static constexpr auto __get_attrs(const _Data&, const _Child& __child) noexcept {
    return ::__ycxx::__detail::__exec::__mapped_attrs<__values_only, __tlist<set_error_t, set_value_t>, __stopped_only>(__child);
  }
  template <class _Index, class _State, class _Rcvr, class _Tag, class... _Args>
    requires(!std::is_same_v<_Tag, set_value_t>) ||
            __bulk_invocable<_Chunked, std::remove_reference_t<decltype(std::declval<_State&>().template get<2>())>,
                           std::remove_cvref_t<decltype(std::declval<_State&>().template get<1>())>, _Args...>
  static constexpr void complete(_Index, _State& state, _Rcvr& __rcvr, _Tag, _Args&&... __args) noexcept {
    if constexpr (std::is_same_v<_Tag, set_value_t>) {
      auto& __shape = state.template get<1>();
      auto& __f = state.template get<2>();
      using _Sp = std::remove_cvref_t<decltype(__shape)>;
      if constexpr (_Chunked) {
        constexpr bool nothrow = noexcept(__f(_Sp(__shape), _Sp(__shape), __args...));
        ::__ycxx::__detail::__exec::__try_eval(__rcvr, [&]() noexcept(nothrow) {
          __f(static_cast<_Sp>(0), _Sp(__shape), __args...);
          _Tag()(static_cast<_Rcvr&&>(__rcvr), static_cast<_Args&&>(__args)...);
        });
      } else {
        constexpr bool nothrow = noexcept(__f(_Sp(__shape), __args...));
        ::__ycxx::__detail::__exec::__try_eval(__rcvr, [&]() noexcept(nothrow) {
          for (_Sp i = 0; i < __shape; ++i)
            __f(_Sp(i), __args...);
          _Tag()(static_cast<_Rcvr&&>(__rcvr), static_cast<_Args&&>(__args)...);
        });
      }
    } else {
      _Tag()(static_cast<_Rcvr&&>(__rcvr), static_cast<_Args&&>(__args)...);
    }
  }
  template <class _Sndr, class... _Env>
  using __csigs = __sigs_map_t<__child_sigs_t<_Sndr, _Env...>,
                           __bulk_sig_map<_Chunked, typename __bulk_data_types<std::remove_cvref_t<__data_type<_Sndr>>>::__func,
                                        typename __bulk_data_types<std::remove_cvref_t<__data_type<_Sndr>>>::__shape>::template __f>;
};

template <class _Policy, class _Shape, class _Fp>
struct __bind_bulk : std::bool_constant<std::is_execution_policy_v<std::remove_cvref_t<_Policy>> && std::integral<std::decay_t<_Shape>> &&
                                      std::copy_constructible<std::decay_t<_Fp>>> {};

template <class _Self>
struct __bulk_adaptor : __pipeable_adaptor<_Self, 3, __bind_bulk> {
  using __pipeable_adaptor<_Self, 3, __bind_bulk>::operator();
  template <std::execution::sender _Sndr, class _Policy, std::integral _Shape, class _Fp>
    requires std::is_execution_policy_v<std::remove_cvref_t<_Policy>> && std::copy_constructible<std::decay_t<_Fp>>
  constexpr auto operator()(this const _Self& __self, _Sndr&& __sndr, _Policy&& __policy, _Shape __shape, _Fp&& __f) {
    using _Pp = std::remove_cvref_t<_Policy>;
    using _PT = std::conditional_t<std::copy_constructible<_Pp>, _Pp, const _Pp&>;
    return ::__ycxx::__detail::__exec::__make_sender(__self, __product_t<_PT, _Shape, std::decay_t<_Fp>>{{__policy}, {__shape}, {static_cast<_Fp&&>(__f)}},
                                             static_cast<_Sndr&&>(__sndr));
  }
};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
struct bulk_chunked_t : __ycxx::__detail::__exec::__bulk_adaptor<bulk_chunked_t> {};
struct bulk_unchunked_t : __ycxx::__detail::__exec::__bulk_adaptor<bulk_unchunked_t> {};
struct bulk_t : __ycxx::__detail::__exec::__bulk_adaptor<bulk_t> {
  // [exec.bulk]/4: bulk becomes bulk_chunked, each chunk a loop over its indices.
  template <class _Sndr, class _Env>
    requires is_same_v<tag_of_t<_Sndr>, bulk_t>
  static constexpr auto transform_sender(set_value_t, _Sndr&& __sndr, const _Env&) {
    auto&& data = static_cast<_Sndr&&>(__sndr).template get<1>();
    using _Shape = remove_cvref_t<decltype(data.template get<1>())>;
    using _Func = remove_cvref_t<decltype(data.template get<2>())>;
    auto __new_f = __ycxx::__detail::__exec::__bulk_chunk_fn<_Shape, _Func>{static_cast<decltype(data)&&>(data).template get<2>()};
    return bulk_chunked_t()(static_cast<_Sndr&&>(__sndr).template get<2>(), data.template get<0>(), _Shape(data.template get<1>()), std::move(__new_f));
  }
};
inline constexpr bulk_t bulk{};
inline constexpr bulk_chunked_t bulk_chunked{};
inline constexpr bulk_unchunked_t bulk_unchunked{};
}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <>
struct __impls_for<std::execution::bulk_t> : __bulk_impls<false> {};
template <>
struct __impls_for<std::execution::bulk_chunked_t> : __bulk_impls<true> {};
template <>
struct __impls_for<std::execution::bulk_unchunked_t> : __bulk_impls<false> {};
}}} // namespace __ycxx::__detail::__exec
