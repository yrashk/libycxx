// libycxx core: the sender adaptors of [exec.adapt] that move work between execution agents or
// combine operations: schedule_from, continues_on, starts_on, on, affine ([exec.affine]),
// when_all, when_all_with_variant, and the exposition-only stop-when ([exec.stop.when]).
#pragma once

#include <ycxx/core/exec_adapt.hpp>

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _Sch>
struct __bind_scheduler : std::bool_constant<std::execution::scheduler<_Sch>> {};
}}} // namespace __ycxx::__detail::__exec

// ---------------------------------------------------------------------------------------------
// [exec.schedule.from], [exec.continues.on]
namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
struct schedule_from_t {
  template <sender _Sndr>
  constexpr auto operator()(_Sndr&& __sndr) const noexcept(is_nothrow_constructible_v<decay_t<_Sndr>, _Sndr>) {
    return __ycxx::__detail::__exec::__make_sender(*this, __ycxx::__detail::__exec::__empty_data(), static_cast<_Sndr&&>(__sndr));
  }
};
inline constexpr schedule_from_t schedule_from{};
struct continues_on_t;
}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {

// The completion signatures of schedule(sch) other than its value completion.
template <class _Sch, class... _Env>
struct __schedule_non_value_sigs {
  using _CS = __csigs_of_t<std::execution::schedule_result_t<_Sch>, __fwd_env_t<_Env>...>;
  template <class _Sig>
  using __f = std::conditional_t<std::is_same_v<typename __sig_tag<_Sig>::type, set_value_t>, __no_sigs, std::execution::completion_signatures<_Sig>>;
  using type = __sigs_map_t<_CS, __f>;
};

template <class _Sch, class _ChildSigs, class... _Env>
struct __continues_on_sigs {
  static auto __pick() {
    using _SchSigs = __csigs_of_t<std::execution::schedule_result_t<_Sch>, __fwd_env_t<_Env>...>;
    if constexpr (!__is_csigs<_SchSigs>)
      return std::type_identity<_SchSigs>{};
    else if constexpr (!__is_csigs<_ChildSigs>)
      return std::type_identity<_ChildSigs>{};
    else if constexpr (!__decay_copyable_sigs<_ChildSigs>)
      return std::type_identity<__invalid_sigs<__result_datums_not_decay_copyable, _ChildSigs>>{};
    else
      return std::type_identity<__sigs_concat_t<__sigs_map_t<_ChildSigs, __decayed_sig_t>,
                                              std::conditional_t<__nothrow_decay_copy_sigs<_ChildSigs>, __no_sigs, __eptr_sigs>,
                                              typename __schedule_non_value_sigs<_Sch, _Env...>::type>>{};
  }
  using type = typename decltype(__pick())::type;
};

template <class _Sig>
struct __as_tuple_of_sig;
template <class _Tag, class... _Args>
struct __as_tuple_of_sig<_Tag(_Args...)> {
  using type = __decayed_tuple<_Tag, _Args...>;
};
template <class _CS>
struct __continues_on_variant;
template <class... _Sigs>
struct __continues_on_variant<std::execution::completion_signatures<_Sigs...>> {
  using type = std::conditional_t<
      (__nothrow_decay_copy_sig<_Sigs> && ...),
      __apply_unique_t<std::variant, std::monostate, typename __as_tuple_of_sig<_Sigs>::type...>,
      __apply_unique_t<std::variant, std::monostate, typename __as_tuple_of_sig<_Sigs>::type..., std::tuple<set_error_t, std::exception_ptr>>>;
};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// The state of continues_on ([exec.continues.on]/5): the child's result, kept until the schedule
// operation completes.
template <class _Sch, class _Child, class _Rcvr>
struct __exec_continues_on_state {
  using __variant_t =
      typename ::__ycxx::__detail::__exec::__continues_on_variant<std::execution::completion_signatures_of_t<_Child, ::__ycxx::__detail::__exec::__fwd_env_t<std::execution::env_of_t<_Rcvr>>>>::type;

  _Rcvr& __rcvr;
  __variant_t __async_result;

  struct __receiver_t {
    using receiver_concept = std::execution::receiver_tag;
    __exec_continues_on_state* state;
    void set_value() && noexcept {
      std::visit(
          [this]<class _Tuple>(_Tuple& result) noexcept -> void {
            if constexpr (!std::is_same_v<std::monostate, _Tuple>) {
              std::apply(
                  [this](auto& tag, auto&... __args) noexcept {
                    tag(static_cast<_Rcvr&&>(state->__rcvr), static_cast<std::remove_reference_t<decltype(__args)>&&>(__args)...);
                  },
                  result);
            }
          },
          state->__async_result);
    }
    template <class _Error>
    void set_error(_Error&& __err) && noexcept {
      std::execution::set_error(static_cast<_Rcvr&&>(state->__rcvr), static_cast<_Error&&>(__err));
    }
    void set_stopped() && noexcept { std::execution::set_stopped(static_cast<_Rcvr&&>(state->__rcvr)); }
    decltype(auto) get_env() const noexcept { return ::__ycxx::__detail::__exec::__fwd_env(std::execution::get_env(state->__rcvr)); }
  };
  using __operation_t = std::execution::connect_result_t<std::execution::schedule_result_t<_Sch&>, __receiver_t>;

  __operation_t __op_state;

  explicit __exec_continues_on_state(_Sch& __sch, _Rcvr& r) noexcept(
      std::is_nothrow_invocable_v<std::execution::connect_t, std::execution::schedule_result_t<_Sch&>, __receiver_t>)
      : __rcvr(r), __op_state(std::execution::connect(std::execution::schedule(__sch), __receiver_t{this})) {}
  __exec_continues_on_state(__exec_continues_on_state&&) = delete;
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// The sources of the T completions of a continues_on sender TS that transfers to a scheduler of
// type Sch ([exec.continues.on]/9-12; DECISIONS §17): each completion of the child arrives
// through the value completion of the schedule sender (an exception from decay-copying the
// child's datums too), and the schedule sender's own error and stopped completions.
template <class _Tp, class _TS, class _Sch, class... _Envs>
struct __via_sched_sources {
  using type = __no_attr;
};
template <class _Tp, class _TS, class _Sch, class... _Envs>
  requires(!std::is_void_v<_TS>) && __attr_has_tag<__own_csigs_t<_TS, _Envs...>, _Tp>
struct __via_sched_sources<_Tp, _TS, _Sch, _Envs...> {
  static auto __pick() {
    if constexpr (std::is_same_v<_Tp, set_value_t>) {
      return std::type_identity<__tlist<__src_sched<set_value_t>>>{};
    } else {
      using _CSc = __csigs_of_t<std::decay_t<__child_type<_TS>>, __fwd_env_t<const _Envs&>...>;
      using _CSs = __csigs_of_t<std::execution::schedule_result_t<_Sch&>, __fwd_env_t<const _Envs&>...>;
      if constexpr (!__is_csigs<_CSc> || !__is_csigs<_CSs>)
        return std::type_identity<__no_attr>{};
      else
        return std::type_identity<__srcs_t<
            __src_if<__sigs_count<_Tp, _CSc> != 0 || (std::is_same_v<_Tp, set_error_t> && !__nothrow_decay_copy_sigs<_CSc>), __src_sched<set_value_t>>,
            __src_if<__sigs_count<_Tp, _CSs> != 0, __src_sched<_Tp>>>>{};
    }
  }
  using type = typename decltype(__pick())::type;
};

template <class _Ap, class _Sch, class _Sndr>
struct __pol_continues_on : __pol_child<_Ap> {
  _Sch __ycxx_sch;
  template <class... _Envs>
  using __sched_t = _Sch;
  template <class... _Envs>
  constexpr _Sch __sched(const _Envs&...) const noexcept {
    return __ycxx_sch;
  }
  template <class _Tp, class... _Envs>
  using __sources = typename __via_sched_sources<_Tp, _Sndr, _Sch, _Envs...>::type;
};

template <>
struct __impls_for<std::execution::continues_on_t> : __default_impls {
  template <class _Sch, class _Child>
  static constexpr auto __get_attrs(const _Sch& __sch, const _Child& __child) noexcept {
    using _Ap = __env_member_t<decltype(std::execution::get_env(__child))>;
    using _Pol = __pol_continues_on<_Ap, _Sch, __basic_sender_t<std::execution::continues_on_t, _Sch, _Child>>;
    return __compl_attrs_t<_Pol>{_Pol{{std::execution::get_env(__child)}, __sch}};
  }
  template <class _Sndr, class _Rcvr>
    requires std::execution::sender_in<__child_type<_Sndr>, __fwd_env_t<std::execution::env_of_t<_Rcvr>>>
  static constexpr auto __get_state(_Sndr&& __sndr, _Rcvr& __rcvr) noexcept(
      std::is_nothrow_constructible_v<::__ycxx::__adl_free::__exec_continues_on_state<std::decay_t<__data_type<_Sndr>>, __child_type<_Sndr>, _Rcvr>,
                                      std::decay_t<__data_type<_Sndr>>&, _Rcvr&>) {
    using __sched_t = std::decay_t<__data_type<_Sndr>>;
    auto __sch = static_cast<_Sndr&&>(__sndr).template get<1>();
    return ::__ycxx::__adl_free::__exec_continues_on_state<__sched_t, __child_type<_Sndr>, _Rcvr>(__sch, __rcvr);
  }
  template <class _Index, class _State, class _Rcvr, class _Tag, class... _Args>
  static constexpr void complete(_Index, _State& state, _Rcvr&, _Tag, _Args&&... __args) noexcept {
    using __result_t = __decayed_tuple<_Tag, _Args...>;
    constexpr bool nothrow = (std::is_nothrow_constructible_v<std::decay_t<_Args>, _Args> && ...);
    if constexpr (nothrow || !__cfg::exceptions) {
      state.__async_result.template emplace<__result_t>(_Tag(), static_cast<_Args&&>(__args)...);
    } else {
      try {
        state.__async_result.template emplace<__result_t>(_Tag(), static_cast<_Args&&>(__args)...);
      } catch (...) {
        state.__async_result.template emplace<std::tuple<set_error_t, std::exception_ptr>>(std::execution::set_error, std::current_exception());
      }
    }
    std::execution::start(state.__op_state);
  }
  template <class _Sndr, class... _Env>
  using __csigs = typename __continues_on_sigs<std::decay_t<__data_type<_Sndr>>, __child_sigs_t<_Sndr, _Env...>, _Env...>::type;
};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
struct continues_on_t : __ycxx::__detail::__exec::__pipeable_adaptor<continues_on_t, 1, __ycxx::__detail::__exec::__bind_scheduler> {
  using __ycxx::__detail::__exec::__pipeable_adaptor<continues_on_t, 1, __ycxx::__detail::__exec::__bind_scheduler>::operator();
  template <sender _Sndr, scheduler _Sch>
  constexpr auto operator()(_Sndr&& __sndr, _Sch&& __sch) const {
    return __ycxx::__detail::__exec::__make_sender(*this, static_cast<_Sch&&>(__sch), schedule_from(static_cast<_Sndr&&>(__sndr)));
  }
};
inline constexpr continues_on_t continues_on{};
}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// [exec.starts.on], [exec.on]
namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
struct starts_on_t {
  template <scheduler _Sch, sender _Sndr>
  constexpr auto operator()(_Sch&& __sch, _Sndr&& __sndr) const {
    return __ycxx::__detail::__exec::__make_sender(*this, static_cast<_Sch&&>(__sch), static_cast<_Sndr&&>(__sndr));
  }
  template <class _OutSndr, class _Env>
    requires is_same_v<tag_of_t<_OutSndr>, starts_on_t>
  static constexpr auto transform_sender(set_value_t, _OutSndr&& __out_sndr, const _Env&) {
    using _Sp = decay_t<__ycxx::__detail::__exec::__child_type<_OutSndr>>;
    return let_value(continues_on(just(), static_cast<_OutSndr&&>(__out_sndr).template get<1>()),
                     [__sndr = static_cast<_OutSndr&&>(__out_sndr).template get<2>()]() mutable noexcept(is_nothrow_move_constructible_v<_Sp>) {
                       return static_cast<_Sp&&>(__sndr);
                     });
  }
};
inline constexpr starts_on_t starts_on{};
}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// starts_on(sch, sndr) as its let_value form ([exec.starts.on]/4): the domains of sndr's
// completions, sndr asked in the environment the let-state gives it (the start scheduler of
// continues_on(just(), sch), [exec.let]/2, /9; as for let, a domain only), and the schedule
// sender's error and stopped completions; moving or connecting sndr can throw on the
// scheduler's agent.
template <class _Ap, class _Sch, class _Child>
struct __pol_starts_on : __pol_child<_Ap> {
  _Sch __ycxx_sch;
  using _Cont = decltype(std::execution::continues_on(std::execution::just(), std::declval<const _Sch&>()));
  template <class _Env>
  using __sndr_env_t = __join_env_t<const __let_env_t<set_value_t, _Cont, _Env>&, __fwd_env_t<_Env>>;
  template <class... _Envs>
  using __sched_t = _Sch;
  template <class... _Envs>
  constexpr _Sch __sched(const _Envs&...) const noexcept {
    return __ycxx_sch;
  }
};
template <class _Tp, class _Pol, class _Sndr, class _Child, class _Sch, class... _Envs>
struct __starts_on_sources {
  using type = __no_attr;
};
template <class _Tp, class _Pol, class _Sndr, class _Child, class _Sch, class _Env>
  requires __attr_has_tag<__own_csigs_t<_Sndr, _Env>, _Tp>
struct __starts_on_sources<_Tp, _Pol, _Sndr, _Child, _Sch, _Env> {
  using _CE = typename _Pol::template __sndr_env_t<_Env>;
  using _CSc = __csigs_of_t<_Child, _CE>;
  using _CSs = __csigs_of_t<std::execution::schedule_result_t<_Sch&>, __fwd_env_t<const _Env&>>;
  static auto __pick() {
    if constexpr (!__is_csigs<_CSc> || !__is_csigs<_CSs>)
      return std::type_identity<__no_attr>{};
    else
      return std::type_identity<__srcs_t<
          __src_if<__sigs_count<_Tp, _CSc> != 0, __src_dom<__compl_domain_of_t<_Tp, _Child, _CE>>>,
          __src_if<!std::is_same_v<_Tp, set_value_t> && __sigs_count<_Tp, _CSs> != 0, __src_sched<_Tp>>,
          __src_if<std::is_same_v<_Tp, set_error_t> &&
                       !(std::is_nothrow_move_constructible_v<_Child> && ::__ycxx::__detail::__exec::__nothrow_connect_in<_Child, _CE>()),
                   __src_sched<set_value_t>>>>{};
  }
  using type = typename decltype(__pick())::type;
};
template <class _Ap, class _Sch, class _Child>
struct __pol_starts_on_full : __pol_starts_on<_Ap, _Sch, _Child> {
  template <class _Tp, class... _Envs>
  using __sources = typename __starts_on_sources<_Tp, __pol_starts_on<_Ap, _Sch, _Child>,
                                                 __basic_sender_t<std::execution::starts_on_t, _Sch, _Child>, _Child, _Sch, _Envs...>::type;
};

template <>
struct __impls_for<std::execution::starts_on_t> : __default_impls {
  template <class _Sch, class _Child>
  static constexpr auto __get_attrs(const _Sch& __sch, const _Child& __child) noexcept {
    using _Ap = __env_member_t<decltype(std::execution::get_env(__child))>;
    using _Pol = __pol_starts_on_full<_Ap, _Sch, _Child>;
    return __compl_attrs_t<_Pol>{_Pol{{{std::execution::get_env(__child)}, __sch}}};
  }
  template <class _Sndr, class... _Env>
  using __csigs = __sigs_concat_t<__child_sigs_t<_Sndr, _Env...>, typename __schedule_non_value_sigs<std::decay_t<__data_type<_Sndr>>, _Env...>::type>;
};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
struct on_t {
  template <scheduler _Sch, sender _Sndr>
    requires(!__ycxx::__detail::__exec::__pipeable_closure<_Sndr>)
  constexpr auto operator()(_Sch&& __sch, _Sndr&& __sndr) const {
    return __ycxx::__detail::__exec::__make_sender(*this, static_cast<_Sch&&>(__sch), static_cast<_Sndr&&>(__sndr));
  }
  template <sender _Sndr, scheduler _Sch, __ycxx::__detail::__exec::__pipeable_closure _Closure>
  constexpr auto operator()(_Sndr&& __sndr, _Sch&& __sch, _Closure&& __closure) const {
    return __ycxx::__detail::__exec::__make_sender(*this, __ycxx::__detail::__exec::__make_product(static_cast<_Sch&&>(__sch), static_cast<_Closure&&>(__closure)),
                                           static_cast<_Sndr&&>(__sndr));
  }
  // on(sch, closure): the pipeable partial application of on(sndr, sch, closure).
  template <scheduler _Sch, __ycxx::__detail::__exec::__pipeable_closure _Closure>
    requires(!sender<_Closure>)
  constexpr auto operator()(_Sch&& __sch, _Closure&& __closure) const {
    return __ycxx::__detail::__exec::__bind_closure(*this, static_cast<_Sch&&>(__sch), static_cast<_Closure&&>(__closure));
  }

  template <class _OutSndr, class _Env>
    requires is_same_v<tag_of_t<_OutSndr>, on_t>
  static constexpr auto transform_sender(set_value_t, _OutSndr&& __out_sndr, const _Env& env) {
    auto&& data = static_cast<_OutSndr&&>(__out_sndr).template get<1>();
    auto&& __child = static_cast<_OutSndr&&>(__out_sndr).template get<2>();
    if constexpr (scheduler<decltype(data)>) {
      auto __orig_sch = __ycxx::__detail::__exec::__call_with_default(get_start_scheduler, __ycxx::__adl_free::__exec_not_a_scheduler(), env);
      return continues_on(starts_on(std::forward_like<_OutSndr>(data), std::forward_like<_OutSndr>(__child)), static_cast<decltype(__orig_sch)&&>(__orig_sch));
    } else {
      auto __orig_sch = __ycxx::__detail::__exec::__call_with_default(get_completion_scheduler<set_value_t>, __ycxx::__adl_free::__exec_not_a_scheduler(),
                                                            get_env(__child), env);
      return continues_on(std::forward_like<_OutSndr>(data.template get<1>())(
                              continues_on(std::forward_like<_OutSndr>(__child), std::forward_like<_OutSndr>(data.template get<0>()))),
                          static_cast<decltype(__orig_sch)&&>(__orig_sch));
    }
  }
};
inline constexpr on_t on{};
}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// on's completions depend on the scheduler the receiver's environment names; it is always
// transformed (to continues_on/starts_on) before it is connected.
// on's attributes: those of the continues_on sender its transformation produces in the
// environment ([exec.on]/6), which transfers to the scheduler the environment names
// (get_start_scheduler) or to the child's value completion scheduler; none without an
// environment.
template <class _Sndr, class... _Envs>
struct __on_lowered {
  using type = void;
  using __sch = void;
};
template <class _Sndr, class _Env>
concept __on_has_orig_sched =
    (std::execution::scheduler<std::decay_t<__data_type<_Sndr>>> && requires(const _Env& env) { std::execution::get_start_scheduler(env); }) ||
    (!std::execution::scheduler<std::decay_t<__data_type<_Sndr>>> &&
     requires(const _Env& env) { std::execution::get_completion_scheduler<set_value_t>(std::execution::get_env(std::declval<__child_type<_Sndr>>()), env); });
template <class _Sndr, class _Env>
  requires __on_has_orig_sched<_Sndr, _Env>
struct __on_lowered<_Sndr, _Env> {
  using type = decltype(std::execution::on_t::transform_sender(set_value_t(), std::declval<_Sndr>(), std::declval<const _Env&>()));
  using __sch = std::decay_t<__data_type<type>>;
};
template <class _Ap, class _Data, class _Child>
struct __pol_on : __pol_child<_Ap> {
  using _Sndr = __basic_sender_t<std::execution::on_t, _Data, _Child>;
  template <class... _Envs>
  using __sched_t = typename __on_lowered<_Sndr, _Envs...>::__sch;
  template <class _Env>
  constexpr auto __sched(const _Env& env) const noexcept {
    if constexpr (std::execution::scheduler<_Data>)
      return ::__ycxx::__detail::__exec::__call_with_default(std::execution::get_start_scheduler, ::__ycxx::__adl_free::__exec_not_a_scheduler(), env);
    else
      return ::__ycxx::__detail::__exec::__call_with_default(std::execution::get_completion_scheduler<set_value_t>,
                                                             ::__ycxx::__adl_free::__exec_not_a_scheduler(), this->__fwd(), env);
  }
  template <class _Tp, class... _Envs>
  using __sources = typename __via_sched_sources<_Tp, typename __on_lowered<_Sndr, _Envs...>::type, __sched_t<_Envs...>, _Envs...>::type;
};

template <>
struct __impls_for<std::execution::on_t> : __default_impls {
  template <class _Data, class _Child>
  static constexpr auto __get_attrs(const _Data&, const _Child& __child) noexcept {
    using _Ap = __env_member_t<decltype(std::execution::get_env(__child))>;
    using _Pol = __pol_on<_Ap, _Data, _Child>;
    return __compl_attrs_t<_Pol>{_Pol{{std::execution::get_env(__child)}}};
  }
  template <class _Sndr, class... _Env>
  struct __sigs {
    using type = __dependent_sigs;
  };
  template <class _Sndr, class _Env>
  struct __sigs<_Sndr, _Env> {
    using type = __invalid_sigs<__environment_has_no_start_scheduler, _Sndr, _Env>;
  };
  template <class _Sndr, class... _Env>
  using __csigs = typename __sigs<_Sndr, _Env...>::type;
};
}}} // namespace __ycxx::__detail::__exec

// ---------------------------------------------------------------------------------------------
// [exec.affine]
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// UNSTOPPABLE-SCHEDULER(sch) ([exec.affine]/4)
template <class _Sch>
struct __exec_unstoppable_scheduler {
  using scheduler_concept = std::execution::scheduler_tag;
  _Sch __sch;
  constexpr auto schedule() const noexcept(noexcept(std::execution::unstoppable(std::execution::schedule(__sch)))) {
    return std::execution::unstoppable(std::execution::schedule(__sch));
  }
  template <class _Qp, class... _As>
    requires requires(const _Sch& s, _Qp __q, _As&&... __as) { s.query(__q, static_cast<_As&&>(__as)...); }
  constexpr decltype(auto) query(_Qp __q, _As&&... __as) const noexcept(noexcept(__sch.query(__q, static_cast<_As&&>(__as)...))) {
    return __sch.query(__q, static_cast<_As&&>(__as)...);
  }
  friend constexpr bool operator==(const __exec_unstoppable_scheduler& a, const __exec_unstoppable_scheduler& b) noexcept {
    return a.__sch == b.__sch;
  }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// infallible-scheduler<Sch, Env> ([exec.sched]/8)
template <class _Sch, class _Env>
concept __infallible_scheduler =
    std::execution::scheduler<_Sch> &&
    (std::same_as<std::execution::completion_signatures<set_value_t()>, __csigs_of_t<decltype(std::execution::schedule(std::declval<_Sch>())), _Env>> ||
     (!std::unstoppable_token<std::stop_token_of_t<_Env>> &&
      (std::same_as<std::execution::completion_signatures<set_value_t(), set_stopped_t()>,
                    __csigs_of_t<decltype(std::execution::schedule(std::declval<_Sch>())), _Env>> ||
       std::same_as<std::execution::completion_signatures<set_stopped_t(), set_value_t()>,
                    __csigs_of_t<decltype(std::execution::schedule(std::declval<_Sch>())), _Env>>)));

template <class _Child>
concept __has_affine_member = requires(_Child&& c) { static_cast<_Child&&>(c).affine(); };

template <class _Sndr, class... _Env>
struct __affine_sigs {
  static auto __pick() {
    using _Child = __child_type<_Sndr>;
    if constexpr (__has_affine_member<_Child>) {
      return std::type_identity<__csigs_of_t<decltype(std::declval<_Child>().affine()), _Env...>>{};
    } else if constexpr (sizeof...(_Env) == 0) {
      return std::type_identity<__dependent_sigs>{};
    } else if constexpr (!requires(const _Env...[0]& e) { std::execution::get_start_scheduler(e); }) {
      return std::type_identity<__invalid_sigs<__environment_has_no_start_scheduler, _Sndr, _Env...>>{};
    } else {
      return std::type_identity<__invalid_sigs<__start_scheduler_is_not_infallible, _Sndr, _Env...>>{};
    }
  }
  using type = typename decltype(__pick())::type;
};

}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
struct affine_t : sender_adaptor_closure<affine_t> {
  template <sender _Sndr>
  constexpr auto operator()(_Sndr&& __sndr) const {
    return __ycxx::__detail::__exec::__make_sender(*this, env<>(), static_cast<_Sndr&&>(__sndr));
  }
  // [exec.affine]/5 (a set_value transformation, like the other lowered adaptors). Without a
  // start scheduler that is infallible in the environment, the sender stays as it is and its
  // completion signatures report the error ([exec.affine]/7).
  template <class _Sndr, class _Env>
    requires is_same_v<tag_of_t<_Sndr>, affine_t> &&
             (__ycxx::__detail::__exec::__has_affine_member<__ycxx::__detail::__exec::__child_type<_Sndr>> ||
              requires(const _Env& __ev) {
                requires __ycxx::__detail::__exec::__infallible_scheduler<decltype(get_start_scheduler(__ev)), _Env>;
              })
  static constexpr auto transform_sender(set_value_t, _Sndr&& __sndr, const _Env& __ev) {
    if constexpr (__ycxx::__detail::__exec::__has_affine_member<__ycxx::__detail::__exec::__child_type<_Sndr>>) {
      return static_cast<_Sndr&&>(__sndr).template get<2>().affine();
    } else {
      using _Sp = decay_t<decltype(get_start_scheduler(__ev))>;
      return continues_on(static_cast<_Sndr&&>(__sndr).template get<2>(), __ycxx::__adl_free::__exec_unstoppable_scheduler<_Sp>{get_start_scheduler(__ev)});
    }
  }
};
inline constexpr affine_t affine{};
}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
// affine's attributes: given an environment, those of continues_on(sndr,
// UNSTOPPABLE-SCHEDULER(get_start_scheduler(env))) ([exec.affine]/5, /7); a child with an
// affine() member keeps its own.
template <class _Sndr, class... _Envs>
struct __affine_lowered {
  using type = void;
  using __sch = void;
};
template <class _Sndr, class _Env>
  requires requires { std::execution::affine_t::transform_sender(set_value_t(), std::declval<_Sndr>(), std::declval<const _Env&>()); }
struct __affine_lowered<_Sndr, _Env> {
  using type = decltype(std::execution::affine_t::transform_sender(set_value_t(), std::declval<_Sndr>(), std::declval<const _Env&>()));
  using __sch = std::decay_t<__data_type<type>>;
};
template <class _Ap, class _Data, class _Child>
struct __pol_affine : __pol_child<_Ap> {
  using _Sndr = __basic_sender_t<std::execution::affine_t, _Data, _Child>;
  template <class... _Envs>
  using __sched_t = typename __affine_lowered<_Sndr, _Envs...>::__sch;
  template <class _Env>
  constexpr auto __sched(const _Env& env) const noexcept {
    return __sched_t<_Env>{std::execution::get_start_scheduler(env)};
  }
  template <class _Tp, class... _Envs>
  using __sources = typename __via_sched_sources<_Tp, typename __affine_lowered<_Sndr, _Envs...>::type, __sched_t<_Envs...>, _Envs...>::type;
};

template <>
struct __impls_for<std::execution::affine_t> : __default_impls {
  template <class _Data, class _Child>
  static constexpr decltype(auto) __get_attrs(const _Data& data, const _Child& __child) noexcept {
    if constexpr (__has_affine_member<_Child>) {
      return __default_impls::__get_attrs(data, __child);
    } else {
      using _Ap = __env_member_t<decltype(std::execution::get_env(__child))>;
      using _Pol = __pol_affine<_Ap, _Data, _Child>;
      return __compl_attrs_t<_Pol>{_Pol{{std::execution::get_env(__child)}}};
    }
  }
  template <class _Sndr, class... _Env>
  using __csigs = typename __affine_sigs<_Sndr, _Env...>::type;
};
}}} // namespace __ycxx::__detail::__exec

// ---------------------------------------------------------------------------------------------
// [exec.when.all]
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// make-when-all-env(stop_src, env) ([exec.when.all]/5)
template <class _Env>
struct __exec_when_all_env {
  const std::inplace_stop_source* __stop_src;
  _Env env;
  std::inplace_stop_token query(std::get_stop_token_t) const noexcept { return __stop_src->get_token(); }
  template <::__ycxx::__detail::__exec::__forwarding_query_c _Qp, class... _As>
    requires(!std::is_same_v<_Qp, std::get_stop_token_t>) && ::__ycxx::__detail::__exec::__has_query<std::remove_cvref_t<_Env>, _Qp, _As...>
  constexpr decltype(auto) query(_Qp __q, _As&&... __as) const
      noexcept(noexcept(::__ycxx::__detail::__exec::__as_const_ref(env).query(__q, static_cast<_As&&>(__as)...))) {
    return ::__ycxx::__detail::__exec::__as_const_ref(env).query(__q, static_cast<_As&&>(__as)...);
  }
};
// on-stop-request ([exec.snd.expos]/16)
struct __exec_on_stop_request {
  std::inplace_stop_source& __stop_src;
  void operator()() noexcept { __stop_src.request_stop(); }
};
struct __exec_none_such {};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _Env>
constexpr auto __make_when_all_env(std::inplace_stop_source& __stop_src, _Env&& env) noexcept {
  return ::__ycxx::__adl_free::__exec_when_all_env<__env_member_t<_Env>>{&__stop_src, static_cast<_Env&&>(env)};
}
template <class _Env>
using __when_all_env_t = decltype(::__ycxx::__detail::__exec::__make_when_all_env(std::declval<std::inplace_stop_source&>(), std::declval<_Env>()));

// The completion signatures of when_all(sndrs...) ([exec.when.all]/9, /15).
template <class _Sig>
struct __when_all_error_sig {
  using type = __no_sigs;
};
template <class _Ep>
struct __when_all_error_sig<set_error_t(_Ep)> {
  using type = std::execution::completion_signatures<set_error_t(std::decay_t<_Ep>)>;
};
template <class _Sig>
using __when_all_error_sig_t = typename __when_all_error_sig<_Sig>::type;
template <class _ArgLists>
struct __only_args {
  using type = __tlist<>;
};
template <class _Args>
struct __only_args<__tlist<_Args>> {
  using type = _Args;
};
template <class _Values>
struct __when_all_value_sig;
template <class... _Vs>
struct __when_all_value_sig<__tlist<_Vs...>> {
  using type = std::execution::completion_signatures<set_value_t(std::decay_t<_Vs>...)>;
};

template <class... _ChildSigs>
struct __when_all_sigs {
  template <class _CS>
  using __non_values = __sigs_map_t<_CS, __when_all_error_sig_t>;

  static auto __pick() {
    if constexpr (!(__is_csigs<_ChildSigs> && ...)) {
      return std::type_identity<__sigs_concat_t<_ChildSigs...>>{};
    } else if constexpr (((__sigs_count<set_value_t, _ChildSigs> >= 2) || ...)) {
      return std::type_identity<__invalid_sigs<__when_all_child_has_more_than_one_value_completion, _ChildSigs...>>{};
    } else if constexpr (!(__decay_copyable_sigs<_ChildSigs> && ...)) {
      return std::type_identity<__invalid_sigs<__result_datums_not_decay_copyable, _ChildSigs...>>{};
    } else {
      constexpr bool __all_values = ((__sigs_count<set_value_t, _ChildSigs> == 1) && ...);
      using __value_sigs = std::conditional_t<
          __all_values,
          typename __when_all_value_sig<typename __tlist_concat<typename __only_args<__sigs_args_t<set_value_t, _ChildSigs>>::type...>::type>::type,
          __no_sigs>;
      constexpr bool nothrow = (__nothrow_decay_copy_sigs<_ChildSigs> && ...);
      constexpr bool __stopped = ((__sigs_count<set_stopped_t, _ChildSigs> != 0) || ...);
      return std::type_identity<__sigs_concat_t<__value_sigs, __non_values<_ChildSigs>..., std::conditional_t<nothrow, __no_sigs, __eptr_sigs>,
                                              std::conditional_t<__stopped, std::execution::completion_signatures<set_stopped_t()>, __no_sigs>>>{};
    }
  }
  using type = typename decltype(__pick())::type;
};

// disposition ([exec.when.all]/11), as plain values: the __atomic builtins take no enumerations.
struct __when_all_disposition {
  static constexpr unsigned char __started = 0, error = 1, __stopped = 2;
};

// values_tuple ([exec.when.all]/13): the values of every child as optionals, or tuple<> when a
// child has no single value completion.
template <class _Env, class... _Children>
struct __when_all_values {
  using type = std::tuple<>;
};
template <class _Env, class... _Children>
  requires(requires { typename std::execution::value_types_of_t<_Children, _Env, __decayed_tuple, std::optional>; } && ...)
struct __when_all_values<_Env, _Children...> {
  using type = std::tuple<std::execution::value_types_of_t<_Children, _Env, __decayed_tuple, std::optional>...>;
};

template <class _Rcvr, class... _Children>
struct __when_all_types {
  using __env_t = __when_all_env_t<std::execution::env_of_t<_Rcvr>>;
  using __values_tuple = typename __when_all_values<__env_t, _Children...>::type;
  using __all_sigs = __sigs_concat_t<std::execution::completion_signatures_of_t<_Children, __env_t>...>;
  using __copy_fail = std::conditional_t<__nothrow_decay_copy_sigs<__all_sigs>, ::__ycxx::__adl_free::__exec_none_such, std::exception_ptr>;
  template <class _Args>
  struct __decayed_error;
  template <class _Ep>
  struct __decayed_error<__tlist<_Ep>> {
    using type = std::decay_t<_Ep>;
  };
  template <class _Lists>
  struct __errors_variant_of;
  template <class... _Lists>
  struct __errors_variant_of<__tlist<_Lists...>> {
    using type = __apply_unique_t<std::variant, ::__ycxx::__adl_free::__exec_none_such, __copy_fail, typename __decayed_error<_Lists>::type...>;
  };
  using __errors_variant = typename __errors_variant_of<__sigs_args_t<set_error_t, __all_sigs>>::type;
  static constexpr bool sends_stopped = __sigs_count<set_stopped_t, __all_sigs> != 0;
  using stop_callback = std::stop_callback_for_t<std::stop_token_of_t<std::execution::env_of_t<_Rcvr>>, ::__ycxx::__adl_free::__exec_on_stop_request>;
};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
template <class _Rcvr, class... _Children>
struct __exec_when_all_state {
  using __types = ::__ycxx::__detail::__exec::__when_all_types<_Rcvr, _Children...>;
  using __disposition = ::__ycxx::__detail::__exec::__when_all_disposition;

  std::size_t count = sizeof...(_Children);
  std::inplace_stop_source __stop_src{};
  unsigned char __disp = __disposition::__started;
  typename __types::__errors_variant __errors{};
  typename __types::__values_tuple values{};
  std::optional<typename __types::stop_callback> __on_stop{std::nullopt};

  __exec_when_all_state() = default;
  __exec_when_all_state(__exec_when_all_state&&) = delete;

  void arrive(_Rcvr& __rcvr) noexcept {
    if (__atomic_sub_fetch(&count, 1, __ATOMIC_ACQ_REL) == 0)
      complete(__rcvr);
  }
  void complete(_Rcvr& __rcvr) noexcept {
    const unsigned char d = __atomic_load_n(&__disp, __ATOMIC_ACQUIRE);
    if (d == __disposition::__started) {
      if constexpr (!std::is_same_v<typename __types::__values_tuple, std::tuple<>>) {
        __on_stop.reset();
        std::apply(
            [&](auto&... __opts) noexcept {
              std::apply(
                  [&](auto&... t) noexcept {
                    std::execution::set_value(static_cast<_Rcvr&&>(__rcvr), static_cast<std::remove_reference_t<decltype(t)>&&>(t)...);
                  },
                         std::tuple_cat(std::apply([](auto&... __v) noexcept { return std::tuple<decltype(__v)&...>(__v...); }, *__opts)...));
            },
            values);
      }
    } else if (d == __disposition::error) {
      __on_stop.reset();
      std::visit(
          [&]<class _Error>(_Error& error) noexcept {
            if constexpr (!std::is_same_v<_Error, __exec_none_such>)
              std::execution::set_error(static_cast<_Rcvr&&>(__rcvr), static_cast<_Error&&>(error));
          },
          __errors);
    } else {
      if constexpr (__types::sends_stopped) {
        __on_stop.reset();
        std::execution::set_stopped(static_cast<_Rcvr&&>(__rcvr));
      }
    }
  }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
struct when_all_t;
struct when_all_with_variant_t;
}} // namespace std::execution

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <class _Rcvr>
struct __when_all_make_state {
  template <class _Tag, class _Data, class... _Children>
  auto operator()(_Tag, _Data, _Children&&...) const {
    return ::__ycxx::__adl_free::__exec_when_all_state<_Rcvr, _Children&&...>();
  }
};

// The sources of when_all's T completions ([exec.when.all]/15-17; DECISIONS §17). The last
// child to complete completes the operation, on its agent: value from every child's value
// completion; error from every child's errors, from its values when decay-copying them can
// throw (TRY-EMPLACE-VALUE), and from every completion of a child when another child can fail;
// stopped from every child's stopped completions, and from the value completions of a child
// when another child can stop (an error completion makes the disposition error).
template <class _CS>
inline constexpr bool __values_nothrow_copy = true;
template <class... _Sigs>
inline constexpr bool __values_nothrow_copy<std::execution::completion_signatures<_Sigs...>> =
    ((!std::is_same_v<typename __sig_tag<_Sigs>::type, set_value_t> || __nothrow_decay_copy_sig<_Sigs>) && ...);

template <class _Tp, class _OwnCS, class _Children, class _Is, class... _Envs>
struct __when_all_sources {
  using type = __no_attr;
};
template <class _Tp, class _OwnCS, class... _Cs, std::size_t... _Is, class... _Envs>
  requires __attr_has_tag<_OwnCS, _Tp> && (__is_csigs<__csigs_of_t<_Cs, __when_all_env_t<const _Envs&>...>> && ...)
struct __when_all_sources<_Tp, _OwnCS, __tlist<_Cs...>, std::index_sequence<_Is...>, _Envs...> {
  template <class _Cc>
  using _CS = __csigs_of_t<_Cc, __when_all_env_t<const _Envs&>...>;
  static constexpr bool __copy_throws[] = {!__values_nothrow_copy<_CS<_Cs>>...};
  static constexpr bool __can_fail[] = {(__sigs_count<set_error_t, _CS<_Cs>> != 0 || !__values_nothrow_copy<_CS<_Cs>>)...};
  static constexpr bool __can_stop[] = {(__sigs_count<set_stopped_t, _CS<_Cs>> != 0)...};
  static constexpr bool __another(const bool (&__a)[sizeof...(_Cs)], std::size_t __i) noexcept {
    for (std::size_t __j = 0; __j != sizeof...(_Cs); ++__j)
      if (__j != __i && __a[__j])
        return true;
    return false;
  }
  template <std::size_t _Ip, class _Cc, class _Sp>
  static constexpr bool __use = __sigs_count<_Sp, _CS<_Cc>> != 0 &&
                                (std::is_same_v<_Tp, set_value_t>   ? std::is_same_v<_Sp, set_value_t>
                                 : std::is_same_v<_Tp, set_error_t> ? (std::is_same_v<_Sp, set_error_t> ||
                                                                       (std::is_same_v<_Sp, set_value_t> && __copy_throws[_Ip]) ||
                                                                       __another(__can_fail, _Ip))
                                                                    : (std::is_same_v<_Sp, set_stopped_t> ||
                                                                       (std::is_same_v<_Sp, set_value_t> && __another(__can_stop, _Ip))));
  using type = __srcs_t<__srcs_t<__src_if<__use<_Is, _Cs, set_value_t>, __src_child<_Is, set_value_t>>,
                                 __src_if<__use<_Is, _Cs, set_error_t>, __src_child<_Is, set_error_t>>,
                                 __src_if<__use<_Is, _Cs, set_stopped_t>, __src_child<_Is, set_stopped_t>>>...>;
};

// The children's attributes; each child asked in when-all-env ([exec.when.all]/5-6, /10).
// Children are sender types, Ap their attributes (into_variant's for when_all_with_variant).
template <class _Sndr, class _Children, class... _Ap>
struct __pol_when_all {
  std::tuple<_Ap...> __ycxx_children;
  constexpr decltype(auto) __fwd() const noexcept
    requires(sizeof...(_Ap) == 1)
  {
    return __as_const_ref(std::get<0>(__ycxx_children));
  }
  template <std::size_t _Ip>
  constexpr decltype(auto) __child_attrs() const noexcept {
    return __as_const_ref(std::get<_Ip>(__ycxx_children));
  }
  template <std::size_t _Ip>
  using __child_attrs_t = std::remove_cvref_t<_Ap...[_Ip]>;
  template <std::size_t, class _Env>
  using __child_env_t = __when_all_env_t<const _Env&>;
  template <std::size_t, class _Fn, class... _Envs>
  static constexpr auto __with_child_env(_Fn __fn, const _Envs&... __envs) noexcept {
    std::inplace_stop_source __src;
    return __fn(::__ycxx::__detail::__exec::__make_when_all_env(__src, __envs)...);
  }
  template <class _Tp, class... _Envs>
  using __sources =
      typename __when_all_sources<_Tp, __own_csigs_t<_Sndr, _Envs...>, _Children, std::index_sequence_for<_Ap...>, _Envs...>::type;
};

template <>
struct __impls_for<std::execution::when_all_t> : __default_impls {
  template <class _Data, class... _Child>
  static constexpr auto __get_attrs(const _Data&, const _Child&... __child) noexcept {
    using _Pol = __pol_when_all<__basic_sender_t<std::execution::when_all_t, _Data, _Child...>, __tlist<_Child...>,
                                __env_member_t<decltype(std::execution::get_env(__child))>...>;
    return __compl_attrs_t<_Pol>{_Pol{{std::execution::get_env(__child)...}}};
  }
  template <class _Index, class _State, class _Rcvr>
  static constexpr auto get_env(_Index, _State& state, const _Rcvr& __rcvr) noexcept {
    return ::__ycxx::__detail::__exec::__make_when_all_env(state.__stop_src, std::execution::get_env(__rcvr));
  }
  template <class _Sndr, class _Rcvr>
  static constexpr auto __get_state(_Sndr&& __sndr, _Rcvr&) noexcept(noexcept(static_cast<_Sndr&&>(__sndr).apply(__when_all_make_state<_Rcvr>()))) {
    return static_cast<_Sndr&&>(__sndr).apply(__when_all_make_state<_Rcvr>());
  }
  template <class _State, class _Rcvr, class... _Ops>
  static constexpr void start(_State& state, _Rcvr& __rcvr, _Ops&... __ops) noexcept {
    state.__on_stop.emplace(std::get_stop_token(std::execution::get_env(__rcvr)), ::__ycxx::__adl_free::__exec_on_stop_request{state.__stop_src});
    (std::execution::start(__ops), ...);
  }
  template <class _Index, class _State, class _Rcvr, class _Set, class... _Args>
  static constexpr void complete(_Index, _State& state, _Rcvr& __rcvr, _Set, _Args&&... __args) noexcept {
    using __disposition = __when_all_disposition;
    if constexpr (std::is_same_v<_Set, set_error_t>) {
      if (__atomic_exchange_n(&state.__disp, __disposition::error, __ATOMIC_ACQ_REL) != __disposition::error) {
        state.__stop_src.request_stop();
        using _Ep = std::decay_t<_Args...[0]>;
        if constexpr (std::is_nothrow_constructible_v<_Ep, _Args...> || !__cfg::exceptions) {
          state.__errors.template emplace<_Ep>(static_cast<_Args&&>(__args)...);
        } else {
          try {
            state.__errors.template emplace<_Ep>(static_cast<_Args&&>(__args)...);
          } catch (...) {
            state.__errors.template emplace<std::exception_ptr>(std::current_exception());
          }
        }
      }
    } else if constexpr (std::is_same_v<_Set, set_stopped_t>) {
      unsigned char expected = __disposition::__started;
      if (__atomic_compare_exchange_n(&state.__disp, &expected, __disposition::__stopped, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
        state.__stop_src.request_stop();
    } else if constexpr (!std::is_same_v<decltype(_State::values), std::tuple<>>) {
      if (__atomic_load_n(&state.__disp, __ATOMIC_ACQUIRE) == __disposition::__started) {
        auto& __opt = std::get<_Index::value>(state.values);
        if constexpr (std::is_nothrow_constructible_v<__decayed_tuple<_Args...>, _Args...> || !__cfg::exceptions) {
          __opt.emplace(static_cast<_Args&&>(__args)...);
        } else {
          try {
            __opt.emplace(static_cast<_Args&&>(__args)...);
          } catch (...) {
            complete(_Index(), state, __rcvr, std::execution::set_error, std::current_exception());
            return;
          }
        }
      }
    }
    state.arrive(__rcvr);
  }
  template <class _Sndr, class _Is, class... _Env>
  struct __sigs;
  template <class _Sndr, std::size_t... _Is, class... _Env>
  struct __sigs<_Sndr, std::index_sequence<_Is...>, _Env...> {
    using type = typename __when_all_sigs<__csigs_of_t<__child_type<_Sndr, _Is>, __when_all_env_t<_Env>...>...>::type;
  };
  template <class _Sndr, class... _Env>
  using __csigs = typename __sigs<_Sndr, __indices_for<_Sndr>, _Env...>::type;
};

template <>
struct __impls_for<std::execution::when_all_with_variant_t> : __default_impls {
  // when_all of into_variant of each child ([exec.when.all]/1, /19).
  template <class _Data, class... _Child>
  static constexpr auto __get_attrs(const _Data&, const _Child&... __child) noexcept {
    using _Pol = __pol_when_all<
        __basic_sender_t<std::execution::when_all_with_variant_t, _Data, _Child...>,
        __tlist<__basic_sender_t<std::execution::into_variant_t, __empty_data, _Child>...>,
        decltype(__impls_for<std::execution::into_variant_t>::__get_attrs(__empty_data(), __child))...>;
    return __compl_attrs_t<_Pol>{_Pol{{__impls_for<std::execution::into_variant_t>::__get_attrs(__empty_data(), __child)...}}};
  }
  template <class _Sndr, class _Is, class... _Env>
  struct __sigs;
  template <class _Sndr, std::size_t... _Is, class... _Env>
  struct __sigs<_Sndr, std::index_sequence<_Is...>, _Env...> {
    using type = typename __impls_for<std::execution::when_all_t>::template __csigs<
        __basic_sender_t<std::execution::when_all_t, __empty_data,
                       __basic_sender_t<std::execution::into_variant_t, __empty_data, std::decay_t<__child_type<_Sndr, _Is>>>...>,
        _Env...>;
  };
  template <class _Sndr, class... _Env>
  using __csigs = typename __sigs<_Sndr, __indices_for<_Sndr>, _Env...>::type;
};
}}} // namespace __ycxx::__detail::__exec

namespace [[__gnu__::__visibility__("hidden")]] std { namespace execution {
struct when_all_t {
  template <sender... _Sndrs>
    requires(sizeof...(_Sndrs) != 0)
  constexpr auto operator()(_Sndrs&&... __sndrs) const noexcept((is_nothrow_constructible_v<decay_t<_Sndrs>, _Sndrs> && ...)) {
    return __ycxx::__detail::__exec::__make_sender(*this, __ycxx::__detail::__exec::__empty_data(), static_cast<_Sndrs&&>(__sndrs)...);
  }
};
inline constexpr when_all_t when_all{};

struct when_all_with_variant_t {
  template <sender... _Sndrs>
    requires(sizeof...(_Sndrs) != 0)
  constexpr auto operator()(_Sndrs&&... __sndrs) const noexcept((is_nothrow_constructible_v<decay_t<_Sndrs>, _Sndrs> && ...)) {
    return __ycxx::__detail::__exec::__make_sender(*this, __ycxx::__detail::__exec::__empty_data(), static_cast<_Sndrs&&>(__sndrs)...);
  }
  template <class _Sndr, class _Env>
    requires is_same_v<tag_of_t<_Sndr>, when_all_with_variant_t>
  static constexpr auto transform_sender(set_value_t, _Sndr&& __sndr, const _Env&) {
    return [&]<size_t... _Is>(index_sequence<_Is...>) {
      return when_all(into_variant(static_cast<_Sndr&&>(__sndr).template get<_Is + 2>())...);
    }(__ycxx::__detail::__exec::__indices_for<_Sndr>());
  }
};
inline constexpr when_all_with_variant_t when_all_with_variant{};
}} // namespace std::execution

// ---------------------------------------------------------------------------------------------
// stop-when(sndr, token) ([exec.stop.when])
namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __adl_free {
// stoken-t: stop requested when either token's is, callbacks run on either's stop request.
template <class _T1, class _T2>
struct __exec_either_stop_token {
  _T1 __t1;
  _T2 __t2;

  template <class _Fn>
  struct callback_type {
    struct forward {
      callback_type* __self;
      void operator()() noexcept {
        if (!__atomic_exchange_n(&__self->__fired, true, __ATOMIC_ACQ_REL))
          static_cast<_Fn&&>(__self->__fn)();
      }
    };
    _Fn __fn;
    bool __fired = false;
    std::stop_callback_for_t<_T1, forward> __cb1;
    std::stop_callback_for_t<_T2, forward> __cb2;

    template <class Init>
    callback_type(const __exec_either_stop_token& __tok, Init&& init) noexcept(std::is_nothrow_constructible_v<_Fn, Init>)
        : __fn(static_cast<Init&&>(init)), __cb1(__tok.__t1, forward{this}), __cb2(__tok.__t2, forward{this}) {}
    callback_type(callback_type&&) = delete;
  };

  bool stop_requested() const noexcept { return __t1.stop_requested() || __t2.stop_requested(); }
  bool stop_possible() const noexcept { return __t1.stop_possible() || __t2.stop_possible(); }
  bool operator==(const __exec_either_stop_token&) const = default;
};

template <class _Sndr, class _Token>
struct __exec_stop_when_sender {
  using sender_concept = std::execution::sender_tag;
  _Sndr __sndr;
  _Token token;

  template <class _Env>
  using __stoken_for = std::conditional_t<std::unstoppable_token<std::stop_token_of_t<_Env>>, _Token,
                                        __exec_either_stop_token<_Token, std::stop_token_of_t<_Env>>>;
  template <class _Self, class _Env>
  using __inner_t = decltype(std::execution::write_env(std::declval<std::remove_cvref_t<_Self>>().__sndr,
                                                     std::execution::prop(std::get_stop_token, std::declval<__stoken_for<_Env>>())));
  template <class _Self, class... _Env>
  struct __sigs {
    using type = ::__ycxx::__detail::__exec::__csigs_of_t<::__ycxx::__detail::__forward_like_t<_Self, _Sndr>>;
  };
  template <class _Self, class _Env>
  struct __sigs<_Self, _Env> {
    using type = ::__ycxx::__detail::__exec::__csigs_of_t<__inner_t<_Self, _Env>, _Env>;
  };
  template <class _Self, class... _Env>
  using __ycxx_csigs = typename __sigs<_Self, _Env...>::type;
  template <class _Self, class... _Env>
  static consteval auto get_completion_signatures() {
    return ::__ycxx::__detail::__exec::__checked_sigs<__ycxx_csigs<_Self, _Env...>>();
  }

  decltype(auto) get_env() const noexcept { return ::__ycxx::__detail::__exec::__fwd_env(std::execution::get_env(__sndr)); }

  template <::__ycxx::__detail::__exec::__decays_to<__exec_stop_when_sender> _Self, std::execution::receiver _Rcvr>
  auto connect(this _Self&& __self, _Rcvr __rcvr) {
    auto __rtoken = std::get_stop_token(std::execution::get_env(__rcvr));
    if constexpr (std::unstoppable_token<decltype(__rtoken)>) {
      return std::execution::connect(std::execution::write_env(std::forward_like<_Self>(__self.__sndr),
                                                               std::execution::prop(std::get_stop_token, std::forward_like<_Self>(__self.token))),
                                     static_cast<_Rcvr&&>(__rcvr));
    } else {
      using __stoken_t = __exec_either_stop_token<_Token, decltype(__rtoken)>;
      return std::execution::connect(
          std::execution::write_env(std::forward_like<_Self>(__self.__sndr),
                                    std::execution::prop(std::get_stop_token, __stoken_t{std::forward_like<_Self>(__self.token), __rtoken})),
          static_cast<_Rcvr&&>(__rcvr));
    }
  }
};
}} // namespace __ycxx::__adl_free

namespace [[__gnu__::__visibility__("hidden")]] __ycxx { namespace __detail { namespace __exec {
template <std::execution::sender _Sndr, std::stoppable_token _Token>
constexpr auto __stop_when(_Sndr&& __sndr, _Token token) {
  if constexpr (std::unstoppable_token<_Token>)
    return std::decay_t<_Sndr>(static_cast<_Sndr&&>(__sndr));
  else
    return ::__ycxx::__adl_free::__exec_stop_when_sender<std::decay_t<_Sndr>, _Token>{static_cast<_Sndr&&>(__sndr), static_cast<_Token&&>(token)};
}
}}} // namespace __ycxx::__detail::__exec
